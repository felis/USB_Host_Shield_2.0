#ifndef __kbdrptparser_h_
#define __kbdrptparser_h_

class KbdRptParser : public KeyboardReportParser {
  private:
    void PrintKey(uint8_t mod, uint8_t key);

  protected:
    virtual void OnControlKeysChanged(uint8_t before, uint8_t after);
    virtual void OnKeyDown(uint8_t mod, uint8_t key);
    virtual void OnKeyUp(uint8_t mod, uint8_t key);
    virtual void OnKeyPressed(uint8_t key);
};

void KbdRptParser::PrintKey(uint8_t m, uint8_t key) {
  MODIFIERKEYS mod;
  *((uint8_t*)&mod) = m;
  Serial.print((mod.bmLeftCtrl == 1) ? F("C") : F(" "));
  Serial.print((mod.bmLeftShift == 1) ? F("S") : F(" "));
  Serial.print((mod.bmLeftAlt == 1) ? F("A") : F(" "));
  Serial.print((mod.bmLeftGUI == 1) ? F("G") : F(" "));

  Serial.print(F(" >"));
  PrintHex<uint8_t>(key, 0x80);
  Serial.print(F("< "));

  Serial.print((mod.bmRightCtrl == 1) ? F("C") : F(" "));
  Serial.print((mod.bmRightShift == 1) ? F("S") : F(" "));
  Serial.print((mod.bmRightAlt == 1) ? F("A") : F(" "));
  Serial.println((mod.bmRightGUI == 1) ? F("G") : F(" "));
};

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key) {
  Serial.print(F("DN "));
  PrintKey(mod, key);
  uint8_t c = OemToAscii(mod, key);

  if (c)
    OnKeyPressed(c);
}

void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after) {
  MODIFIERKEYS beforeMod;
  *((uint8_t*)&beforeMod) = before;

  MODIFIERKEYS afterMod;
  *((uint8_t*)&afterMod) = after;

  if (beforeMod.bmLeftCtrl != afterMod.bmLeftCtrl)
    Serial.println(F("LeftCtrl changed"));
  if (beforeMod.bmLeftShift != afterMod.bmLeftShift)
    Serial.println(F("LeftShift changed"));
  if (beforeMod.bmLeftAlt != afterMod.bmLeftAlt)
    Serial.println(F("LeftAlt changed"));
  if (beforeMod.bmLeftGUI != afterMod.bmLeftGUI)
    Serial.println(F("LeftGUI changed"));

  if (beforeMod.bmRightCtrl != afterMod.bmRightCtrl)
    Serial.println(F("RightCtrl changed"));
  if (beforeMod.bmRightShift != afterMod.bmRightShift)
    Serial.println(F("RightShift changed"));
  if (beforeMod.bmRightAlt != afterMod.bmRightAlt)
    Serial.println(F("RightAlt changed"));
  if (beforeMod.bmRightGUI != afterMod.bmRightGUI)
    Serial.println(F("RightGUI changed"));
}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key) {
  Serial.print(F("UP "));
  PrintKey(mod, key);
}

void KbdRptParser::OnKeyPressed(uint8_t key) {
  Serial.print(F("ASCII: "));
  Serial.println((char)key);
};

#endif
