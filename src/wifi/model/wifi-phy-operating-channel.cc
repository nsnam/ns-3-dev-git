/*
 * Copyright (c) 2021
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Stefano Avallone <stavallo@unina.it>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-phy-operating-channel.h"

#include "wifi-phy-common.h"
#include "wifi-utils.h"

#include "ns3/assert.h"
#include "ns3/log.h"

#include <algorithm>
#include <numeric>
#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiPhyOperatingChannel");

const std::set<FrequencyChannelInfo> WifiPhyOperatingChannel::m_frequencyChannels = {{
    // 2.4 GHz channels
    //  802.11b uses width of 22, while OFDM modes use width of 20
    {1, MHz_u{2412}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    {1, MHz_u{2412}, MHz_u{20}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {2, MHz_u{2417}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    {2, MHz_u{2417}, MHz_u{20}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {3, MHz_u{2422}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    {3, MHz_u{2422}, MHz_u{20}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {4, MHz_u{2427}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    {4, MHz_u{2427}, MHz_u{20}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {5, MHz_u{2432}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    {5, MHz_u{2432}, MHz_u{20}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {6, MHz_u{2437}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    {6, MHz_u{2437}, MHz_u{20}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {7, MHz_u{2442}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    {7, MHz_u{2442}, MHz_u{20}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {8, MHz_u{2447}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    {8, MHz_u{2447}, MHz_u{20}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {9, MHz_u{2452}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    {9, MHz_u{2452}, MHz_u{20}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {10, MHz_u{2457}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    {10, MHz_u{2457}, MHz_u{20}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {11, MHz_u{2462}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    {11, MHz_u{2462}, MHz_u{20}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {12, MHz_u{2467}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    {12, MHz_u{2467}, MHz_u{20}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {13, MHz_u{2472}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    {13, MHz_u{2472}, MHz_u{20}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    // Only defined for 802.11b
    {14, MHz_u{2484}, MHz_u{22}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::DSSS},
    // 40 MHz channels
    {3, MHz_u{2422}, MHz_u{40}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {4, MHz_u{2427}, MHz_u{40}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {5, MHz_u{2432}, MHz_u{40}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {6, MHz_u{2437}, MHz_u{40}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {7, MHz_u{2442}, MHz_u{40}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {8, MHz_u{2447}, MHz_u{40}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {9, MHz_u{2452}, MHz_u{40}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {10, MHz_u{2457}, MHz_u{40}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},
    {11, MHz_u{2462}, MHz_u{40}, WIFI_PHY_BAND_2_4GHZ, FrequencyChannelType::OFDM},

    // Now the 5 GHz channels used for 802.11a/n/ac/ax/be
    // 20 MHz channels
    {36, MHz_u{5180}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {40, MHz_u{5200}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {44, MHz_u{5220}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {48, MHz_u{5240}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {52, MHz_u{5260}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {56, MHz_u{5280}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {60, MHz_u{5300}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {64, MHz_u{5320}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {100, MHz_u{5500}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {104, MHz_u{5520}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {108, MHz_u{5540}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {112, MHz_u{5560}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {116, MHz_u{5580}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {120, MHz_u{5600}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {124, MHz_u{5620}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {128, MHz_u{5640}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {132, MHz_u{5660}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {136, MHz_u{5680}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {140, MHz_u{5700}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {144, MHz_u{5720}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {149, MHz_u{5745}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {153, MHz_u{5765}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {157, MHz_u{5785}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {161, MHz_u{5805}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {165, MHz_u{5825}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {169, MHz_u{5845}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {173, MHz_u{5865}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {177, MHz_u{5885}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {181, MHz_u{5905}, MHz_u{20}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    // 40 MHz channels
    {38, MHz_u{5190}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {46, MHz_u{5230}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {54, MHz_u{5270}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {62, MHz_u{5310}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {102, MHz_u{5510}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {110, MHz_u{5550}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {118, MHz_u{5590}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {126, MHz_u{5630}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {134, MHz_u{5670}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {142, MHz_u{5710}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {151, MHz_u{5755}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {159, MHz_u{5795}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {167, MHz_u{5835}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {175, MHz_u{5875}, MHz_u{40}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    // 80 MHz channels
    {42, MHz_u{5210}, MHz_u{80}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {58, MHz_u{5290}, MHz_u{80}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {106, MHz_u{5530}, MHz_u{80}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {122, MHz_u{5610}, MHz_u{80}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {138, MHz_u{5690}, MHz_u{80}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {155, MHz_u{5775}, MHz_u{80}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {171, MHz_u{5855}, MHz_u{80}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    // 160 MHz channels
    {50, MHz_u{5250}, MHz_u{160}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {114, MHz_u{5570}, MHz_u{160}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},
    {163, MHz_u{5815}, MHz_u{160}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::OFDM},

    // 802.11p 10 MHz channels at the 5.855-5.925 band
    {172, MHz_u{5860}, MHz_u{10}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},
    {174, MHz_u{5870}, MHz_u{10}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},
    {176, MHz_u{5880}, MHz_u{10}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},
    {178, MHz_u{5890}, MHz_u{10}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},
    {180, MHz_u{5900}, MHz_u{10}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},
    {182, MHz_u{5910}, MHz_u{10}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},
    {184, MHz_u{5920}, MHz_u{10}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},

    // 802.11p 5 MHz channels at the 5.855-5.925 band (for simplification, we consider the same
    // center frequencies as the 10 MHz channels)
    {171, MHz_u{5860}, MHz_u{5}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},
    {173, MHz_u{5870}, MHz_u{5}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},
    {175, MHz_u{5880}, MHz_u{5}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},
    {177, MHz_u{5890}, MHz_u{5}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},
    {179, MHz_u{5900}, MHz_u{5}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},
    {181, MHz_u{5910}, MHz_u{5}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},
    {183, MHz_u{5920}, MHz_u{5}, WIFI_PHY_BAND_5GHZ, FrequencyChannelType::CH_80211P},

    // Now the 6 GHz channels for 802.11ax/be
    // 20 MHz channels
    {1, MHz_u{5955}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {5, MHz_u{5975}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {9, MHz_u{5995}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {13, MHz_u{6015}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {17, MHz_u{6035}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {21, MHz_u{6055}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {25, MHz_u{6075}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {29, MHz_u{6095}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {33, MHz_u{6115}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {37, MHz_u{6135}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {41, MHz_u{6155}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {45, MHz_u{6175}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {49, MHz_u{6195}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {53, MHz_u{6215}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {57, MHz_u{6235}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {61, MHz_u{6255}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {65, MHz_u{6275}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {69, MHz_u{6295}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {73, MHz_u{6315}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {77, MHz_u{6335}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {81, MHz_u{6355}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {85, MHz_u{6375}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {89, MHz_u{6395}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {93, MHz_u{6415}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {97, MHz_u{6435}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {101, MHz_u{6455}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {105, MHz_u{6475}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {109, MHz_u{6495}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {113, MHz_u{6515}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {117, MHz_u{6535}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {121, MHz_u{6555}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {125, MHz_u{6575}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {129, MHz_u{6595}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {133, MHz_u{6615}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {137, MHz_u{6635}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {141, MHz_u{6655}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {145, MHz_u{6675}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {149, MHz_u{6695}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {153, MHz_u{6715}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {157, MHz_u{6735}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {161, MHz_u{6755}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {165, MHz_u{6775}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {169, MHz_u{6795}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {173, MHz_u{6815}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {177, MHz_u{6835}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {181, MHz_u{6855}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {185, MHz_u{6875}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {189, MHz_u{6895}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {193, MHz_u{6915}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {197, MHz_u{6935}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {201, MHz_u{6955}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {205, MHz_u{6975}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {209, MHz_u{6995}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {213, MHz_u{7015}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {217, MHz_u{7035}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {221, MHz_u{7055}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {225, MHz_u{7075}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {229, MHz_u{7095}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {233, MHz_u{7115}, MHz_u{20}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    // 40 MHz channels
    {3, MHz_u{5965}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {11, MHz_u{6005}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {19, MHz_u{6045}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {27, MHz_u{6085}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {35, MHz_u{6125}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {43, MHz_u{6165}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {51, MHz_u{6205}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {59, MHz_u{6245}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {67, MHz_u{6285}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {75, MHz_u{6325}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {83, MHz_u{6365}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {91, MHz_u{6405}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {99, MHz_u{6445}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {107, MHz_u{6485}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {115, MHz_u{6525}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {123, MHz_u{6565}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {131, MHz_u{6605}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {139, MHz_u{6645}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {147, MHz_u{6685}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {155, MHz_u{6725}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {163, MHz_u{6765}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {171, MHz_u{6805}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {179, MHz_u{6845}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {187, MHz_u{6885}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {195, MHz_u{6925}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {203, MHz_u{6965}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {211, MHz_u{7005}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {219, MHz_u{7045}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {227, MHz_u{7085}, MHz_u{40}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    // 80 MHz channels
    {7, MHz_u{5985}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {23, MHz_u{6065}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {39, MHz_u{6145}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {55, MHz_u{6225}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {71, MHz_u{6305}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {87, MHz_u{6385}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {103, MHz_u{6465}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {119, MHz_u{6545}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {135, MHz_u{6625}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {151, MHz_u{6705}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {167, MHz_u{6785}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {183, MHz_u{6865}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {199, MHz_u{6945}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {215, MHz_u{7025}, MHz_u{80}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    // 160 MHz channels
    {15, MHz_u{6025}, MHz_u{160}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {47, MHz_u{6185}, MHz_u{160}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {79, MHz_u{6345}, MHz_u{160}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {111, MHz_u{6505}, MHz_u{160}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {143, MHz_u{6665}, MHz_u{160}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {175, MHz_u{6825}, MHz_u{160}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
    {207, MHz_u{6985}, MHz_u{160}, WIFI_PHY_BAND_6GHZ, FrequencyChannelType::OFDM},
}};

std::ostream&
operator<<(std::ostream& os, const FrequencyChannelInfo& info)
{
    os << "{" << +info.number << " " << info.frequency << " " << info.width << " " << info.band
       << "}";
    return os;
}

bool
WifiPhyOperatingChannel::Compare::operator()(const ConstIterator& first,
                                             const ConstIterator& second) const
{
    return first->frequency < second->frequency;
}

WifiPhyOperatingChannel::WifiPhyOperatingChannel()
    : WifiPhyOperatingChannel(ConstIteratorSet{})
{
}

WifiPhyOperatingChannel::WifiPhyOperatingChannel(ConstIterator it)
    : WifiPhyOperatingChannel(ConstIteratorSet{it})
{
}

WifiPhyOperatingChannel::WifiPhyOperatingChannel(const ConstIteratorSet& channelIts)
    : m_channelIts(channelIts),
      m_primary20Index(0)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(channelIts.size() <= 2,
                  "Operating channel does not support more than 2 segments");
}

WifiPhyOperatingChannel::~WifiPhyOperatingChannel()
{
    NS_LOG_FUNCTION_NOARGS();
}

bool
WifiPhyOperatingChannel::IsSet() const
{
    return !m_channelIts.empty();
}

void
WifiPhyOperatingChannel::Set(const std::vector<FrequencyChannelInfo>& segments,
                             WifiStandard standard)
{
    std::stringstream ss;
    for (const auto& segment : segments)
    {
        ss << segment;
    }
    NS_LOG_FUNCTION(this << ss.str() << standard);

    NS_ASSERT_MSG(!segments.empty(), "At least one frequency segment has to be provided");

    ConstIteratorSet channelIts{};
    for (const auto& segment : segments)
    {
        if (const auto channelIt =
                FindFirst(segment.number, segment.frequency, segment.width, standard, segment.band);
            channelIt != m_frequencyChannels.cend() &&
            FindFirst(segment.number,
                      segment.frequency,
                      segment.width,
                      standard,
                      segment.band,
                      std::next(channelIt)) == m_frequencyChannels.cend())
        {
            // a unique channel matches the specified criteria
            channelIts.insert(channelIt);
        }
    }

    if (channelIts.size() != segments.size())
    {
        // if a unique channel was not found, throw an exception (mainly for unit testing this code)
        throw std::runtime_error(
            "WifiPhyOperatingChannel: No unique channel found given the specified criteria");
    }

    auto it = channelIts.begin();
    for (std::size_t segment = 0; segment < (channelIts.size() - 1); ++segment)
    {
        const auto freq = (*it)->frequency;
        const auto width = (*it)->width;
        const auto band = (*it)->band;
        const auto maxFreq = freq + (width / 2);
        ++it;
        const auto nextFreq = (*it)->frequency;
        const auto nextWidth = (*it)->width;
        const auto nextBand = (*it)->band;
        const auto nextMinFreq = nextFreq - (nextWidth / 2);
        if (maxFreq >= nextMinFreq)
        {
            throw std::runtime_error(
                "WifiPhyOperatingChannel is invalid: segments cannot be adjacent nor overlap");
        }
        if (band != nextBand)
        {
            throw std::runtime_error("WifiPhyOperatingChannel is invalid: all segments shall "
                                     "belong to the same band");
        }
    }

    if ((channelIts.size() > 2) ||
        ((channelIts.size() == 2) &&
         !std::all_of(channelIts.cbegin(), channelIts.cend(), [](const auto& channel) {
             return channel->width == MHz_u{80};
         })))
    {
        throw std::runtime_error("WifiPhyOperatingChannel is invalid: only 80+80MHz is "
                                 "expected as non-contiguous channel");
    }

    m_channelIts = channelIts;
    m_primary20Index = 0;
}

void
WifiPhyOperatingChannel::SetDefault(MHz_u width, WifiStandard standard, WifiPhyBand band)
{
    NS_LOG_FUNCTION(this << width << standard << band);
    Set({{GetDefaultChannelNumber(width, standard, band), MHz_u{0}, width, band}}, standard);
}

uint8_t
WifiPhyOperatingChannel::GetDefaultChannelNumber(
    MHz_u width,
    WifiStandard standard,
    WifiPhyBand band,
    std::optional<uint8_t> previousChannelNumber /* = std::nullopt */)
{
    auto start = m_frequencyChannels.begin();
    auto prevSegmentChannelIt = m_frequencyChannels.end();
    if (previousChannelNumber)
    {
        prevSegmentChannelIt =
            FindFirst(*previousChannelNumber, MHz_u{0}, width, standard, band, start);
        if (prevSegmentChannelIt != m_frequencyChannels.end())
        {
            start = std::next(prevSegmentChannelIt);
        }
    }
    auto channelIt = FindFirst(0, MHz_u{0}, width, standard, band, start);
    if (prevSegmentChannelIt != m_frequencyChannels.end() && channelIt != m_frequencyChannels.end())
    {
        const auto prevFreq = prevSegmentChannelIt->frequency;
        const auto prevWidth = prevSegmentChannelIt->width;
        const auto prevMaxFreq = prevFreq + (prevWidth / 2);
        const auto nextFreq = channelIt->frequency;
        const auto nextWidth = channelIt->width;
        const auto nextMinFreq = nextFreq - (nextWidth / 2);
        if (prevMaxFreq <= nextMinFreq)
        {
            // segments are contiguous to each others, find next segment to make sure they are
            // not contiguous
            channelIt = FindFirst(0, MHz_u{0}, width, standard, band, std::next(channelIt));
        }
    }
    if (channelIt != m_frequencyChannels.end())
    {
        // a channel matches the specified criteria
        return channelIt->number;
    }

    // if a default channel was not found, throw an exception (mainly for unit testing this code)
    throw std::runtime_error("WifiPhyOperatingChannel: No default channel found of the given width "
                             "and for the given PHY standard and band");
}

