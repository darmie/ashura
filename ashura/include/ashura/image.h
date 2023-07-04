#pragma once
#include "ashura/primitives.h"
#include "stx/memory.h"
#include "stx/span.h"

namespace ash
{

namespace gfx
{

/// NOTE: resource image with index 0 must be a transparent white image
using image = u64;

constexpr image WHITE_IMAGE = 0;

}        // namespace gfx

enum class ImageFormat : u8
{
  /// R8G8B8A8 image, stored on GPU as is
  Rgba8888,
  /// B8G8R8A8 image, stored on GPU as is
  Bgra8888,
  /// R8G8B8 image, stored on GPU as R8G8B8A8 with alpha = 255
  Rgb888,
  /// R8 image, stored on GPU as is
  R8
};

enum class ColorSpace : u8
{
  Rgb,
  Srgb,
  AdobeRgb,
  Dp3,
  DciP3
};

inline u8 nchannel_bytes(ImageFormat fmt)
{
  switch (fmt)
  {
    case ImageFormat::Rgba8888:
      return 4;
    case ImageFormat::Bgra8888:
      return 4;
    case ImageFormat::Rgb888:
      return 3;
    case ImageFormat::R8:
      return 1;
    default:
      ASH_UNREACHABLE();
  }
}

struct ImageView
{
  stx::Span<u8 const> data;
  ash::extent         extent;
  ImageFormat         format = ImageFormat::Rgba8888;
};

struct ImageViewMut
{
  stx::Span<u8> data;
  ash::extent   extent;
  ImageFormat   format = ImageFormat::Rgba8888;

  constexpr operator ImageView() const
  {
    return ImageView{.data = data, .extent = extent, .format = format};
  }
};

struct ImageBuffer
{
  stx::Memory memory;
  ash::extent extent;
  ImageFormat format = ImageFormat::Rgba8888;

  stx::Span<u8 const> span() const
  {
    return stx::Span{AS(u8 const *, memory.handle), extent.area() * nchannel_bytes(format)};
  }

  stx::Span<u8> span()
  {
    return stx::Span{AS(u8 *, memory.handle), extent.area() * nchannel_bytes(format)};
  }

  operator ImageView() const
  {
    return ImageView{.data = span(), .extent = extent, .format = format};
  }

  operator ImageViewMut()
  {
    return ImageViewMut{.data = span(), .extent = extent, .format = format};
  }

  void resize(ash::extent new_extent)
  {
    if (extent != new_extent)
    {
      if (extent.area() != new_extent.area())
      {
        stx::mem::reallocate(memory, new_extent.area() * nchannel_bytes(format)).unwrap();
      }
      extent = new_extent;
    }
  }
};

}        // namespace ash
