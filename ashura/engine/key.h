#pragma once

#include "ashura/std/types.h"

namespace ash
{

enum class KeyModifiers : u32
{
  None       = 0x0000,
  LeftShift  = 0x0001,
  RightShift = 0x0002,
  LeftCtrl   = 0x0004,
  RightCtrl  = 0x0008,
  LeftAlt    = 0x0010,
  RightAlt   = 0x0020,
  LeftWin    = 0x0040,
  RightWin   = 0x0080,
  Num        = 0x0100,
  Caps       = 0x0200,
  AltGr      = 0x0400,
  ScrollLock = 0x0800,
  Ctrl       = 0x1000,
  Shift      = 0x2000,
  Alt        = 0x4000,
  Gui        = 0x8000,
  All        = 0xFFFF
};

ASH_DEFINE_ENUM_BIT_OPS(KeyModifiers)

/// https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input?redirectedfrom=MSDN#_win32_Keyboard_Input_Model
enum Key : u32
{
  Key_UNKNOWN            = 0,
  Key_RETURN             = '\r',
  Key_ESCAPE             = '\x1B',
  Key_BACKSPACE          = '\b',
  Key_TAB                = '\t',
  Key_SPACE              = ' ',
  Key_EXCLAIM            = '!',
  Key_QUOTEDBL           = '"',
  Key_HASH               = '#',
  Key_PERCENT            = '%',
  Key_DOLLAR             = '$',
  Key_AMPERSAND          = '&',
  Key_QUOTE              = '\'',
  Key_LEFTPAREN          = '(',
  Key_RIGHTPAREN         = ')',
  Key_ASTERISK           = '*',
  Key_PLUS               = '+',
  Key_COMMA              = ',',
  Key_MINUS              = '-',
  Key_PERIOD             = '.',
  Key_SLASH              = '/',
  Key_NUM_0              = '0',
  Key_NUM_1              = '1',
  Key_NUM_2              = '2',
  Key_NUM_3              = '3',
  Key_NUM_4              = '4',
  Key_NUM_5              = '5',
  Key_NUM_6              = '6',
  Key_NUM_7              = '7',
  Key_NUM_8              = '8',
  Key_NUM_9              = '9',
  Key_COLON              = ':',
  Key_SEMICOLON          = ';',
  Key_LESS               = '<',
  Key_EQUALS             = '=',
  Key_GREATER            = '>',
  Key_QUESTION           = '?',
  Key_AT                 = '@',
  Key_LEFTBRACKET        = '[',
  Key_BACKSLASH          = '\\',
  Key_RIGHTBRACKET       = ']',
  Key_CARET              = '^',
  Key_UNDERSCORE         = '_',
  Key_BACKQUOTE          = '`',
  Key_a                  = 'a',
  Key_b                  = 'b',
  Key_c                  = 'c',
  Key_d                  = 'd',
  Key_e                  = 'e',
  Key_f                  = 'f',
  Key_g                  = 'g',
  Key_h                  = 'h',
  Key_i                  = 'i',
  Key_j                  = 'j',
  Key_k                  = 'k',
  Key_l                  = 'l',
  Key_m                  = 'm',
  Key_n                  = 'n',
  Key_o                  = 'o',
  Key_p                  = 'p',
  Key_q                  = 'q',
  Key_r                  = 'r',
  Key_s                  = 's',
  Key_t                  = 't',
  Key_u                  = 'u',
  Key_v                  = 'v',
  Key_w                  = 'w',
  Key_x                  = 'x',
  Key_y                  = 'y',
  Key_z                  = 'z',
  Key_CAPSLOCK           = 0x40000000 | 57,
  Key_F1                 = 0x40000000 | 58,
  Key_F2                 = 0x40000000 | 59,
  Key_F3                 = 0x40000000 | 60,
  Key_F4                 = 0x40000000 | 61,
  Key_F5                 = 0x40000000 | 62,
  Key_F6                 = 0x40000000 | 63,
  Key_F7                 = 0x40000000 | 64,
  Key_F8                 = 0x40000000 | 65,
  Key_F9                 = 0x40000000 | 66,
  Key_F10                = 0x40000000 | 67,
  Key_F11                = 0x40000000 | 68,
  Key_F12                = 0x40000000 | 69,
  Key_PRINTSCREEN        = 0x40000000 | 70,
  Key_SCROLLLOCK         = 0x40000000 | 71,
  Key_PAUSE              = 0x40000000 | 72,
  Key_INSERT             = 0x40000000 | 73,
  Key_HOME               = 0x40000000 | 74,
  Key_PAGEUP             = 0x40000000 | 75,
  Key_DELETE             = 0x40000000 | 76,
  Key_END                = 0x40000000 | 77,
  Key_PAGEDOWN           = 0x40000000 | 78,
  Key_RIGHT              = 0x40000000 | 79,
  Key_LEFT               = 0x40000000 | 80,
  Key_DOWN               = 0x40000000 | 81,
  Key_UP                 = 0x40000000 | 82,
  Key_NUMLOCKCLEAR       = 0x40000000 | 83,
  Key_KP_DIVIDE          = 0x40000000 | 84,
  Key_KP_MULTIPLY        = 0x40000000 | 85,
  Key_KP_MINUS           = 0x40000000 | 86,
  Key_KP_PLUS            = 0x40000000 | 87,
  Key_KP_ENTER           = 0x40000000 | 88,
  Key_KP_1               = 0x40000000 | 89,
  Key_KP_2               = 0x40000000 | 90,
  Key_KP_3               = 0x40000000 | 91,
  Key_KP_4               = 0x40000000 | 92,
  Key_KP_5               = 0x40000000 | 93,
  Key_KP_6               = 0x40000000 | 94,
  Key_KP_7               = 0x40000000 | 95,
  Key_KP_8               = 0x40000000 | 96,
  Key_KP_9               = 0x40000000 | 97,
  Key_KP_0               = 0x40000000 | 98,
  Key_KP_PERIOD          = 0x40000000 | 99,
  Key_APPLICATION        = 0x40000000 | 101,
  Key_POWER              = 0x40000000 | 102,
  Key_KP_EQUALS          = 0x40000000 | 103,
  Key_F13                = 0x40000000 | 104,
  Key_F14                = 0x40000000 | 105,
  Key_F15                = 0x40000000 | 106,
  Key_F16                = 0x40000000 | 107,
  Key_F17                = 0x40000000 | 108,
  Key_F18                = 0x40000000 | 109,
  Key_F19                = 0x40000000 | 110,
  Key_F20                = 0x40000000 | 111,
  Key_F21                = 0x40000000 | 112,
  Key_F22                = 0x40000000 | 113,
  Key_F23                = 0x40000000 | 114,
  Key_F24                = 0x40000000 | 115,
  Key_EXECUTE            = 0x40000000 | 116,
  Key_HELP               = 0x40000000 | 117,
  Key_MENU               = 0x40000000 | 118,
  Key_SELECT             = 0x40000000 | 119,
  Key_STOP               = 0x40000000 | 120,
  Key_AGAIN              = 0x40000000 | 121,
  Key_UNDO               = 0x40000000 | 122,
  Key_CUT                = 0x40000000 | 123,
  Key_COPY               = 0x40000000 | 124,
  Key_PASTE              = 0x40000000 | 125,
  Key_FIND               = 0x40000000 | 126,
  Key_MUTE               = 0x40000000 | 127,
  Key_VOLUMEUP           = 0x40000000 | 128,
  Key_VOLUMEDOWN         = 0x40000000 | 129,
  Key_KP_COMMA           = 0x40000000 | 133,
  Key_KP_EQUALSAS400     = 0x40000000 | 134,
  Key_ALTERASE           = 0x40000000 | 153,
  Key_SYSREQ             = 0x40000000 | 154,
  Key_CANCEL             = 0x40000000 | 155,
  Key_CLEAR              = 0x40000000 | 156,
  Key_PRIOR              = 0x40000000 | 157,
  Key_RETURN2            = 0x40000000 | 158,
  Key_SEPARATOR          = 0x40000000 | 159,
  Key_OUT                = 0x40000000 | 160,
  Key_OPER               = 0x40000000 | 161,
  Key_CLEARAGAIN         = 0x40000000 | 162,
  Key_CRSEL              = 0x40000000 | 163,
  Key_EXSEL              = 0x40000000 | 164,
  Key_KP_00              = 0x40000000 | 176,
  Key_KP_000             = 0x40000000 | 177,
  Key_THOUSANDSSEPARATOR = 0x40000000 | 178,
  Key_DECIMALSEPARATOR   = 0x40000000 | 179,
  Key_CURRENCYUNIT       = 0x40000000 | 180,
  Key_CURRENCYSUBUNIT    = 0x40000000 | 181,
  Key_KP_LEFTPAREN       = 0x40000000 | 182,
  Key_KP_RIGHTPAREN      = 0x40000000 | 183,
  Key_KP_LEFTBRACE       = 0x40000000 | 184,
  Key_KP_RIGHTBRACE      = 0x40000000 | 185,
  Key_KP_TAB             = 0x40000000 | 186,
  Key_KP_BACKSPACE       = 0x40000000 | 187,
  Key_KP_A               = 0x40000000 | 188,
  Key_KP_B               = 0x40000000 | 189,
  Key_KP_C               = 0x40000000 | 190,
  Key_KP_D               = 0x40000000 | 191,
  Key_KP_E               = 0x40000000 | 192,
  Key_KP_F               = 0x40000000 | 193,
  Key_KP_XOR             = 0x40000000 | 194,
  Key_KP_POWER           = 0x40000000 | 195,
  Key_KP_PERCENT         = 0x40000000 | 196,
  Key_KP_LESS            = 0x40000000 | 197,
  Key_KP_GREATER         = 0x40000000 | 198,
  Key_KP_AMPERSAND       = 0x40000000 | 199,
  Key_KP_DBLAMPERSAND    = 0x40000000 | 200,
  Key_KP_VERTICALBAR     = 0x40000000 | 201,
  Key_KP_DBLVERTICALBAR  = 0x40000000 | 202,
  Key_KP_COLON           = 0x40000000 | 203,
  Key_KP_HASH            = 0x40000000 | 204,
  Key_KP_SPACE           = 0x40000000 | 205,
  Key_KP_AT              = 0x40000000 | 206,
  Key_KP_EXCLAM          = 0x40000000 | 207,
  Key_KP_MEMSTORE        = 0x40000000 | 208,
  Key_KP_MEMRECALL       = 0x40000000 | 209,
  Key_KP_MEMCLEAR        = 0x40000000 | 210,
  Key_KP_MEMADD          = 0x40000000 | 211,
  Key_KP_MEMSUBTRACT     = 0x40000000 | 212,
  Key_KP_MEMMULTIPLY     = 0x40000000 | 213,
  Key_KP_MEMDIVIDE       = 0x40000000 | 214,
  Key_KP_PLUSMINUS       = 0x40000000 | 215,
  Key_KP_CLEAR           = 0x40000000 | 216,
  Key_KP_CLEARENTRY      = 0x40000000 | 217,
  Key_KP_BINARY          = 0x40000000 | 218,
  Key_KP_OCTAL           = 0x40000000 | 219,
  Key_KP_DECIMAL         = 0x40000000 | 220,
  Key_KP_HEXADECIMAL     = 0x40000000 | 221,
  Key_LCTRL              = 0x40000000 | 224,
  Key_LSHIFT             = 0x40000000 | 225,
  Key_LALT               = 0x40000000 | 226,
  Key_LGUI               = 0x40000000 | 227,
  Key_RCTRL              = 0x40000000 | 228,
  Key_RSHIFT             = 0x40000000 | 229,
  Key_RALT               = 0x40000000 | 230,
  Key_RGUI               = 0x40000000 | 231,
  Key_MODE               = 0x40000000 | 257,
  Key_AUDIONEXT          = 0x40000000 | 258,
  Key_AUDIOPREV          = 0x40000000 | 259,
  Key_AUDIOSTOP          = 0x40000000 | 260,
  Key_AUDIOPLAY          = 0x40000000 | 261,
  Key_AUDIOMUTE          = 0x40000000 | 262,
  Key_MEDIASELECT        = 0x40000000 | 263,
  Key_WWW                = 0x40000000 | 264,
  Key_MAIL               = 0x40000000 | 265,
  Key_CALCULATOR         = 0x40000000 | 266,
  Key_COMPUTER           = 0x40000000 | 267,
  Key_AC_SEARCH          = 0x40000000 | 268,
  Key_AC_HOME            = 0x40000000 | 269,
  Key_AC_BACK            = 0x40000000 | 270,
  Key_AC_FORWARD         = 0x40000000 | 271,
  Key_AC_STOP            = 0x40000000 | 272,
  Key_AC_REFRESH         = 0x40000000 | 273,
  Key_AC_BOOKMARKS       = 0x40000000 | 274,
  Key_BRIGHTNESSDOWN     = 0x40000000 | 275,
  Key_BRIGHTNESSUP       = 0x40000000 | 276,
  Key_DISPLAYSWITCH      = 0x40000000 | 277,
  Key_KBDILLUMTOGGLE     = 0x40000000 | 278,
  Key_KBDILLUMDOWN       = 0x40000000 | 279,
  Key_KBDILLUMUP         = 0x40000000 | 280,
  Key_EJECT              = 0x40000000 | 281,
  Key_SLEEP              = 0x40000000 | 282,
  Key_APP1               = 0x40000000 | 283,
  Key_APP2               = 0x40000000 | 284,
  Key_AUDIOREWIND        = 0x40000000 | 285,
  Key_AUDIOFASTFORWARD   = 0x40000000 | 286,
  Key_SOFTLEFT           = 0x40000000 | 287,
  Key_SOFTRIGHT          = 0x40000000 | 288,
  Key_CALL               = 0x40000000 | 289,
  Key_ENDCAL             = 0x40000000 | 290
};

enum class MouseButtons : u32
{
  None      = 0x00,
  Primary   = 0x01,
  Secondary = 0x02,
  Middle    = 0x04,
  A1        = 0x08,
  A2        = 0x10,
  A3        = 0x20,
  A4        = 0x40,
  A5        = 0x80,
  All       = 0xFFFFFFFF
};

ASH_DEFINE_ENUM_BIT_OPS(MouseButtons)

}        // namespace ash
