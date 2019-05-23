/*
 * Definitions for requests for the system
 */

#pragma once

typedef enum request {

    /* Simple Echo For Testing */
    ECHO                      = 0,
    /* Check if IO Boards Okay */
    CHK_ALL_IOBOARDS          = 1,

    /* Update Outputs */
    UPDATE_XCOORD_DISP        = 2,
    UPDATE_YCOORD_DISP        = 3,
    UPDATE_DEPTH_DISP         = 4,
    UPDATE_HEADING_DISP       = 5,
    UPDATE_PITCH_DISP         = 6,
    UPDATE_POWER_DISP         = 7,
    UPDATE_SPEED_DISP         = 8,
    UPDATE_LOAD_TORPEDO_LIGHT = 9,
    UPDATE_LOAD_MINE_LIGHT    = 10,
    UPDATE_TORPEDO_COUNT_DISP = 11,
    UPDATE_MINE_COUNT_DISP    = 12,
    UPDATE_RANGE_DISP         = 13,
    UPDATE_LED_LIGHTS         = 14,
    UPDATE_FIRE_BUTTON_LIGHT  = 15,

    /* Get Status */
    GET_TRIM_ROTATIONS        = 16,
    GET_LEFT_PEDAL            = 17,
    GET_RIGHT_PEDAL           = 18,
    GET_SPEED_SLIDER          = 19,
    GET_SYSTEM_TOGGLES        = 20,
    GET_TUBE_ARM_STATUS       = 21,
    GET_TORPEDO_ARM_STATUS    = 22,
    GET_MINE_ARM_STATUS       = 23,
    GET_ACTIVE_PASSIVE        = 24,
    GET_RANGE_ROTATIONS       = 25,
    GET_FIRE_BUTTON_STATUS    = 26,

    /* Control Flow */
    ACKNOWLEGED               = 0xA1,
    ACK_WITH_RESPONSE         = 0xA2

} request_t;
