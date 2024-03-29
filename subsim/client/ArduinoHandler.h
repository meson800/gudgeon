#pragma once
#include "../common/SimulationEvents.h"

class ArduinoHandler : public EventReceiver
{
public:
    ArduinoHandler(uint32_t team_, uint32_t unit_);
    ~ArduinoHandler();

    HandleResult handleUnitState(UnitState* state);

private:
    uint32_t team;
    uint32_t unit;

    /// Thread for the serial communications loop
    std::thread loopThread;

    /// Shutdown flag atomic
    std::atomic<bool> shouldShutdown;

    /// Mutex to protect lastState
    std::mutex stateMux;

    UnitState lastState;

    // These structs must be kept in sync with the corresponding structs in the
    // Arduino code
    struct Display {
      uint8_t tubeOccupancy[5];
    } __attribute__((packed));
    struct Control {
      uint8_t throttle;
      uint8_t steer;
      uint8_t stealth;
      uint8_t tubeArmed[5];
      uint8_t tubeLoadTorpedo[5];
      uint8_t tubeLoadMine[5];
      uint8_t fire;
      uint32_t debugValue;
    } __attribute__((packed));

    Display disp;
    Control cont, lastCont;

    bool openSerialPort();

    void runLoop();

    static const int inputBufSize = (sizeof(Control) + 1) * 2;
    uint8_t inputBuf[inputBufSize];
    int inputPos;
    uint8_t inputChecksum;

    int serialPortFD;

    bool receiveInput();
    int receiveInputChar();
    void sendOutput();
    void sendOutputChar(uint8_t c);
};
