/*
 * This is the include file for the hardware interface. Note that this
 * will also define the types of IO Boards
 */

#pragma once

/* Includes */
#include <usb.h> /* this is libusb */

#include "usbapi/opendevice.h"
#include "SimulationEvents.h"

class HardwareInterface {

public:

    /* Constructor */
    HardwareInterface();

    /* Open USB Interface */


    /* SendUnitStateToHardware
     * Sends a unit state to the hardware controller and returns [TODO]
     */
    UnitState SendUnitStateToHardware(UnitState state);

private:
    U

};

