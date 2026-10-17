#include "wifi-ns3-constants.h"

namespace ns3
{
Time
DEFAULT_BEACON_INTERVAL()
{
    static Time beaconInterval = MicroSeconds(102400);
    return beaconInterval;
}
} // namespace ns3
