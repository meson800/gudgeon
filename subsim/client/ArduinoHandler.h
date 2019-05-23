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

    UnitState lastState;

    /// Thread for the serial communications loop
    std::thread loopThread;

    /// Shutdown flag atomic
    std::atomic<bool> shouldShutdown;

    /// Mutex to protect internal stsate
    std::mutex stateMux;

    struct Display {
      uint32_t value;
    } disp;
    struct Control {
      uint32_t value;
    } cont;

    void runLoop();

    static const int inputBufSize = (sizeof(Control) + 1) * 2;
    uint8_t inputBuf[inputBufSize];
    int inputPos;
    uint8_t inputChecksum;

    FILE* serialPort;

    void receiveInput();
    void sendOutput();
};