WifiPhyOperatingChannel::ConstIterator
WifiPhyOperatingChannel::FindFirst(uint8_t number,
                                   MHz_u frequency,
                                   MHz_u width,
                                   WifiStandard standard,
                                   WifiPhyBand band,
                                   ConstIterator start)
{
    // lambda used to match channels against the specified criteria
    auto predicate = [&](const FrequencyChannelInfo& channel) {
        if (number != 0 && channel.number != number)
        {
            return false;
        }
        if (frequency != MHz_u{0} && channel.frequency != frequency)
        {
            return false;
        }
        if (width != MHz_u{0} && channel.width != width)
        {
            return false;
        }
        if (standard != WIFI_STANDARD_UNSPECIFIED &&
            channel.type != GetFrequencyChannelType(standard))
        {
            return false;
        }
        if (band != WIFI_PHY_BAND_UNSPECIFIED && channel.band != band)
        {
            return false;
        }
        return true;
    };

    // Do not search for a channel matching the specified criteria if the given PHY band
    // is not allowed for the given standard (if any) or the given channel width is not
    // allowed for the given standard (if any)
    if (const auto standardIt = wifiStandards.find(standard);
        standardIt != wifiStandards.cend() &&
        (std::find(standardIt->second.cbegin(), standardIt->second.cend(), band) ==
             standardIt->second.cend() ||
         width > GetMaximumChannelWidth(GetModulationClassForStandard(standard))))
    {
        return m_frequencyChannels.cend();
    }

    return std::find_if(start, m_frequencyChannels.cend(), predicate);
}

