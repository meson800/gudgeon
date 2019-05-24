#include "ArduinoHandler.h"

#include "../common/Messages.h"
#include "../common/Log.h"
#include <memory.h>
#include <termios.h>

ArduinoHandler::ArduinoHandler(uint32_t team_, uint32_t unit_)
    : EventReceiver({
        dispatchEvent<ArduinoHandler, UnitState, &ArduinoHandler::handleUnitState>,
    })
    , team(team_)
    , unit(unit_)
    , shouldShutdown(false)
    , inputPos(0)
    , inputChecksum(0)
    , serialPortFD(-1)
{
    if (openSerialPort())
    {
        loopThread = std::thread(&ArduinoHandler::runLoop, this);
    }
    else
    {
        Log::writeToLog(Log::ERR, "Could not connect to Arduino; falling back to keyboard control.");
    }
}

bool ArduinoHandler::openSerialPort()
{
    const char *serialPortPath = "/dev/ttyACM0";
    Log::writeToLog(Log::WARN, "Arduino handler connecting to Arduino at ", serialPortPath);
    serialPortFD = open(serialPortPath, O_RDWR|O_NONBLOCK);
    if (serialPortFD == -1)
    {
        Log::writeToLog(Log::WARN, "Arduino handler: open() failed: ", strerror(errno));
        return false;
    }

    /* shamelessly stolen from:
    https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
    */

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(serialPortFD, &tty) != 0)
    {
        Log::writeToLog(Log::WARN, "Arduino handler: tcgetattr() failed: ", strerror(errno));
        return false;
    }

    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(serialPortFD, TCSANOW, &tty) != 0)
    {
        Log::writeToLog(Log::WARN, "Arduino handler: tcsetattr() failed: ", strerror(errno));
        return false;
    }

    /* Make sure we can receive a valid Control from the Arduino */
    int attempts = 0;
    while (!receiveInput())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (attempts++ > 100)
        {
            Log::writeToLog(Log::WARN, "Arduino handler: did not receive input from Arduino within 1 second");
            return false;
        }
    }

    /* Make sure we can send a valid Display to the Arduino */
    memset(&disp, 0, sizeof(disp));
    try
    {
        sendOutput();
    }
    catch (std::runtime_error)
    {
        return false;
    }

    return true;
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
    close(serialPortFD);
}

HandleResult ArduinoHandler::handleUnitState(UnitState* state)
{
    std::lock_guard<std::mutex> lock(stateMux);
    if (state->team == team && state->unit == unit)
    {
        lastState = *state;
    }
    return HandleResult::Continue;
}

void ArduinoHandler::runLoop()
{
    Log::writeToLog(Log::INFO, "Arduino thread started");
    while (!shouldShutdown)
    {
        {
            std::lock_guard<std::mutex> lock(stateMux);
            for (int tube = 0; tube < lastState.tubeOccupancy.size(); ++tube)
            {
                disp.tubeOccupancy[tube] = lastState.tubeOccupancy[tube];
            }
        }
        sendOutput();

        receiveInput();
        for (int tube = 0; tube < 5; ++tube)
        {
            if (cont.tubeArmed[tube] != lastCont.tubeArmed[tube])
            {
                TubeArmEvent tubeArm;
                tubeArm.team = team;
                tubeArm.unit = unit;
                tubeArm.tube = tube;
                tubeArm.isArmed = cont.tubeArmed[tube];
                EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(tubeArm));
            }
            if (cont.tubeLoadTorpedo[tube] && !lastCont.tubeLoadTorpedo[tube])
            {
                TubeLoadEvent tubeLoad;
                tubeLoad.team = team;
                tubeLoad.unit = unit;
                tubeLoad.tube = tube;
                tubeLoad.type = TubeLoadEvent::AmmoType::Torpedo;
                EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(tubeLoad));
            }
            if (cont.tubeLoadMine[tube] && !lastCont.tubeLoadMine[tube])
            {
                TubeLoadEvent tubeLoad;
                tubeLoad.team = team;
                tubeLoad.unit = unit;
                tubeLoad.tube = tube;
                tubeLoad.type = TubeLoadEvent::AmmoType::Mine;
                EventSystem::getGlobalInstance()->queueEvent(EnvelopeMessage(tubeLoad));
            }
        }
        Log::writeToLog(Log::L_DEBUG, "Arduino debugValue:", cont.debugValue);
        lastCont = cont;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

