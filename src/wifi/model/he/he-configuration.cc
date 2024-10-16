/*
 * Copyright (c) 2018 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "he-configuration.h"

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("HeConfiguration");
NS_OBJECT_ENSURE_REGISTERED(HeConfiguration);

HeConfiguration::HeConfiguration()
{
    NS_LOG_FUNCTION(this);
}

TypeId
HeConfiguration::GetTypeId()
{
    static ns3::TypeId tid =
        ns3::TypeId("ns3::HeConfiguration")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddConstructor<HeConfiguration>()
            .AddAttribute("GuardInterval",
                          "Specify the shortest guard interval duration that can be used for HE "
                          "transmissions."
                          "Possible values are 800ns, 1600ns or 3200ns.",
                          TimeValue(NanoSeconds(3200)),
                          MakeTimeAccessor(&HeConfiguration::GetGuardInterval,
                                           &HeConfiguration::SetGuardInterval),
                          MakeTimeChecker(NanoSeconds(800), NanoSeconds(3200)))
            .AddAttribute("BssColor",
                          "The BSS color",
                          UintegerValue(0),
                          MakeUintegerAccessor(&HeConfiguration::m_bssColor),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("MaxTbPpduDelay",
                          "If positive, the value of this attribute specifies the maximum "
                          "delay with which a TB PPDU can be received after the reception of "
                          "the first TB PPDU. If the delay is higher than this value, the "
                          "TB PPDU is dropped and considered interference. The maximum delay "
                          "is anyway capped at the duration of the training fields in the PPDU. "
                          "This attribute is only valid for APs.",
                          TimeValue(Seconds(0)),
                          MakeTimeAccessor(&HeConfiguration::m_maxTbPpduDelay),
                          MakeTimeChecker(Seconds(0)))
            .AddAttribute("MuBeAifsn",
                          "AIFSN used by BE EDCA when the MU EDCA Timer is running. "
                          "It must be either zero (EDCA disabled) or a value from 2 to 15.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&HeConfiguration::m_muBeAifsn),
                          MakeUintegerChecker<uint8_t>(0, 15))
            .AddAttribute("MuBkAifsn",
                          "AIFSN used by BK EDCA when the MU EDCA Timer is running. "
                          "It must be either zero (EDCA disabled) or a value from 2 to 15.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&HeConfiguration::m_muBkAifsn),
                          MakeUintegerChecker<uint8_t>(0, 15))
            .AddAttribute("MuViAifsn",
                          "AIFSN used by VI EDCA when the MU EDCA Timer is running. "
                          "It must be either zero (EDCA disabled) or a value from 2 to 15.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&HeConfiguration::m_muViAifsn),
                          MakeUintegerChecker<uint8_t>(0, 15))
            .AddAttribute("MuVoAifsn",
                          "AIFSN used by VO EDCA when the MU EDCA Timer is running. "
                          "It must be either zero (EDCA disabled) or a value from 2 to 15.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&HeConfiguration::m_muVoAifsn),
                          MakeUintegerChecker<uint8_t>(0, 15))
            .AddAttribute("MuBeCwMin",
                          "CWmin used by BE EDCA when the MU EDCA Timer is running. "
                          "It must be a power of 2 minus 1 in the range from 0 to 32767.",
                          UintegerValue(15),
                          MakeUintegerAccessor(&HeConfiguration::m_muBeCwMin),
                          MakeUintegerChecker<uint16_t>(0, 32767))
            .AddAttribute("MuBkCwMin",
                          "CWmin used by BK EDCA when the MU EDCA Timer is running. "
                          "It must be a power of 2 minus 1 in the range from 0 to 32767.",
                          UintegerValue(15),
                          MakeUintegerAccessor(&HeConfiguration::m_muBkCwMin),
                          MakeUintegerChecker<uint16_t>(0, 32767))
            .AddAttribute("MuViCwMin",
                          "CWmin used by VI EDCA when the MU EDCA Timer is running. "
                          "It must be a power of 2 minus 1 in the range from 0 to 32767.",
                          UintegerValue(15),
                          MakeUintegerAccessor(&HeConfiguration::m_muViCwMin),
                          MakeUintegerChecker<uint16_t>(0, 32767))
            .AddAttribute("MuVoCwMin",
                          "CWmin used by VO EDCA when the MU EDCA Timer is running. "
                          "It must be a power of 2 minus 1 in the range from 0 to 32767.",
                          UintegerValue(15),
                          MakeUintegerAccessor(&HeConfiguration::m_muVoCwMin),
                          MakeUintegerChecker<uint16_t>(0, 32767))
            .AddAttribute("MuBeCwMax",
                          "CWmax used by BE EDCA when the MU EDCA Timer is running. "
                          "It must be a power of 2 minus 1 in the range from 0 to 32767.",
                          UintegerValue(1023),
                          MakeUintegerAccessor(&HeConfiguration::m_muBeCwMax),
                          MakeUintegerChecker<uint16_t>(0, 32767))
            .AddAttribute("MuBkCwMax",
                          "CWmax used by BK EDCA when the MU EDCA Timer is running. "
                          "It must be a power of 2 minus 1 in the range from 0 to 32767.",
                          UintegerValue(1023),
                          MakeUintegerAccessor(&HeConfiguration::m_muBkCwMax),
                          MakeUintegerChecker<uint16_t>(0, 32767))
            .AddAttribute("MuViCwMax",
                          "CWmax used by VI EDCA when the MU EDCA Timer is running. "
                          "It must be a power of 2 minus 1 in the range from 0 to 32767.",
                          UintegerValue(1023),
                          MakeUintegerAccessor(&HeConfiguration::m_muViCwMax),
                          MakeUintegerChecker<uint16_t>(0, 32767))
            .AddAttribute("MuVoCwMax",
                          "CWmax used by VO EDCA when the MU EDCA Timer is running. "
                          "It must be a power of 2 minus 1 in the range from 0 to 32767.",
                          UintegerValue(1023),
                          MakeUintegerAccessor(&HeConfiguration::m_muVoCwMax),
                          MakeUintegerChecker<uint16_t>(0, 32767))
            .AddAttribute("BeMuEdcaTimer",
                          "The MU EDCA Timer used by BE EDCA. It must be a multiple of "
                          "8192 us and must be in the range from 8.192 ms to 2088.96 ms. "
                          "0 is a reserved value, but we allow to use this value to indicate "
                          "that an MU EDCA Parameter Set element must not be sent. Therefore, "
                          "0 can only be used if the MU EDCA Timer for all ACs is set to 0.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&HeConfiguration::m_beMuEdcaTimer),
                          MakeTimeChecker(MicroSeconds(0), MicroSeconds(2088960)))
            .AddAttribute("BkMuEdcaTimer",
                          "The MU EDCA Timer used by BK EDCA. It must be a multiple of "
                          "8192 us and must be in the range from 8.192 ms to 2088.96 ms."
                          "0 is a reserved value, but we allow to use this value to indicate "
                          "that an MU EDCA Parameter Set element must not be sent. Therefore, "
                          "0 can only be used if the MU EDCA Timer for all ACs is set to 0.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&HeConfiguration::m_bkMuEdcaTimer),
                          MakeTimeChecker(MicroSeconds(0), MicroSeconds(2088960)))
            .AddAttribute("ViMuEdcaTimer",
                          "The MU EDCA Timer used by VI EDCA. It must be a multiple of "
                          "8192 us and must be in the range from 8.192 ms to 2088.96 ms."
                          "0 is a reserved value, but we allow to use this value to indicate "
                          "that an MU EDCA Parameter Set element must not be sent. Therefore, "
                          "0 can only be used if the MU EDCA Timer for all ACs is set to 0.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&HeConfiguration::m_viMuEdcaTimer),
                          MakeTimeChecker(MicroSeconds(0), MicroSeconds(2088960)))
            .AddAttribute("VoMuEdcaTimer",
                          "The MU EDCA Timer used by VO EDCA. It must be a multiple of "
                          "8192 us and must be in the range from 8.192 ms to 2088.96 ms."
                          "0 is a reserved value, but we allow to use this value to indicate "
                          "that an MU EDCA Parameter Set element must not be sent. Therefore, "
                          "0 can only be used if the MU EDCA Timer for all ACs is set to 0.",
                          TimeValue(MicroSeconds(0)),
                          MakeTimeAccessor(&HeConfiguration::m_voMuEdcaTimer),
                          MakeTimeChecker(MicroSeconds(0), MicroSeconds(2088960)));
    return tid;
}

void
HeConfiguration::SetGuardInterval(Time guardInterval)
{
    NS_LOG_FUNCTION(this << guardInterval);
    [[maybe_unused]] const auto gi = guardInterval.GetNanoSeconds();
    NS_ASSERT((gi == 800) || (gi == 1600) || (gi == 3200));
    m_guardInterval = guardInterval;
}

Time
HeConfiguration::GetGuardInterval() const
{
    return m_guardInterval;
}

void
HeConfiguration::SetBssColor(uint8_t bssColor)
{
    NS_LOG_FUNCTION(this << +bssColor);
    m_bssColor = bssColor;
}

uint8_t
HeConfiguration::GetBssColor() const
{
    return m_bssColor;
}

Time
HeConfiguration::GetMaxTbPpduDelay() const
{
    return m_maxTbPpduDelay;
}

void
HeConfiguration::SetMaxTbPpduDelay(Time maxTbPpduDelay)
{
    m_maxTbPpduDelay = maxTbPpduDelay;
}

} // namespace ns3