uint8_t
WifiPhyOperatingChannel::GetNumber(std::size_t segment /* = 0 */) const
{
    NS_ASSERT(IsSet());
    return (*std::next(m_channelIts.begin(), segment))->number;
}

MHz_u
WifiPhyOperatingChannel::GetFrequency(std::size_t segment /* = 0 */) const
{
    NS_ASSERT(IsSet());
    return (*std::next(m_channelIts.begin(), segment))->frequency;
}

MHz_u
WifiPhyOperatingChannel::GetWidth(std::size_t /* segment = 0 */) const
{
    NS_ASSERT(IsSet());
    // Current specs only allow all segments to be the same width
    return (*m_channelIts.cbegin())->width;
}

WifiPhyBand
WifiPhyOperatingChannel::GetPhyBand() const
{
    NS_ASSERT(IsSet());
    // Current specs only allow all segments to be the same band
    return (*m_channelIts.cbegin())->band;
}

bool
WifiPhyOperatingChannel::IsOfdm() const
{
    NS_ASSERT(IsSet());
    return ((*m_channelIts.cbegin())->type == FrequencyChannelType::OFDM);
}

bool
WifiPhyOperatingChannel::IsDsss() const
{
    NS_ASSERT(IsSet());
    return ((*m_channelIts.cbegin())->type == FrequencyChannelType::DSSS);
}

