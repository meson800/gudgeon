#pragma once

class ArduinoHandler : public EventHandler
{
private:
  static const int inputBufSize = (sizeof(Input) + 1) * 2;
  uint8_t inputBuf[inputBufSize];
  int inputPos;
  uint8_t inputChecksum;
}
