/*
 * Copyright (c) 2021
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

#include "wifi-phy-common.h"

#include "wifi-mode.h"
#include "wifi-net-device.h"

#include "ns3/he-configuration.h"
#include "ns3/ht-configuration.h"

namespace ns3
{

uint16_t
ConvertGuardIntervalToNanoSeconds(WifiMode mode, const Ptr<WifiNetDevice> device)
{
    uint16_t gi = 800;
    if (mode.GetModulationClass() >= WIFI_MOD_CLASS_HE)
    {
        Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration();
        NS_ASSERT(heConfiguration); // If HE/EHT modulation is used, we should have a HE
                                    // configuration attached
        gi = static_cast<uint16_t>(heConfiguration->GetGuardInterval().GetNanoSeconds());
    }
    else if (mode.GetModulationClass() == WIFI_MOD_CLASS_HT ||
             mode.GetModulationClass() == WIFI_MOD_CLASS_VHT)
    {
        Ptr<HtConfiguration> htConfiguration = device->GetHtConfiguration();
        NS_ASSERT(htConfiguration); // If HT/VHT modulation is used, we should have a HT
                                    // configuration attached
        gi = htConfiguration->GetShortGuardIntervalSupported() ? 400 : 800;
    }
    return gi;
}

uint16_t
ConvertGuardIntervalToNanoSeconds(WifiMode mode, bool htShortGuardInterval, Time heGuardInterval)
{
    uint16_t gi;
    if (mode.GetModulationClass() >= WIFI_MOD_CLASS_HE)
    {
        gi = static_cast<uint16_t>(heGuardInterval.GetNanoSeconds());
    }
    else if (mode.GetModulationClass() == WIFI_MOD_CLASS_HT ||
             mode.GetModulationClass() == WIFI_MOD_CLASS_VHT)
    {
        gi = htShortGuardInterval ? 400 : 800;
    }
    else
    {
        gi = 800;
    }
    return gi;
}

WifiPreamble
GetPreambleForTransmission(WifiModulationClass modulation, bool useShortPreamble)
{
    if (modulation == WIFI_MOD_CLASS_EHT)
    {
        return WIFI_PREAMBLE_EHT_MU;
    }
    else if (modulation == WIFI_MOD_CLASS_HE)
    {
        return WIFI_PREAMBLE_HE_SU;
    }
    else if (modulation == WIFI_MOD_CLASS_DMG_CTRL)
    {
        return WIFI_PREAMBLE_DMG_CTRL;
    }
    else if (modulation == WIFI_MOD_CLASS_DMG_SC)
    {
        return WIFI_PREAMBLE_DMG_SC;
    }
    else if (modulation == WIFI_MOD_CLASS_DMG_OFDM)
    {
        return WIFI_PREAMBLE_DMG_OFDM;
    }
    else if (modulation == WIFI_MOD_CLASS_VHT)
    {
        return WIFI_PREAMBLE_VHT_SU;
    }
    else if (modulation == WIFI_MOD_CLASS_HT)
    {
        return WIFI_PREAMBLE_HT_MF; // HT_GF has been removed
    }
    else if (modulation == WIFI_MOD_CLASS_HR_DSSS &&
             useShortPreamble) // ERP_DSSS is modeled through HR_DSSS (since same preamble and
                               // modulation)
    {
        return WIFI_PREAMBLE_SHORT;
    }
    else
    {
        return WIFI_PREAMBLE_LONG;
    }
}

WifiModulationClass
GetModulationClassForPreamble(WifiPreamble preamble)
{
    switch (preamble)
    {
    case WIFI_PREAMBLE_HT_MF:
        return WIFI_MOD_CLASS_HT;
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_VHT_MU:
        return WIFI_MOD_CLASS_VHT;
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_HE_ER_SU:
    case WIFI_PREAMBLE_HE_MU:
    case WIFI_PREAMBLE_HE_TB:
        return WIFI_MOD_CLASS_HE;
    case WIFI_PREAMBLE_EHT_MU:
    case WIFI_PREAMBLE_EHT_TB:
        return WIFI_MOD_CLASS_EHT;
    default:
        NS_ABORT_MSG("Unsupported preamble type: " << preamble);
    }
    return WIFI_MOD_CLASS_UNKNOWN;
}

bool
IsAllowedControlAnswerModulationClass(WifiModulationClass modClassReq,
                                      WifiModulationClass modClassAnswer)
{
    switch (modClassReq)
    {
    case WIFI_MOD_CLASS_DSSS:
        return (modClassAnswer == WIFI_MOD_CLASS_DSSS);
    case WIFI_MOD_CLASS_HR_DSSS:
        return (modClassAnswer == WIFI_MOD_CLASS_DSSS || modClassAnswer == WIFI_MOD_CLASS_HR_DSSS);
    case WIFI_MOD_CLASS_ERP_OFDM:
        return (modClassAnswer == WIFI_MOD_CLASS_DSSS || modClassAnswer == WIFI_MOD_CLASS_HR_DSSS ||
                modClassAnswer == WIFI_MOD_CLASS_ERP_OFDM);
    case WIFI_MOD_CLASS_OFDM:
        return (modClassAnswer == WIFI_MOD_CLASS_OFDM);
    case WIFI_MOD_CLASS_HT:
    case WIFI_MOD_CLASS_VHT:
    case WIFI_MOD_CLASS_HE:
    case WIFI_MOD_CLASS_EHT:
        return true;
    default:
        NS_FATAL_ERROR("Modulation class not defined");
        return false;
    }
}

Time
GetPpduMaxTime(WifiPreamble preamble)
{
    Time duration;

    switch (preamble)
    {
    case WIFI_PREAMBLE_HT_MF:
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_VHT_MU:
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_HE_ER_SU:
    case WIFI_PREAMBLE_HE_MU:
    case WIFI_PREAMBLE_HE_TB:
    case WIFI_PREAMBLE_EHT_MU:
    case WIFI_PREAMBLE_EHT_TB:
        duration = MicroSeconds(5484);
        break;
    default:
        duration = MicroSeconds(0);
        break;
    }
    return duration;
}

bool
IsMu(WifiPreamble preamble)
{
    return (IsDlMu(preamble) || IsUlMu(preamble));
}

bool
IsDlMu(WifiPreamble preamble)
{
    return ((preamble == WIFI_PREAMBLE_HE_MU) || (preamble == WIFI_PREAMBLE_EHT_MU));
}

bool
IsUlMu(WifiPreamble preamble)
{
    return ((preamble == WIFI_PREAMBLE_HE_TB) || (preamble == WIFI_PREAMBLE_EHT_TB));
}

WifiModulationClass
GetModulationClassForStandard(WifiStandard standard)
{
    WifiModulationClass modulationClass{WIFI_MOD_CLASS_UNKNOWN};
    switch (standard)
    {
    case WIFI_STANDARD_80211a:
        [[fallthrough]];
    case WIFI_STANDARD_80211p:
        modulationClass = WIFI_MOD_CLASS_OFDM;
        break;
    case WIFI_STANDARD_80211b:
        modulationClass = WIFI_MOD_CLASS_DSSS;
        break;
    case WIFI_STANDARD_80211g:
        modulationClass = WIFI_MOD_CLASS_ERP_OFDM;
        break;
    case WIFI_STANDARD_80211n:
        modulationClass = WIFI_MOD_CLASS_HT;
        break;
    case WIFI_STANDARD_80211ac:
        modulationClass = WIFI_MOD_CLASS_VHT;
        break;
    case WIFI_STANDARD_80211ax:
        modulationClass = WIFI_MOD_CLASS_HE;
        break;
    case WIFI_STANDARD_80211be:
        modulationClass = WIFI_MOD_CLASS_EHT;
        break;
    case WIFI_STANDARD_UNSPECIFIED:
        [[fallthrough]];
    default:
        NS_ASSERT_MSG(false, "Unsupported standard " << standard);
        break;
    }
    return modulationClass;
}

uint16_t
GetMaximumChannelWidth(WifiModulationClass modulation)
{
    switch (modulation)
    {
    case WIFI_MOD_CLASS_DSSS:
    case WIFI_MOD_CLASS_HR_DSSS:
        return 22;
    case WIFI_MOD_CLASS_OFDM:
    case WIFI_MOD_CLASS_ERP_OFDM:
        return 20;
    case WIFI_MOD_CLASS_HT:
        return 40;
    case WIFI_MOD_CLASS_VHT:
    case WIFI_MOD_CLASS_HE:
        return 160;
    case WIFI_MOD_CLASS_EHT:
        return 160; // TODO update when 320 MHz channels are supported
    default:
        NS_ABORT_MSG("Unknown modulation class: " << modulation);
        return 0;
    }
}

bool
IsEht(WifiPreamble preamble)
{
    return ((preamble == WIFI_PREAMBLE_EHT_MU) || (preamble == WIFI_PREAMBLE_EHT_TB));
}

} // namespace ns3
