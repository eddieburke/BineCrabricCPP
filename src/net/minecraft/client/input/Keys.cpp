#include "net/minecraft/client/input/Keys.hpp"
#include <string>
namespace net::minecraft::client::input {
std::string keyDisplayName(int key) {
 if(key >= 2 && key <= 11) {
  static constexpr char names[] = "1234567890";
  return std::string(1, names[key - 2]);
 }
 if(key >= 59 && key <= 68) {
  return "F" + std::to_string(key - 58);
 }
 switch(key) {
 case 0: return "None";
 case 1: return "Escape";
 case 12: return "-";
 case 13: return "=";
 case 14: return "Backspace";
 case 15: return "Tab";
 case 16: return "Q";
 case 17: return "W";
 case 18: return "E";
 case 19: return "R";
 case 20: return "T";
 case 21: return "Y";
 case 22: return "U";
 case 23: return "I";
 case 24: return "O";
 case 25: return "P";
 case 26: return "[";
 case 27: return "]";
 case 28: return "Enter";
 case 29: return "Left Ctrl";
 case 30: return "A";
 case 31: return "S";
 case 32: return "D";
 case 33: return "F";
 case 34: return "G";
 case 35: return "H";
 case 36: return "J";
 case 37: return "K";
 case 38: return "L";
 case 39: return ";";
 case 40: return "'";
 case 41: return "`";
 case 42: return "Left Shift";
 case 43: return "\\";
 case 44: return "Z";
 case 45: return "X";
 case 46: return "C";
 case 47: return "V";
 case 48: return "B";
 case 49: return "N";
 case 50: return "M";
 case 51: return ",";
 case 52: return ".";
 case 53: return "/";
 case 54: return "Right Shift";
 case 55: return "Numpad *";
 case 56: return "Left Alt";
 case 57: return "Space";
 case 58: return "Caps Lock";
 case 69: return "Num Lock";
 case 70: return "Scroll Lock";
 case 71: return "Numpad 7";
 case 72: return "Numpad 8";
 case 73: return "Numpad 9";
 case 74: return "Numpad -";
 case 75: return "Numpad 4";
 case 76: return "Numpad 5";
 case 77: return "Numpad 6";
 case 78: return "Numpad +";
 case 79: return "Numpad 1";
 case 80: return "Numpad 2";
 case 81: return "Numpad 3";
 case 82: return "Numpad 0";
 case 83: return "Numpad .";
 case 87: return "F11";
 case 88: return "F12";
 case 156: return "Numpad Enter";
 case 157: return "Right Ctrl";
 case 181: return "Numpad /";
 case 183: return "Print Screen";
 case 184: return "Right Alt";
 case 197: return "Pause";
 case 199: return "Home";
 case 200: return "Up";
 case 201: return "Page Up";
 case 203: return "Left";
 case 205: return "Right";
 case 207: return "End";
 case 208: return "Down";
 case 209: return "Page Down";
 case 210: return "Insert";
 case 211: return "Delete";
 case 219: return "Left Meta";
 case 220: return "Right Meta";
 case 221: return "Menu";
 default: return "Key " + std::to_string(key);
 }
}
}