bool
WifiPhyOperatingChannel::Is80211p() const
{
    NS_ASSERT(IsSet());
    return ((*m_channelIts.cbegin())->type == FrequencyChannelType::CH_80211P);
}

std::vector<uint8_t>
WifiPhyOperatingChannel::GetNumbers() const
{
    NS_ASSERT(IsSet());
    std::vector<uint8_t> channelNumbers{};
    std::transform(m_channelIts.cbegin(),
                   m_channelIts.cend(),
                   std::back_inserter(channelNumbers),
                   [](const auto& channel) { return channel->number; });
    return channelNumbers;
}

std::vector<MHz_u>
WifiPhyOperatingChannel::GetFrequencies() const
{
    NS_ASSERT(IsSet());
    std::vector<MHz_u> centerFrequencies{};
    std::transform(m_channelIts.cbegin(),
                   m_channelIts.cend(),
                   std::back_inserter(centerFrequencies),
                   [](const auto& channel) { return channel->frequency; });
    return centerFrequencies;
}

std::vector<MHz_u>
WifiPhyOperatingChannel::GetWidths() const
{
    NS_ASSERT(IsSet());
    std::vector<MHz_u> channelWidths{};
    std::transform(m_channelIts.cbegin(),
                   m_channelIts.cend(),
                   std::back_inserter(channelWidths),
                   [](const auto& channel) { return channel->width; });
    return channelWidths;
}

