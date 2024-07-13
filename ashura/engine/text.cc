/// SPDX-License-Identifier: MIT
#include "ashura/engine/text.h"
#include "ashura/engine/font_impl.h"
#include "ashura/std/vec.h"

extern "C"
{
#include "SBAlgorithm.h"
#include "SBLine.h"
#include "SBParagraph.h"
#include "SBScriptLocator.h"
#include "hb.h"
}

namespace ash
{

/// layout is output in PT_UNIT units. so it is independent of the actual
/// font-height and can be cached as necessary. text must have been sanitized
/// with invalid codepoints replaced before calling this.
/// @param script OpenType (ISO15924) Script
/// Tag. See: https://unicode.org/reports/tr24/#Relation_To_ISO15924
static inline void shape(hb_font_t *font, hb_buffer_t *buffer,
                         Span<u32 const> text, u32 first, u32 count,
                         hb_script_t script, hb_direction_t direction,
                         hb_language_t language, bool use_kerning,
                         bool use_ligatures, Span<hb_glyph_info_t const> &infos,
                         Span<hb_glyph_position_t const> &positions)
{
  // tags are opentype feature tags
  hb_feature_t const shaping_features[] = {
      // kerning operations
      {.tag   = HB_TAG('k', 'e', 'r', 'n'),
       .value = use_kerning,
       .start = HB_FEATURE_GLOBAL_START,
       .end   = HB_FEATURE_GLOBAL_END},
      // standard ligature glyph substitution
      {.tag   = HB_TAG('l', 'i', 'g', 'a'),
       .value = use_ligatures,
       .start = HB_FEATURE_GLOBAL_START,
       .end   = HB_FEATURE_GLOBAL_END},
      // contextual ligature glyph substitution
      {.tag   = HB_TAG('c', 'l', 'i', 'g'),
       .value = use_ligatures,
       .start = HB_FEATURE_GLOBAL_START,
       .end   = HB_FEATURE_GLOBAL_END}};

  hb_buffer_reset(buffer);
  // invalid character replacement
  hb_buffer_set_replacement_codepoint(buffer,
                                      HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT);
  hb_buffer_set_script(buffer, script);
  hb_buffer_set_direction(buffer, direction);
  // OpenType BCP-47 language tag specifying locale-sensitive shaping operations
  // as defined in the font
  hb_buffer_set_language(buffer, language);
  hb_buffer_add_codepoints(buffer, text.data(), (i32) text.size(), first,
                           (i32) count);
  hb_shape(font, buffer, shaping_features, (u32) size(shaping_features));

  u32                        num_pos;
  hb_glyph_position_t const *glyph_pos =
      hb_buffer_get_glyph_positions(buffer, &num_pos);
  CHECK(!(glyph_pos == nullptr && num_pos > 0));

  u32                    num_info;
  hb_glyph_info_t const *glyph_info =
      hb_buffer_get_glyph_infos(buffer, &num_info);
  CHECK(!(glyph_info == nullptr && num_info > 0));

  CHECK(num_pos == num_info);

  infos     = Span{glyph_info, num_info};
  positions = Span{glyph_pos, num_pos};
}

/// @brief only needs to be called if it contains multiple paragraphs
static inline void segment_paragraphs(Span<u32 const>   text,
                                      Span<TextSegment> segments)
{
  u32 const text_size = text.size32();
  for (u32 i = 0; i < text_size;)
  {
    segments[i].paragraph_begin = true;
    while (i < text_size)
    {
      if (text[i] == '\r' && ((i + 1) < text_size) && text[i + 1] == '\n')
      {
        segments[i].paragraph_end = true;
        i += 2;
        break;
      }
      else if (text[i] == '\n' || text[i] == '\r')
      {
        segments[i].paragraph_end = true;
        i += 1;
        break;
      }
      i++;
    }
  }
}

/// @brief only needs to be called if it contains multiple scripts
/// outputs iso15924 or OpenType tags
static inline void segment_scripts(Span<u32 const>   text,
                                   Span<TextSegment> segments)
{
  SBCodepointSequence codepoints{.stringEncoding = SBStringEncodingUTF32,
                                 .stringBuffer   = (void *) text.data(),
                                 .stringLength   = text.size()};

  SBScriptLocatorRef locator = SBScriptLocatorCreate();
  CHECK(locator != nullptr);
  SBScriptLocatorLoadCodepoints(locator, &codepoints);

  SBScriptAgent const *agent = SBScriptLocatorGetAgent(locator);
  CHECK(agent != nullptr);

  while (SBScriptLocatorMoveNext(locator) == SBTrue)
  {
    for (u32 i = 0; i < (u32) agent->length; i++)
    {
      segments[agent->offset + i].script = TextScript{agent->script};
    }
  }

  SBScriptLocatorRelease(locator);
}

/// @brief only needs to be called if it is a bidirectional text
static inline void segment_levels(Span<u32 const> text,
                                  SBAlgorithmRef algorithm, TextDirection base,
                                  Span<TextSegment> segments)
{
  // The embedding level is an integer value. LTR text segments have even
  // embedding levels (e.g., 0, 2, 4), and RTL text segments have odd embedding
  // levels (e.g., 1, 3, 5).
  u32 const text_size = text.size32();
  for (u32 i = 0; i < text_size;)
  {
    u32 first = i;
    while (i < text_size && !segments[i].paragraph_end)
    {
      i++;
    }

    u32 const length = i - first;

    if (length > 0)
    {
      SBParagraphRef paragraph = SBAlgorithmCreateParagraph(
          algorithm, first, length,
          (base == TextDirection::LeftToRight) ? SBLevelDefaultLTR :
                                                 SBLevelDefaultRTL);
      CHECK(paragraph != nullptr);

      CHECK(SBParagraphGetLength(paragraph) == length);
      SBLevel const  base_level = SBParagraphGetBaseLevel(paragraph);
      SBLevel const *levels     = SBParagraphGetLevelsPtr(paragraph);
      CHECK(levels != nullptr);
      for (u32 i = 0; i < length; i++)
      {
        segments[first + i].base_level = base_level;
        segments[first + i].level      = levels[i];
      }
      SBParagraphRelease(paragraph);
    }

    i++;
    while (i < text_size && !segments[i].paragraph_begin)
    {
      i++;
    }
  }
}

/// @brief only needs to be called if line breaking is required.
static inline void segment_breakpoints(Span<u32 const>   text,
                                       Span<TextSegment> segments)
{
  u32 const text_size = text.size32();
  for (u32 i = 0; i < text_size;)
  {
    segments[i].breakable = true;
    while (i < text_size && text[i] != ' ' && text[i] != '\t')
    {
      i++;
    }

    while (i < text_size && (text[i] == ' ' || text[i] == '\t'))
    {
      i++;
    }
  }
}

static inline void insert_run(TextLayout &l, FontStyle const &s, u32 first,
                              u32 count, u16 style,
                              FontMetrics const &font_metrics, u8 base_level,
                              u8 level, bool paragraph, bool breakable,
                              Span<hb_glyph_info_t const>     infos,
                              Span<hb_glyph_position_t const> positions)
{
  u32 const num_glyphs  = infos.size32();
  u32 const first_glyph = l.glyphs.size32();
  i32       advance     = 0;

  CHECK(l.glyphs.extend_uninitialized(num_glyphs));

  for (u32 i = 0; i < num_glyphs; i++)
  {
    hb_glyph_info_t const     &info = infos[i];
    hb_glyph_position_t const &pos  = positions[i];
    GlyphShape                 shape{.glyph   = info.codepoint,
                                     .cluster = info.cluster,
                                     .advance = {pos.x_advance, pos.y_advance},
                                     .offset  = {pos.x_offset, -pos.y_offset}};

    l.glyphs[first_glyph + i] = shape;
    advance += pos.x_advance;
  }

  CHECK(l.runs.push(
      TextRun{.first       = first,
              .count       = count,
              .style       = style,
              .font_height = s.font_height,
              .line_height = max(s.line_height, 1.0f),
              .first_glyph = first_glyph,
              .num_glyphs  = num_glyphs,
              .metrics     = TextRunMetrics{.advance = advance,
                                            .ascent  = font_metrics.ascent,
                                            .descent = font_metrics.descent},
              .base_level  = base_level,
              .level       = level,
              .paragraph   = paragraph,
              .breakable   = breakable}));
}

/// See Unicode Embedding Level Reordering:
/// https://www.unicode.org/reports/tr9/#L1 -
/// https://www.unicode.org/reports/tr9/#L2
static inline void reorder_line(Span<TextRun> runs)
{
  u8 max_level = 0;
  for (TextRun const &r : runs)
  {
    max_level = max(r.level, max_level);
  }

  u8 level = max_level;
  while (level > 0)
  {
    // re-order consecutive runs with embedding levels greater or equal than
    // the current embedding level
    for (u32 i = 0; i < runs.size32();)
    {
      u32 const first     = i++;
      u8 const  run_level = runs[first].level;
      while (i < runs.size32() && runs[i].level == run_level)
      {
        i++;
      }

      if (run_level >= level)
      {
        reverse(runs.slice(first, i - first));
      }
    }
    level--;
  }
}

/// see:
/// https://stackoverflow.com/questions/62374506/how-do-i-align-glyphs-along-the-baseline-with-freetype
///
void layout_text(TextBlock const &block, f32 max_width, TextLayout &layout)
{
  layout.clear();
  u32 const text_size = block.text.size32();
  CHECK(block.text.size() <= I32_MAX);
  CHECK(block.fonts.size() <= U16_MAX);
  CHECK(block.runs.size() == block.fonts.size());

  CHECK(layout.segments.resize_defaulted(block.text.size()));
  Span segments = to_span(layout.segments);

  for (TextSegment &s : segments)
  {
    s = TextSegment{};
  }

  {
    u32 prev_run_end = 0;
    for (u32 irun = 0; irun < block.runs.size32(); irun++)
    {
      u32 const run_end = min(block.runs[irun], text_size);
      CHECK(prev_run_end <= block.text.size());
      CHECK(prev_run_end <= run_end);
      for (u32 i = prev_run_end; i < run_end; i++)
      {
        segments[i].style = irun;
      }
      prev_run_end = run_end;
    }
  }

  segment_paragraphs(block.text, segments);
  segment_scripts(block.text, segments);
  segment_breakpoints(block.text, segments);

  {
    SBCodepointSequence codepoints{.stringEncoding = SBStringEncodingUTF32,
                                   .stringBuffer   = (void *) block.text.data(),
                                   .stringLength   = text_size};
    SBAlgorithmRef      algorithm = SBAlgorithmCreate(&codepoints);
    CHECK(algorithm != nullptr);
    defer algorithm_del{[&] { SBAlgorithmRelease(algorithm); }};
    segment_levels(block.text, algorithm, block.direction, segments);
  }

  {
    hb_language_t language =
        block.language.is_empty() ?
            hb_language_get_default() :
            hb_language_from_string(block.language.data(),
                                    (i32) block.language.size());
    hb_buffer_t *buffer = hb_buffer_create();
    CHECK(buffer != nullptr);
    defer buffer_del{[&] { hb_buffer_destroy(buffer); }};

    for (u32 p = 0; p < text_size;)
    {
      u32 const paragraph_begin = p;
      while (p < text_size && !segments[p].paragraph_end)
      {
        p++;
      }
      u32 const paragraph_end = p;

      for (u32 i = paragraph_begin; i < paragraph_end;)
      {
        u32 const          first         = i++;
        TextSegment const &first_segment = segments[first];
        while (i < paragraph_end && first_segment.style == segments[i].style &&
               first_segment.script == segments[i].script &&
               first_segment.level == segments[i].level &&
               !segments[i].breakable)
        {
          i++;
        }

        FontStyle const                &s = block.fonts[first_segment.style];
        FontImpl const                 *f = (FontImpl const *) s.font;
        Span<hb_glyph_info_t const>     infos     = {};
        Span<hb_glyph_position_t const> positions = {};
        shape(f->hb_font, buffer, block.text, first, i - first,
              hb_script_from_iso15924_tag(
                  SBScriptGetOpenTypeTag(SBScript{(u8) first_segment.script})),
              ((first_segment.level & 0x1) == 0) ? HB_DIRECTION_LTR :
                                                   HB_DIRECTION_RTL,
              language, block.use_kerning, block.use_ligatures, infos,
              positions);

        insert_run(layout, s, first, i - first, first_segment.style, f->metrics,
                   first_segment.base_level, first_segment.level,
                   first_segment.paragraph_begin, first_segment.breakable,
                   infos, positions);
      }

      p++;
      while (p < text_size && !segments[p].paragraph_begin)
      {
        p++;
      }
    }
  }

  u32 const num_runs = layout.runs.size32();
  Vec2      extent{};

  for (u32 i = 0; i < num_runs;)
  {
    u32 const      first      = i++;
    TextRun const &first_run  = layout.runs[first];
    u8 const       base_level = first_run.base_level;
    bool const     paragraph  = first_run.paragraph;
    f32 width   = pt_to_px(first_run.metrics.advance, first_run.font_height);
    f32 ascent  = pt_to_px(first_run.metrics.ascent, first_run.font_height);
    f32 descent = pt_to_px(first_run.metrics.descent, first_run.font_height);
    f32 height  = pt_to_px(first_run.metrics.ascent + first_run.metrics.descent,
                           first_run.font_height) *
                 first_run.line_height;

    while (
        i < num_runs && !layout.runs[i].paragraph &&
        !(layout.runs[i].breakable && (pt_to_px(layout.runs[i].metrics.advance,
                                                layout.runs[i].font_height) +
                                       width) > max_width))
    {
      TextRun const        &r = layout.runs[i];
      TextRunMetrics const &m = r.metrics;
      width += pt_to_px(m.advance, r.font_height);
      ascent  = max(ascent, pt_to_px(m.ascent, r.font_height));
      descent = max(descent, pt_to_px(m.descent, r.font_height));
      height  = max(height, pt_to_px(m.ascent + m.descent, r.font_height) *
                                r.line_height);
      i++;
    }

    TextRun const &last_run        = layout.runs[i - 1];
    u32 const      first_codepoint = first_run.first;
    u32 const      num_codepoints =
        (last_run.first + last_run.count) - first_codepoint;

    Line line{.first     = first_codepoint,
              .count     = num_codepoints,
              .first_run = first,
              .num_runs  = (i - first),
              .metrics   = LineMetrics{.width   = width,
                                       .height  = height,
                                       .ascent  = ascent,
                                       .descent = descent,
                                       .level   = base_level},
              .paragraph = paragraph};

    CHECK(layout.lines.push(line));

    reorder_line(to_span(layout.runs).slice(first, i - first));

    extent.x = max(extent.x, width);
    extent.y += height;
  }

  layout.max_width = max_width;
  layout.extent    = extent;
}

TextHitResult hit_text(TextLayout const &layout, Vec2 pos)
{
  f32       current_top = 0;
  u32       l           = 0;
  u32 const num_lines   = layout.lines.size32();

  if (num_lines == 0)
  {
    return TextHitResult{.cluster = 0, .line = 0, .column = 0};
  }

  // separated vertical and horizontal clamped hit test
  for (; l < num_lines; l++)
  {
    if (current_top <= pos.y &&
        (current_top + layout.lines[l].metrics.height) >= pos.y)
    {
      break;
    }
  }

  l = min(l, num_lines - 1);

  Line const &ln = layout.lines[l];

  f32 cursor = 0;

  for (u32 r = 0; r < ln.num_runs;)
  {
    u32 const      first     = r++;
    TextRun const &first_run = layout.runs[ln.first_run + first];
    f32            dir_advance =
        pt_to_px(first_run.metrics.advance, first_run.font_height);

    while (r < ln.num_runs &&
           layout.runs[ln.first_run + r].level == first_run.level)
    {
      TextRun const &run = layout.runs[ln.first_run + r];
      dir_advance += pt_to_px(run.metrics.advance, run.font_height);
      r++;
    }

    // check in run, if in run or at end of run count, break, if at last run
    // based on visual ordering and at the last char on run, return
    //
    // needs better intersection testing given multiple runs will ne on a single
    // line.
    //
    //

    bool const intersects = (cursor <= pos.x) || (r == ln.num_runs);
    if (!intersects)
    {
      continue;
    }
    for (u32 g = 0; g < first_run.num_glyphs; g++)
    {
      // TODO(lamarrr): not correct, needs to perform actual intersection
      // test also, need to take care of directionality
      GlyphShape const &glyph = layout.glyphs[first_run.first_glyph + g];
      // based on direction, find first cluster that is lesser, doesn't have
      // to strictly intersect
      bool const intersects =
          (pt_to_px(glyph.advance.x, first_run.font_height) + cursor <=
           pos.x) ||
          (g == first_run.num_glyphs - 1 && r == ln.num_runs - 1);
      if (intersects)
      {
        u32 const column =
            (glyph.cluster > ln.first) ? (glyph.cluster - ln.first) : 0;
        return TextHitResult{
            .cluster = glyph.cluster, .line = l, .column = column};
      }
    }
    cursor += dir_advance;
  }

  u32 const column = (ln.count == 0) ? 0 : (ln.count - 1);
  return TextHitResult{
      .cluster = ln.first + column, .line = l, .column = column};
}

}        // namespace ash
