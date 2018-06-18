#include <iostream>

#include "version.h"

int main()
{
    std::cerr << "Subsim client version v" << VERSION_MAJOR << "." << VERSION_MINOR << " started\n";
    return 0;
}