MHz_u
WifiPhyOperatingChannel::GetTotalWidth() const
{
    NS_ASSERT(IsSet());
    return std::accumulate(m_channelIts.cbegin(),
                           m_channelIts.cend(),
                           MHz_u{0},
                           [](MHz_u sum, const auto& channel) { return sum + channel->width; });
}

WifiChannelWidthType
WifiPhyOperatingChannel::GetWidthType() const
{
    NS_ASSERT(IsSet());
    switch (static_cast<uint16_t>(GetTotalWidth()))
    {
    case 20:
        return WifiChannelWidthType::CW_20MHZ;
    case 22:
        return WifiChannelWidthType::CW_22MHZ;
    case 5:
        return WifiChannelWidthType::CW_5MHZ;
    case 10:
        return WifiChannelWidthType::CW_10MHZ;
    case 40:
        return WifiChannelWidthType::CW_40MHZ;
    case 80:
        return WifiChannelWidthType::CW_80MHZ;
    case 160:
        return (m_channelIts.size() == 2) ? WifiChannelWidthType::CW_80_PLUS_80MHZ
                                          : WifiChannelWidthType::CW_160MHZ;
    case 2160:
        return WifiChannelWidthType::CW_2160MHZ;
    case 0:
    default:
        return WifiChannelWidthType::UNKNOWN;
    }
}

uint8_t
WifiPhyOperatingChannel::GetPrimaryChannelIndex(MHz_u primaryChannelWidth) const
{
    if (static_cast<uint16_t>(primaryChannelWidth) % 20 != 0)
    {
        NS_LOG_DEBUG("The operating channel width is not a multiple of 20 MHz; return 0");
        return 0;
    }

    NS_ASSERT(primaryChannelWidth <= GetTotalWidth());

    // the index of primary40 is half the index of primary20; the index of
    // primary80 is half the index of primary40, ...
    MHz_u width{20};
    uint8_t index = m_primary20Index;

    while (width < primaryChannelWidth)
    {
        index /= 2;
        width *= 2;
    }
    return index;
}

