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

// Update view for right-aligned rendering
// viewOffset = 0 means showing rightmost chars (end of string)
// viewOffset > 0 means scrolled left to show earlier chars
void updateViewRight(int &viewOffset, int cursor, int viewWidth, int stringLength) {
  // Calculate where cursor is in the visible window
  int visibleEnd = stringLength - viewOffset;
  int visibleStart = max(0, visibleEnd - viewWidth);
  
  // If cursor is past the visible end, scroll right (decrease viewOffset)
  if (cursor > visibleEnd) {
    viewOffset = max(0, stringLength - cursor);
  }
  // If cursor is before visible start, scroll left (increase viewOffset)
  else if (cursor < visibleStart) {
    viewOffset = stringLength - cursor - viewWidth;
    if (viewOffset < 0) viewOffset = 0;
  }
}

String renderWithCursor(const String &s, int index) {
  if (index < 0) index = 0;
  if (index > s.length()) index = s.length();

  return s.substring(0, index) + "_" + s.substring(index + (index < s.length()));
}

// Right-aligned input field: show rightmost characters that fit in viewWidth
// Returns substring to display and updates viewOffset to keep cursor visible
String getVisibleRight(const String &s, int cursor, int viewWidth, int &viewOffset) {
  int len = s.length();
  
  // Keep cursor visible: adjust viewOffset so cursor is in view
  if (cursor < viewOffset) {
    viewOffset = cursor;  // Cursor scrolled left
  } else if (cursor > viewOffset + viewWidth) {
    viewOffset = cursor - viewWidth;  // Cursor scrolled right
  }
  
  // Calculate start position (rightmost characters)
  int startIdx = max(0, len - viewWidth - viewOffset);
  int endIdx = min(len, len - viewOffset);
  
  return s.substring(startIdx, endIdx);
}

// Get cursor X position in pixels from RIGHT edge for right-aligned text
// cursorPos: cursor position in full string
// visibleStart: start index of visible substring in full string
// visibleText: the visible substring being displayed
// Returns pixel distance from right edge where cursor should be drawn
int getCursorPixelRight(TFT_eSprite &sprite, int cursorPos, int visibleStart, const String &visibleText) {
  // Calculate cursor position within visible text
  int cursorInVisible = cursorPos - visibleStart;
  
  // Clamp to visible range
  if (cursorInVisible < 0) return sprite.textWidth(visibleText);
  if (cursorInVisible > visibleText.length()) return 0;
  
  // Measure text from cursor to end (right side)
  String rightPart = visibleText.substring(cursorInVisible);
  return sprite.textWidth(rightPart);
}