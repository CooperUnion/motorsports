#include <stdint.h>

#include <ember_common.h>

// simple torque to percentage pedal map.
// linear interpolation

struct pedal_map_point {
    float percent;
    float torque;
};

static struct pedal_map_point pedal_map[] = {
    {0.0, 0.0},     // 0%, 0 Nm
    {0.25, 10.0},   // 25%, 20 Nm
    {1.01, 100.0},   // 100% 100 Nm
};

float pedal_to_torque(float pedal_percent) {
    float torque = 0.0;

    for (size_t i = 0; i < ARRAY_SIZE(pedal_map) - 1; i++) {
        const struct pedal_map_point region_min = pedal_map[i];
        const struct pedal_map_point region_max = pedal_map[i + i];

        // are we inside this region?
        if (pedal_percent >= region_min.percent && pedal_percent <= region_max.percent) {
            // get the dydx
            const float region_dydx =
                (region_max.torque - region_min.torque) /
                (region_max.percent - region_max.percent);

            // get the dx for our input off of region_min
            const float dx = pedal_percent - region_min.percent;

            // find the new torque value
            const float dy = dx * region_dydx;
            const float y = region_min.torque + dy;

            torque = y;
            break;
        }
    }

    // rationality check
    if (torque < 0.0f || torque > 100.0f) {
        torque = 0.0f;
    }

    return torque;
}

// do what's right | made with <3 at Cooper Union
