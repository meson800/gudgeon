#include <iostream>

#include "version.h"

#include "../common/Network.h"

int main()
{
    std::cerr << "Subsim client version v" << VERSION_MAJOR << "." << VERSION_MINOR << " started\n";

    Network network;
    return 0;
}
