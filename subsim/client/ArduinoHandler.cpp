#include "ArduinoHandler.h"

#include "../common/Log.h"
#include <memory.h>

ArduinoHandler::ArduinoHandler()
    : EventReceiver({})
    , shouldShutdown(false)
    , inputPos(0)
    , inputChecksum(0)
{
    const char *serialPortPath = "/dev/ttyACM0";
    serialPort = fopen(serialPortPath, "w+");
    if (serialPort == NULL)
    {
        Log::writeToLog(Log::WARN, "Arduino handler failed to connect to Arduino!",
            " Device path: ", serialPortPath,
            " Error code: ", strerror(errno));
    }
    else
    {
        loopThread = std::thread(&ArduinoHandler::runLoop, this);
    }
}

ArduinoHandler::~ArduinoHandler()
{
    Log::writeToLog(Log::INFO, "Arduino handler shutting down the Arduino thread...");
    shouldShutdown = true;
    if (loopThread.joinable())
    {
        loopThread.join();
    }
    Log::writeToLog(Log::INFO, "Arduino thread shutdown successfully.");
    fclose(serialPort);
}

void ArduinoHandler::runLoop()
{
    while (true)
    {
        disp.value = 11;
        Log::writeToLog(Log::L_DEBUG, "value received from Arduino:", cont.value);
        receiveInput();
        sendOutput();
    }
}

int parseHex(int character) {
  if (character >= '0' && character <= '9') {
    return character - '0';
  } else if (character >= 'a' && character <= 'f') {
    return 10 + character - 'a';
  } else if (character >= 'A' && character <= 'F') {
    return 10 + character - 'A';
  } else {
    return -1;
  }
}

void ArduinoHandler::receiveInput() {
  while (true) {
    int c = fgetc(serialPort);
    if (c == EOF) {
      return;
    } else if (c == '[') {
      inputPos = 0;
      inputChecksum = 0;
    } else if (c == ']') {
      if (inputPos == inputBufSize && inputChecksum == 0) {
        memcpy(&cont, inputBuf, sizeof(Control));
        Log::writeToLog(Log::L_DEBUG, "Received a Control from Arduino");
      }
      inputPos = sizeof(Control) * 2;
    } else {
      int h = parseHex(c);
      if (h == -1 || inputPos == inputBufSize) {
        inputPos = sizeof(Control) * 2;
        continue;
      }
      int bytePos = inputPos >> 1;
      int nibblePos = inputPos & 0x01;
      if (nibblePos == 0) {
        inputBuf[bytePos] = h << 4;
      } else {
        inputBuf[bytePos] |= h;
        inputChecksum ^= inputBuf[bytePos];
      }
      inputPos += 1;
    }
  }
}

int generateHex(int value) {
  if (value < 10) {
    return '0' + value;
  } else {
    return 'A' + (value - 10);
  }
}

void ArduinoHandler::sendOutput() {
  fputc('[', serialPort);
  uint8_t checksum = 0;
  const uint8_t *outputBuf = (const uint8_t *)&disp;
  for (int i = 0; i < sizeof(Display); ++i) {
    fputc(generateHex((outputBuf[i] >> 4) & 0x0F), serialPort);
    fputc(generateHex(outputBuf[i] & 0x0F), serialPort);
    checksum ^= outputBuf[i];
  }
  fputc(generateHex((checksum >> 4) & 0x0F), serialPort);
  fputc(generateHex(checksum & 0x0F), serialPort);
  fputc(']', serialPort);
}