uint8_t
WifiPhyOperatingChannel::GetSecondaryChannelIndex(MHz_u secondaryChannelWidth) const
{
    const uint8_t primaryIndex = GetPrimaryChannelIndex(secondaryChannelWidth);
    const uint8_t secondaryIndex =
        (primaryIndex % 2 == 0) ? (primaryIndex + 1) : (primaryIndex - 1);
    return secondaryIndex;
}

void
WifiPhyOperatingChannel::SetPrimary20Index(uint8_t index)
{
    NS_LOG_FUNCTION(this << +index);

    NS_ABORT_MSG_IF(index > 0 && index >= Count20MHzSubchannels(GetTotalWidth()),
                    "Primary20 index out of range");
    m_primary20Index = index;
}

uint8_t
WifiPhyOperatingChannel::GetPrimarySegmentIndex(MHz_u primaryChannelWidth) const
{
    if (m_channelIts.size() < 2)
    {
        return 0;
    }
    // Note: this function assumes no more than 2 segments are used
    const auto numIndices = GetTotalWidth() / primaryChannelWidth;
    const auto primaryIndex = GetPrimaryChannelIndex(primaryChannelWidth);
    return (primaryIndex >= (numIndices / 2)) ? 1 : 0;
}

uint8_t
WifiPhyOperatingChannel::GetSecondarySegmentIndex(MHz_u primaryChannelWidth) const
{
    NS_ABORT_MSG_IF(primaryChannelWidth > GetWidth(),
                    "Primary channel width cannot be larger than the width of a frequency segment");
    if (m_channelIts.size() < 2)
    {
        return 0;
    }
    // Note: this function assumes no more than 2 segments are used
    const auto numIndices = GetTotalWidth() / primaryChannelWidth;
    const auto secondaryIndex = GetSecondaryChannelIndex(primaryChannelWidth);
    return (secondaryIndex >= (numIndices / 2)) ? 1 : 0;
}

MHz_u
WifiPhyOperatingChannel::GetPrimaryChannelCenterFrequency(MHz_u primaryChannelWidth) const
{
    const auto segmentIndex = GetPrimarySegmentIndex(primaryChannelWidth);
    // we assume here that all segments have the same width
    const auto segmentWidth = GetWidth(segmentIndex);
    // segmentOffset has to be an (unsigned) integer to ensure correct calculation
    const uint8_t segmentOffset = (segmentIndex * (segmentWidth / primaryChannelWidth));
    return GetFrequency(segmentIndex) - segmentWidth / 2. +
           (GetPrimaryChannelIndex(primaryChannelWidth) - segmentOffset + 0.5) *
               primaryChannelWidth;
}

MHz_u
WifiPhyOperatingChannel::GetSecondaryChannelCenterFrequency(MHz_u secondaryChannelWidth) const
{
    const auto segmentIndex = GetSecondarySegmentIndex(secondaryChannelWidth);
    // we assume here that all segments have the same width
    const auto segmentWidth = GetWidth(segmentIndex);
    // segmentOffset has to be an (unsigned) integer to ensure correct calculation
    const uint8_t segmentOffset = (segmentIndex * (segmentWidth / secondaryChannelWidth));
    return GetFrequency(segmentIndex) - segmentWidth / 2. +
           (GetSecondaryChannelIndex(secondaryChannelWidth) - segmentOffset + 0.5) *
               secondaryChannelWidth;
}

uint8_t
WifiPhyOperatingChannel::GetPrimaryChannelNumber(MHz_u primaryChannelWidth,
                                                 WifiStandard standard) const
{
    NS_ABORT_MSG_IF(primaryChannelWidth > GetWidth(),
                    "Primary channel width cannot be larger than the width of a frequency segment");
    auto frequency = GetPrimaryChannelCenterFrequency(primaryChannelWidth);
    NS_ASSERT_MSG(IsSet(), "No channel set");
    auto primaryChanIt = FindFirst(0, frequency, primaryChannelWidth, standard, GetPhyBand());
    NS_ASSERT_MSG(primaryChanIt != m_frequencyChannels.end(), "Primary channel number not found");
    return primaryChanIt->number;
}