bool ArduinoHandler::receiveInput() {
  bool gotAMessage = false;
  Log::writeToLog(Log::L_DEBUG, "Arduino begin receiveInput()");
  while (true) {
    int c = receiveInputChar();
    Log::writeToLog(Log::L_DEBUG, "Serial port character: ", c);
    if (c == EOF) {
      break;
    } else if (c == '[') {
      inputPos = 0;
      inputChecksum = 0;
    } else if (c == ']') {
      if (inputPos == inputBufSize && inputChecksum == 0) {
        memcpy(&cont, inputBuf, sizeof(Control));
        gotAMessage = true;
        Log::writeToLog(Log::L_DEBUG, "Received a Control from Arduino");
      } else {
        if (inputPos < inputBufSize) {
          Log::writeToLog(Log::ERR, "Arduino sent too-small message");
        }
        if (inputChecksum != 0) {
          Log::writeToLog(Log::ERR, "Arduino sent message with incorrect checksum");
        }
      }
      inputPos = inputBufSize + 1;
    } else {
      int h = parseHex(c);
      if (h == -1) {
        Log::writeToLog(Log::ERR, "Arduino sent unrecognized character");
        inputPos = inputBufSize + 1;
        continue;
      }
      if (inputPos == inputBufSize) {
        Log::writeToLog(Log::ERR, "Arduino sent too-large message");
        inputPos = inputBufSize + 1;
        continue;
      }
      if (inputPos > inputBufSize) {
        inputPos = inputBufSize + 1;
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
  Log::writeToLog(Log::L_DEBUG, "Arduino end receiveInput()");
  return gotAMessage;
}

int ArduinoHandler::receiveInputChar() {
  char c;
  int res = read(serialPortFD, &c, 1);
  if (res == 1) {
    return c;
  } else if ((res == -1 && errno == EAGAIN) || res == 0) {
    return -1;
  } else {
    Log::writeToLog(Log::ERR, "Arduino read() failed: ", strerror(errno));
    return -1;
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
  Log::writeToLog(Log::L_DEBUG, "Arduino begin sendOutput()");
  sendOutputChar('[');
  uint8_t checksum = 0;
  const uint8_t *outputBuf = (const uint8_t *)&disp;
  for (int i = 0; i < sizeof(Display); ++i) {
    sendOutputChar(generateHex((outputBuf[i] >> 4) & 0x0F));
    sendOutputChar(generateHex(outputBuf[i] & 0x0F));
    checksum ^= outputBuf[i];
  }
  sendOutputChar(generateHex((checksum >> 4) & 0x0F));
  sendOutputChar(generateHex(checksum & 0x0F));
  sendOutputChar(']');
  Log::writeToLog(Log::L_DEBUG, "Arduino end sendOutput()");
}

void ArduinoHandler::sendOutputChar(uint8_t c) {
  int tryCount = 0;
  while (true) {
    int res = write(serialPortFD, &c, 1);
    if (res == 1) {
      return;
    } else if ((res == -1 && errno == EAGAIN) || res == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      ++tryCount;
      if (tryCount > 1000) {
        Log::writeToLog(Log::ERR, "Arduino write() blocked for a very long time");
        throw std::runtime_error("can't write to arduino");
      }
    } else {
      Log::writeToLog(Log::ERR, "Arduino write() failed: ", strerror(errno));
      throw std::runtime_error("can't write to arduino");
    }
  }
}
