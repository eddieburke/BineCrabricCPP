#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif
namespace net::minecraft::client::input {
#ifdef _WIN32
/// Beta 1.7.3 keyboard key codes (used by KeyBinding and game logic).
inline int keyFromVirtualKey(int vk) {
 switch(vk) {
 case VK_ESCAPE:
  return 1;
 case '1':
  return 2;
 case '2':
  return 3;
 case '3':
  return 4;
 case '4':
  return 5;
 case '5':
  return 6;
 case '6':
  return 7;
 case '7':
  return 8;
 case '8':
  return 9;
 case '9':
  return 10;
 case '0':
  return 11;
 case VK_OEM_MINUS:
  return 12;
 case VK_OEM_PLUS:
  return 13;
 case VK_BACK:
  return 14;
 case VK_TAB:
  return 15;
 case 'Q':
  return 16;
 case 'W':
  return 17;
 case 'E':
  return 18;
 case 'R':
  return 19;
 case 'T':
  return 20;
 case 'Y':
  return 21;
 case 'U':
  return 22;
 case 'I':
  return 23;
 case 'O':
  return 24;
 case 'P':
  return 25;
 case VK_OEM_4:
  return 26;
 case VK_OEM_6:
  return 27;
 case VK_RETURN:
  return 28;
 case VK_LCONTROL:
  return 29;
 case 'A':
  return 30;
 case 'S':
  return 31;
 case 'D':
  return 32;
 case 'F':
  return 33;
 case 'G':
  return 34;
 case 'H':
  return 35;
 case 'J':
  return 36;
 case 'K':
  return 37;
 case 'L':
  return 38;
 case VK_OEM_1:
  return 39;
 case VK_OEM_7:
  return 40;
 case VK_OEM_3:
  return 41;
 case VK_LSHIFT:
  return 42;
 case VK_OEM_5:
  return 43;
 case 'Z':
  return 44;
 case 'X':
  return 45;
 case 'C':
  return 46;
 case 'V':
  return 47;
 case 'B':
  return 48;
 case 'N':
  return 49;
 case 'M':
  return 50;
 case VK_OEM_COMMA:
  return 51;
 case VK_OEM_PERIOD:
  return 52;
 case VK_OEM_2:
  return 53;
 case VK_RSHIFT:
  return 54;
 case VK_MULTIPLY:
  return 55;
 case VK_LMENU:
  return 56;
 case VK_SPACE:
  return 57;
 case VK_CAPITAL:
  return 58;
 case VK_F1:
  return 59;
 case VK_F2:
  return 60;
 case VK_F3:
  return 61;
 case VK_F4:
  return 62;
 case VK_F5:
  return 63;
 case VK_F6:
  return 64;
 case VK_F7:
  return 65;
 case VK_F8:
  return 66;
 case VK_F9:
  return 67;
 case VK_F10:
  return 68;
 case VK_NUMLOCK:
  return 69;
 case VK_SCROLL:
  return 70;
 case VK_F11:
  return 87;
 case VK_F12:
  return 88;
 default:
  return -1;
 }
}
/// Windows delivers the generic VK_SHIFT/VK_CONTROL/VK_MENU in wParam for
/// WM_KEY* messages; the left/right specific code only lives in the scancode +
/// extended bit of lParam. Resolve the generic codes to their side-specific VK
/// so keyFromVirtualKey can map LSHIFT(42)/RSHIFT(54)/LCONTROL(29)/RCONTROL(157)/
/// LMENU(56)/RMENU(184) correctly. Non-modifier keys pass through unchanged.
inline int resolveExtendedVirtualKey(int vk, LPARAM lParam) {
 const UINT scancode = static_cast<UINT>((lParam >> 16) & 0xFF);
 const bool extended = (lParam & (1 << 24)) != 0;
 switch(vk) {
 case VK_SHIFT: {
  const UINT mapped = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
  return mapped != 0 ? static_cast<int>(mapped) : VK_LSHIFT;
 }
 case VK_CONTROL:
  return extended ? VK_RCONTROL : VK_LCONTROL;
 case VK_MENU:
  return extended ? VK_RMENU : VK_LMENU;
 default:
  return vk;
 }
}
inline int virtualKeyFromKey(int key) {
 if(key >= 2 && key <= 10) {
  return '1' + (key - 2);
 }
 if(key == 11) {
  return '0';
 }
 if(key >= 16 && key <= 25) {
  static constexpr int kRow[] = {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'};
  return kRow[key - 16];
 }
 if(key >= 30 && key <= 38) {
  static constexpr int kRow[] = {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L'};
  return kRow[key - 30];
 }
 if(key >= 44 && key <= 50) {
  static constexpr int kRow[] = {'Z', 'X', 'C', 'V', 'B', 'N', 'M'};
  return kRow[key - 44];
 }
 switch(key) {
 case 1:
  return VK_ESCAPE;
 case 12:
  return VK_OEM_MINUS;
 case 13:
  return VK_OEM_PLUS;
 case 14:
  return VK_BACK;
 case 15:
  return VK_TAB;
 case 26:
  return VK_OEM_4;
 case 27:
  return VK_OEM_6;
 case 28:
  return VK_RETURN;
 case 29:
  return VK_LCONTROL;
 case 39:
  return VK_OEM_1;
 case 40:
  return VK_OEM_7;
 case 41:
  return VK_OEM_3;
 case 42:
  return VK_LSHIFT;
 case 43:
  return VK_OEM_5;
 case 51:
  return VK_OEM_COMMA;
 case 52:
  return VK_OEM_PERIOD;
 case 53:
  return VK_OEM_2;
 case 54:
  return VK_RSHIFT;
 case 55:
  return VK_MULTIPLY;
 case 56:
  return VK_LMENU;
 case 57:
  return VK_SPACE;
 case 58:
  return VK_CAPITAL;
 case 59:
  return VK_F1;
 case 60:
  return VK_F2;
 case 61:
  return VK_F3;
 case 62:
  return VK_F4;
 case 63:
  return VK_F5;
 case 64:
  return VK_F6;
 case 65:
  return VK_F7;
 case 66:
  return VK_F8;
 case 67:
  return VK_F9;
 case 68:
  return VK_F10;
 case 69:
  return VK_NUMLOCK;
 case 70:
  return VK_SCROLL;
 case 87:
  return VK_F11;
 case 88:
  return VK_F12;
 case 157:
  return VK_RCONTROL;
 case 184:
  return VK_RMENU;
 default:
  return -1;
 }
}
inline const char* keyDisplayName(int key) {
 switch(key) {
 case 1:
  return "ESC";
 case 15:
  return "TAB";
 case 28:
  return "RETURN";
 case 42:
  return "LSHIFT";
 case 54:
  return "RSHIFT";
 case 29:
  return "LCONTROL";
 case 57:
  return "SPACE";
 case 17:
  return "W";
 case 30:
  return "A";
 case 31:
  return "S";
 case 32:
  return "D";
 case 16:
  return "Q";
 case 18:
  return "E";
 case 20:
  return "T";
 case 33:
  return "F";
 case 87:
  return "F11";
 case 88:
  return "F12";
 default:
  if(key >= 2 && key <= 11) {
   static thread_local char digit[2] = {'0', '\0'};
   digit[0] = static_cast<char>('0' + (key - 1));
   return digit;
  }
  return "?";
 }
}
#endif
} // namespace net::minecraft::client::input
