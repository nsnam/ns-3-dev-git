/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include <algorithm>
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/mobility-model.h"
#include "ns3/random-variable-stream.h"
#include "ns3/error-model.h"
#include "wifi-phy.h"
#include "ampdu-tag.h"
#include "wifi-utils.h"
#include "sta-wifi-mac.h"
#include "frame-capture-model.h"
#include "preamble-detection-model.h"
#include "wifi-radio-energy-model.h"
#include "error-rate-model.h"
#include "wifi-net-device.h"
#include "ht-configuration.h"
#include "he-configuration.h"
#include "mpdu-aggregator.h"
#include "wifi-psdu.h"
#include "he-ppdu.h"
#include "ap-wifi-mac.h"
#include "dsss-phy.h"
#include "erp-ofdm-phy.h"
#include "he-phy.h" //includes OFDM, HT, and VHT

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiPhy");

/****************************************************************
 *       The actual WifiPhy class
 ****************************************************************/

NS_OBJECT_ENSURE_REGISTERED (WifiPhy);

uint64_t WifiPhy::m_globalPpduUid = 0;

/**
 * This table maintains the mapping of valid ChannelNumber to
 * Frequency/ChannelWidth pairs.  If you want to make a channel applicable
 * to all standards, then you may use the WIFI_PHY_STANDARD_UNSPECIFIED
 * standard to represent this, as a wildcard.  If you want to limit the
 * configuration of a particular channel/frequency/width to a particular
 * standard(s), then you can specify one or more such bindings.
 */
