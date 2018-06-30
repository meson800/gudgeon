#pragma once

#include "MessageIdentifiers.h"

enum MessageType
{
    ID_VERSION = ID_USER_PACKET_ENUM + 1,
    ID_VERSION_MISMATCH
};
