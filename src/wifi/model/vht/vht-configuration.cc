/*
 * Copyright (c) 2018  Sébastien Deronne
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "vht-configuration.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/tuple.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("VhtConfiguration");

NS_OBJECT_ENSURE_REGISTERED(VhtConfiguration);

VhtConfiguration::VhtConfiguration()
{
    NS_LOG_FUNCTION(this);
}

VhtConfiguration::~VhtConfiguration()
{
    NS_LOG_FUNCTION(this);
}

TypeId
VhtConfiguration::GetTypeId()
{
    static ns3::TypeId tid =
        ns3::TypeId("ns3::VhtConfiguration")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddConstructor<VhtConfiguration>()
            .AddAttribute("Support160MHzOperation",
                          "Whether or not 160 MHz operation is to be supported.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&VhtConfiguration::Get160MHzOperationSupported,
                                              &VhtConfiguration::Set160MHzOperationSupported),
                          MakeBooleanChecker())
            .AddAttribute("SecondaryCcaSensitivityThresholds",
                          "Tuple {threshold for 20MHz PPDUs, threshold for 40MHz PPDUs, threshold "
                          "for 80MHz PPDUs} "
                          "describing the CCA sensitivity thresholds for PPDUs that do not occupy "
                          "the primary channel. "
                          "The power of a received PPDU that does not occupy the primary channel "
                          "should be higher than "
                          "the threshold (dBm) associated to the PPDU bandwidth to allow the PHY "
                          "layer to declare CCA BUSY state.",
                          StringValue("{-72.0, -72.0, -69.0}"),
                          MakeTupleAccessor<DoubleValue, DoubleValue, DoubleValue>(
                              &VhtConfiguration::SetSecondaryCcaSensitivityThresholds,
                              &VhtConfiguration::GetSecondaryCcaSensitivityThresholds),
                          MakeTupleChecker<DoubleValue, DoubleValue, DoubleValue>(
                              MakeDoubleChecker<double>(),
                              MakeDoubleChecker<double>(),
                              MakeDoubleChecker<double>()));
    return tid;
}

void
VhtConfiguration::Set160MHzOperationSupported(bool enable)
{
    NS_LOG_FUNCTION(this << enable);
    m_160MHzSupported = enable;
}

bool
VhtConfiguration::Get160MHzOperationSupported() const
{
    return m_160MHzSupported;
}

void
VhtConfiguration::SetSecondaryCcaSensitivityThresholds(
    const SecondaryCcaSensitivityThresholds& thresholds)
{
    NS_LOG_FUNCTION(this);
    m_secondaryCcaSensitivityThresholds[20] = std::get<0>(thresholds);
    m_secondaryCcaSensitivityThresholds[40] = std::get<1>(thresholds);
    m_secondaryCcaSensitivityThresholds[80] = std::get<2>(thresholds);
}

VhtConfiguration::SecondaryCcaSensitivityThresholds
VhtConfiguration::GetSecondaryCcaSensitivityThresholds() const
{
    return {m_secondaryCcaSensitivityThresholds.at(20),
            m_secondaryCcaSensitivityThresholds.at(40),
            m_secondaryCcaSensitivityThresholds.at(80)};
}

const std::map<uint16_t, double>&
VhtConfiguration::GetSecondaryCcaSensitivityThresholdsPerBw() const
{
    return m_secondaryCcaSensitivityThresholds;
}

} // namespace ns3
