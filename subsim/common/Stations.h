#pragma once

#include <map>
#include <string>

/*!
 * Stores the different possible stations as numeric values
 */
enum StationType
{
    Unassigned = 0,
    Helm
};

/*!
 * Static array that stores the string version of a station
 */
static const char* StationNames[] = {"Unassigned", "Helm"};

/*!
 * Static map that allows for lookups of StationType by a string
 */
static const std::map<std::string, StationType> StationTypeLookup = {
{"helm", StationType::Helm}};