WifiPhyOperatingChannel
WifiPhyOperatingChannel::GetPrimaryChannel(MHz_u primaryChannelWidth) const
{
    NS_ASSERT_MSG(IsSet(), "No channel set");
    NS_ASSERT_MSG(primaryChannelWidth <= GetTotalWidth(),
                  "Requested primary channel width ("
                      << primaryChannelWidth << " MHz) exceeds total width (" << GetTotalWidth()
                      << " MHz)");

    if (primaryChannelWidth == GetTotalWidth())
    {
        return *this;
    }

    const auto frequency = GetPrimaryChannelCenterFrequency(primaryChannelWidth);
    auto primaryChanIt =
        FindFirst(0, frequency, primaryChannelWidth, WIFI_STANDARD_UNSPECIFIED, GetPhyBand());
    NS_ABORT_MSG_IF(primaryChanIt == m_frequencyChannels.end(), "Primary channel number not found");

    WifiPhyOperatingChannel primaryChannel(primaryChanIt);

    const auto primaryIndex = m_primary20Index - (GetPrimaryChannelIndex(primaryChannelWidth) *
                                                  Count20MHzSubchannels(primaryChannelWidth));
    primaryChannel.SetPrimary20Index(primaryIndex);

    return primaryChannel;
}

std::set<uint8_t>
WifiPhyOperatingChannel::GetAll20MHzChannelIndicesInPrimary(MHz_u width) const
{
    if (width > GetTotalWidth())
    {
        // a primary channel of the given width does not exist
        return {};
    }

    MHz_u currWidth{20};
    std::set<uint8_t> indices;
    indices.insert(m_primary20Index);

    while (currWidth < width)
    {
        indices.merge(GetAll20MHzChannelIndicesInSecondary(indices));
        currWidth *= 2;
    }

    return indices;
}

std::set<uint8_t>
WifiPhyOperatingChannel::GetAll20MHzChannelIndicesInSecondary(MHz_u width) const
{
    return GetAll20MHzChannelIndicesInSecondary(GetAll20MHzChannelIndicesInPrimary(width));
}

std::set<uint8_t>
WifiPhyOperatingChannel::GetAll20MHzChannelIndicesInSecondary(
    const std::set<uint8_t>& primaryIndices) const
{
    if (primaryIndices.empty() || GetTotalWidth() == MHz_u{20})
    {
        return {};
    }

    uint8_t size = 1;
    MHz_u primaryWidth{20};

    // find the width of the primary channel corresponding to the size of the given set
    while (size != primaryIndices.size())
    {
        size <<= 1;
        primaryWidth *= 2;

        if (primaryWidth >= GetTotalWidth())
        {
            // the width of the primary channel resulting from the given indices
            // exceeds the width of the operating channel
            return {};
        }
    }

    std::set<uint8_t> secondaryIndices;
    for (const auto& index : primaryIndices)
    {
        secondaryIndices.insert(index ^ size);
    }

    return secondaryIndices;
}

