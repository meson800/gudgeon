#pragma once
#include "../common/SimulationEvents.h"

/// Helper functions for aiming torpedos

/// Looks at all the units in the list, and picks the one we are "pointing at".
/// If a unit is found, returns true and sets chosenTeamOut and chosenUnitOut.
bool chooseTarget(
    int64_t x, int64_t y, int16_t heading,
    int16_t maxAngle, int16_t maxDist,
    const std::map<uint32_t, std::vector<UnitState>> &candidates,
    uint32_t *chosenTeamOut, uint32_t *chosenUnitOut);

/// If a sub located at (x, y) wants to fire a torpedo and hit the given target,
/// aimAtTarget() returns the heading the torpedo should be fired at, accounting
/// for the target's movement.
int16_t aimAtTarget(int64_t x, int64_t y, const UnitState &target, const Config &config);
