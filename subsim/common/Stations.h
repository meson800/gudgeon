#pragma once

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