WifiPhy::ChannelToFrequencyWidthMap WifiPhy::m_channelToFrequencyWidth =
{
  //2.4 GHz channels
  // 802.11b uses width of 22, while OFDM modes use width of 20
  { { {1, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2412, 22} },
  { { {1, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {2412, 20} },
  { { {2, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2417, 22} },
  { { {2, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {2417, 20} },
  { { {3, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2422, 22} },
  { { {3, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {2422, 20} },
  { { {4, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2427, 22} },
  { { {4, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {2427, 20} },
  { { {5, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2432, 22} },
  { { {5, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {2432, 20} },
  { { {6, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2437, 22} },
  { { {6, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {2437, 20} },
  { { {7, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2442, 22} },
  { { {7, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {2442, 20} },
  { { {8, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2447, 22} },
  { { {8, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {2447, 20} },
  { { {9, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2452, 22} },
  { { {9, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {2452, 20} },
  { { {10, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2457, 22} },
  { { {10, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {2457, 20} },
  { { {11, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2462, 22} },
  { { {11, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {2462, 20} },
  { { {12, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2467, 22} },
  { { {12, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {2467, 20} },
  { { {13, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2472, 22} },
  { { {13, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {2472, 20} },
  // Only defined for 802.11b
  { { {14, WIFI_PHY_BAND_2_4GHZ}, WIFI_PHY_STANDARD_80211b}, {2484, 22} },

  // Now the 5 GHz channels; UNSPECIFIED for 802.11a/n/ac/ax channels
  // 20 MHz channels
  { { {36, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5180, 20} },
  { { {40, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5200, 20} },
  { { {44, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5220, 20} },
  { { {48, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5240, 20} },
  { { {52, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5260, 20} },
  { { {56, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5280, 20} },
  { { {60, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5300, 20} },
  { { {64, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5320, 20} },
  { { {100, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5500, 20} },
  { { {104, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5520, 20} },
  { { {108, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5540, 20} },
  { { {112, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5560, 20} },
  { { {116, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5580, 20} },
  { { {120, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5600, 20} },
  { { {124, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5620, 20} },
  { { {128, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5640, 20} },
  { { {132, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5660, 20} },
  { { {136, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5680, 20} },
  { { {140, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5700, 20} },
  { { {144, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5720, 20} },
  { { {149, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5745, 20} },
  { { {153, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5765, 20} },
  { { {157, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5785, 20} },
  { { {161, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5805, 20} },
  { { {165, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5825, 20} },
  { { {169, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5845, 20} },
  { { {173, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5865, 20} },
  { { {177, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5885, 20} },
  { { {181, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5905, 20} },
  // 40 MHz channels
  { { {38, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5190, 40} },
  { { {46, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5230, 40} },
  { { {54, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5270, 40} },
  { { {62, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5310, 40} },
  { { {102, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5510, 40} },
  { { {110, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5550, 40} },
  { { {118, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5590, 40} },
  { { {126, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5630, 40} },
  { { {134, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5670, 40} },
  { { {142, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5710, 40} },
  { { {151, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5755, 40} },
  { { {159, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5795, 40} },
  { { {167, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5835, 40} },
  { { {175, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5875, 40} },
  // 80 MHz channels
  { { {42, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5210, 80} },
  { { {58, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5290, 80} },
  { { {106, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5530, 80} },
  { { {122, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5610, 80} },
  { { {138, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5690, 80} },
  { { {155, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5775, 80} },
  { { {171, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5855, 80} },
  // 160 MHz channels
  { { {50, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5250, 160} },
  { { {114, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5570, 160} },
  { { {163, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_UNSPECIFIED}, {5815, 160} },

  // 802.11p 10 MHz channels at the 5.855-5.925 band
  { { {172, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5860, 10} },
  { { {174, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5870, 10} },
  { { {176, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5880, 10} },
  { { {178, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5890, 10} },
  { { {180, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5900, 10} },
  { { {182, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5910, 10} },
  { { {184, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5920, 10} },

  // 802.11p 5 MHz channels at the 5.855-5.925 band (for simplification, we consider the same center frequencies as the 10 MHz channels)
  { { {171, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5860, 5} },
  { { {173, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5870, 5} },
  { { {175, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5880, 5} },
  { { {177, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5890, 5} },
  { { {179, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5900, 5} },
  { { {181, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5910, 5} },
  { { {183, WIFI_PHY_BAND_5GHZ}, WIFI_PHY_STANDARD_80211p}, {5920, 5} },

  // Now the 6 GHz channels (802.11ax only)
  // 20 MHz channels
  { { {1, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {5945, 20} },
  { { {5, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {5965, 20} },
  { { {9, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {5985, 20} },
  { { {13, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6005, 20} },
  { { {17, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6025, 20} },
  { { {21, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6045, 20} },
  { { {25, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6065, 20} },
  { { {29, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6085, 20} },
  { { {33, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6105, 20} },
  { { {37, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6125, 20} },
  { { {41, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6145, 20} },
  { { {45, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6165, 20} },
  { { {49, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6185, 20} },
  { { {53, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6205, 20} },
  { { {57, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6225, 20} },
  { { {61, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6245, 20} },
  { { {65, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6265, 20} },
  { { {69, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6285, 20} },
  { { {73, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6305, 20} },
  { { {77, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6325, 20} },
  { { {81, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6345, 20} },
  { { {85, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6365, 20} },
  { { {89, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6385, 20} },
  { { {93, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6405, 20} },
  { { {97, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6425, 20} },
  { { {101, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6445, 20} },
  { { {105, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6465, 20} },
  { { {109, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6485, 20} },
  { { {113, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6505, 20} },
  { { {117, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6525, 20} },
  { { {121, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6545, 20} },
  { { {125, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6565, 20} },
  { { {129, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6585, 20} },
  { { {133, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6605, 20} },
  { { {137, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6625, 20} },
  { { {141, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6645, 20} },
  { { {145, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6665, 20} },
  { { {149, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6685, 20} },
  { { {153, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6705, 20} },
  { { {157, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6725, 20} },
  { { {161, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6745, 20} },
  { { {165, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6765, 20} },
  { { {169, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6785, 20} },
  { { {173, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6805, 20} },
  { { {177, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6825, 20} },
  { { {181, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6845, 20} },
  { { {185, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6865, 20} },
  { { {189, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6885, 20} },
  { { {193, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6905, 20} },
  { { {197, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6925, 20} },
  { { {201, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6945, 20} },
  { { {205, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6965, 20} },
  { { {209, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6985, 20} },
  { { {213, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {7005, 20} },
  { { {217, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {7025, 20} },
  { { {221, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {7045, 20} },
  { { {225, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {7065, 20} },
  { { {229, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {7085, 20} },
  { { {233, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {7105, 20} },
  // 40 MHz channels
  { { {3, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {5955, 40} },
  { { {11, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {5995, 40} },
  { { {19, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6035, 40} },
  { { {27, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6075, 40} },
  { { {35, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6115, 40} },
  { { {43, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6155, 40} },
  { { {51, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6195, 40} },
  { { {59, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6235, 40} },
  { { {67, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6275, 40} },
  { { {75, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6315, 40} },
  { { {83, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6355, 40} },
  { { {91, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6395, 40} },
  { { {99, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6435, 40} },
  { { {107, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6475, 40} },
  { { {115, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6515, 40} },
  { { {123, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6555, 40} },
  { { {131, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6595, 40} },
  { { {139, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6635, 40} },
  { { {147, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6675, 40} },
  { { {155, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6715, 40} },
  { { {163, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6755, 40} },
  { { {171, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6795, 40} },
  { { {179, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6835, 40} },
  { { {187, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6875, 40} },
  { { {195, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6915, 40} },
  { { {203, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6955, 40} },
  { { {211, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6995, 40} },
  { { {219, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {7035, 40} },
  { { {227, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {7075, 40} },
  // 80 MHz channels
  { { {7, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {5975, 80} },
  { { {23, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6055, 80} },
  { { {39, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6135, 80} },
  { { {55, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6215, 80} },
  { { {71, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6295, 80} },
  { { {87, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6375, 80} },
  { { {103, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6455, 80} },
  { { {119, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6535, 80} },
  { { {135, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6615, 80} },
  { { {151, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6695, 80} },
  { { {167, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6775, 80} },
  { { {183, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6855, 80} },
  { { {199, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6935, 80} },
  { { {215, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {7015, 80} },
  // 160 MHz channels
  { { {15, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6015, 160} },
  { { {47, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6175, 160} },
  { { {79, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6335, 160} },
  { { {111, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6495, 160} },
  { { {143, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6655, 160} },
  { { {175, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6815, 160} },
  { { {207, WIFI_PHY_BAND_6GHZ}, WIFI_PHY_STANDARD_80211ax}, {6975, 160} }
};

std::map<WifiModulationClass, Ptr<PhyEntity> > WifiPhy::m_staticPhyEntities; //will be filled by g_constructor_XXX

TypeId
WifiPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiPhy")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddAttribute ("Frequency",
                   "The operating center frequency (MHz)",
                   UintegerValue (0),
                   MakeUintegerAccessor (&WifiPhy::GetFrequency,
                                         &WifiPhy::SetFrequency),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ChannelWidth",
                   "Whether 5MHz, 10MHz, 20MHz, 22MHz, 40MHz, 80 MHz or 160 MHz.",
                   UintegerValue (20),
                   MakeUintegerAccessor (&WifiPhy::GetChannelWidth,
                                         &WifiPhy::SetChannelWidth),
                   MakeUintegerChecker<uint16_t> (5, 160))
    .AddAttribute ("ChannelNumber",
                   "If set to non-zero defined value, will control Frequency and ChannelWidth assignment",
                   UintegerValue (0),
                   MakeUintegerAccessor (&WifiPhy::SetChannelNumber,
                                         &WifiPhy::GetChannelNumber),
                   MakeUintegerChecker<uint8_t> (0, 233))
    .AddAttribute ("RxSensitivity",
                   "The energy of a received signal should be higher than "
                   "this threshold (dBm) for the PHY to detect the signal.",
                   DoubleValue (-101.0),
                   MakeDoubleAccessor (&WifiPhy::SetRxSensitivity,
                                       &WifiPhy::GetRxSensitivity),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("CcaEdThreshold",
                   "The energy of a non Wi-Fi received signal should be higher than "
                   "this threshold (dBm) to allow the PHY layer to declare CCA BUSY state. "
                   "This check is performed on the 20 MHz primary channel only.",
                   DoubleValue (-62.0),
                   MakeDoubleAccessor (&WifiPhy::SetCcaEdThreshold,
                                       &WifiPhy::GetCcaEdThreshold),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxGain",
                   "Transmission gain (dB).",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&WifiPhy::SetTxGain,
                                       &WifiPhy::GetTxGain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxGain",
                   "Reception gain (dB).",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&WifiPhy::SetRxGain,
                                       &WifiPhy::GetRxGain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxPowerLevels",
                   "Number of transmission power levels available between "
                   "TxPowerStart and TxPowerEnd included.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&WifiPhy::m_nTxPower),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("TxPowerEnd",
                   "Maximum available transmission level (dBm).",
                   DoubleValue (16.0206),
                   MakeDoubleAccessor (&WifiPhy::SetTxPowerEnd,
                                       &WifiPhy::GetTxPowerEnd),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxPowerStart",
                   "Minimum available transmission level (dBm).",
                   DoubleValue (16.0206),
                   MakeDoubleAccessor (&WifiPhy::SetTxPowerStart,
                                       &WifiPhy::GetTxPowerStart),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxNoiseFigure",
                   "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                   " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                   "\"the difference in decibels (dB) between"
                   " the noise output of the actual receiver to the noise output of an "
                   " ideal receiver with the same overall gain and bandwidth when the receivers "
                   " are connected to sources at the standard noise temperature T0 (usually 290 K)\".",
                   DoubleValue (7),
                   MakeDoubleAccessor (&WifiPhy::SetRxNoiseFigure),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("State",
                   "The state of the PHY layer.",
                   PointerValue (),
                   MakePointerAccessor (&WifiPhy::m_state),
                   MakePointerChecker<WifiPhyStateHelper> ())
    .AddAttribute ("ChannelSwitchDelay",
                   "Delay between two short frames transmitted on different frequencies.",
                   TimeValue (MicroSeconds (250)),
                   MakeTimeAccessor (&WifiPhy::m_channelSwitchDelay),
                   MakeTimeChecker ())
    .AddAttribute ("Antennas",
                   "The number of antennas on the device.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&WifiPhy::GetNumberOfAntennas,
                                         &WifiPhy::SetNumberOfAntennas),
                   MakeUintegerChecker<uint8_t> (1, 8))
    .AddAttribute ("MaxSupportedTxSpatialStreams",
                   "The maximum number of supported TX spatial streams."
                   "This parameter is only valuable for 802.11n/ac/ax STAs and APs.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&WifiPhy::GetMaxSupportedTxSpatialStreams,
                                         &WifiPhy::SetMaxSupportedTxSpatialStreams),
                   MakeUintegerChecker<uint8_t> (1, 8))
    .AddAttribute ("MaxSupportedRxSpatialStreams",
                   "The maximum number of supported RX spatial streams."
                   "This parameter is only valuable for 802.11n/ac/ax STAs and APs.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&WifiPhy::GetMaxSupportedRxSpatialStreams,
                                         &WifiPhy::SetMaxSupportedRxSpatialStreams),
                   MakeUintegerChecker<uint8_t> (1, 8))
    .AddAttribute ("ShortPlcpPreambleSupported",
                   "Whether or not short PHY preamble is supported."
                   "This parameter is only valuable for 802.11b STAs and APs."
                   "Note: 802.11g APs and STAs always support short PHY preamble.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&WifiPhy::GetShortPhyPreambleSupported,
                                        &WifiPhy::SetShortPhyPreambleSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("FrameCaptureModel",
                   "Ptr to an object that implements the frame capture model",
                   PointerValue (),
                   MakePointerAccessor (&WifiPhy::m_frameCaptureModel),
                   MakePointerChecker <FrameCaptureModel> ())
    .AddAttribute ("PreambleDetectionModel",
                   "Ptr to an object that implements the preamble detection model",
                   PointerValue (),
                   MakePointerAccessor (&WifiPhy::m_preambleDetectionModel),
                   MakePointerChecker <PreambleDetectionModel> ())
    .AddAttribute ("PostReceptionErrorModel",
                   "An optional packet error model can be added to the receive "
                   "packet process after any propagation-based (SNR-based) error "
                   "models have been applied. Typically this is used to force "
                   "specific packet drops, for testing purposes.",
                   PointerValue (),
                   MakePointerAccessor (&WifiPhy::m_postReceptionErrorModel),
                   MakePointerChecker<ErrorModel> ())
    .AddAttribute ("Sifs",
                   "The duration of the Short Interframe Space. "
                   "NOTE that the default value is overwritten by the value defined "
                   "by the standard; if you want to set this attribute, you have to "
                   "do it after that the PHY object is initialized.",
                   TimeValue (MicroSeconds (0)),
                   MakeTimeAccessor (&WifiPhy::m_sifs),
                   MakeTimeChecker ())
    .AddAttribute ("Slot",
                   "The duration of a slot. "
                   "NOTE that the default value is overwritten by the value defined "
                   "by the standard; if you want to set this attribute, you have to "
                   "do it after that the PHY object is initialized.",
                   TimeValue (MicroSeconds (0)),
                   MakeTimeAccessor (&WifiPhy::m_slot),
                   MakeTimeChecker ())
    .AddAttribute ("Pifs",
                   "The duration of the PCF Interframe Space. "
                   "NOTE that the default value is overwritten by the value defined "
                   "by the standard; if you want to set this attribute, you have to "
                   "do it after that the PHY object is initialized.",
                   TimeValue (MicroSeconds (0)),
                   MakeTimeAccessor (&WifiPhy::m_pifs),
                   MakeTimeChecker ())
    .AddAttribute ("PowerDensityLimit",
                   "The mean equivalent isotropically radiated power density"
                   "limit (in dBm/MHz) set by regulators.",
                   DoubleValue (100.0), //set to a high value so as to have no effect
                   MakeDoubleAccessor (&WifiPhy::m_powerDensityLimit),
                   MakeDoubleChecker<double> ())
    .AddTraceSource ("PhyTxBegin",
                     "Trace source indicating a packet "
                     "has begun transmitting over the channel medium",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyTxBeginTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxPsduBegin",
                     "Trace source indicating a PSDU "
                     "has begun transmitting over the channel medium",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyTxPsduBeginTrace),
                     "ns3::WifiPhy::PsduTxBeginCallback")
    .AddTraceSource ("PhyTxEnd",
                     "Trace source indicating a packet "
                     "has been completely transmitted over the channel.",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyTxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyTxDrop",
                     "Trace source indicating a packet "
                     "has been dropped by the device during transmission",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyTxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyRxBegin",
                     "Trace source indicating a packet "
                     "has begun being received from the channel medium "
                     "by the device",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyRxBeginTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyRxPayloadBegin",
                     "Trace source indicating the reception of the "
                     "payload of a PPDU has begun",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyRxPayloadBeginTrace),
                     "ns3::WifiPhy::PhyRxPayloadBeginTracedCallback")
    .AddTraceSource ("PhyRxEnd",
                     "Trace source indicating a packet "
                     "has been completely received from the channel medium "
                     "by the device",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyRxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PhyRxDrop",
                     "Trace source indicating a packet "
                     "has been dropped by the device during reception",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyRxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MonitorSnifferRx",
                     "Trace source simulating a wifi device in monitor mode "
                     "sniffing all received frames",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyMonitorSniffRxTrace),
                     "ns3::WifiPhy::MonitorSnifferRxTracedCallback")
    .AddTraceSource ("MonitorSnifferTx",
                     "Trace source simulating the capability of a wifi device "
                     "in monitor mode to sniff all frames being transmitted",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyMonitorSniffTxTrace),
                     "ns3::WifiPhy::MonitorSnifferTxTracedCallback")
    .AddTraceSource ("EndOfHeSigA",
                     "Trace source indicating the end of the HE-SIG-A field for HE PPDUs (or more recent)",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyEndOfHeSigATrace),
                     "ns3::WifiPhy::EndOfHeSigATracedCallback")
  ;
  return tid;
}

WifiPhy::WifiPhy ()
  : m_txMpduReferenceNumber (0xffffffff),
    m_rxMpduReferenceNumber (0xffffffff),
    m_endPhyRxEvent (),
    m_endTxEvent (),
    m_currentEvent (0),
    m_previouslyRxPpduUid (UINT64_MAX),
    m_previouslyTxPpduUid (UINT64_MAX),
    m_currentHeTbPpduUid (UINT64_MAX),
    m_standard (WIFI_PHY_STANDARD_UNSPECIFIED),
    m_band (WIFI_PHY_BAND_UNSPECIFIED),
    m_isConstructed (false),
    m_channelCenterFrequency (0),
    m_initialFrequency (0),
    m_frequencyChannelNumberInitialized (false),
    m_channelWidth (0),
    m_sifs (Seconds (0)),
    m_slot (Seconds (0)),
    m_pifs (Seconds (0)),
    m_ackTxTime (Seconds (0)),
    m_blockAckTxTime (Seconds (0)),
    m_powerRestricted (false),
    m_channelAccessRequested (false),
    m_txSpatialStreams (0),
    m_rxSpatialStreams (0),
    m_channelNumber (0),
    m_initialChannelNumber (0),
    m_wifiRadioEnergyModel (0),
    m_timeLastPreambleDetected (Seconds (0))
{
  NS_LOG_FUNCTION (this);
  m_random = CreateObject<UniformRandomVariable> ();
  m_state = CreateObject<WifiPhyStateHelper> ();
}

WifiPhy::~WifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

void
WifiPhy::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_endTxEvent.Cancel ();
  m_endPhyRxEvent.Cancel ();
  for (auto & endRxEvent : m_endRxEvents)
    {
      endRxEvent.Cancel ();
    }
  m_endRxEvents.clear ();
  for (auto & endPreambleDetectionEvent : m_endPreambleDetectionEvents)
    {
      endPreambleDetectionEvent.Cancel ();
    }
  m_endPreambleDetectionEvents.clear ();
  for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
    {
      beginOfdmaPayloadRxEvent.second.Cancel ();
    }
  m_beginOfdmaPayloadRxEvents.clear ();
  m_device = 0;
  m_mobility = 0;
  m_state = 0;
  m_wifiRadioEnergyModel = 0;
  m_postReceptionErrorModel = 0;
  m_currentPreambleEvents.clear ();

  for (auto & phyEntity : m_phyEntities)
    {
      phyEntity.second = 0;
    }
  m_phyEntities.clear ();
}

void
WifiPhy::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_isConstructed = true;
  if (m_frequencyChannelNumberInitialized == true)
    {
      NS_LOG_DEBUG ("Frequency already initialized");
      return;
    }
  InitializeFrequencyChannelNumber ();
}

Ptr<WifiPhyStateHelper>
WifiPhy::GetState (void) const
{
  return m_state;
}

void
WifiPhy::SetReceiveOkCallback (RxOkCallback callback)
{
  m_state->SetReceiveOkCallback (callback);
}

void
WifiPhy::SetReceiveErrorCallback (RxErrorCallback callback)
{
  m_state->SetReceiveErrorCallback (callback);
}

void
WifiPhy::RegisterListener (WifiPhyListener *listener)
{
  m_state->RegisterListener (listener);
}

void
WifiPhy::UnregisterListener (WifiPhyListener *listener)
{
  m_state->UnregisterListener (listener);
}

void
WifiPhy::SetCapabilitiesChangedCallback (Callback<void> callback)
{
  m_capabilitiesChangedCallback = callback;
}

void
WifiPhy::InitializeFrequencyChannelNumber (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT_MSG (m_frequencyChannelNumberInitialized == false, "Initialization called twice");

  // If frequency has been set to a non-zero value during attribute
  // construction phase, the frequency and channel width will drive the
  // initial configuration.  If frequency has not been set, but both
  // standard and channel number have been set, that pair will instead
  // drive the configuration, and frequency and channel number will be
  // aligned
  if (m_initialFrequency != 0)
    {
      SetFrequency (m_initialFrequency);
    }
  else if (m_initialChannelNumber != 0 && GetPhyStandard () != WIFI_PHY_STANDARD_UNSPECIFIED)
    {
      SetChannelNumber (m_initialChannelNumber);
    }
  else if (m_initialChannelNumber != 0 && GetPhyStandard () == WIFI_PHY_STANDARD_UNSPECIFIED)
    {
      NS_FATAL_ERROR ("Error, ChannelNumber " << +GetChannelNumber () << " was set by user, but neither a standard nor a frequency");
    }
  m_frequencyChannelNumberInitialized = true;
}

void
WifiPhy::SetRxSensitivity (double threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_rxSensitivityW = DbmToW (threshold);
}

double
WifiPhy::GetRxSensitivity (void) const
{
  return WToDbm (m_rxSensitivityW);
}

void
WifiPhy::SetCcaEdThreshold (double threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_ccaEdThresholdW = DbmToW (threshold);
}

double
WifiPhy::GetCcaEdThreshold (void) const
{
  return WToDbm (m_ccaEdThresholdW);
}

void
WifiPhy::SetRxNoiseFigure (double noiseFigureDb)
{
  NS_LOG_FUNCTION (this << noiseFigureDb);
  m_interference.SetNoiseFigure (DbToRatio (noiseFigureDb));
  m_interference.SetNumberOfReceiveAntennas (GetNumberOfAntennas ());
}

void
WifiPhy::SetTxPowerStart (double start)
{
  NS_LOG_FUNCTION (this << start);
  m_txPowerBaseDbm = start;
}

double
WifiPhy::GetTxPowerStart (void) const
{
  return m_txPowerBaseDbm;
}

void
WifiPhy::SetTxPowerEnd (double end)
{
  NS_LOG_FUNCTION (this << end);
  m_txPowerEndDbm = end;
}

double
WifiPhy::GetTxPowerEnd (void) const
{
  return m_txPowerEndDbm;
}

void
WifiPhy::SetNTxPower (uint8_t n)
{
  NS_LOG_FUNCTION (this << +n);
  m_nTxPower = n;
}

uint8_t
WifiPhy::GetNTxPower (void) const
{
  return m_nTxPower;
}

void
WifiPhy::SetTxGain (double gain)
{
  NS_LOG_FUNCTION (this << gain);
  m_txGainDb = gain;
}

double
WifiPhy::GetTxGain (void) const
{
  return m_txGainDb;
}

void
WifiPhy::SetRxGain (double gain)
{
  NS_LOG_FUNCTION (this << gain);
  m_rxGainDb = gain;
}

double
WifiPhy::GetRxGain (void) const
{
  return m_rxGainDb;
}

void
WifiPhy::SetShortPhyPreambleSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_shortPreamble = enable;
}

bool
WifiPhy::GetShortPhyPreambleSupported (void) const
{
  return m_shortPreamble;
}

void
WifiPhy::SetDevice (const Ptr<NetDevice> device)
{
  m_device = device;
}

Ptr<NetDevice>
WifiPhy::GetDevice (void) const
{
  return m_device;
}

void
WifiPhy::SetMobility (const Ptr<MobilityModel> mobility)
{
  m_mobility = mobility;
}

Ptr<MobilityModel>
WifiPhy::GetMobility (void) const
{
  if (m_mobility != 0)
    {
      return m_mobility;
    }
  else
    {
      return m_device->GetNode ()->GetObject<MobilityModel> ();
    }
}

void
WifiPhy::SetErrorRateModel (const Ptr<ErrorRateModel> rate)
{
  m_interference.SetErrorRateModel (rate);
  m_interference.SetNumberOfReceiveAntennas (GetNumberOfAntennas ());
}

void
WifiPhy::SetPostReceptionErrorModel (const Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_postReceptionErrorModel = em;
}

void
WifiPhy::SetFrameCaptureModel (const Ptr<FrameCaptureModel> model)
{
  m_frameCaptureModel = model;
}

void
WifiPhy::SetPreambleDetectionModel (const Ptr<PreambleDetectionModel> model)
{
  m_preambleDetectionModel = model;
}

void
WifiPhy::SetWifiRadioEnergyModel (const Ptr<WifiRadioEnergyModel> wifiRadioEnergyModel)
{
  m_wifiRadioEnergyModel = wifiRadioEnergyModel;
}

double
WifiPhy::GetPowerDbm (uint8_t power) const
{
  NS_ASSERT (m_txPowerBaseDbm <= m_txPowerEndDbm);
  NS_ASSERT (m_nTxPower > 0);
  double dbm;
  if (m_nTxPower > 1)
    {
      dbm = m_txPowerBaseDbm + power * (m_txPowerEndDbm - m_txPowerBaseDbm) / (m_nTxPower - 1);
    }
  else
    {
      NS_ASSERT_MSG (m_txPowerBaseDbm == m_txPowerEndDbm, "cannot have TxPowerEnd != TxPowerStart with TxPowerLevels == 1");
      dbm = m_txPowerBaseDbm;
    }
  return dbm;
}

Time
WifiPhy::GetChannelSwitchDelay (void) const
{
  return m_channelSwitchDelay;
}

double
WifiPhy::CalculateSnr (WifiTxVector txVector, double ber) const
{
  return m_interference.GetErrorRateModel ()->CalculateSnr (txVector, ber);
}

void
WifiPhy::ConfigureDefaultsForStandard (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_standard)
    {
    case WIFI_PHY_STANDARD_80211a:
      SetChannelWidth (20);
      SetFrequency (5180);
      // Channel number should be aligned by SetFrequency () to 36
      NS_ASSERT (GetChannelNumber () == 36);
      break;
    case WIFI_PHY_STANDARD_80211b:
      SetChannelWidth (22);
      SetFrequency (2412);
      // Channel number should be aligned by SetFrequency () to 1
      NS_ASSERT (GetChannelNumber () == 1);
      break;
    case WIFI_PHY_STANDARD_80211g:
      SetChannelWidth (20);
      SetFrequency (2412);
      // Channel number should be aligned by SetFrequency () to 1
      NS_ASSERT (GetChannelNumber () == 1);
      break;
    case WIFI_PHY_STANDARD_80211p:
      if (GetChannelWidth () > 10)
        {
          SetChannelWidth (10);
        }
      SetFrequency (5860);
      // Channel number should be aligned by SetFrequency () to either 172 or 171
      NS_ASSERT ((GetChannelWidth () == 10 && GetChannelNumber () == 172) || (GetChannelWidth () == 5 && GetChannelNumber () == 171)) ;
      break;
    case WIFI_PHY_STANDARD_holland:
      SetChannelWidth (20);
      SetFrequency (5180);
      // Channel number should be aligned by SetFrequency () to 36
      NS_ASSERT (GetChannelNumber () == 36);
      break;
    case WIFI_PHY_STANDARD_80211n:
      SetChannelWidth (20);
      if (m_band == WIFI_PHY_BAND_2_4GHZ)
        {
          SetFrequency (2412);
          // Channel number should be aligned by SetFrequency () to 1
          NS_ASSERT (GetChannelNumber () == 1);
        }
      else if (m_band == WIFI_PHY_BAND_5GHZ)
        {
          SetFrequency (5180);
          // Channel number should be aligned by SetFrequency () to 36
          NS_ASSERT (GetChannelNumber () == 36);
        }
      else
        {
          NS_FATAL_ERROR ("Invalid band");
        }
      break;
    case WIFI_PHY_STANDARD_80211ac:
      SetChannelWidth (80);
      SetFrequency (5210);
      // Channel number should be aligned by SetFrequency () to 42
      NS_ASSERT (GetChannelNumber () == 42);
      break;
    case WIFI_PHY_STANDARD_80211ax:
      if (m_band == WIFI_PHY_BAND_2_4GHZ)
        {
          SetChannelWidth (20);
          SetFrequency (2412);
          // Channel number should be aligned by SetFrequency () to 1
          NS_ASSERT (GetChannelNumber () == 1);
        }
      else if (m_band == WIFI_PHY_BAND_5GHZ)
        {
          SetChannelWidth (80);
          SetFrequency (5210);
          // Channel number should be aligned by SetFrequency () to 42
          NS_ASSERT (GetChannelNumber () == 42);
        }
      else if (m_band == WIFI_PHY_BAND_6GHZ)
        {
          SetChannelWidth (80);
          SetFrequency (5975);
          // Channel number should be aligned by SetFrequency () to 7
          NS_ASSERT (GetChannelNumber () == 7);
        }
      else
        {
          NS_FATAL_ERROR ("Invalid band");
        }
      break;
    case WIFI_PHY_STANDARD_UNSPECIFIED:
    default:
      NS_LOG_WARN ("Configuring unspecified standard; performing no action");
      break;
    }
}

const Ptr<const PhyEntity>
WifiPhy::GetStaticPhyEntity (WifiModulationClass modulation)
{
  const auto it = m_staticPhyEntities.find (modulation);
  NS_ABORT_MSG_IF (it == m_staticPhyEntities.end (), "Unimplemented Wi-Fi modulation class");
  return it->second;
}

const Ptr<const PhyEntity>
WifiPhy::GetPhyEntity (WifiModulationClass modulation) const
{
  const auto it = m_phyEntities.find (modulation);
  NS_ABORT_MSG_IF (it == m_phyEntities.end (), "Unsupported Wi-Fi modulation class");
  return it->second;
}

Ptr<PhyEntity>
WifiPhy::GetPhyEntity (WifiModulationClass modulation)
{
  //This method returns a non-const pointer to be used by child classes
  const auto it = m_phyEntities.find (modulation);
  NS_ABORT_MSG_IF (it == m_phyEntities.end (), "Unsupported Wi-Fi modulation class");
  return it->second;
}

void
WifiPhy::AddStaticPhyEntity (WifiModulationClass modulation, Ptr<PhyEntity> phyEntity)
{
  NS_LOG_FUNCTION (modulation);
  NS_ASSERT_MSG (m_staticPhyEntities.find (modulation) == m_staticPhyEntities.end (), "The PHY entity has already been added. The setting should only be done once per modulation class");
  m_staticPhyEntities[modulation] = phyEntity;
}

void
WifiPhy::AddPhyEntity (WifiModulationClass modulation, Ptr<PhyEntity> phyEntity)
{
  NS_LOG_FUNCTION (this << modulation);
  NS_ABORT_MSG_IF (m_staticPhyEntities.find (modulation) == m_staticPhyEntities.end (), "Cannot add an unimplemented PHY to supported list. Update the former first.");
  NS_ASSERT_MSG (m_phyEntities.find (modulation) == m_phyEntities.end (), "The PHY entity has already been added. The setting should only be done once per modulation class");
  m_phyEntities[modulation] = phyEntity;
}

void
WifiPhy::SetSifs (Time sifs)
{
  m_sifs = sifs;
}

Time
WifiPhy::GetSifs (void) const
{
  return m_sifs;
}

void
WifiPhy::SetSlot (Time slot)
{
  m_slot = slot;
}

Time
WifiPhy::GetSlot (void) const
{
  return m_slot;
}

void
WifiPhy::SetPifs (Time pifs)
{
  m_pifs = pifs;
}

Time
WifiPhy::GetPifs (void) const
{
  return m_pifs;
}

Time
WifiPhy::GetAckTxTime (void) const
{
  return m_ackTxTime;
}

Time
WifiPhy::GetBlockAckTxTime (void) const
{
  return m_blockAckTxTime;
}

void
WifiPhy::Configure80211a (void)
{
  NS_LOG_FUNCTION (this);
  AddPhyEntity (WIFI_MOD_CLASS_OFDM, Create<OfdmPhy> ());

  // See Table 17-21 "OFDM PHY characteristics" of 802.11-2016
  SetSifs (MicroSeconds (16));
  SetSlot (MicroSeconds (9));
  SetPifs (GetSifs () + GetSlot ());
  // See Table 10-5 "Determination of the EstimatedAckTxTime based on properties
  // of the PPDU causing the EIFS" of 802.11-2016
  m_ackTxTime = MicroSeconds (44);
}

void
WifiPhy::Configure80211b (void)
{
  NS_LOG_FUNCTION (this);
  Ptr<DsssPhy> phyEntity = Create<DsssPhy> ();
  AddPhyEntity (WIFI_MOD_CLASS_HR_DSSS, phyEntity);
  AddPhyEntity (WIFI_MOD_CLASS_DSSS, phyEntity); //when plain DSSS modes are used

  // See Table 16-4 "HR/DSSS PHY characteristics" of 802.11-2016
  SetSifs (MicroSeconds (10));
  SetSlot (MicroSeconds (20));
  SetPifs (GetSifs () + GetSlot ());
  // See Table 10-5 "Determination of the EstimatedAckTxTime based on properties
  // of the PPDU causing the EIFS" of 802.11-2016
  m_ackTxTime = MicroSeconds (304);
}

void
WifiPhy::Configure80211g (void)
{
  NS_LOG_FUNCTION (this);
  // See Table 18-5 "ERP characteristics" of 802.11-2016
  // Slot time defaults to the "long slot time" of 20 us in the standard
  // according to mixed 802.11b/g deployments.  Short slot time is enabled
  // if the user sets the ShortSlotTimeSupported flag to true and when the BSS
  // consists of only ERP STAs capable of supporting this option.
  Configure80211b ();
  AddPhyEntity (WIFI_MOD_CLASS_ERP_OFDM, Create<ErpOfdmPhy> ());
}

void
WifiPhy::Configure80211p (void)
{
  NS_LOG_FUNCTION (this);
  if (GetChannelWidth () == 10)
    {
      AddPhyEntity (WIFI_MOD_CLASS_OFDM, Create<OfdmPhy> (OFDM_PHY_10_MHZ));

      // See Table 17-21 "OFDM PHY characteristics" of 802.11-2016
      SetSifs (MicroSeconds (32));
      SetSlot (MicroSeconds (13));
      SetPifs (GetSifs () + GetSlot ());
      m_ackTxTime = MicroSeconds (88);
    }
  else if (GetChannelWidth () == 5)
    {
      AddPhyEntity (WIFI_MOD_CLASS_OFDM, Create<OfdmPhy> (OFDM_PHY_5_MHZ));

      // See Table 17-21 "OFDM PHY characteristics" of 802.11-2016
      SetSifs (MicroSeconds (64));
      SetSlot (MicroSeconds (21));
      SetPifs (GetSifs () + GetSlot ());
      m_ackTxTime = MicroSeconds (176);
    }
  else
    {
      NS_FATAL_ERROR ("802.11p configured with a wrong channel width!");
    }
}

void
WifiPhy::ConfigureHolland (void)
{
  NS_LOG_FUNCTION (this);
  AddPhyEntity (WIFI_MOD_CLASS_OFDM, Create<OfdmPhy> (OFDM_PHY_HOLLAND));

  SetSifs (MicroSeconds (16));
  SetSlot (MicroSeconds (9));
  SetPifs (GetSifs () + GetSlot ());
}

void
WifiPhy::Configure80211n (void)
{
  NS_LOG_FUNCTION (this);
  if (m_band == WIFI_PHY_BAND_2_4GHZ)
    {
      Configure80211g ();
    }
  else
    {
      Configure80211a ();
    }
  AddPhyEntity (WIFI_MOD_CLASS_HT, Create<HtPhy> (m_txSpatialStreams));

  // See Table 10-5 "Determination of the EstimatedAckTxTime based on properties
  // of the PPDU causing the EIFS" of 802.11-2016
  m_blockAckTxTime = MicroSeconds (68);
}

void
WifiPhy::Configure80211ac (void)
{
  NS_LOG_FUNCTION (this);
  Configure80211n ();
  AddPhyEntity (WIFI_MOD_CLASS_VHT, Create<VhtPhy> ());
}

void
WifiPhy::Configure80211ax (void)
{
  NS_LOG_FUNCTION (this);
  if (m_band == WIFI_PHY_BAND_2_4GHZ)
    {
      Configure80211n ();
    }
  else
    {
      Configure80211ac ();
    }
  AddPhyEntity (WIFI_MOD_CLASS_HE, Create<HePhy> ());
}

bool
WifiPhy::DefineChannelNumber (uint8_t channelNumber, WifiPhyBand band, WifiPhyStandard standard, uint16_t frequency, uint16_t channelWidth)
{
  NS_LOG_FUNCTION (this << +channelNumber << band << standard << frequency << channelWidth);
  ChannelNumberStandardPair p = std::make_pair (std::make_pair (channelNumber, band), standard);
  ChannelToFrequencyWidthMap::const_iterator it;
  it = m_channelToFrequencyWidth.find (p);
  if (it != m_channelToFrequencyWidth.end ())
    {
      NS_LOG_DEBUG ("channel number/standard already defined; returning false");
      return false;
    }
  FrequencyWidthPair f = std::make_pair (frequency, channelWidth);
  m_channelToFrequencyWidth[p] = f;
  return true;
}

uint8_t
WifiPhy::FindChannelNumberForFrequencyWidth (uint16_t frequency, uint16_t width) const
{
  NS_LOG_FUNCTION (this << frequency << width);
  bool found = false;
  FrequencyWidthPair f = std::make_pair (frequency, width);
  ChannelToFrequencyWidthMap::const_iterator it = m_channelToFrequencyWidth.begin ();
  while (it != m_channelToFrequencyWidth.end ())
    {
      if (it->second == f)
        {
          found = true;
          break;
        }
      ++it;
    }
  if (found)
    {
      NS_LOG_DEBUG ("Found, returning " << +it->first.first.first);
      return (it->first.first.first);
    }
  else
    {
      NS_LOG_DEBUG ("Not found, returning 0");
      return 0;
    }
}

void
WifiPhy::ConfigureChannelForStandard (void)
{
  NS_LOG_FUNCTION (this);
  // If the user has configured both Frequency and ChannelNumber, Frequency
  // takes precedence
  if (GetFrequency () != 0)
    {
      // If Frequency is already set, then see whether a ChannelNumber can
      // be found that matches Frequency and ChannelWidth. If so, configure
      // the ChannelNumber to that channel number. If not, set ChannelNumber to zero.
      NS_LOG_DEBUG ("Frequency set; checking whether a channel number corresponds");
      uint8_t channelNumberSearched = FindChannelNumberForFrequencyWidth (GetFrequency (), GetChannelWidth ());
      if (channelNumberSearched)
        {
          NS_LOG_DEBUG ("Channel number found; setting to " << +channelNumberSearched);
          SetChannelNumber (channelNumberSearched);
        }
      else
        {
          NS_LOG_DEBUG ("Channel number not found; setting to zero");
          SetChannelNumber (0);
        }
    }
  else if (GetChannelNumber () != 0)
    {
      // If the channel number is known for this particular standard or for
      // the unspecified standard, configure using the known values;
      // otherwise, this is a configuration error
      NS_LOG_DEBUG ("Configuring for channel number " << +GetChannelNumber ());
      FrequencyWidthPair f = GetFrequencyWidthForChannelNumberStandard (GetChannelNumber (), GetPhyBand (), GetPhyStandard ());
      if (f.first == 0)
        {
          // the specific pair of number/standard is not known
          NS_LOG_DEBUG ("Falling back to check WIFI_PHY_STANDARD_UNSPECIFIED");
          f = GetFrequencyWidthForChannelNumberStandard (GetChannelNumber (), GetPhyBand (), WIFI_PHY_STANDARD_UNSPECIFIED);
        }
      if (f.first == 0)
        {
          NS_FATAL_ERROR ("Error, ChannelNumber " << +GetChannelNumber () << " is unknown for this standard");
        }
      else
        {
          NS_LOG_DEBUG ("Setting frequency to " << f.first << "; width to " << +f.second);
          SetFrequency (f.first);
          SetChannelWidth (f.second);
        }
    }
}

void
WifiPhy::ConfigureStandardAndBand (WifiPhyStandard standard, WifiPhyBand band)
{
  NS_LOG_FUNCTION (this << standard << band);
  m_standard = standard;
  m_band = band;
  m_isConstructed = true;
  if (m_frequencyChannelNumberInitialized == false)
    {
      InitializeFrequencyChannelNumber ();
    }
  if (GetFrequency () == 0 && GetChannelNumber () == 0)
    {
      ConfigureDefaultsForStandard ();
    }
  else
    {
      // The user has configured either (or both) Frequency or ChannelNumber
      ConfigureChannelForStandard ();
    }
  switch (standard)
    {
    case WIFI_PHY_STANDARD_80211a:
      Configure80211a ();
      break;
    case WIFI_PHY_STANDARD_80211b:
      Configure80211b ();
      break;
    case WIFI_PHY_STANDARD_80211g:
      Configure80211g ();
      break;
    case WIFI_PHY_STANDARD_80211p:
      Configure80211p ();
      break;
    case WIFI_PHY_STANDARD_holland:
      ConfigureHolland ();
      break;
    case WIFI_PHY_STANDARD_80211n:
      Configure80211n ();
      break;
    case WIFI_PHY_STANDARD_80211ac:
      Configure80211ac ();
      break;
    case WIFI_PHY_STANDARD_80211ax:
      Configure80211ax ();
      break;
    case WIFI_PHY_STANDARD_UNSPECIFIED:
    default:
      NS_ASSERT_MSG (false, "Unsupported standard");
      break;
    }
}

WifiPhyBand
WifiPhy::GetPhyBand (void) const
{
  return m_band;
}


WifiPhyStandard
WifiPhy::GetPhyStandard (void) const
{
  return m_standard;
}

void
WifiPhy::SetFrequency (uint16_t frequency)
{
  NS_LOG_FUNCTION (this << frequency);
  if (m_isConstructed == false)
    {
      NS_LOG_DEBUG ("Saving frequency configuration for initialization");
      m_initialFrequency = frequency;
      return;
    }
  if (GetFrequency () == frequency)
    {
      NS_LOG_DEBUG ("No frequency change requested");
      return;
    }
  if (frequency == 0)
    {
      DoFrequencySwitch (0);
      NS_LOG_DEBUG ("Setting frequency and channel number to zero");
      m_channelCenterFrequency = 0;
      m_channelNumber = 0;
      return;
    }
  // If the user has configured both Frequency and ChannelNumber, Frequency
  // takes precedence.  Lookup the channel number corresponding to the
  // requested frequency.
  uint8_t nch = FindChannelNumberForFrequencyWidth (frequency, GetChannelWidth ());
  if (nch != 0)
    {
      NS_LOG_DEBUG ("Setting frequency " << frequency << " corresponds to channel " << +nch);
      if (DoFrequencySwitch (frequency))
        {
          NS_LOG_DEBUG ("Channel frequency switched to " << frequency << "; channel number to " << +nch);
          m_channelCenterFrequency = frequency;
          m_channelNumber = nch;
        }
      else
        {
          NS_LOG_DEBUG ("Suppressing reassignment of frequency");
        }
    }
  else
    {
      NS_LOG_DEBUG ("Channel number is unknown for frequency " << frequency);
      if (DoFrequencySwitch (frequency))
        {
          NS_LOG_DEBUG ("Channel frequency switched to " << frequency << "; channel number to " << 0);
          m_channelCenterFrequency = frequency;
          m_channelNumber = 0;
        }
      else
        {
          NS_LOG_DEBUG ("Suppressing reassignment of frequency");
        }
    }
}

uint16_t
WifiPhy::GetFrequency (void) const
{
  return m_channelCenterFrequency;
}

void
WifiPhy::SetChannelWidth (uint16_t channelWidth)
{
  NS_LOG_FUNCTION (this << channelWidth);
  NS_ASSERT_MSG (channelWidth == 5 || channelWidth == 10 || channelWidth == 20 || channelWidth == 22 || channelWidth == 40 || channelWidth == 80 || channelWidth == 160, "wrong channel width value");
  bool changed = (m_channelWidth != channelWidth);
  m_channelWidth = channelWidth;
  AddSupportedChannelWidth (channelWidth);
  if (changed && !m_capabilitiesChangedCallback.IsNull ())
    {
      m_capabilitiesChangedCallback ();
    }
}

uint16_t
WifiPhy::GetChannelWidth (void) const
{
  return m_channelWidth;
}

void
WifiPhy::SetNumberOfAntennas (uint8_t antennas)
{
  NS_ASSERT_MSG (antennas > 0 && antennas <= 4, "unsupported number of antennas");
  m_numberOfAntennas = antennas;
  m_interference.SetNumberOfReceiveAntennas (antennas);
}

uint8_t
WifiPhy::GetNumberOfAntennas (void) const
{
  return m_numberOfAntennas;
}

void
WifiPhy::SetMaxSupportedTxSpatialStreams (uint8_t streams)
{
  NS_ASSERT (streams <= GetNumberOfAntennas ());
  bool changed = (m_txSpatialStreams != streams);
  m_txSpatialStreams = streams;
  if (changed)
    {
      auto phyEntity = m_phyEntities.find (WIFI_MOD_CLASS_HT);
      if (phyEntity != m_phyEntities.end ())
        {
          Ptr<HtPhy> htPhy = DynamicCast<HtPhy> (phyEntity->second);
          if (htPhy)
            {
              htPhy->SetMaxSupportedNss (m_txSpatialStreams); //this is essential to have the right MCSs configured
            }

          if (!m_capabilitiesChangedCallback.IsNull ())
            {
              m_capabilitiesChangedCallback ();
            }
        }
    }
}

uint8_t
WifiPhy::GetMaxSupportedTxSpatialStreams (void) const
{
  return m_txSpatialStreams;
}

void
WifiPhy::SetMaxSupportedRxSpatialStreams (uint8_t streams)
{
  NS_ASSERT (streams <= GetNumberOfAntennas ());
  bool changed = (m_rxSpatialStreams != streams);
  m_rxSpatialStreams = streams;
  if (changed && !m_capabilitiesChangedCallback.IsNull ())
    {
      m_capabilitiesChangedCallback ();
    }
}

uint8_t
WifiPhy::GetMaxSupportedRxSpatialStreams (void) const
{
  return m_rxSpatialStreams;
}

std::list<uint8_t>
WifiPhy::GetBssMembershipSelectorList (void) const
{
  std::list<uint8_t> list;
  for (const auto & phyEntity : m_phyEntities)
    {
      Ptr<HtPhy> htPhy = DynamicCast<HtPhy> (phyEntity.second);
      if (htPhy)
        {
          list.emplace_back (htPhy->GetBssMembershipSelector ());
        }
    }
  return list;
}

void
WifiPhy::AddSupportedChannelWidth (uint16_t width)
{
  NS_LOG_FUNCTION (this << width);
  for (std::vector<uint32_t>::size_type i = 0; i != m_supportedChannelWidthSet.size (); i++)
    {
      if (m_supportedChannelWidthSet[i] == width)
        {
          return;
        }
    }
  NS_LOG_FUNCTION ("Adding " << width << " to supported channel width set");
  m_supportedChannelWidthSet.push_back (width);
}

std::vector<uint16_t>
WifiPhy::GetSupportedChannelWidthSet (void) const
{
  return m_supportedChannelWidthSet;
}

WifiPhy::FrequencyWidthPair
WifiPhy::GetFrequencyWidthForChannelNumberStandard (uint8_t channelNumber, WifiPhyBand band, WifiPhyStandard standard) const
{
  ChannelNumberStandardPair p = std::make_pair (std::make_pair (channelNumber, band), standard);
  FrequencyWidthPair f = m_channelToFrequencyWidth[p];
  return f;
}

void
WifiPhy::SetChannelNumber (uint8_t nch)
{
  NS_LOG_FUNCTION (this << +nch);
  if (m_isConstructed == false)
    {
      NS_LOG_DEBUG ("Saving channel number configuration for initialization");
      m_initialChannelNumber = nch;
      return;
    }
  if (GetChannelNumber () == nch)
    {
      NS_LOG_DEBUG ("No channel change requested");
      return;
    }
  if (nch == 0)
    {
      // This case corresponds to when there is not a known channel
      // number for the requested frequency.  There is no need to call
      // DoChannelSwitch () because DoFrequencySwitch () should have been
      // called by the client
      NS_LOG_DEBUG ("Setting channel number to zero");
      m_channelNumber = 0;
      return;
    }

  // First make sure that the channel number is defined for the standard in use
  FrequencyWidthPair f = GetFrequencyWidthForChannelNumberStandard (nch, GetPhyBand (), GetPhyStandard ());
  if (f.first == 0)
    {
      f = GetFrequencyWidthForChannelNumberStandard (nch, GetPhyBand (), WIFI_PHY_STANDARD_UNSPECIFIED);
    }
  if (f.first != 0)
    {
      if (DoChannelSwitch (nch))
        {
          NS_LOG_DEBUG ("Setting frequency to " << f.first << "; width to " << +f.second);
          m_channelCenterFrequency = f.first;
          SetChannelWidth (f.second);
          m_channelNumber = nch;
        }
      else
        {
          // Subclass may have suppressed (e.g. waiting for state change)
          NS_LOG_DEBUG ("Channel switch suppressed");
        }
    }
  else
    {
      NS_FATAL_ERROR ("Frequency not found for channel number " << +nch);
    }
}

uint8_t
WifiPhy::GetChannelNumber (void) const
{
  return m_channelNumber;
}

bool
WifiPhy::DoChannelSwitch (uint8_t nch)
{
  m_powerRestricted = false;
  m_channelAccessRequested = false;
  m_currentEvent = 0;
  m_currentPreambleEvents.clear ();
  if (!IsInitialized ())
    {
      //this is not channel switch, this is initialization
      NS_LOG_DEBUG ("initialize to channel " << +nch);
      return true;
    }

  NS_ASSERT (!IsStateSwitching ());
  switch (m_state->GetState ())
    {
    case WifiPhyState::RX:
      NS_LOG_DEBUG ("drop packet because of channel switching while reception");
      m_endPhyRxEvent.Cancel ();
      for (auto & endRxEvent : m_endRxEvents)
        {
          endRxEvent.Cancel ();
        }
      m_endRxEvents.clear ();
      for (auto & endPreambleDetectionEvent : m_endPreambleDetectionEvents)
        {
          endPreambleDetectionEvent.Cancel ();
        }
      m_endPreambleDetectionEvents.clear ();
      for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
        {
          beginOfdmaPayloadRxEvent.second.Cancel ();
        }
      m_beginOfdmaPayloadRxEvents.clear ();
      goto switchChannel;
      break;
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("channel switching postponed until end of current transmission");
      Simulator::Schedule (GetDelayUntilIdle (), &WifiPhy::SetChannelNumber, this, nch);
      break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
        {
          for (auto & endPreambleDetectionEvent : m_endPreambleDetectionEvents)
            {
              endPreambleDetectionEvent.Cancel ();
            }
          m_endPreambleDetectionEvents.clear ();
          for (auto & endRxEvent : m_endRxEvents)
            {
              endRxEvent.Cancel ();
            }
          m_endRxEvents.clear ();
          for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
            {
              beginOfdmaPayloadRxEvent.second.Cancel ();
            }
          m_beginOfdmaPayloadRxEvents.clear ();
        }
      goto switchChannel;
      break;
    case WifiPhyState::SLEEP:
      NS_LOG_DEBUG ("channel switching ignored in sleep mode");
      break;
    default:
      NS_ASSERT (false);
      break;
    }

  return false;

switchChannel:

  NS_LOG_DEBUG ("switching channel " << +GetChannelNumber () << " -> " << +nch);
  m_state->SwitchToChannelSwitching (GetChannelSwitchDelay ());
  m_interference.EraseEvents ();
  /*
   * Needed here to be able to correctly sensed the medium for the first
   * time after the switching. The actual switching is not performed until
   * after m_channelSwitchDelay. Packets received during the switching
   * state are added to the event list and are employed later to figure
   * out the state of the medium after the switching.
   */
  return true;
}

bool
WifiPhy::DoFrequencySwitch (uint16_t frequency)
{
  m_powerRestricted = false;
  m_channelAccessRequested = false;
  m_currentEvent = 0;
  m_currentPreambleEvents.clear ();
  if (!IsInitialized ())
    {
      //this is not channel switch, this is initialization
      NS_LOG_DEBUG ("start at frequency " << frequency);
      return true;
    }

  NS_ASSERT (!IsStateSwitching ());
  switch (m_state->GetState ())
    {
    case WifiPhyState::RX:
      NS_LOG_DEBUG ("drop packet because of channel/frequency switching while reception");
      m_endPhyRxEvent.Cancel ();
      for (auto & endRxEvent : m_endRxEvents)
        {
          endRxEvent.Cancel ();
        }
      m_endRxEvents.clear ();
      for (auto & endPreambleDetectionEvent : m_endPreambleDetectionEvents)
        {
          endPreambleDetectionEvent.Cancel ();
        }
      m_endPreambleDetectionEvents.clear ();
      for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
        {
          beginOfdmaPayloadRxEvent.second.Cancel ();
        }
      m_beginOfdmaPayloadRxEvents.clear ();
      goto switchFrequency;
      break;
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("channel/frequency switching postponed until end of current transmission");
      Simulator::Schedule (GetDelayUntilIdle (), &WifiPhy::SetFrequency, this, frequency);
      break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
      for (auto & endPreambleDetectionEvent : m_endPreambleDetectionEvents)
        {
          endPreambleDetectionEvent.Cancel ();
        }
      m_endPreambleDetectionEvents.clear ();
      for (auto & endRxEvent : m_endRxEvents)
        {
          endRxEvent.Cancel ();
        }
      m_endRxEvents.clear ();
      goto switchFrequency;
      for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
        {
          beginOfdmaPayloadRxEvent.second.Cancel ();
        }
      m_beginOfdmaPayloadRxEvents.clear ();
      break;
    case WifiPhyState::SLEEP:
      NS_LOG_DEBUG ("frequency switching ignored in sleep mode");
      break;
    default:
      NS_ASSERT (false);
      break;
    }

  return false;

switchFrequency:

  NS_LOG_DEBUG ("switching frequency " << GetFrequency () << " -> " << frequency);
  m_state->SwitchToChannelSwitching (GetChannelSwitchDelay ());
  m_interference.EraseEvents ();
  /*
   * Needed here to be able to correctly sensed the medium for the first
   * time after the switching. The actual switching is not performed until
   * after m_channelSwitchDelay. Packets received during the switching
   * state are added to the event list and are employed later to figure
   * out the state of the medium after the switching.
   */
  return true;
}

void
WifiPhy::SetSleepMode (void)
{
  NS_LOG_FUNCTION (this);
  m_powerRestricted = false;
  m_channelAccessRequested = false;
  switch (m_state->GetState ())
    {
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("setting sleep mode postponed until end of current transmission");
      Simulator::Schedule (GetDelayUntilIdle (), &WifiPhy::SetSleepMode, this);
      break;
    case WifiPhyState::RX:
      NS_LOG_DEBUG ("setting sleep mode postponed until end of current reception");
      Simulator::Schedule (GetDelayUntilIdle (), &WifiPhy::SetSleepMode, this);
      break;
    case WifiPhyState::SWITCHING:
      NS_LOG_DEBUG ("setting sleep mode postponed until end of channel switching");
      Simulator::Schedule (GetDelayUntilIdle (), &WifiPhy::SetSleepMode, this);
      break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
      NS_LOG_DEBUG ("setting sleep mode");
      m_state->SwitchToSleep ();
      break;
    case WifiPhyState::SLEEP:
      NS_LOG_DEBUG ("already in sleep mode");
      break;
    default:
      NS_ASSERT (false);
      break;
    }
}

void
WifiPhy::SetOffMode (void)
{
  NS_LOG_FUNCTION (this);
  m_powerRestricted = false;
  m_channelAccessRequested = false;
  m_endPhyRxEvent.Cancel ();
  m_endTxEvent.Cancel ();
  for (auto & endRxEvent : m_endRxEvents)
    {
      endRxEvent.Cancel ();
    }
  m_endRxEvents.clear ();
  for (auto & endPreambleDetectionEvent : m_endPreambleDetectionEvents)
    {
      endPreambleDetectionEvent.Cancel ();
    }
  m_endPreambleDetectionEvents.clear ();
  for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
    {
      beginOfdmaPayloadRxEvent.second.Cancel ();
    }
  m_beginOfdmaPayloadRxEvents.clear ();
  m_state->SwitchToOff ();
}

void
WifiPhy::ResumeFromSleep (void)
{
  NS_LOG_FUNCTION (this);
  m_currentPreambleEvents.clear ();
  switch (m_state->GetState ())
    {
    case WifiPhyState::TX:
    case WifiPhyState::RX:
    case WifiPhyState::IDLE:
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::SWITCHING:
      {
        NS_LOG_DEBUG ("not in sleep mode, there is nothing to resume");
        break;
      }
    case WifiPhyState::SLEEP:
      {
        NS_LOG_DEBUG ("resuming from sleep mode");
        Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW, GetBand (GetMeasurementChannelWidth (nullptr)));
        m_state->SwitchFromSleep (delayUntilCcaEnd);
        break;
      }
    default:
      {
        NS_ASSERT (false);
        break;
      }
    }
}

void
WifiPhy::ResumeFromOff (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state->GetState ())
    {
    case WifiPhyState::TX:
    case WifiPhyState::RX:
    case WifiPhyState::IDLE:
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::SWITCHING:
    case WifiPhyState::SLEEP:
      {
        NS_LOG_DEBUG ("not in off mode, there is nothing to resume");
        break;
      }
    case WifiPhyState::OFF:
      {
        NS_LOG_DEBUG ("resuming from off mode");
        Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW, GetBand (GetMeasurementChannelWidth (nullptr)));
        m_state->SwitchFromOff (delayUntilCcaEnd);
        break;
      }
    default:
      {
        NS_ASSERT (false);
        break;
      }
    }
}

Time
WifiPhy::GetPreambleDetectionDuration (void)
{
  return MicroSeconds (4);
}

Time
WifiPhy::GetStartOfPacketDuration (WifiTxVector txVector)
{
  return MicroSeconds (4);
}

Time
WifiPhy::GetPayloadDuration (uint32_t size, WifiTxVector txVector, WifiPhyBand band, MpduType mpdutype, uint16_t staId)
{
  uint32_t totalAmpduSize;
  double totalAmpduNumSymbols;
  return GetPayloadDuration (size, txVector, band, mpdutype, false, totalAmpduSize, totalAmpduNumSymbols, staId);
}

Time
WifiPhy::GetPayloadDuration (uint32_t size, WifiTxVector txVector, WifiPhyBand band, MpduType mpdutype,
                             bool incFlag, uint32_t &totalAmpduSize, double &totalAmpduNumSymbols,
                             uint16_t staId)
{
  return GetStaticPhyEntity (txVector.GetModulationClass ())->GetPayloadDuration (size, txVector, band, mpdutype,
                                                                                  incFlag, totalAmpduSize, totalAmpduNumSymbols,
                                                                                  staId);
}

Time
WifiPhy::CalculatePhyPreambleAndHeaderDuration (WifiTxVector txVector)
{
  return GetStaticPhyEntity (txVector.GetModulationClass ())->CalculatePhyPreambleAndHeaderDuration (txVector);
}

Time
WifiPhy::GetPpduFieldDuration (WifiPpduField field, WifiTxVector txVector)
{
  return GetStaticPhyEntity (txVector.GetModulationClass ())->GetDuration (field, txVector);
}

WifiMode
WifiPhy::GetNonHtHeaderMode (WifiTxVector txVector)
{
  //Wrapper for InterferenceHelper. TODO: cleanup once amendment-specific logic has been moved out of the latter
  return GetStaticPhyEntity (txVector.GetModulationClass ())->GetSigMode (WIFI_PPDU_FIELD_NON_HT_HEADER, txVector);
}

WifiMode
WifiPhy::GetSigMode (WifiTxVector txVector)
{
  //Wrapper for InterferenceHelper. TODO: cleanup once amendment-specific logic has been moved out of the latter
  WifiPpduField field = (txVector.GetModulationClass () == WIFI_MOD_CLASS_HT) ? WIFI_PPDU_FIELD_HT_SIG : WIFI_PPDU_FIELD_SIG_A;
  return GetStaticPhyEntity (txVector.GetModulationClass ())->GetSigMode (field, txVector);
}

Time
WifiPhy::CalculateNonOfdmaDurationForHeTb (WifiTxVector txVector)
{
  NS_ASSERT (txVector.GetModulationClass () == WIFI_MOD_CLASS_HE);
  auto hePhy = DynamicCast<const HePhy> (GetStaticPhyEntity (txVector.GetModulationClass ()));
  return (hePhy ? hePhy->CalculateNonOfdmaDurationForHeTb (txVector) : NanoSeconds (0));
}

Time
WifiPhy::CalculateTxDuration (uint32_t size, WifiTxVector txVector, WifiPhyBand band, uint16_t staId)
{
  Time duration = CalculatePhyPreambleAndHeaderDuration (txVector)
    + GetPayloadDuration (size, txVector, band, NORMAL_MPDU, staId);
  NS_ASSERT (duration.IsStrictlyPositive ());
  return duration;
}

Time
WifiPhy::CalculateTxDuration (WifiConstPsduMap psduMap, WifiTxVector txVector, WifiPhyBand band)
{
  //TODO: Move this logic to HePhy
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB)
    {
      return ConvertLSigLengthToHeTbPpduDuration (txVector.GetLength (), txVector, band);
    }

  Time maxDuration = Seconds (0);
  for (auto & staIdPsdu : psduMap)
    {
      if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_MU)
        {
          WifiTxVector::HeMuUserInfoMap userInfoMap = txVector.GetHeMuUserInfoMap ();
          NS_ABORT_MSG_IF (userInfoMap.find (staIdPsdu.first) == userInfoMap.end (), "STA-ID in psduMap (" << staIdPsdu.first << ") should be referenced in txVector");
        }
      Time current = CalculateTxDuration (staIdPsdu.second->GetSize (), txVector, band, staIdPsdu.first);
      if (current > maxDuration)
        {
          maxDuration = current;
        }
    }
  NS_ASSERT (maxDuration.IsStrictlyPositive ());
  return maxDuration;
}

void
WifiPhy::NotifyTxBegin (WifiConstPsduMap psdus, double txPowerW)
{
  for (auto const& psdu : psdus)
    {
      for (auto& mpdu : *PeekPointer (psdu.second))
        {
          m_phyTxBeginTrace (mpdu->GetProtocolDataUnit (), txPowerW);
        }
    }
}

void
WifiPhy::NotifyTxEnd (WifiConstPsduMap psdus)
{
  for (auto const& psdu : psdus)
    {
      for (auto& mpdu : *PeekPointer (psdu.second))
        {
          m_phyTxEndTrace (mpdu->GetProtocolDataUnit ());
        }
    }
}

void
WifiPhy::NotifyTxDrop (Ptr<const WifiPsdu> psdu)
{
  for (auto& mpdu : *PeekPointer (psdu))
    {
      m_phyTxDropTrace (mpdu->GetProtocolDataUnit ());
    }
}

void
WifiPhy::NotifyRxBegin (Ptr<const WifiPsdu> psdu, RxPowerWattPerChannelBand rxPowersW)
{
  if (psdu)
    {
      for (auto& mpdu : *PeekPointer (psdu))
        {
          m_phyRxBeginTrace (mpdu->GetProtocolDataUnit (), rxPowersW);
        }
    }
}

void
WifiPhy::NotifyRxEnd (Ptr<const WifiPsdu> psdu)
{
  if (psdu)
    {
      for (auto& mpdu : *PeekPointer (psdu))
        {
          m_phyRxEndTrace (mpdu->GetProtocolDataUnit ());
        }
    }
}

void
WifiPhy::NotifyRxDrop (Ptr<const WifiPsdu> psdu, WifiPhyRxfailureReason reason)
{
  if (psdu)
    {
      for (auto& mpdu : *PeekPointer (psdu))
        {
          m_phyRxDropTrace (mpdu->GetProtocolDataUnit (), reason);
        }
    }
}

void
WifiPhy::NotifyMonitorSniffRx (Ptr<const WifiPsdu> psdu, uint16_t channelFreqMhz, WifiTxVector txVector,
                               SignalNoiseDbm signalNoise, std::vector<bool> statusPerMpdu, uint16_t staId)
{
  MpduInfo aMpdu;
  if (psdu->IsAggregate ())
    {
      //Expand A-MPDU
      NS_ASSERT_MSG (txVector.IsAggregation (), "TxVector with aggregate flag expected here according to PSDU");
      aMpdu.mpduRefNumber = ++m_rxMpduReferenceNumber;
      size_t nMpdus = psdu->GetNMpdus ();
      NS_ASSERT_MSG (statusPerMpdu.size () == nMpdus, "Should have one reception status per MPDU");
      aMpdu.type = (psdu->IsSingle ()) ? SINGLE_MPDU : FIRST_MPDU_IN_AGGREGATE;
      for (size_t i = 0; i < nMpdus;)
        {
          if (statusPerMpdu.at (i)) //packet received without error, hand over to sniffer
            {
              m_phyMonitorSniffRxTrace (psdu->GetAmpduSubframe (i), channelFreqMhz, txVector, aMpdu, signalNoise, staId);
            }
          ++i;
          aMpdu.type = (i == (nMpdus - 1)) ? LAST_MPDU_IN_AGGREGATE : MIDDLE_MPDU_IN_AGGREGATE;
        }
    }
  else
    {
      aMpdu.type = NORMAL_MPDU;
      NS_ASSERT_MSG (statusPerMpdu.size () == 1, "Should have one reception status for normal MPDU");
      m_phyMonitorSniffRxTrace (psdu->GetPacket (), channelFreqMhz, txVector, aMpdu, signalNoise, staId);
    }
}

void
WifiPhy::NotifyMonitorSniffTx (Ptr<const WifiPsdu> psdu, uint16_t channelFreqMhz, WifiTxVector txVector, uint16_t staId)
{
  MpduInfo aMpdu;
  if (psdu->IsAggregate ())
    {
      //Expand A-MPDU
      NS_ASSERT_MSG (txVector.IsAggregation (), "TxVector with aggregate flag expected here according to PSDU");
      aMpdu.mpduRefNumber = ++m_rxMpduReferenceNumber;
      size_t nMpdus = psdu->GetNMpdus ();
      aMpdu.type = (psdu->IsSingle ()) ? SINGLE_MPDU: FIRST_MPDU_IN_AGGREGATE;
      for (size_t i = 0; i < nMpdus;)
        {
          m_phyMonitorSniffTxTrace (psdu->GetAmpduSubframe (i), channelFreqMhz, txVector, aMpdu, staId);
          ++i;
          aMpdu.type = (i == (nMpdus - 1)) ? LAST_MPDU_IN_AGGREGATE : MIDDLE_MPDU_IN_AGGREGATE;
        }
    }
  else
    {
      aMpdu.type = NORMAL_MPDU;
      m_phyMonitorSniffTxTrace (psdu->GetPacket (), channelFreqMhz, txVector, aMpdu, staId);
    }
}

void
WifiPhy::NotifyEndOfHeSigA (HeSigAParameters params)
{
  m_phyEndOfHeSigATrace (params);
}

void
WifiPhy::Send (Ptr<const WifiPsdu> psdu, WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << *psdu << txVector);
  WifiConstPsduMap psdus;
  psdus.insert (std::make_pair (SU_STA_ID, psdu));
  Send (psdus, txVector);
}

void
WifiPhy::Send (WifiConstPsduMap psdus, WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << psdus << txVector);
  /* Transmission can happen if:
   *  - we are syncing on a packet. It is the responsibility of the
   *    MAC layer to avoid doing this but the PHY does nothing to
   *    prevent it.
   *  - we are idle
   */
  NS_ASSERT (!m_state->IsStateTx () && !m_state->IsStateSwitching ());
  NS_ASSERT (m_endTxEvent.IsExpired ());

  if (txVector.GetNssMax () > GetMaxSupportedTxSpatialStreams ())
    {
      NS_FATAL_ERROR ("Unsupported number of spatial streams!");
    }

  if (m_state->IsStateSleep ())
    {
      NS_LOG_DEBUG ("Dropping packet because in sleep mode");
      for (auto const& psdu : psdus)
        {
          NotifyTxDrop (psdu.second);
        }
      return;
    }
  
  Time txDuration;
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB)
    {
      NS_ASSERT (txVector.GetLength () > 0);
      txDuration = ConvertLSigLengthToHeTbPpduDuration (txVector.GetLength (), txVector, GetPhyBand ());
    }
  else
    {
      txDuration = CalculateTxDuration (psdus, txVector, GetPhyBand ());
    }

  if (!m_endPreambleDetectionEvents.empty () || ((m_currentEvent != 0) && (m_currentEvent->GetEndTime () > (Simulator::Now () + m_state->GetDelayUntilIdle ()))))
    {
      AbortCurrentReception (RECEPTION_ABORTED_BY_TX);
      //that packet will be noise _after_ the transmission.
      MaybeCcaBusyDuration (GetMeasurementChannelWidth (m_currentEvent != 0 ? m_currentEvent->GetPpdu () : nullptr));
    }

  for (auto & endPreambleDetectionEvent : m_endPreambleDetectionEvents)
    {
      if (endPreambleDetectionEvent.IsRunning ())
        {
          endPreambleDetectionEvent.Cancel ();
        }
    }
  m_currentPreambleEvents.clear ();

  if (m_powerRestricted)
    {
      NS_LOG_DEBUG ("Transmitting with power restriction");
    }
  else
    {
      NS_LOG_DEBUG ("Transmitting without power restriction");
    }

  if (m_state->GetState () == WifiPhyState::OFF)
    {
      NS_LOG_DEBUG ("Transmission canceled because device is OFF");
      return;
    }

  double txPowerW = DbmToW (GetTxPowerForTransmission (txVector) + GetTxGain ());
  NotifyTxBegin (psdus, txPowerW);
  m_phyTxPsduBeginTrace (psdus, txVector, txPowerW);
  for (auto const& psdu : psdus)
    {
      NotifyMonitorSniffTx (psdu.second, GetFrequency (), txVector, psdu.first);
    }
  m_state->SwitchToTx (txDuration, psdus, GetPowerDbm (txVector.GetTxPowerLevel ()), txVector);

  uint64_t uid;
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB)
    {
      //Use UID of PPDU containing trigger frame to identify resulting HE TB PPDUs, since the latter should immediately follow the former
      NS_ASSERT (m_previouslyRxPpduUid != UINT64_MAX);
      uid = m_previouslyRxPpduUid;
    }
  else
    {
      uid = m_globalPpduUid++;
    }
  m_previouslyRxPpduUid = UINT64_MAX; //reset to use it only once
  m_previouslyTxPpduUid = uid; //to be able to identify solicited HE TB PPDUs

  Ptr<WifiPpdu> ppdu = GetPhyEntity (txVector.GetModulationClass ())->BuildPpdu (psdus, txVector, txDuration, GetPhyBand (), uid);

  if (m_wifiRadioEnergyModel != 0 && m_wifiRadioEnergyModel->GetMaximumTimeInState (WifiPhyState::TX) < txDuration)
    {
      ppdu->SetTruncatedTx ();
    }

  m_endTxEvent = Simulator::Schedule (txDuration, &WifiPhy::NotifyTxEnd, this, psdus); //TODO: fix for MU

  StartTx (ppdu, txVector.GetTxPowerLevel ()); //now that the content of the TXVECTOR is stored in the WifiPpdu through PHY headers, the method calling StartTx has to specify the TX power level to use upon transmission

  m_channelAccessRequested = false;
  m_powerRestricted = false;

  Simulator::Schedule (txDuration, &WifiPhy::Reset, this);
}

void
WifiPhy::Reset (void)
{
  NS_LOG_FUNCTION (this);
  m_currentPreambleEvents.clear ();
  m_currentEvent = 0;
  for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
    {
      beginOfdmaPayloadRxEvent.second.Cancel ();
    }
  m_beginOfdmaPayloadRxEvents.clear ();
}

void
WifiPhy::StartReceiveHeader (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (!IsStateRx ());
  NS_ASSERT (m_endPhyRxEvent.IsExpired ());

  //calculate PER on the measurement channel for PHY headers
  uint16_t measurementChannelWidth = GetMeasurementChannelWidth (event->GetPpdu ());
  auto measurementBand = GetBand (measurementChannelWidth);
  double maxRxPowerW = -1; //in case current event may not be sent on measurement channel (rxPowerW would be equal to 0)
  Ptr<Event> maxEvent;
  NS_ASSERT (!m_currentPreambleEvents.empty ());
  for (auto preambleEvent : m_currentPreambleEvents)
    {
      double rxPowerW = preambleEvent.second->GetRxPowerW (measurementBand);
      if (rxPowerW > maxRxPowerW)
        {
          maxRxPowerW = rxPowerW;
          maxEvent = preambleEvent.second;
        }
    }

  NS_ASSERT (maxEvent != 0);
  if (maxEvent != event)
    {
      NS_LOG_DEBUG ("Receiver got a stronger packet with UID " << maxEvent->GetPpdu ()->GetUid () << " during preamble detection: drop packet with UID " << event->GetPpdu ()->GetUid ());
      NotifyRxDrop (GetAddressedPsduInPpdu (event->GetPpdu ()), BUSY_DECODING_PREAMBLE);
      auto it = m_currentPreambleEvents.find (std::make_pair (event->GetPpdu ()->GetUid (), event->GetPpdu ()->GetPreamble ()));
      m_currentPreambleEvents.erase (it);
      //This is needed to cleanup the m_firstPowerPerBand so that the first power corresponds to the power at the start of the PPDU
      m_interference.NotifyRxEnd (maxEvent->GetStartTime ());
      //Make sure InterferenceHelper keeps recording events
      m_interference.NotifyRxStart ();
      return;
    }

  m_currentEvent = event;
  
  double snr = m_interference.CalculateSnr (m_currentEvent, measurementChannelWidth, 1, measurementBand);
  NS_LOG_DEBUG ("SNR(dB)=" << RatioToDb (snr) << " at start of non-HT preamble");

  Time headerPayloadDuration = m_currentEvent->GetStartTime () + m_currentEvent->GetPpdu ()->GetTxDuration () - Simulator::Now ();

  if ((!m_preambleDetectionModel && maxRxPowerW > 0.0)
      || (m_preambleDetectionModel && m_preambleDetectionModel->IsPreambleDetected (m_currentEvent->GetRxPowerW (measurementBand), snr, measurementChannelWidth)))
    {
      for (auto & endPreambleDetectionEvent : m_endPreambleDetectionEvents)
        {
          endPreambleDetectionEvent.Cancel ();
        }
      m_endPreambleDetectionEvents.clear ();

      for (auto it = m_currentPreambleEvents.begin (); it != m_currentPreambleEvents.end (); )
        {
          if (it->second != m_currentEvent)
            {
              NS_LOG_DEBUG ("Drop packet with UID " << it->first.first << " and preamble " << it->first.second << " arrived at time " << it->second->GetStartTime ());
              WifiPhyRxfailureReason reason;
              if (m_currentEvent->GetPpdu ()->GetUid () > it->first.first)
                {
                  reason = PREAMBLE_DETECTION_PACKET_SWITCH;
                  //This is needed to cleanup the m_firstPowerPerBand so that the first power corresponds to the power at the start of the PPDU
                  m_interference.NotifyRxEnd (m_currentEvent->GetStartTime ());
                }
              else
                {
                  reason = BUSY_DECODING_PREAMBLE;
                }
              NotifyRxDrop (GetAddressedPsduInPpdu (it->second->GetPpdu ()), reason);
              it = m_currentPreambleEvents.erase (it);
            }
          else
            {
              it++;
            }
        }

      //Make sure InterferenceHelper keeps recording events
      m_interference.NotifyRxStart ();

      NotifyRxBegin (GetAddressedPsduInPpdu (m_currentEvent->GetPpdu ()), m_currentEvent->GetRxPowerWPerBand ());

      m_timeLastPreambleDetected = Simulator::Now ();
      WifiTxVector txVector = m_currentEvent->GetTxVector ();

      if (txVector.GetPreambleType () == WIFI_PREAMBLE_HT_GF)
        {
          //No non-HT PHY header for HT GF, in addition, remaining HT-LTFs follow after HT-SIG
          Time remainingPreambleHeaderDuration = GetPpduFieldDuration (WIFI_PPDU_FIELD_PREAMBLE, txVector)
                                                 + GetPpduFieldDuration (WIFI_PPDU_FIELD_HT_SIG, txVector)
                                                 - (Simulator::Now () - m_currentEvent->GetStartTime ());
          m_state->SwitchMaybeToCcaBusy (remainingPreambleHeaderDuration);
          m_endPhyRxEvent = Simulator::Schedule (remainingPreambleHeaderDuration, &WifiPhy::EndReceiveCommonHeader, this, m_currentEvent);
        }
      else
        {
          //Schedule end of non-HT PHY header
          Time remainingPreambleAndNonHtHeaderDuration = GetPpduFieldDuration (WIFI_PPDU_FIELD_PREAMBLE, txVector)
                                                         + GetPpduFieldDuration (WIFI_PPDU_FIELD_NON_HT_HEADER, txVector)
                                                         - (Simulator::Now () - m_currentEvent->GetStartTime ());
          m_state->SwitchMaybeToCcaBusy (remainingPreambleAndNonHtHeaderDuration);
          m_endPhyRxEvent = Simulator::Schedule (remainingPreambleAndNonHtHeaderDuration, &WifiPhy::ContinueReceiveHeader, this, m_currentEvent);
        }
    }
  else
    {
      NS_LOG_DEBUG ("Drop packet because PHY preamble detection failed");
      // Like CCA-SD, CCA-ED is governed by the 4 us CCA window to flag CCA-BUSY
      // for any received signal greater than the CCA-ED threshold.
      DropPreambleEvent (m_currentEvent->GetPpdu (), PREAMBLE_DETECT_FAILURE, m_currentEvent->GetEndTime (), GetMeasurementChannelWidth (m_currentEvent->GetPpdu ()));
      if (m_currentPreambleEvents.empty())
        {
          //Do not erase events if there are still pending preamble events to be processed
          m_interference.NotifyRxEnd (Simulator::Now ());
        }
      m_currentEvent = 0;
    }
}

void
WifiPhy::ContinueReceiveHeader (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (m_endPhyRxEvent.IsExpired ());

  uint16_t measurementChannelWidth = GetMeasurementChannelWidth (event->GetPpdu ());
  InterferenceHelper::SnrPer snrPer = m_interference.CalculatePhyHeaderSnrPer (event, measurementChannelWidth, GetBand (measurementChannelWidth),
                                                                               WIFI_PPDU_FIELD_NON_HT_HEADER);

  NS_LOG_DEBUG ("L-SIG/RL-SIG: SNR(dB)=" << RatioToDb (snrPer.snr) << ", PER=" << snrPer.per);
  if (m_random->GetValue () > snrPer.per) //non-HT PHY header reception succeeded
    {
      NS_LOG_DEBUG ("Received non-HT PHY header");
      WifiTxVector txVector = event->GetTxVector ();
      Time remainingRxDuration = event->GetEndTime () - Simulator::Now ();
      m_state->SwitchMaybeToCcaBusy (remainingRxDuration);
      Time headerDuration = GetPpduFieldDuration (WIFI_PPDU_FIELD_HT_SIG, txVector)
                            + GetPpduFieldDuration (WIFI_PPDU_FIELD_SIG_A, txVector);
      m_endPhyRxEvent = Simulator::Schedule (headerDuration, &WifiPhy::EndReceiveCommonHeader, this, event);
    }
  else //non-HT PHY header reception failed
    {
      NS_LOG_DEBUG ("Abort reception because non-HT PHY header reception failed");
      AbortCurrentReception (L_SIG_FAILURE);
      if (event->GetEndTime () > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          MaybeCcaBusyDuration (GetMeasurementChannelWidth (event->GetPpdu ()));
        }
    }
}

void
WifiPhy::StartReceivePreamble (Ptr<WifiPpdu> ppdu, RxPowerWattPerChannelBand rxPowersW)
{
  //The total RX power corresponds to the maximum over all the bands
  auto it = std::max_element (rxPowersW.begin (), rxPowersW.end (),
    [] (const std::pair<WifiSpectrumBand, double>& p1, const std::pair<WifiSpectrumBand, double>& p2) {
      return p1.second < p2.second;
    });
  NS_LOG_FUNCTION (this << ppdu << it->second);
  WifiTxVector txVector = ppdu->GetTxVector ();
  Time rxDuration = ppdu->GetTxDuration ();

  Ptr<Event> event;
  //We store all incoming preamble events, and a decision is made at the end of the preamble detection window.
  //If a preamble is received after the preamble detection window, it is stored anyway because this is needed for HE TB PPDUs in
  //order to properly update the received power in InterferenceHelper. The map is cleaned anyway at the end of the current reception.
  auto uidPreamblePair = std::make_pair (ppdu->GetUid (), ppdu->GetPreamble ());
  if (ppdu->GetType () == WIFI_PPDU_TYPE_UL_MU)
    {
      rxDuration = CalculateNonOfdmaDurationForHeTb (txVector); //the OFDMA part of the transmission will be added later on
      auto it = m_currentPreambleEvents.find (uidPreamblePair);
      if (it != m_currentPreambleEvents.end ())
        {
          NS_LOG_DEBUG ("Received another HE TB PPDU for UID " << ppdu->GetUid () << " from STA-ID " << ppdu->GetStaId () << " and BSS color " << +txVector.GetBssColor ());
          event = it->second;
          if (Simulator::Now () - event->GetStartTime () > NanoSeconds (400))
            {
              //Section 27.3.14.3 from 802.11ax Draft 4.0: Pre-correction accuracy requirements.
              //A STA that transmits an HE TB PPDU, non-HT PPDU, or non-HT duplicate PPDU in response to a triggering PPDU
              //shall ensure that the transmission start time of the HE TB PPDU, non-HT PPDU, or non-HT duplicate PPDU is
              //within ±0.4 µs + 16 µs from the end, at the STA’s antenna connector, of the last OFDM symbol of the triggering
              //PPDU (if it contains no PE field) or of the PE field of the triggering PPDU (if the PE field is present).
              //As a result, if an HE TB PPDU arrives later than 0.4 µs, it is added as an interference but PPDU is dropped.
              event = m_interference.Add (ppdu, txVector, rxDuration, rxPowersW);
              NS_LOG_DEBUG ("Drop packet because not received within the 400ns window");
              NotifyRxDrop (GetAddressedPsduInPpdu (ppdu), HE_TB_PPDU_TOO_LATE);
            }
          else
            {
              //Update received power of the event associated to that UL MU transmission
              m_interference.UpdateEvent (event, rxPowersW);
            }
          if ((m_currentEvent != 0) && (m_currentEvent->GetPpdu ()->GetUid () != ppdu->GetUid ()))
            {
              NS_LOG_DEBUG ("Drop packet because already receiving another HE TB PPDU");
              NotifyRxDrop (GetAddressedPsduInPpdu (ppdu), RXING);
            }
          return;
        }
      else
        {
          NS_LOG_DEBUG ("Received a new HE TB PPDU for UID " << ppdu->GetUid () << " from STA-ID " << ppdu->GetStaId () << " and BSS color " << +txVector.GetBssColor ());
          event = m_interference.Add (ppdu, txVector, rxDuration, rxPowersW);
          m_currentPreambleEvents.insert ({std::make_pair (ppdu->GetUid (), ppdu->GetPreamble ()), event});
        }
    }
  else
    {
      event = m_interference.Add (ppdu, txVector, rxDuration, rxPowersW);
      NS_ASSERT (m_currentPreambleEvents.find (uidPreamblePair) == m_currentPreambleEvents.end ());
      m_currentPreambleEvents.insert ({uidPreamblePair, event});
    }

  Time endRx = Simulator::Now () + rxDuration;
  if (m_state->GetState () == WifiPhyState::OFF)
    {
      NS_LOG_DEBUG ("Cannot start RX because device is OFF");
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          MaybeCcaBusyDuration (GetMeasurementChannelWidth (nullptr));
        }
      return;
    }

  if (ppdu->IsTruncatedTx ())
    {
      NS_LOG_DEBUG ("Packet reception stopped because transmitter has been switched off");
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          MaybeCcaBusyDuration (GetMeasurementChannelWidth (ppdu));
        }
      return;
    }

  switch (m_state->GetState ())
    {
    case WifiPhyState::SWITCHING:
      NS_LOG_DEBUG ("Drop packet because of channel switching");
      /*
       * Packets received on the upcoming channel are added to the event list
       * during the switching state. This way the medium can be correctly sensed
       * when the device listens to the channel for the first time after the
       * switching e.g. after channel switching, the channel may be sensed as
       * busy due to other devices' transmissions started before the end of
       * the switching.
       */
      DropPreambleEvent (ppdu, CHANNEL_SWITCHING, endRx, GetMeasurementChannelWidth (ppdu));
      break;
    case WifiPhyState::RX:
      if (m_frameCaptureModel != 0
          && m_frameCaptureModel->IsInCaptureWindow (m_timeLastPreambleDetected)
          && m_frameCaptureModel->CaptureNewFrame (m_currentEvent, event))
        {
          AbortCurrentReception (FRAME_CAPTURE_PACKET_SWITCH);
          NS_LOG_DEBUG ("Switch to new packet");
          StartRx (event);
        }
      else
        {
          NS_LOG_DEBUG ("Drop packet because already in Rx");
          DropPreambleEvent (ppdu, RXING, endRx, GetMeasurementChannelWidth (ppdu));
          if (m_currentEvent == 0)
            {
              /*
               * We are here because the non-legacy PHY header has not been successfully received.
               * The PHY is kept in RX state for the duration of the PPDU, but EndReceive function is
               * not called when the reception of the PPDU is finished, which is responsible to clear
               * m_currentPreambleEvents. As a result, m_currentPreambleEvents should be cleared here.
               */
              m_currentPreambleEvents.clear ();
            }
        }
      break;
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("Drop packet because already in Tx");
      DropPreambleEvent (ppdu, TXING, endRx, GetMeasurementChannelWidth (ppdu));
      break;
    case WifiPhyState::CCA_BUSY:
      if (m_currentEvent != 0)
        {
          if (m_frameCaptureModel != 0
              && m_frameCaptureModel->IsInCaptureWindow (m_timeLastPreambleDetected)
              && m_frameCaptureModel->CaptureNewFrame (m_currentEvent, event))
            {
              AbortCurrentReception (FRAME_CAPTURE_PACKET_SWITCH);
              NS_LOG_DEBUG ("Switch to new packet");
              StartRx (event);
            }
          else
            {
              NS_LOG_DEBUG ("Drop packet because already decoding preamble");
              DropPreambleEvent (ppdu, BUSY_DECODING_PREAMBLE, endRx, GetMeasurementChannelWidth (ppdu));
            }
        }
      else
        {
          StartRx (event);
        }
      break;
    case WifiPhyState::IDLE:
      NS_ASSERT (m_currentEvent == 0);
      StartRx (event);
      break;
    case WifiPhyState::SLEEP:
      NS_LOG_DEBUG ("Drop packet because in sleep mode");
      DropPreambleEvent (ppdu, SLEEPING, endRx, GetMeasurementChannelWidth (nullptr));
      break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
}

void
WifiPhy::DropPreambleEvent (Ptr<const WifiPpdu> ppdu, WifiPhyRxfailureReason reason, Time endRx, uint16_t measurementChannelWidth)
{
  NS_LOG_FUNCTION (this << ppdu << reason << endRx << measurementChannelWidth);
  NotifyRxDrop (GetAddressedPsduInPpdu (ppdu), reason);
  auto it = m_currentPreambleEvents.find (std::make_pair (ppdu->GetUid (), ppdu->GetPreamble ()));
  if (it != m_currentPreambleEvents.end ())
    {
      m_currentPreambleEvents.erase (it);
    }
  if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
    {
      //that PPDU will be noise _after_ the end of the current event.
      MaybeCcaBusyDuration (measurementChannelWidth);
    }
}

void
WifiPhy::MaybeCcaBusyDuration (uint16_t channelWidth)
{
  //We are here because we have received the first bit of a packet and we are
  //not going to be able to synchronize on it
  //In this model, CCA becomes busy when the aggregation of all signals as
  //tracked by the InterferenceHelper class is higher than the CcaBusyThreshold
  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW, GetBand (channelWidth));
  if (!delayUntilCcaEnd.IsZero ())
    {
      m_state->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
    }
}

void
WifiPhy::StartReceiveOfdmaPayload (Ptr<Event> event)
{
  Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
  RxPowerWattPerChannelBand rxPowersW = event->GetRxPowerWPerBand ();
  //The total RX power corresponds to the maximum over all the bands
  auto it = std::max_element (rxPowersW.begin (), rxPowersW.end (),
    [] (const std::pair<WifiSpectrumBand, double>& p1, const std::pair<WifiSpectrumBand, double>& p2) {
      return p1.second < p2.second;
    });
  NS_LOG_FUNCTION (this << event << ppdu << it->second);
  NS_ASSERT (m_currentEvent != 0);
  auto itEvent = m_beginOfdmaPayloadRxEvents.find (GetStaId (ppdu));
  /**
   * m_beginOfdmaPayloadRxEvents should still be running only for APs, since canceled in StartReceivePayload for STAs.
   * This is because SpectrumWifiPhy does not have access to the device type and thus blindly schedules things, letting
   * the parent WifiPhy class take into account device type.
   */
  NS_ASSERT (itEvent != m_beginOfdmaPayloadRxEvents.end () && itEvent->second.IsExpired ());
  m_beginOfdmaPayloadRxEvents.erase (itEvent);

  Time payloadDuration = ppdu->GetTxDuration () - CalculatePhyPreambleAndHeaderDuration (ppdu->GetTxVector ());
  Ptr<const WifiPsdu> psdu = GetAddressedPsduInPpdu (ppdu);
  ScheduleEndOfMpdus (event);
  m_endRxEvents.push_back (Simulator::Schedule (payloadDuration, &WifiPhy::EndReceive, this, event));
  m_signalNoiseMap.insert ({std::make_pair (ppdu->GetUid (), ppdu->GetStaId ()), SignalNoiseDbm ()});
  m_statusPerMpduMap.insert ({std::make_pair (ppdu->GetUid (), ppdu->GetStaId ()), std::vector<bool> ()});
}

void
WifiPhy::EndReceiveCommonHeader (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (m_endPhyRxEvent.IsExpired ());
  Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
  WifiModulationClass modulation = ppdu->GetModulation ();
  //calculate PER on the measurement channel for PHY headers
  uint16_t measurementChannelWidth = GetMeasurementChannelWidth (ppdu);
  auto measurementBand = GetBand (measurementChannelWidth);
  bool commonHeaderReceived = true; //If we are here, this means non-HT PHY header was already successfully received
  if (modulation >= WIFI_MOD_CLASS_HT)
    {
      WifiPpduField header = (modulation == WIFI_MOD_CLASS_HT) ? WIFI_PPDU_FIELD_HT_SIG : WIFI_PPDU_FIELD_SIG_A;
      InterferenceHelper::SnrPer snrPer = m_interference.CalculatePhyHeaderSnrPer (event, measurementChannelWidth, measurementBand,
                                                                                   header);
      NS_LOG_DEBUG (header << ": SNR(dB)=" << RatioToDb (snrPer.snr) << ", PER=" << snrPer.per);
      commonHeaderReceived = (m_random->GetValue () > snrPer.per);
    }
  WifiTxVector txVector = event->GetTxVector ();
  bool success = false;
  bool filteredOut = false;
  if (commonHeaderReceived) //PHY reception succeeded
    {
      Ptr<const WifiPsdu> psdu = GetAddressedPsduInPpdu (ppdu);
      if ((txVector.GetChannelWidth () >= 40) && (txVector.GetChannelWidth () > GetChannelWidth ()))
        {
          NS_LOG_DEBUG ("Packet reception could not be started because not enough channel width");
          NotifyRxDrop (psdu, UNSUPPORTED_SETTINGS);
        }

      if (ppdu->GetType () == WIFI_PPDU_TYPE_DL_MU) //Final decision on content of DL MU is reported to end of SIG-B (unless the PPDU is filtered)
        {
          if (psdu) //a valid pointer is returned only if BSS colors match
            {
              NS_LOG_INFO ("The BSS color of this DL MU PPDU matches device's. Schedule SIG-B reception.");
              Time timeBetweenSigAAndSigB = (ppdu->GetModulation () == WIFI_MOD_CLASS_VHT) ? //SIG-B just before payload for VHT whereas before training for HE
                                            GetPpduFieldDuration (WIFI_PPDU_FIELD_TRAINING, txVector) :
                                            NanoSeconds (0);
              m_endPhyRxEvent = Simulator::Schedule (timeBetweenSigAAndSigB + GetPpduFieldDuration (WIFI_PPDU_FIELD_SIG_B, txVector),
                                                     &WifiPhy::EndReceiveSigB, this, event);
              success = true;
            }
          else
            {
              NS_LOG_DEBUG ("The BSS color of this DL MU PPDU does not match the device's. The PPDU is filtered.");
              filteredOut = true;
              m_phyRxPayloadBeginTrace (txVector, NanoSeconds (0)); //this callback (equivalent to PHY-RXSTART primitive) is also triggered for filtered PPDUs
            }
        }
      else if (psdu)
        {
          success = ScheduleStartReceivePayload (event, GetPpduFieldDuration (WIFI_PPDU_FIELD_TRAINING, txVector));
        }
      else
        {
          NS_ASSERT (ppdu->GetType () == WIFI_PPDU_TYPE_UL_MU);
          NS_LOG_DEBUG ("No PSDU addressed to that PHY in the received MU PPDU. The PPDU is filtered.");
          filteredOut = true;
          m_phyRxPayloadBeginTrace (txVector, NanoSeconds (0)); //this callback (equivalent to PHY-RXSTART primitive) is also triggered for filtered PPDUs
        }
      if (modulation == WIFI_MOD_CLASS_HE)
        {
          HeSigAParameters params;
          params.rssiW = event->GetRxPowerW (measurementBand);
          params.bssColor = event->GetTxVector ().GetBssColor ();
          Simulator::ScheduleNow (&WifiPhy::NotifyEndOfHeSigA, this, params);
        }
    }
  else //PHY reception failed
    {
      NS_LOG_DEBUG ("Drop packet because SIG-A or HT-SIG reception failed");
      NotifyRxDrop (GetAddressedPsduInPpdu (ppdu), SIG_A_FAILURE);
    }
  if (!success)
    {
      if (filteredOut)
        {
          AbortCurrentReception (FILTERED); //PHY-RXSTART is immediately followed by PHY-RXEND (Filtered)
          if (event->GetEndTime () > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
            {
              MaybeCcaBusyDuration (GetMeasurementChannelWidth (ppdu));
            }
        }
      else
        {
          Time remainingDuration = ppdu->GetTxDuration () - CalculatePhyPreambleAndHeaderDuration (txVector)
                                   + GetPpduFieldDuration (WIFI_PPDU_FIELD_TRAINING, txVector)
                                   + GetPpduFieldDuration (WIFI_PPDU_FIELD_SIG_B, txVector);
          m_endRxEvents.push_back (Simulator::Schedule (remainingDuration,
                                                        &WifiPhy::ResetReceive, this, event));
        }
    }
}

void
WifiPhy::EndReceiveSigB (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (m_endPhyRxEvent.IsExpired ());
  Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
  WifiTxVector txVector = event->GetTxVector ();
  NS_ASSERT (ppdu->GetType () == WIFI_PPDU_TYPE_DL_MU);

  Time timeBetweenSigBAndPayload = (ppdu->GetModulation () == WIFI_MOD_CLASS_VHT) ? //SIG-B just before payload for VHT whereas before training for HE
                                   NanoSeconds (0) :
                                   GetPpduFieldDuration (WIFI_PPDU_FIELD_TRAINING, txVector);

  //calculate PER of SIG-B on measurement channel
  uint16_t measurementChannelWidth = GetMeasurementChannelWidth (ppdu);
  InterferenceHelper::SnrPer snrPer = m_interference.CalculatePhyHeaderSnrPer (event, measurementChannelWidth, GetBand (measurementChannelWidth),
                                                                               WIFI_PPDU_FIELD_SIG_B);
  NS_LOG_DEBUG ("SIG-B: SNR(dB)=" << RatioToDb (snrPer.snr) << ", PER=" << snrPer.per);

  bool success = false;
  bool filteredOut = false;
  if (m_random->GetValue () > snrPer.per) //SIG-B decoding succeeded
    {
      Ptr<const WifiPsdu> psdu = GetAddressedPsduInPpdu (ppdu);
      if (psdu)
        {
          success = ScheduleStartReceivePayload (event, timeBetweenSigBAndPayload);
        }
      else
        {
          NS_LOG_DEBUG ("No PSDU addressed to that PHY in the received MU PPDU. The PPDU is filtered.");
          filteredOut = true;
          m_phyRxPayloadBeginTrace (txVector, NanoSeconds (0)); //this callback (equivalent to PHY-RXSTART primitive) is also triggered for filtered PPDUs
        }
    }
  else //PHY reception failed
    {
      NS_LOG_DEBUG ("Drop packet because SIG-B reception failed");
      NotifyRxDrop (GetAddressedPsduInPpdu (ppdu), SIG_B_FAILURE);
    }
  if (!success)
    {
      if (filteredOut)
        {
          AbortCurrentReception (FILTERED); //PHY-RXSTART is immediately followed by PHY-RXEND (Filtered)
          if (event->GetEndTime () > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
            {
              MaybeCcaBusyDuration (GetMeasurementChannelWidth (ppdu));
            }
        }
      else
        {
          Time remainingDuration = ppdu->GetTxDuration () - CalculatePhyPreambleAndHeaderDuration (txVector)
                                   + timeBetweenSigBAndPayload;
          m_endRxEvents.push_back (Simulator::Schedule (remainingDuration,
                                                        &WifiPhy::ResetReceive, this, event));
        }
    }
}

bool
WifiPhy::ScheduleStartReceivePayload (Ptr<Event> event, Time timeToPayloadStart)
{
  Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
  Ptr<const WifiPsdu> psdu = GetAddressedPsduInPpdu (ppdu);
  NS_ASSERT (psdu);
  WifiTxVector txVector = event->GetTxVector ();
  uint16_t staId = GetStaId (ppdu);
  WifiMode txMode = txVector.GetMode (staId);
  uint8_t nss = txVector.GetNssMax();
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_MU)
    {
      for (auto info : txVector.GetHeMuUserInfoMap ())
        {
          if (info.first == staId)
            {
              nss = info.second.nss; //no need to look at other PSDUs
              break;
            }
        }
    }

  bool success = false;
  if (nss > GetMaxSupportedRxSpatialStreams ())
    {
      NS_LOG_DEBUG ("Packet reception could not be started because not enough RX antennas");
      NotifyRxDrop (psdu, UNSUPPORTED_SETTINGS);
    }
  else if (IsModeSupported (txMode))
    {
      NS_LOG_INFO ("SIG correctly decoded and with supported settings. Schedule start of payload.");
      m_state->SwitchMaybeToCcaBusy (timeToPayloadStart);
      m_endPhyRxEvent = Simulator::Schedule (timeToPayloadStart, &WifiPhy::StartReceivePayload, this, event);
      if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB)
        {
          m_currentHeTbPpduUid = ppdu->GetUid (); //to be able to correctly schedule start of OFDMA payload
        }
      success = true;
    }
  else //mode is not allowed
    {
      NS_LOG_DEBUG ("Drop packet because it was sent using an unsupported mode (" << txMode << ")");
      NotifyRxDrop (psdu, UNSUPPORTED_SETTINGS);
    }
  return success;
}

void
WifiPhy::StartReceivePayload (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (m_endPhyRxEvent.IsExpired ());
  Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
  WifiTxVector txVector = event->GetTxVector ();
  Time payloadDuration = ppdu->GetTxDuration () - CalculatePhyPreambleAndHeaderDuration (txVector);
  m_state->SwitchToRx (payloadDuration);
  m_phyRxPayloadBeginTrace (txVector, payloadDuration); //this callback (equivalent to PHY-RXSTART primitive) is triggered only if headers have been correctly decoded and that the mode within is supported

  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetDevice ());
  bool isAp = device != 0 && (DynamicCast<ApWifiMac> (device->GetMac ()) != 0);
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB && !isAp)
    {
      NS_LOG_DEBUG ("Ignore HE TB PPDU payload received by STA but keep state in Rx");
      m_endRxEvents.push_back (Simulator::Schedule (payloadDuration,
                                                    &WifiPhy::ResetReceive, this, event));
      //Cancel all scheduled events for OFDMA payload reception
      NS_ASSERT (!m_beginOfdmaPayloadRxEvents.empty () && m_beginOfdmaPayloadRxEvents.begin ()->second.IsRunning ());
      for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
        {
          beginOfdmaPayloadRxEvent.second.Cancel ();
        }
      m_beginOfdmaPayloadRxEvents.clear ();
    }
  else
    {
      NS_LOG_DEBUG ("Receiving PSDU");
      uint16_t staId = GetStaId (ppdu);
      m_signalNoiseMap.insert ({std::make_pair (ppdu->GetUid (), staId), SignalNoiseDbm ()});
      m_statusPerMpduMap.insert ({std::make_pair (ppdu->GetUid (), staId), std::vector<bool> ()});
      if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB)
        {
          //for HE TB PPDUs, ScheduleEndOfMpdus and EndReceive are scheduled by StartReceiveOfdmaPayload
          NS_ASSERT (isAp);
          NS_ASSERT (!m_beginOfdmaPayloadRxEvents.empty ());
          for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
            {
              NS_ASSERT (beginOfdmaPayloadRxEvent.second.IsRunning ());
            }
        }
      else
        {
          ScheduleEndOfMpdus (event);
          m_endRxEvents.push_back (Simulator::Schedule (payloadDuration, &WifiPhy::EndReceive, this, event));
        }
    }
}

void
WifiPhy::ScheduleEndOfMpdus (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
  Ptr<const WifiPsdu> psdu = GetAddressedPsduInPpdu (ppdu);
  WifiTxVector txVector = event->GetTxVector ();
  uint16_t staId = GetStaId (ppdu);
  Time endOfMpduDuration = NanoSeconds (0);
  Time relativeStart = NanoSeconds (0);
  Time psduDuration = ppdu->GetTxDuration () - CalculatePhyPreambleAndHeaderDuration (txVector);
  Time remainingAmpduDuration = psduDuration;
  size_t nMpdus = psdu->GetNMpdus ();
  MpduType mpduType = (nMpdus > 1) ? FIRST_MPDU_IN_AGGREGATE : (psdu->IsSingle () ? SINGLE_MPDU : NORMAL_MPDU);
  uint32_t totalAmpduSize = 0;
  double totalAmpduNumSymbols = 0.0;
  auto mpdu = psdu->begin ();
  for (size_t i = 0; i < nMpdus && mpdu != psdu->end (); ++mpdu)
    {
      uint32_t size = (mpduType == NORMAL_MPDU) ? psdu->GetSize () : psdu->GetAmpduSubframeSize (i);
      Time mpduDuration = GetPayloadDuration (size, txVector,
                                              GetPhyBand (), mpduType, true, totalAmpduSize,
                                              totalAmpduNumSymbols, staId);

      remainingAmpduDuration -= mpduDuration;
      if (i == (nMpdus - 1) && !remainingAmpduDuration.IsZero ()) //no more MPDUs coming
        {
          if (remainingAmpduDuration < NanoSeconds (txVector.GetGuardInterval ())) //enables to ignore padding
            {
              mpduDuration += remainingAmpduDuration; //apply a correction just in case rounding had induced slight shift
            }
        }

      endOfMpduDuration += mpduDuration;
      NS_LOG_INFO ("Schedule end of MPDU #" << i << " in " << endOfMpduDuration.As (Time::NS)
                   << " (relativeStart=" << relativeStart.As (Time::NS) << ", mpduDuration=" << mpduDuration.As (Time::NS)
                   << ", remainingAmdpuDuration=" << remainingAmpduDuration.As (Time::NS) << ")");
      m_endOfMpduEvents.push_back (Simulator::Schedule (endOfMpduDuration, &WifiPhy::EndOfMpdu, this, event, Create<WifiPsdu> (*mpdu, false), i, relativeStart, mpduDuration));

      //Prepare next iteration
      ++i;
      relativeStart += mpduDuration;
      mpduType = (i == (nMpdus - 1)) ? LAST_MPDU_IN_AGGREGATE : MIDDLE_MPDU_IN_AGGREGATE;
    }
}

void
WifiPhy::EndOfMpdu (Ptr<Event> event, Ptr<const WifiPsdu> psdu, size_t mpduIndex, Time relativeStart, Time mpduDuration)
{
  NS_LOG_FUNCTION (this << *event << mpduIndex << relativeStart << mpduDuration);
  Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
  WifiTxVector txVector = event->GetTxVector ();
  uint16_t staId = GetStaId (ppdu);

  uint16_t channelWidth = std::min (GetChannelWidth (), event->GetTxVector ().GetChannelWidth ());
  WifiSpectrumBand band;
  if (txVector.IsMu ())
    {
      band = GetRuBand (txVector, staId);
      channelWidth = HeRu::GetBandwidth (txVector.GetRu (staId).ruType);
    }
  else
    {
      band = GetBand (channelWidth);
    }
  double snr = m_interference.CalculateSnr (event, channelWidth, txVector.GetNss (staId), band);

  std::pair<bool, SignalNoiseDbm> rxInfo = GetReceptionStatus (psdu, event, staId, relativeStart, mpduDuration);
  NS_LOG_DEBUG ("Extracted MPDU #" << mpduIndex << ": duration: " << mpduDuration.GetNanoSeconds () << "ns" <<
                ", correct reception: " << rxInfo.first << ", Signal/Noise: " << rxInfo.second.signal << "/" << rxInfo.second.noise << "dBm");

  auto signalNoiseIt = m_signalNoiseMap.find (std::make_pair (ppdu->GetUid (), staId));
  NS_ASSERT (signalNoiseIt != m_signalNoiseMap.end ());
  signalNoiseIt->second = rxInfo.second;

  RxSignalInfo rxSignalInfo;
  rxSignalInfo.snr = snr;
  rxSignalInfo.rssi = rxInfo.second.signal;

  auto statusPerMpduIt = m_statusPerMpduMap.find (std::make_pair (ppdu->GetUid (), staId));
  NS_ASSERT (statusPerMpduIt != m_statusPerMpduMap.end ());
  statusPerMpduIt->second.push_back (rxInfo.first);

  if (rxInfo.first && GetAddressedPsduInPpdu (ppdu)->GetNMpdus () > 1)
    {
      //only done for correct MPDU that is part of an A-MPDU
      m_state->ContinueRxNextMpdu (Copy (psdu), rxSignalInfo, txVector);
    }
}

void
WifiPhy::EndReceive (Ptr<Event> event)
{
  Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
  WifiTxVector txVector = event->GetTxVector ();
  Time psduDuration = ppdu->GetTxDuration () - CalculatePhyPreambleAndHeaderDuration (txVector);
  NS_LOG_FUNCTION (this << *event << psduDuration);
  if (ppdu->GetType () != WIFI_PPDU_TYPE_UL_MU)
    {
      NS_ASSERT (GetLastRxEndTime () == Simulator::Now ());
    }
  NS_ASSERT (event->GetEndTime () == Simulator::Now ());

  uint16_t staId = GetStaId (ppdu);
  WifiSpectrumBand band;
  uint16_t channelWidth = std::min (GetChannelWidth (), txVector.GetChannelWidth ());
  if (txVector.IsMu ())
    {
      band = GetRuBand (txVector, staId);
      channelWidth = HeRu::GetBandwidth (txVector.GetRu (staId).ruType);
    }
  else
    {
      band = GetBand (channelWidth);
    }
  double snr = m_interference.CalculateSnr (event, channelWidth, txVector.GetNss (staId), band);

  Ptr<const WifiPsdu> psdu = GetAddressedPsduInPpdu (ppdu);
  NotifyRxEnd (psdu);

  auto signalNoiseIt = m_signalNoiseMap.find (std::make_pair (ppdu->GetUid (), staId));
  NS_ASSERT (signalNoiseIt != m_signalNoiseMap.end ());
  auto statusPerMpduIt = m_statusPerMpduMap.find (std::make_pair (ppdu->GetUid (), staId));
  NS_ASSERT (statusPerMpduIt != m_statusPerMpduMap.end ());

  if (std::count (statusPerMpduIt->second.begin (), statusPerMpduIt->second.end (), true))
   {
      //At least one MPDU has been successfully received
      NotifyMonitorSniffRx (psdu, GetFrequency (), txVector, signalNoiseIt->second, statusPerMpduIt->second, staId);
      RxSignalInfo rxSignalInfo;
      rxSignalInfo.snr = snr;
      rxSignalInfo.rssi = signalNoiseIt->second.signal; //same information for all MPDUs
      m_state->SwitchFromRxEndOk (Copy (psdu), rxSignalInfo, txVector, staId, statusPerMpduIt->second);
      m_previouslyRxPpduUid = event->GetPpdu ()->GetUid (); //store UID only if reception is successful (because otherwise trigger won't be read by MAC layer)
    }
  else
    {
      m_state->SwitchFromRxEndError (Copy (psdu), snr);
    }

  if (ppdu->GetType () == WIFI_PPDU_TYPE_UL_MU)
    {
      for (auto it = m_endRxEvents.begin (); it != m_endRxEvents.end (); )
        {
          if (it->IsExpired ())
            {
              it = m_endRxEvents.erase (it);
            }
          else
            {
              it++;
            }
        }
      if (m_endRxEvents.empty ())
        {
          //We got the last PPDU of the UL-OFDMA transmission
          m_interference.NotifyRxEnd (Simulator::Now ());
          m_signalNoiseMap.clear ();
          m_statusPerMpduMap.clear ();
          for (const auto & endOfMpduEvent : m_endOfMpduEvents)
            {
              NS_ASSERT (endOfMpduEvent.IsExpired ());
            }
          m_endOfMpduEvents.clear ();
          Reset ();
        }
    }
  else
    {
      m_interference.NotifyRxEnd (Simulator::Now ());
      MaybeCcaBusyDuration (GetMeasurementChannelWidth (ppdu));
      m_currentEvent = 0;
      m_currentPreambleEvents.clear ();
      m_endRxEvents.clear ();
      m_signalNoiseMap.clear ();
      m_statusPerMpduMap.clear ();
      for (const auto & endOfMpduEvent : m_endOfMpduEvents)
        {
          NS_ASSERT (endOfMpduEvent.IsExpired ());
        }
      m_endOfMpduEvents.clear ();
    }
  MaybeCcaBusyDuration (GetMeasurementChannelWidth (ppdu));
}

std::pair<bool, SignalNoiseDbm>
WifiPhy::GetReceptionStatus (Ptr<const WifiPsdu> psdu, Ptr<Event> event, uint16_t staId,
                             Time relativeMpduStart, Time mpduDuration)
{
  NS_LOG_FUNCTION (this << *psdu << *event << staId << relativeMpduStart << mpduDuration);
  uint16_t channelWidth = std::min (GetChannelWidth (), event->GetTxVector ().GetChannelWidth ());
  WifiTxVector txVector = event->GetTxVector ();
  WifiSpectrumBand band;
  if (txVector.IsMu ())
    {
      band = GetRuBand (txVector, staId);
      channelWidth = HeRu::GetBandwidth (txVector.GetRu (staId).ruType);
    }
  else
    {
      band = GetBand (channelWidth);
    }
  InterferenceHelper::SnrPer snrPer = m_interference.CalculatePayloadSnrPer (event, channelWidth, band, staId, std::make_pair (relativeMpduStart, relativeMpduStart + mpduDuration));

  WifiMode mode = event->GetTxVector ().GetMode (staId);
  NS_LOG_DEBUG ("rate=" << (mode.GetDataRate (event->GetTxVector (), staId)) <<
                ", SNR(dB)=" << RatioToDb (snrPer.snr) << ", PER=" << snrPer.per << ", size=" << psdu->GetSize () <<
                ", relativeStart = " << relativeMpduStart.As (Time::NS) << ", duration = " << mpduDuration.As (Time::NS));

  // There are two error checks: PER and receive error model check.
  // PER check models is typical for Wi-Fi and is based on signal modulation;
  // Receive error model is optional, if we have an error model and
  // it indicates that the packet is corrupt, drop the packet.
  SignalNoiseDbm signalNoise;
  signalNoise.signal = WToDbm (event->GetRxPowerW (band));
  signalNoise.noise = WToDbm (event->GetRxPowerW (band) / snrPer.snr);
  if (m_random->GetValue () > snrPer.per &&
      !(m_postReceptionErrorModel && m_postReceptionErrorModel->IsCorrupt (psdu->GetPacket ()->Copy ())))
    {
      NS_LOG_DEBUG ("Reception succeeded: " << psdu);
      return std::make_pair (true, signalNoise);
    }
  else
    {
      NS_LOG_DEBUG ("Reception failed: " << psdu);
      return std::make_pair (false, signalNoise);
    }
}

WifiSpectrumBand
WifiPhy::GetRuBand (WifiTxVector txVector, uint16_t staId) const
{
  NS_ASSERT (txVector.IsMu ());
  WifiSpectrumBand band;
  HeRu::RuSpec ru = txVector.GetRu (staId);
  uint16_t channelWidth = txVector.GetChannelWidth ();
  NS_ASSERT (channelWidth <= GetChannelWidth ());
  HeRu::SubcarrierGroup group = HeRu::GetSubcarrierGroup (channelWidth, ru.ruType, ru.index);
  HeRu::SubcarrierRange range = std::make_pair (group.front ().first, group.back ().second);
  band = ConvertHeRuSubcarriers (channelWidth, range);
  return band;
}

WifiSpectrumBand
WifiPhy::GetNonOfdmaBand (WifiTxVector txVector, uint16_t staId) const
{
  NS_ASSERT (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB);
  uint16_t channelWidth = txVector.GetChannelWidth ();
  NS_ASSERT (channelWidth <= GetChannelWidth ());

  HeRu::RuSpec ru = txVector.GetRu (staId);
  uint16_t ruWidth = HeRu::GetBandwidth (ru.ruType);
  uint16_t nonOfdmaWidth = ruWidth < 20 ? 20 : ruWidth;

  // Find the RU that encompasses the non-OFDMA part of the HE TB PPDU for the STA-ID
  HeRu::RuSpec nonOfdmaRu = HeRu::FindOverlappingRu (channelWidth, ru, HeRu::GetRuType (nonOfdmaWidth));

  HeRu::SubcarrierGroup groupPreamble = HeRu::GetSubcarrierGroup (channelWidth, nonOfdmaRu.ruType, nonOfdmaRu.index);
  HeRu::SubcarrierRange range = std::make_pair (groupPreamble.front ().first, groupPreamble.back ().second);
  return ConvertHeRuSubcarriers (channelWidth, range);
}

WifiSpectrumBand
WifiPhy::ConvertHeRuSubcarriers (uint16_t channelWidth, HeRu::SubcarrierRange range) const
{
  NS_ASSERT_MSG (false, "802.11ax can only be used with SpectrumWifiPhy");
  WifiSpectrumBand convertedSubcarriers;
  return convertedSubcarriers;
}

void
WifiPhy::EndReceiveInterBss (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_channelAccessRequested)
    {
      m_powerRestricted = false;
    }
}

void
WifiPhy::ResetReceive (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
  if (ppdu->GetType () != WIFI_PPDU_TYPE_UL_MU)
    {
      NS_ASSERT (event->GetEndTime () == Simulator::Now ());
    }
  NS_ASSERT (!IsStateRx ());
  NS_ASSERT (m_endRxEvents.size () == 1 && m_endRxEvents.front ().IsExpired ());
  m_endRxEvents.clear ();
  m_interference.NotifyRxEnd (Simulator::Now ());
  m_currentEvent = 0;
  m_currentPreambleEvents.clear ();
  for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
    {
      beginOfdmaPayloadRxEvent.second.Cancel ();
    }
  m_beginOfdmaPayloadRxEvents.clear ();
  MaybeCcaBusyDuration (GetMeasurementChannelWidth (ppdu));
}

void
WifiPhy::NotifyChannelAccessRequested (void)
{
  NS_LOG_FUNCTION (this);
  m_channelAccessRequested = true;
}

bool
WifiPhy::IsModeSupported (WifiMode mode) const
{
  for (const auto & phyEntity : m_phyEntities)
    {
      if (phyEntity.second->IsModeSupported (mode))
        {
          return true;
        }
    }
  return false;
}

WifiMode
WifiPhy::GetDefaultMode (void) const
{
  //Start from oldest standards and move up (guaranteed by fact that WifModulationClass is ordered)
  for (const auto & phyEntity : m_phyEntities)
    {
      for (const auto & mode : *(phyEntity.second))
        {
          return mode;
        }
    }
  NS_ASSERT_MSG (false, "Should have found at least one default mode");
  return WifiMode ();
}

bool
WifiPhy::IsMcsSupported (WifiModulationClass modulation, uint8_t mcs) const
{
  const auto phyEntity = m_phyEntities.find (modulation);
  if (phyEntity == m_phyEntities.end ())
    {
      return false;
    }
  return phyEntity->second->IsMcsSupported (mcs);
}

std::list<WifiMode>
WifiPhy::GetModeList (void) const
{
  std::list<WifiMode> list;
  for (const auto & phyEntity : m_phyEntities)
    {
      if (!phyEntity.second->HandlesMcsModes ()) //to exclude MCSs from search
        {
          for (const auto & mode : *(phyEntity.second))
            {
              list.emplace_back (mode);
            }
        }
    }
  return list;
}

std::list<WifiMode>
WifiPhy::GetModeList (WifiModulationClass modulation) const
{
  std::list<WifiMode> list;
  const auto phyEntity = m_phyEntities.find (modulation);
  if (phyEntity != m_phyEntities.end ())
    {
      if (!phyEntity->second->HandlesMcsModes ()) //to exclude MCSs from search
        {
          for (const auto & mode : *(phyEntity->second))
            {
              list.emplace_back (mode);
            }
        }
    }
  return list;
}

uint16_t
WifiPhy::GetNMcs (void) const
{
  uint16_t numMcs = 0;
  for (const auto & phyEntity : m_phyEntities)
    {
      if (phyEntity.second->HandlesMcsModes ()) //to exclude non-MCS modes from search
        {
          numMcs += phyEntity.second->GetNumModes ();
        }
    }
  return numMcs;
}

std::list<WifiMode>
WifiPhy::GetMcsList (void) const
{
  std::list<WifiMode> list;
  for (const auto & phyEntity : m_phyEntities)
    {
      if (phyEntity.second->HandlesMcsModes ()) //to exclude non-MCS modes from search
        {
          for (const auto & mode : *(phyEntity.second))
            {
              list.emplace_back (mode);
            }
        }
    }
  return list;
}

std::list<WifiMode>
WifiPhy::GetMcsList (WifiModulationClass modulation) const
{
  std::list<WifiMode> list;
  auto phyEntity = m_phyEntities.find (modulation);
  if (phyEntity != m_phyEntities.end ())
    {
      if (phyEntity->second->HandlesMcsModes ()) //to exclude non-MCS modes from search
        {
          for (const auto & mode : *(phyEntity->second))
            {
              list.emplace_back (mode);
            }
        }
    }
  return list;
}

WifiMode
WifiPhy::GetMcs (WifiModulationClass modulation, uint8_t mcs) const
{
  NS_ASSERT_MSG (IsMcsSupported (modulation, mcs), "Unsupported MCS");
  return m_phyEntities.at (modulation)->GetMcs (mcs);
}

bool
WifiPhy::IsStateCcaBusy (void) const
{
  return m_state->IsStateCcaBusy ();
}

bool
WifiPhy::IsStateIdle (void) const
{
  return m_state->IsStateIdle ();
}

bool
WifiPhy::IsStateRx (void) const
{
  return m_state->IsStateRx ();
}

bool
WifiPhy::IsStateTx (void) const
{
  return m_state->IsStateTx ();
}

bool
WifiPhy::IsStateSwitching (void) const
{
  return m_state->IsStateSwitching ();
}

bool
WifiPhy::IsStateSleep (void) const
{
  return m_state->IsStateSleep ();
}

bool
WifiPhy::IsStateOff (void) const
{
  return m_state->IsStateOff ();
}

Time
WifiPhy::GetDelayUntilIdle (void)
{
  return m_state->GetDelayUntilIdle ();
}

Time
WifiPhy::GetLastRxStartTime (void) const
{
  return m_state->GetLastRxStartTime ();
}

Time
WifiPhy::GetLastRxEndTime (void) const
{
  return m_state->GetLastRxEndTime ();
}

void
WifiPhy::SwitchMaybeToCcaBusy (uint16_t channelWidth)
{
  NS_LOG_FUNCTION (this << channelWidth);
  //We are here because we have received the first bit of a packet and we are
  //not going to be able to synchronize on it
  //In this model, CCA becomes busy when the aggregation of all signals as
  //tracked by the InterferenceHelper class is higher than the CcaBusyThreshold
  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW, GetBand (channelWidth));
  if (!delayUntilCcaEnd.IsZero ())
    {
      NS_LOG_DEBUG ("Calling SwitchMaybeToCcaBusy for " << delayUntilCcaEnd.As (Time::S));
      m_state->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
    }
}

void
WifiPhy::AbortCurrentReception (WifiPhyRxfailureReason reason)
{
  NS_LOG_FUNCTION (this << reason);
  if (reason != OBSS_PD_CCA_RESET || m_currentEvent) //Otherwise abort has already been called just before with FILTERED reason
    {
      for (auto & endPreambleDetectionEvent : m_endPreambleDetectionEvents)
        {
          if (endPreambleDetectionEvent.IsRunning ())
            {
              endPreambleDetectionEvent.Cancel ();
            }
        }
      m_endPreambleDetectionEvents.clear ();
      if (m_endPhyRxEvent.IsRunning ())
        {
          m_endPhyRxEvent.Cancel ();
        }
      for (auto & endMpduEvent : m_endOfMpduEvents)
        {
          endMpduEvent.Cancel ();
        }
      m_endOfMpduEvents.clear ();
      for (auto & endRxEvent : m_endRxEvents)
        {
          endRxEvent.Cancel ();
        }
      m_endRxEvents.clear ();
      for (auto & beginOfdmaPayloadRxEvent : m_beginOfdmaPayloadRxEvents)
        {
          beginOfdmaPayloadRxEvent.second.Cancel ();
        }
      m_beginOfdmaPayloadRxEvents.clear ();
      m_interference.NotifyRxEnd (Simulator::Now ());
      if (!m_currentEvent)
        {
          return;
        }
      NotifyRxDrop (GetAddressedPsduInPpdu (m_currentEvent->GetPpdu ()), reason);
      if (reason == OBSS_PD_CCA_RESET)
        {
          m_state->SwitchFromRxAbort ();
        }
      for (auto it = m_currentPreambleEvents.begin (); it != m_currentPreambleEvents.end (); ++it)
        {
          if (it->second == m_currentEvent)
            {
              it = m_currentPreambleEvents.erase (it);
              break;
            }
        }
      m_currentEvent = 0;
    }
  else if (reason == OBSS_PD_CCA_RESET)
    {
      m_state->SwitchFromRxAbort ();
    }
}

void
WifiPhy::ResetCca (bool powerRestricted, double txPowerMaxSiso, double txPowerMaxMimo)
{
  NS_LOG_FUNCTION (this << powerRestricted << txPowerMaxSiso << txPowerMaxMimo);
  m_powerRestricted = powerRestricted;
  m_txPowerMaxSiso = txPowerMaxSiso;
  m_txPowerMaxMimo = txPowerMaxMimo;
  NS_ASSERT ((m_currentEvent->GetEndTime () - Simulator::Now ()).IsPositive ());
  Simulator::Schedule (m_currentEvent->GetEndTime () - Simulator::Now (), &WifiPhy::EndReceiveInterBss, this);
  AbortCurrentReception (OBSS_PD_CCA_RESET);
}

double
WifiPhy::GetTxPowerForTransmission (WifiTxVector txVector, uint16_t staId /* = SU_STA_ID */, TxPsdFlag flag /* = PSD_NON_HE_TB */) const
{
  NS_LOG_FUNCTION (this << m_powerRestricted << txVector << staId << flag);
  // Get transmit power before antenna gain
  double txPowerDbm;
  if (!m_powerRestricted)
    {
      txPowerDbm = GetPowerDbm (txVector.GetTxPowerLevel ());
    }
  else
    {
      if (txVector.GetNssMax () > 1)
        {
          txPowerDbm = std::min (m_txPowerMaxMimo, GetPowerDbm (txVector.GetTxPowerLevel ()));
        }
      else
        {
          txPowerDbm = std::min (m_txPowerMaxSiso, GetPowerDbm (txVector.GetTxPowerLevel ()));
        }
    }

  //Apply power density constraint on EIRP
  uint16_t channelWidth = txVector.GetChannelWidth ();
  if (txVector.GetPreambleType () == WIFI_PREAMBLE_HE_TB && staId != SU_STA_ID)
    {
      NS_ASSERT (flag > PSD_NON_HE_TB);
      uint16_t ruWidth = HeRu::GetBandwidth (txVector.GetRu (staId).ruType);
      channelWidth = (flag == PSD_HE_TB_NON_OFDMA_PORTION && ruWidth < 20) ? 20 : ruWidth;
      NS_LOG_INFO ("Use channelWidth=" << channelWidth << " MHz for HE TB from " << staId << " for " << flag);
    }
  double txPowerDbmPerMhz = (txPowerDbm + GetTxGain ()) - RatioToDb (channelWidth); //account for antenna gain since EIRP
  NS_LOG_INFO ("txPowerDbm=" << txPowerDbm << " with txPowerDbmPerMhz=" << txPowerDbmPerMhz << " over " << channelWidth << " MHz");
  txPowerDbm = std::min (txPowerDbmPerMhz, m_powerDensityLimit) + RatioToDb (channelWidth);
  txPowerDbm -= GetTxGain(); //remove antenna gain since will be added right afterwards
  NS_LOG_INFO ("txPowerDbm=" << txPowerDbm << " after applying m_powerDensityLimit=" << m_powerDensityLimit);
  return txPowerDbm;
}

void
WifiPhy::StartRx (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_LOG_DEBUG ("sync to signal (power=" << event->GetRxPowerW (GetBand (GetMeasurementChannelWidth (event->GetPpdu ()))) << "W)");
  m_interference.NotifyRxStart (); //We need to notify it now so that it starts recording events
  m_endPreambleDetectionEvents.push_back (Simulator::Schedule (GetPreambleDetectionDuration (), &WifiPhy::StartReceiveHeader, this, event));
}

Ptr<const WifiPsdu>
WifiPhy::GetAddressedPsduInPpdu (Ptr<const WifiPpdu> ppdu) const
{
  Ptr<const WifiPsdu> psdu;
  switch (ppdu->GetType ())
    {
      case WIFI_PPDU_TYPE_UL_MU:
        {
          uint8_t bssColor = 0;
          Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetDevice ());
          if (device)
            {
              Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
              if (heConfiguration)
                {
                  UintegerValue bssColorAttribute;
                  heConfiguration->GetAttribute ("BssColor", bssColorAttribute);
                  bssColor = bssColorAttribute.Get ();
                }
            }
          //TODO Move this to HePhy
          psdu = DynamicCast<const HePpdu> (ppdu)->GetPsdu (bssColor);
        }
        break;
      case WIFI_PPDU_TYPE_DL_MU:
        {
          uint8_t bssColor = 0;
          Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetDevice ());
          if (device)
            {
              Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
              if (heConfiguration)
                {
                  UintegerValue bssColorAttribute;
                  heConfiguration->GetAttribute ("BssColor", bssColorAttribute);
                  bssColor = bssColorAttribute.Get ();
                }
            }
          //TODO Move this to HePhy
          psdu = DynamicCast<const HePpdu> (ppdu)->GetPsdu (bssColor, GetStaId (ppdu));
        }
        break;
      case WIFI_PPDU_TYPE_SU:
      default:
        psdu = ppdu->GetPsdu ();
    }
   return psdu;
}

uint16_t
WifiPhy::GetStaId (const Ptr<const WifiPpdu> ppdu) const
{
  uint16_t staId = SU_STA_ID;
  if (ppdu->GetType () == WIFI_PPDU_TYPE_UL_MU)
    {
      staId = ppdu->GetStaId ();
    }
  else if (ppdu->GetType () == WIFI_PPDU_TYPE_DL_MU)
    {
      Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (GetDevice ());
      if (device)
        {
          Ptr<StaWifiMac> mac = DynamicCast<StaWifiMac> (device->GetMac ());
          if (mac && mac->IsAssociated ())
            {
              return mac->GetAssociationId ();
            }
        }
    }
  return staId;
}

uint16_t
WifiPhy::GetMeasurementChannelWidth (const Ptr<const WifiPpdu> ppdu) const
{
  uint16_t channelWidth = GetChannelWidth ();
  if (ppdu == nullptr)
    {
      // Here because PHY was not receiving anything (e.g. resuming from OFF) nor expecting anything (e.g. sleep)
      // nor processing a Wi-Fi signal.
      return channelWidth >= 40 ? 20 : channelWidth;
    }

  channelWidth = std::min (channelWidth, ppdu->GetTxVector ().GetChannelWidth ());
  /**
   * The PHY shall not issue a PHY-RXSTART.indication primitive in response to a PPDU that does not overlap
   * the primary channel unless the PHY at an AP receives the HE TB PPDU solicited by the AP. For the HE
   * TB PPDU solicited by the AP, the PHY shall issue a PHY-RXSTART.indication primitive for a PPDU
   * received in the primary or at the secondary 20 MHz channel, the secondary 40 MHz channel, or the secondary
   * 80 MHz channel.
   */
  if (channelWidth >= 40 && ppdu->GetUid () != m_previouslyTxPpduUid)
    {
      channelWidth = 20;
    }
  return channelWidth;
}

WifiSpectrumBand
WifiPhy::GetBand (uint16_t /*bandWidth*/, uint8_t /*bandIndex*/)
{
  WifiSpectrumBand band;
  band.first = 0;
  band.second = 0;
  return band;
}

uint16_t
WifiPhy::ConvertHeTbPpduDurationToLSigLength (Time ppduDuration, WifiPhyBand band)
{
  return DynamicCast<const HePhy> (GetStaticPhyEntity (WIFI_MOD_CLASS_HE))->ConvertHeTbPpduDurationToLSigLength (ppduDuration, band);
}

Time
WifiPhy::ConvertLSigLengthToHeTbPpduDuration (uint16_t length, WifiTxVector txVector, WifiPhyBand band)
{
  NS_ASSERT (txVector.GetModulationClass () == WIFI_MOD_CLASS_HE);
  auto hePhy = DynamicCast<const HePhy> (GetStaticPhyEntity (txVector.GetModulationClass ()));
  return (hePhy ? hePhy->ConvertLSigLengthToHeTbPpduDuration (length, txVector, band) : NanoSeconds (0));
}

int64_t
WifiPhy::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_random->SetStream (stream);
  return 1;
}

std::ostream& operator<< (std::ostream& os, RxSignalInfo rxSignalInfo)
{
  os << "SNR:" << RatioToDb (rxSignalInfo.snr) << " dB"
     << ", RSSI:" << rxSignalInfo.rssi << " dBm";
  return os;
}

} //namespace ns3
