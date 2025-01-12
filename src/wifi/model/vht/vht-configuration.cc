/*
 * Copyright (c) 2018  Sébastien Deronne
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "vht-configuration.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/tuple.h"
#include "ns3/units-energy.h"

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
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          BooleanValue(true),
                          MakeBooleanAccessor(&VhtConfiguration::m_160MHzSupported),
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
                          StringValue("{-72.0dBm, -72.0dBm, -69.0dBm}"),
                          MakeTupleAccessor<dBmValue, dBmValue, dBmValue>(
                              &VhtConfiguration::SetSecondaryCcaSensitivityThresholds,
                              &VhtConfiguration::GetSecondaryCcaSensitivityThresholds),
                          MakeTupleChecker<dBmValue, dBmValue, dBmValue>(MakedBmChecker(),
                                                                         MakedBmChecker(),
                                                                         MakedBmChecker()));
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
    m_secondaryCcaSensitivityThresholds[MHz_t{20}] = std::get<0>(thresholds);
    m_secondaryCcaSensitivityThresholds[MHz_t{40}] = std::get<1>(thresholds);
    m_secondaryCcaSensitivityThresholds[MHz_t{80}] = std::get<2>(thresholds);
}

VhtConfiguration::SecondaryCcaSensitivityThresholds
VhtConfiguration::GetSecondaryCcaSensitivityThresholds() const
{
    return {m_secondaryCcaSensitivityThresholds.at(MHz_t{20}),
            m_secondaryCcaSensitivityThresholds.at(MHz_t{40}),
            m_secondaryCcaSensitivityThresholds.at(MHz_t{80})};
}

const std::map<MHz_t, dBm_t>&
VhtConfiguration::GetSecondaryCcaSensitivityThresholdsPerBw() const
{
    return m_secondaryCcaSensitivityThresholds;
}

} // namespace ns3
