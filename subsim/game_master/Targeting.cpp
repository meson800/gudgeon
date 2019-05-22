#include "Targeting.h"

#include "math.h"

bool chooseTarget(
    int64_t x, int64_t y, int16_t heading,
    int16_t maxAngle, int16_t maxDist,
    const std::map<uint32_t, std::vector<UnitState>> &candidates,
    uint32_t *chosenTeamOut, uint32_t *chosenUnitOut)
{
    bool chosen = false;
    int16_t bestDist;
    for (const auto &teamPair : candidates) {
        for (const UnitState &u : teamPair.second) {
            if (u.isStealth && u.stealthCooldown == 0 && !u.hasFlag)
            {
                // skip stealthed units without flags
                continue;
            }
            int16_t candidateDist = sqrt((u.x-x)*(u.x-x) + (u.y-y)*(u.y-y));
            int16_t candidateHeading = atan2(u.y-y, u.x-x) / (2*M_PI/360.0);

            if (candidateDist > maxDist) continue;
            if (candidateDist == 0) continue;

            int16_t angle = candidateHeading - heading;
            if (angle > 180) angle -= 360;
            if (angle < -180) angle += 360;
            if (abs(angle) > maxAngle) continue;

            if (!chosen || candidateDist < bestDist) {
                chosen = true;
                *chosenTeamOut = u.team;
                *chosenUnitOut = u.unit;
                bestDist = candidateDist;
            }
        }
    }

    return chosen;
}

int16_t aimAtTarget(int64_t x, int64_t y, const UnitState &target, const Config &config)
{
    // Projected position of the target
    int64_t targX = target.x;
    int64_t targY = target.y;
    int16_t targHeading = target.heading;

    // Projected distance the torpedo has traveled
    int16_t r = 0;

    // Update target position and increase "r" until the torpedo could have hit
    // the target
    while ((x-targX)*(x-targX) + (y-targY)*(y-targY) > r*r) {
        r += config.torpedoSpeed;

        if (target.direction == UnitState::SteeringDirection::Right)
        {
            int32_t newHeading = static_cast<int32_t>(targHeading) - config.subTurningSpeed;
            targHeading = newHeading < 0 ? newHeading + 360 : newHeading;
        }
        if (target.direction == UnitState::SteeringDirection::Left)
        {
            int32_t newHeading = static_cast<int32_t>(targHeading) + config.subTurningSpeed;
            targHeading = newHeading > 360 ? newHeading - 360 : newHeading;
        }
        targX += target.speed * cos(targHeading * 2*M_PI/360.0);
        targY += target.speed * sin(targHeading * 2*M_PI/360.0);

        // If it takes too long to find a targeting solution, give up. In theory
        // this should never happen, but in practice let's avoid an infinite
        // loop.
        if (r > config.sonarRange * 2) {
            break;
        }
    }

    return atan2(targY-y, targX-x) / (2*M_PI/360.0);
}

