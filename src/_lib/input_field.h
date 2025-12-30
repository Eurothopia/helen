#include "Arduino.h"

void insertChar(String &s, int &cursor, char c) {
  s = s.substring(0, cursor) + c + s.substring(cursor);
  cursor++;
}
void backspace(String &s, int &cursor) {
  if (cursor == 0) return;
  s = s.substring(0, cursor - 1) + s.substring(cursor);
  cursor--;
}
void deleteAtCursor(String &s, int cursor) {
  if (cursor >= s.length()) return;
  s = s.substring(0, cursor) + s.substring(cursor + 1);
}
void moveLeft(int &cursor) {
  if (cursor > 0) cursor--;
}

void moveRight(int &cursor, const String &s) {
  if (cursor < s.length()) cursor++;
}
void updateView(int &viewOffset, int cursor, int viewWidth) {
  if (cursor < viewOffset)
    viewOffset = cursor;
  else if (cursor > viewOffset + viewWidth)
    viewOffset = cursor - viewWidth;
}

String renderWithCursor(const String &s, int index) {
  if (index < 0) index = 0;
  if (index > s.length()) index = s.length();

  return s.substring(0, index) + "_" + s.substring(index + (index < s.length()));
}