std::set<uint8_t>
WifiPhyOperatingChannel::Get20MHzIndicesCoveringRu(HeRu::RuSpec ru, MHz_u width) const
{
    auto ruType = ru.GetRuType();

    NS_ASSERT_MSG(HeRu::GetBandwidth(ruType) <= width,
                  "No RU of type " << ruType << " is contained in a " << width << " MHz channel");
    NS_ASSERT_MSG(width <= GetTotalWidth(),
                  "The given width (" << width << " MHz) exceeds the operational width ("
                                      << GetTotalWidth() << ")");

    // trivial case: 2x996-tone RU
    if (ruType == HeRu::RU_2x996_TONE)
    {
        return {0, 1, 2, 3, 4, 5, 6, 7};
    }

    // handle first the special case of center 26-tone RUs
    if (ruType == HeRu::RU_26_TONE && ru.GetIndex() == 19)
    {
        NS_ASSERT_MSG(width >= MHz_u{80},
                      "26-tone RU with index 19 is only present in channels of at least 80 MHz");
        // the center 26-tone RU in an 80 MHz channel is not fully covered by
        // any 20 MHz channel, but by the two central 20 MHz channels in the 80 MHz channel
        auto indices = ru.GetPrimary80MHz() ? GetAll20MHzChannelIndicesInPrimary(MHz_u{80})
                                            : GetAll20MHzChannelIndicesInSecondary(MHz_u{80});
        indices.erase(indices.begin());
        indices.erase(std::prev(indices.end()));
        return indices;
    }

    auto ruIndex = ru.GetIndex();

    if (ruType == HeRu::RU_26_TONE && ruIndex > 19)
    {
        // "ignore" the center 26-tone RU in an 80 MHz channel
        ruIndex--;
    }

    // if the RU refers to a 160 MHz channel, we have to update the RU index (which
    // refers to an 80 MHz channel) if the RU is not in the lower 80 MHz channel
    if (width == MHz_u{160})
    {
        bool primary80IsLower80 = (m_primary20Index < 4);
        if (primary80IsLower80 != ru.GetPrimary80MHz())
        {
            auto nRusIn80MHz = HeRu::GetNRus(MHz_u{80}, ruType);
            // "ignore" the center 26-tone RU in an 80 MHz channel
            if (ruType == HeRu::RU_26_TONE)
            {
                nRusIn80MHz--;
            }
            ruIndex += nRusIn80MHz;
        }
    }

    uint8_t n20MHzChannels; // number of 20 MHz channels in the channel covering the RU

    switch (ruType)
    {
    case HeRu::RU_26_TONE:
    case HeRu::RU_52_TONE:
    case HeRu::RU_106_TONE:
    case HeRu::RU_242_TONE:
        n20MHzChannels = 1;
        break;
    case HeRu::RU_484_TONE:
        n20MHzChannels = 2;
        break;
    case HeRu::RU_996_TONE:
        n20MHzChannels = 4;
        break;
    default:
        NS_ABORT_MSG("Unhandled RU type: " << ruType);
    }

    auto nRusInCoveringChannel = HeRu::GetNRus(n20MHzChannels * MHz_u{20}, ruType);
    // compute the index (starting at 0) of the covering channel within the given width
    std::size_t indexOfCoveringChannelInGivenWidth = (ruIndex - 1) / nRusInCoveringChannel;

    // expand the index of the covering channel in the indices of its constituent
    // 20 MHz channels (within the given width)
    NS_ASSERT(indexOfCoveringChannelInGivenWidth < 8); // max number of 20 MHz channels
    std::set<uint8_t> indices({static_cast<uint8_t>(indexOfCoveringChannelInGivenWidth)});

    while (n20MHzChannels > 1)
    {
        std::set<uint8_t> updatedIndices;
        for (const auto& idx : indices)
        {
            updatedIndices.insert(idx * 2);
            updatedIndices.insert(idx * 2 + 1);
        }
        indices.swap(updatedIndices);
        n20MHzChannels /= 2;
    }

    // finally, add the appropriate offset if width is less than the operational channel width
    auto offset = GetPrimaryChannelIndex(width) * Count20MHzSubchannels(width);

    if (offset > 0)
    {
        std::set<uint8_t> updatedIndices;
        for (const auto& idx : indices)
        {
            updatedIndices.insert(idx + offset);
        }
        indices.swap(updatedIndices);
    }

    return indices;
}

std::size_t
WifiPhyOperatingChannel::GetNSegments() const
{
    return m_channelIts.size();
}

bool
WifiPhyOperatingChannel::operator==(const WifiPhyOperatingChannel& other) const
{
    return m_channelIts == other.m_channelIts;
}

bool
WifiPhyOperatingChannel::operator!=(const WifiPhyOperatingChannel& other) const
{
    return !(*this == other);
}

std::ostream&
operator<<(std::ostream& os, const WifiPhyOperatingChannel& channel)
{
    if (channel.IsSet())
    {
        const auto numSegments = channel.GetNSegments();
        for (std::size_t segmentId = 0; segmentId < numSegments; ++segmentId)
        {
            if (numSegments > 1)
            {
                os << "segment " << segmentId << " ";
            }
            os << "channel " << +channel.GetNumber() << " frequency " << channel.GetFrequency()
               << " width " << channel.GetWidth() << " band " << channel.GetPhyBand();
            if ((segmentId == 0) && (static_cast<uint16_t>(channel.GetTotalWidth()) % 20 == 0))
            {
                os << " primary20 " << +channel.GetPrimaryChannelIndex(MHz_u{20});
            }
            if (segmentId < numSegments - 1)
            {
                os << " ";
            }
        }
    }
    else
    {
        os << "channel not set";
    }
    return os;
}

} // namespace ns3
