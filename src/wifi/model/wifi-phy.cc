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
#include "frame-capture-model.h"
#include "preamble-detection-model.h"
#include "wifi-radio-energy-model.h"
#include "error-rate-model.h"
#include "wifi-net-device.h"
#include "ht-configuration.h"
#include "he-configuration.h"
#include "mpdu-aggregator.h"
#include "wifi-psdu.h"
#include "wifi-ppdu.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiPhy");

/****************************************************************
 *       The actual WifiPhy class
 ****************************************************************/

NS_OBJECT_ENSURE_REGISTERED (WifiPhy);

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
                   MakeUintegerChecker<uint8_t> (0, 196))
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
    .AddTraceSource ("EndOfHePreamble",
                     "Trace source indicating the end of the 802.11ax preamble (after training fields)",
                     MakeTraceSourceAccessor (&WifiPhy::m_phyEndOfHePreambleTrace),
                     "ns3::WifiPhy::EndOfHePreambleTracedCallback")
  ;
  return tid;
}

WifiPhy::WifiPhy ()
  : m_txMpduReferenceNumber (0xffffffff),
    m_rxMpduReferenceNumber (0xffffffff),
    m_endRxEvent (),
    m_endPhyRxEvent (),
    m_endPreambleDetectionEvent (),
    m_endTxEvent (),
    m_standard (WIFI_PHY_STANDARD_UNSPECIFIED),
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
    m_currentEvent (0),
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
  m_endRxEvent.Cancel ();
  m_endPhyRxEvent.Cancel ();
  m_endPreambleDetectionEvent.Cancel ();
  m_device = 0;
  m_mobility = 0;
  m_state = 0;
  m_wifiRadioEnergyModel = 0;
  m_postReceptionErrorModel = 0;
  m_deviceRateSet.clear ();
  m_deviceMcsSet.clear ();
  m_mcsIndexMap.clear ();
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

  // See Table 17-21 "OFDM PHY characteristics" of 802.11-2016
  SetSifs (MicroSeconds (16));
  SetSlot (MicroSeconds (9));
  SetPifs (GetSifs () + GetSlot ());
  // See Table 10-5 "Determination of the EstimatedAckTxTime based on properties
  // of the PPDU causing the EIFS" of 802.11-2016
  m_ackTxTime = MicroSeconds (44);

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate48Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate54Mbps ());
}

void
WifiPhy::Configure80211b (void)
{
  NS_LOG_FUNCTION (this);

  // See Table 16-4 "HR/DSSS PHY characteristics" of 802.11-2016
  SetSifs (MicroSeconds (10));
  SetSlot (MicroSeconds (20));
  SetPifs (GetSifs () + GetSlot ());
  // See Table 10-5 "Determination of the EstimatedAckTxTime based on properties
  // of the PPDU causing the EIFS" of 802.11-2016
  m_ackTxTime = MicroSeconds (304);

  m_deviceRateSet.push_back (WifiPhy::GetDsssRate1Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate2Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate5_5Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate11Mbps ());
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

  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate9Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate24Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate48Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate54Mbps ());
}

void
WifiPhy::Configure80211p (void)
{
  NS_LOG_FUNCTION (this);
  if (GetChannelWidth () == 10)
    {
      // See Table 17-21 "OFDM PHY characteristics" of 802.11-2016
      SetSifs (MicroSeconds (32));
      SetSlot (MicroSeconds (13));
      SetPifs (GetSifs () + GetSlot ());
      m_ackTxTime = MicroSeconds (88);

      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate3MbpsBW10MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate4_5MbpsBW10MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6MbpsBW10MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9MbpsBW10MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12MbpsBW10MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18MbpsBW10MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24MbpsBW10MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate27MbpsBW10MHz ());
    }
  else if (GetChannelWidth () == 5)
    {
      // See Table 17-21 "OFDM PHY characteristics" of 802.11-2016
      SetSifs (MicroSeconds (64));
      SetSlot (MicroSeconds (21));
      SetPifs (GetSifs () + GetSlot ());
      m_ackTxTime = MicroSeconds (176);

      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate1_5MbpsBW5MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate2_25MbpsBW5MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate3MbpsBW5MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate4_5MbpsBW5MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6MbpsBW5MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9MbpsBW5MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12MbpsBW5MHz ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate13_5MbpsBW5MHz ());
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

  SetSifs (MicroSeconds (16));
  SetSlot (MicroSeconds (9));
  SetPifs (GetSifs () + GetSlot ());

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate54Mbps ());
}

void
WifiPhy::PushMcs (WifiMode mode)
{
  NS_LOG_FUNCTION (this << mode);

  WifiModulationClass modulation = mode.GetModulationClass ();
  NS_ASSERT (modulation == WIFI_MOD_CLASS_HT || modulation == WIFI_MOD_CLASS_VHT
             || modulation == WIFI_MOD_CLASS_HE);

  m_mcsIndexMap[modulation][mode.GetMcsValue ()] = m_deviceMcsSet.size ();
  m_deviceMcsSet.push_back (mode);
}

void
WifiPhy::RebuildMcsMap (void)
{
  NS_LOG_FUNCTION (this);
  m_mcsIndexMap.clear ();
  uint8_t index = 0;
  for (auto& mode : m_deviceMcsSet)
    {
      m_mcsIndexMap[mode.GetModulationClass ()][mode.GetMcsValue ()] = index++;
    }
}

void
WifiPhy::ConfigureHtDeviceMcsSet (void)
{
  NS_LOG_FUNCTION (this);

  bool htFound = false;
  for (std::vector<uint8_t>::size_type i = 0; i < m_bssMembershipSelectorSet.size (); i++)
    {
      if (m_bssMembershipSelectorSet[i] == HT_PHY)
        {
          htFound = true;
          break;
        }
    }
  if (htFound)
    {
      // erase all HtMcs modes from deviceMcsSet
      std::size_t index = m_deviceMcsSet.size () - 1;
      for (std::vector<WifiMode>::reverse_iterator rit = m_deviceMcsSet.rbegin (); rit != m_deviceMcsSet.rend (); ++rit, --index)
        {
          if (m_deviceMcsSet[index].GetModulationClass () == WIFI_MOD_CLASS_HT)
            {
              m_deviceMcsSet.erase (m_deviceMcsSet.begin () + index);
            }
        }
      RebuildMcsMap ();
      PushMcs (WifiPhy::GetHtMcs0 ());
      PushMcs (WifiPhy::GetHtMcs1 ());
      PushMcs (WifiPhy::GetHtMcs2 ());
      PushMcs (WifiPhy::GetHtMcs3 ());
      PushMcs (WifiPhy::GetHtMcs4 ());
      PushMcs (WifiPhy::GetHtMcs5 ());
      PushMcs (WifiPhy::GetHtMcs6 ());
      PushMcs (WifiPhy::GetHtMcs7 ());
      if (GetMaxSupportedTxSpatialStreams () > 1)
        {
          PushMcs (WifiPhy::GetHtMcs8 ());
          PushMcs (WifiPhy::GetHtMcs9 ());
          PushMcs (WifiPhy::GetHtMcs10 ());
          PushMcs (WifiPhy::GetHtMcs11 ());
          PushMcs (WifiPhy::GetHtMcs12 ());
          PushMcs (WifiPhy::GetHtMcs13 ());
          PushMcs (WifiPhy::GetHtMcs14 ());
          PushMcs (WifiPhy::GetHtMcs15 ());
        }
      if (GetMaxSupportedTxSpatialStreams () > 2)
        {
          PushMcs (WifiPhy::GetHtMcs16 ());
          PushMcs (WifiPhy::GetHtMcs17 ());
          PushMcs (WifiPhy::GetHtMcs18 ());
          PushMcs (WifiPhy::GetHtMcs19 ());
          PushMcs (WifiPhy::GetHtMcs20 ());
          PushMcs (WifiPhy::GetHtMcs21 ());
          PushMcs (WifiPhy::GetHtMcs22 ());
          PushMcs (WifiPhy::GetHtMcs23 ());
        }
      if (GetMaxSupportedTxSpatialStreams () > 3)
        {
          PushMcs (WifiPhy::GetHtMcs24 ());
          PushMcs (WifiPhy::GetHtMcs25 ());
          PushMcs (WifiPhy::GetHtMcs26 ());
          PushMcs (WifiPhy::GetHtMcs27 ());
          PushMcs (WifiPhy::GetHtMcs28 ());
          PushMcs (WifiPhy::GetHtMcs29 ());
          PushMcs (WifiPhy::GetHtMcs30 ());
          PushMcs (WifiPhy::GetHtMcs31 ());
        }
    }
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
  // See Table 10-5 "Determination of the EstimatedAckTxTime based on properties
  // of the PPDU causing the EIFS" of 802.11-2016
  m_blockAckTxTime = MicroSeconds (68);
  m_bssMembershipSelectorSet.push_back (HT_PHY);
  ConfigureHtDeviceMcsSet ();
}

void
WifiPhy::Configure80211ac (void)
{
  NS_LOG_FUNCTION (this);
  Configure80211n ();

  PushMcs (WifiPhy::GetVhtMcs0 ());
  PushMcs (WifiPhy::GetVhtMcs1 ());
  PushMcs (WifiPhy::GetVhtMcs2 ());
  PushMcs (WifiPhy::GetVhtMcs3 ());
  PushMcs (WifiPhy::GetVhtMcs4 ());
  PushMcs (WifiPhy::GetVhtMcs5 ());
  PushMcs (WifiPhy::GetVhtMcs6 ());
  PushMcs (WifiPhy::GetVhtMcs7 ());
  PushMcs (WifiPhy::GetVhtMcs8 ());
  PushMcs (WifiPhy::GetVhtMcs9 ());

  m_bssMembershipSelectorSet.push_back (VHT_PHY);
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

  PushMcs (WifiPhy::GetHeMcs0 ());
  PushMcs (WifiPhy::GetHeMcs1 ());
  PushMcs (WifiPhy::GetHeMcs2 ());
  PushMcs (WifiPhy::GetHeMcs3 ());
  PushMcs (WifiPhy::GetHeMcs4 ());
  PushMcs (WifiPhy::GetHeMcs5 ());
  PushMcs (WifiPhy::GetHeMcs6 ());
  PushMcs (WifiPhy::GetHeMcs7 ());
  PushMcs (WifiPhy::GetHeMcs8 ());
  PushMcs (WifiPhy::GetHeMcs9 ());
  PushMcs (WifiPhy::GetHeMcs10 ());
  PushMcs (WifiPhy::GetHeMcs11 ());

  m_bssMembershipSelectorSet.push_back (HE_PHY);
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
      NS_ASSERT (false);
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
  ConfigureHtDeviceMcsSet ();
  if (changed && !m_capabilitiesChangedCallback.IsNull ())
    {
      m_capabilitiesChangedCallback ();
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

uint8_t
WifiPhy::GetNBssMembershipSelectors (void) const
{
  return static_cast<uint8_t> (m_bssMembershipSelectorSet.size ());
}

uint8_t
WifiPhy::GetBssMembershipSelector (uint8_t selector) const
{
  return m_bssMembershipSelectorSet[selector];
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
      m_endRxEvent.Cancel ();
      m_endPreambleDetectionEvent.Cancel ();
      goto switchChannel;
      break;
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("channel switching postponed until end of current transmission");
      Simulator::Schedule (GetDelayUntilIdle (), &WifiPhy::SetChannelNumber, this, nch);
      break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
      if (m_endPreambleDetectionEvent.IsRunning ())
        {
          m_endPreambleDetectionEvent.Cancel ();
          m_endRxEvent.Cancel ();
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
      m_endRxEvent.Cancel ();
      m_endPreambleDetectionEvent.Cancel ();
      goto switchFrequency;
      break;
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("channel/frequency switching postponed until end of current transmission");
      Simulator::Schedule (GetDelayUntilIdle (), &WifiPhy::SetFrequency, this, frequency);
      break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
      if (m_endPreambleDetectionEvent.IsRunning ())
        {
          m_endPreambleDetectionEvent.Cancel ();
          m_endRxEvent.Cancel ();
        }
      goto switchFrequency;
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
  m_endRxEvent.Cancel ();
  m_endPreambleDetectionEvent.Cancel ();
  m_endTxEvent.Cancel ();
  m_state->SwitchToOff ();
}

void
WifiPhy::ResumeFromSleep (void)
{
  NS_LOG_FUNCTION (this);
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
        Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW);
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
        Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW);
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

WifiMode
WifiPhy::GetHtPhyHeaderMode ()
{
  return WifiPhy::GetHtMcs0 ();
}

WifiMode
WifiPhy::GetVhtPhyHeaderMode ()
{
  return WifiPhy::GetVhtMcs0 ();
}

WifiMode
WifiPhy::GetHePhyHeaderMode ()
{
  return WifiPhy::GetHeMcs0 ();
}

Time
WifiPhy::GetPreambleDetectionDuration (void)
{
  return MicroSeconds (4);
}

Time
WifiPhy::GetPhyTrainingSymbolDuration (WifiTxVector txVector)
{
  uint8_t Ndltf, Neltf;
  //We suppose here that STBC = 0.
  //If STBC > 0, we need a different mapping between Nss and Nltf (IEEE 802.11n-2012 standard, page 1682).
  if (txVector.GetNss () < 3)
    {
      Ndltf = txVector.GetNss ();
    }
  else if (txVector.GetNss () < 5)
    {
      Ndltf = 4;
    }
  else if (txVector.GetNss () < 7)
    {
      Ndltf = 6;
    }
  else
    {
      Ndltf = 8;
    }

  if (txVector.GetNess () < 3)
    {
      Neltf = txVector.GetNess ();
    }
  else
    {
      Neltf = 4;
    }

  switch (txVector.GetPreambleType ())
    {
    case WIFI_PREAMBLE_HT_MF:
      return MicroSeconds (4 + (4 * Ndltf) + (4 * Neltf));
    case WIFI_PREAMBLE_HT_GF:
      return MicroSeconds ((4 * Ndltf) + (4 * Neltf));
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_VHT_MU:
      return MicroSeconds (4 + (4 * Ndltf));
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_HE_MU:
      return MicroSeconds (4 + (8 * Ndltf));
    default:
      return MicroSeconds (0);
    }
}

Time
WifiPhy::GetPhyHtSigHeaderDuration (WifiPreamble preamble)
{
  switch (preamble)
    {
    case WIFI_PREAMBLE_HT_MF:
    case WIFI_PREAMBLE_HT_GF:
      //HT-SIG
      return MicroSeconds (8);
    default:
      //no HT-SIG for non HT
      return MicroSeconds (0);
    }
}

Time
WifiPhy::GetPhySigA1Duration (WifiPreamble preamble)
{
  switch (preamble)
    {
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_VHT_MU:
    case WIFI_PREAMBLE_HE_MU:
      //VHT-SIG-A1 and HE-SIG-A1
      return MicroSeconds (4);
    default:
      // no SIG-A1
      return MicroSeconds (0);
    }
}

Time
WifiPhy::GetPhySigA2Duration (WifiPreamble preamble)
{
  switch (preamble)
    {
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_VHT_MU:
    case WIFI_PREAMBLE_HE_MU:
      //VHT-SIG-A2 and HE-SIG-A2
      return MicroSeconds (4);
    default:
      // no SIG-A2
      return MicroSeconds (0);
    }
}

Time
WifiPhy::GetPhySigBDuration (WifiPreamble preamble)
{
  switch (preamble)
    {
    case WIFI_PREAMBLE_VHT_MU:
    case WIFI_PREAMBLE_HE_MU:
      return MicroSeconds (4);
    default:
      // no SIG-B
      return MicroSeconds (0);
    }
}

WifiMode
WifiPhy::GetPhyHeaderMode (WifiTxVector txVector)
{
  WifiPreamble preamble = txVector.GetPreambleType ();
  switch (preamble)
    {
    case WIFI_PREAMBLE_LONG:
    case WIFI_PREAMBLE_SHORT:
      {
        switch (txVector.GetMode ().GetModulationClass ())
          {
            case WIFI_MOD_CLASS_OFDM:
              {
                switch (txVector.GetChannelWidth ())
                  {
                    case 5:
                      return WifiPhy::GetOfdmRate1_5MbpsBW5MHz ();
                    case 10:
                      return WifiPhy::GetOfdmRate3MbpsBW10MHz ();
                    case 20:
                    default:
                      //(Section 17.3.2 "PPDU frame format"; IEEE Std 802.11-2016)
                      //actually this is only the first part of the PhyHeader,
                      //because the last 16 bits of the PhyHeader are using the
                      //same mode of the payload
                      return WifiPhy::GetOfdmRate6Mbps ();
                  }
              }
            case WIFI_MOD_CLASS_ERP_OFDM:
              return WifiPhy::GetErpOfdmRate6Mbps ();
            case WIFI_MOD_CLASS_DSSS:
            case WIFI_MOD_CLASS_HR_DSSS:
              {
                if (preamble == WIFI_PREAMBLE_LONG || txVector.GetMode () == WifiPhy::GetDsssRate1Mbps ())
                  {
                    //(Section 16.2.3 "PPDU field definitions" and Section 16.2.2.2 "Long PPDU format"; IEEE Std 802.11-2016)
                    return WifiPhy::GetDsssRate1Mbps ();
                  }
                else
                  {
                    //(Section 16.2.2.3 "Short PPDU format"; IEEE Std 802.11-2016)
                    return WifiPhy::GetDsssRate2Mbps ();
                  }
              }
            default:
              NS_FATAL_ERROR ("unsupported modulation class");
              return WifiMode ();
          }
      }
    case WIFI_PREAMBLE_HT_MF:
    case WIFI_PREAMBLE_HT_GF:
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_VHT_MU:
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_HE_ER_SU:
    case WIFI_PREAMBLE_HE_MU:
    case WIFI_PREAMBLE_HE_TB:
      return WifiPhy::GetOfdmRate6Mbps ();
    default:
      NS_FATAL_ERROR ("unsupported preamble type");
      return WifiMode ();
    }
}

Time
WifiPhy::GetPhyHeaderDuration (WifiTxVector txVector)
{
  WifiPreamble preamble = txVector.GetPreambleType ();
  switch (txVector.GetPreambleType ())
    {
    case WIFI_PREAMBLE_LONG:
    case WIFI_PREAMBLE_SHORT:
      {
        switch (txVector.GetMode ().GetModulationClass ())
          {
          case WIFI_MOD_CLASS_OFDM:
            {
              switch (txVector.GetChannelWidth ())
                {
                case 20:
                default:
                  //(Section 17.3.3 "PHY preamble (SYNC))" and Figure 17-4 "OFDM training structure"; IEEE Std 802.11-2016)
                  //also (Section 17.3.2.4 "Timing related parameters" Table 17-5 "Timing-related parameters"; IEEE Std 802.11-2016)
                  //We return the duration of the SIGNAL field only, since the
                  //SERVICE field (which strictly speaking belongs to the PHY
                  //header, see Section 17.3.2 and Figure 17-1) is sent using the
                  //payload mode.
                  return MicroSeconds (4);
                case 10:
                  //(Section 17.3.2.4 "Timing related parameters" Table 17-5 "Timing-related parameters"; IEEE Std 802.11-2016)
                  return MicroSeconds (8);
                case 5:
                  //(Section 17.3.2.4 "Timing related parameters" Table 17-5 "Timing-related parameters"; IEEE Std 802.11-2016)
                  return MicroSeconds (16);
                }
            }
          case WIFI_MOD_CLASS_ERP_OFDM:
            return MicroSeconds (4);
          case WIFI_MOD_CLASS_DSSS:
          case WIFI_MOD_CLASS_HR_DSSS:
            {
              if ((preamble == WIFI_PREAMBLE_SHORT) && (txVector.GetMode ().GetDataRate (22) > 1000000))
                {
                  //(Section 16.2.2.3 "Short PPDU format" and Figure 16-2 "Short PPDU format"; IEEE Std 802.11-2016)
                  return MicroSeconds (24);
                }
              else
                {
                  //(Section 16.2.2.2 "Long PPDU format" and Figure 16-1 "Short PPDU format"; IEEE Std 802.11-2016)
                  return MicroSeconds (48);
                }
            }
          default:
            NS_FATAL_ERROR ("modulation class is not matching the preamble type");
            return MicroSeconds (0);
          }
      }
    case WIFI_PREAMBLE_HT_MF:
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_VHT_MU:
      //L-SIG
      return MicroSeconds (4);
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_HE_ER_SU:
    case WIFI_PREAMBLE_HE_MU:
    case WIFI_PREAMBLE_HE_TB:
      //LSIG + R-LSIG
      return MicroSeconds (8);
    case WIFI_PREAMBLE_HT_GF:
      return MicroSeconds (0);
    default:
      NS_FATAL_ERROR ("unsupported preamble type");
      return MicroSeconds (0);
    }
}

Time
WifiPhy::GetStartOfPacketDuration (WifiTxVector txVector)
{
  return MicroSeconds (4);
}

Time
WifiPhy::GetPhyPreambleDuration (WifiTxVector txVector)
{
  WifiPreamble preamble = txVector.GetPreambleType ();
  switch (txVector.GetPreambleType ())
    {
    case WIFI_PREAMBLE_LONG:
    case WIFI_PREAMBLE_SHORT:
      {
        switch (txVector.GetMode ().GetModulationClass ())
          {
            case WIFI_MOD_CLASS_OFDM:
              {
                switch (txVector.GetChannelWidth ())
                  {
                    case 20:
                    default:
                      //(Section 17.3.3 "PHY preamble (SYNC))" Figure 17-4 "OFDM training structure"
                      //also Section 17.3.2.3 "Modulation-dependent parameters" Table 17-4 "Modulation-dependent parameters"; IEEE Std 802.11-2016)
                      return MicroSeconds (16);
                    case 10:
                      //(Section 17.3.3 "PHY preamble (SYNC))" Figure 17-4 "OFDM training structure"
                      //also Section 17.3.2.3 "Modulation-dependent parameters" Table 17-4 "Modulation-dependent parameters"; IEEE Std 802.11-2016)
                      return MicroSeconds (32);
                    case 5:
                      //(Section 17.3.3 "PHY preamble (SYNC))" Figure 17-4 "OFDM training structure"
                      //also Section 17.3.2.3 "Modulation-dependent parameters" Table 17-4 "Modulation-dependent parameters"; IEEE Std 802.11-2016)
                      return MicroSeconds (64);
                  }
              }
            case WIFI_MOD_CLASS_ERP_OFDM:
              return MicroSeconds (16);
            case WIFI_MOD_CLASS_DSSS:
            case WIFI_MOD_CLASS_HR_DSSS:
              {
                if ((preamble == WIFI_PREAMBLE_SHORT) && (txVector.GetMode ().GetDataRate (22) > 1000000))
                  {
                    //(Section 17.2.2.3 "Short PPDU format)" Figure 17-2 "Short PPDU format"; IEEE Std 802.11-2012)
                    return MicroSeconds (72);
                  }
                else
                  {
                    //(Section 17.2.2.2 "Long PPDU format)" Figure 17-1 "Long PPDU format"; IEEE Std 802.11-2012)
                    return MicroSeconds (144);
                  }
              }
            default:
              NS_FATAL_ERROR ("modulation class is not matching the preamble type");
              return MicroSeconds (0);
          }
      }
    case WIFI_PREAMBLE_HT_MF:
    case WIFI_PREAMBLE_VHT_SU:
    case WIFI_PREAMBLE_VHT_MU:
    case WIFI_PREAMBLE_HE_SU:
    case WIFI_PREAMBLE_HE_ER_SU:
    case WIFI_PREAMBLE_HE_MU:
    case WIFI_PREAMBLE_HE_TB:
      //L-STF + L-LTF
      return MicroSeconds (16);
    case WIFI_PREAMBLE_HT_GF:
      //HT-GF-STF + HT-LTF1
      return MicroSeconds (16);
    default:
      NS_FATAL_ERROR ("unsupported preamble type");
      return MicroSeconds (0);
    }
}

Time
WifiPhy::GetPayloadDuration (uint32_t size, WifiTxVector txVector, WifiPhyBand band, MpduType mpdutype)
{
  uint32_t totalAmpduSize;
  double totalAmpduNumSymbols;
  return GetPayloadDuration (size, txVector, band, mpdutype, false, totalAmpduSize, totalAmpduNumSymbols);
}

Time
WifiPhy::GetPayloadDuration (uint32_t size, WifiTxVector txVector, WifiPhyBand band, MpduType mpdutype,
                             bool incFlag, uint32_t &totalAmpduSize, double &totalAmpduNumSymbols)
{
  WifiMode payloadMode = txVector.GetMode ();
  NS_LOG_FUNCTION (size << payloadMode);

  double stbc = 1;
  if (txVector.IsStbc ()
      && (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_HT
          || payloadMode.GetModulationClass () == WIFI_MOD_CLASS_VHT))
    {
      stbc = 2;
    }

  double Nes = 1;
  //todo: improve logic to reduce the number of if cases
  //todo: extend to NSS > 4 for VHT rates
  if (payloadMode == GetHtMcs21 ()
      || payloadMode == GetHtMcs22 ()
      || payloadMode == GetHtMcs23 ()
      || payloadMode == GetHtMcs28 ()
      || payloadMode == GetHtMcs29 ()
      || payloadMode == GetHtMcs30 ()
      || payloadMode == GetHtMcs31 ())
    {
      Nes = 2;
    }
  if (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_VHT)
    {
      if (txVector.GetChannelWidth () == 40
          && txVector.GetNss () == 3
          && payloadMode.GetMcsValue () >= 8)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 80
          && txVector.GetNss () == 2
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 80
          && txVector.GetNss () == 3
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 80
          && txVector.GetNss () == 3
          && payloadMode.GetMcsValue () == 9)
        {
          Nes = 3;
        }
      if (txVector.GetChannelWidth () == 80
          && txVector.GetNss () == 4
          && payloadMode.GetMcsValue () >= 4)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 80
          && txVector.GetNss () == 4
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 3;
        }
      if (txVector.GetChannelWidth () == 160
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 2
          && payloadMode.GetMcsValue () >= 4)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 2
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 3;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 3
          && payloadMode.GetMcsValue () >= 3)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 3
          && payloadMode.GetMcsValue () >= 5)
        {
          Nes = 3;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 3
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 4;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 4
          && payloadMode.GetMcsValue () >= 2)
        {
          Nes = 2;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 4
          && payloadMode.GetMcsValue () >= 4)
        {
          Nes = 3;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 4
          && payloadMode.GetMcsValue () >= 5)
        {
          Nes = 4;
        }
      if (txVector.GetChannelWidth () == 160
          && txVector.GetNss () == 4
          && payloadMode.GetMcsValue () >= 7)
        {
          Nes = 6;
        }
    }

  Time symbolDuration = Seconds (0);
  switch (payloadMode.GetModulationClass ())
    {
    case WIFI_MOD_CLASS_OFDM:
    case WIFI_MOD_CLASS_ERP_OFDM:
      {
        //(Section 18.3.2.4 "Timing related parameters" Table 18-5 "Timing-related parameters"; IEEE Std 802.11-2012
        //corresponds to T_{SYM} in the table)
        switch (txVector.GetChannelWidth ())
          {
          case 20:
          default:
            symbolDuration = MicroSeconds (4);
            break;
          case 10:
            symbolDuration = MicroSeconds (8);
            break;
          case 5:
            symbolDuration = MicroSeconds (16);
            break;
          }
        break;
      }
    case WIFI_MOD_CLASS_HT:
    case WIFI_MOD_CLASS_VHT:
      {
        //if short GI data rate is used then symbol duration is 3.6us else symbol duration is 4us
        //In the future has to create a station manager that only uses these data rates if sender and receiver support GI
        uint16_t gi = txVector.GetGuardInterval ();
        NS_ASSERT (gi == 400 || gi == 800);
        symbolDuration = NanoSeconds (3200 + gi);
      }
      break;
    case WIFI_MOD_CLASS_HE:
      {
        //if short GI data rate is used then symbol duration is 3.6us else symbol duration is 4us
        //In the future has to create a station manager that only uses these data rates if sender and receiver support GI
        uint16_t gi = txVector.GetGuardInterval ();
        NS_ASSERT (gi == 800 || gi == 1600 || gi == 3200);
        symbolDuration = NanoSeconds (12800 + gi);
      }
      break;
    default:
      break;
    }

  double numDataBitsPerSymbol = payloadMode.GetDataRate (txVector) * symbolDuration.GetNanoSeconds () / 1e9;

  double numSymbols = 0;
  if (mpdutype == FIRST_MPDU_IN_AGGREGATE)
    {
      //First packet in an A-MPDU
      numSymbols = (stbc * (16 + size * 8.0 + 6 * Nes) / (stbc * numDataBitsPerSymbol));
      if (incFlag == 1)
        {
          totalAmpduSize += size;
          totalAmpduNumSymbols += numSymbols;
        }
    }
  else if (mpdutype == MIDDLE_MPDU_IN_AGGREGATE)
    {
      //consecutive packets in an A-MPDU
      numSymbols = (stbc * size * 8.0) / (stbc * numDataBitsPerSymbol);
      if (incFlag == 1)
        {
          totalAmpduSize += size;
          totalAmpduNumSymbols += numSymbols;
        }
    }
  else if (mpdutype == LAST_MPDU_IN_AGGREGATE)
    {
      //last packet in an A-MPDU
      uint32_t totalSize = totalAmpduSize + size;
      numSymbols = lrint (stbc * ceil ((16 + totalSize * 8.0 + 6 * Nes) / (stbc * numDataBitsPerSymbol)));
      NS_ASSERT (totalAmpduNumSymbols <= numSymbols);
      numSymbols -= totalAmpduNumSymbols;
      if (incFlag == 1)
        {
          totalAmpduSize = 0;
          totalAmpduNumSymbols = 0;
        }
    }
  else if (mpdutype == NORMAL_MPDU || mpdutype == SINGLE_MPDU)
    {
      //Not an A-MPDU or single MPDU (i.e. the current payload contains both service and padding)
      //The number of OFDM symbols in the data field when BCC encoding
      //is used is given in equation 19-32 of the IEEE 802.11-2016 standard.
      numSymbols = lrint (stbc * ceil ((16 + size * 8.0 + 6.0 * Nes) / (stbc * numDataBitsPerSymbol)));
    }
  else
    {
      NS_FATAL_ERROR ("Unknown MPDU type");
    }

  switch (payloadMode.GetModulationClass ())
    {
    case WIFI_MOD_CLASS_OFDM:
    case WIFI_MOD_CLASS_ERP_OFDM:
      {
        //Add signal extension for ERP PHY
        if (payloadMode.GetModulationClass () == WIFI_MOD_CLASS_ERP_OFDM)
          {
            return FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ())) + MicroSeconds (6);
          }
        else
          {
            return FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ()));
          }
      }
    case WIFI_MOD_CLASS_HT:
    case WIFI_MOD_CLASS_VHT:
      {
        if ((payloadMode.GetModulationClass () == WIFI_MOD_CLASS_HT) && (band == WIFI_PHY_BAND_2_4GHZ)
            && (mpdutype == NORMAL_MPDU || mpdutype == SINGLE_MPDU || mpdutype == LAST_MPDU_IN_AGGREGATE)) //at 2.4 GHz
          {
            return FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ())) + MicroSeconds (6);
          }
        else //at 5 GHz
          {
            return FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ()));
          }
      }
    case WIFI_MOD_CLASS_HE:
      {
        if ((band == WIFI_PHY_BAND_2_4GHZ)
            && ((mpdutype == NORMAL_MPDU || mpdutype == SINGLE_MPDU || mpdutype == LAST_MPDU_IN_AGGREGATE))) //at 2.4 GHz
          {
            return FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ())) + MicroSeconds (6);
          }
        else //at 5 GHz or 6 GHz
          {
            return FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ()));
          }
      }
    case WIFI_MOD_CLASS_DSSS:
    case WIFI_MOD_CLASS_HR_DSSS:
      return MicroSeconds (lrint (ceil ((size * 8.0) / (payloadMode.GetDataRate (22) / 1.0e6))));
    default:
      NS_FATAL_ERROR ("unsupported modulation class");
      return MicroSeconds (0);
    }
}

Time
WifiPhy::CalculatePhyPreambleAndHeaderDuration (WifiTxVector txVector)
{
  WifiPreamble preamble = txVector.GetPreambleType ();
  Time duration = GetPhyPreambleDuration (txVector)
    + GetPhyHeaderDuration (txVector)
    + GetPhyHtSigHeaderDuration (preamble)
    + GetPhySigA1Duration (preamble)
    + GetPhySigA2Duration (preamble)
    + GetPhyTrainingSymbolDuration (txVector)
    + GetPhySigBDuration (preamble);
  return duration;
}

Time
WifiPhy::CalculateTxDuration (uint32_t size, WifiTxVector txVector, WifiPhyBand band)
{
  Time duration = CalculatePhyPreambleAndHeaderDuration (txVector)
    + GetPayloadDuration (size, txVector, band);
  return duration;
}

void
WifiPhy::NotifyTxBegin (Ptr<const WifiPsdu> psdu, double txPowerW)
{
  for (auto& mpdu : *PeekPointer (psdu))
    {
      m_phyTxBeginTrace (mpdu->GetProtocolDataUnit (), txPowerW);
    }
}

void
WifiPhy::NotifyTxEnd (Ptr<const WifiPsdu> psdu)
{
  for (auto& mpdu : *PeekPointer (psdu))
    {
      m_phyTxEndTrace (mpdu->GetProtocolDataUnit ());
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
WifiPhy::NotifyRxBegin (Ptr<const WifiPsdu> psdu)
{
  for (auto& mpdu : *PeekPointer (psdu))
    {
      m_phyRxBeginTrace (mpdu->GetProtocolDataUnit ());
    }
}

void
WifiPhy::NotifyRxEnd (Ptr<const WifiPsdu> psdu)
{
  for (auto& mpdu : *PeekPointer (psdu))
    {
      m_phyRxEndTrace (mpdu->GetProtocolDataUnit ());
    }
}

void
WifiPhy::NotifyRxDrop (Ptr<const WifiPsdu> psdu, WifiPhyRxfailureReason reason)
{
  for (auto& mpdu : *PeekPointer (psdu))
    {
      m_phyRxDropTrace (mpdu->GetProtocolDataUnit (), reason);
    }
}

void
WifiPhy::NotifyMonitorSniffRx (Ptr<const WifiPsdu> psdu, uint16_t channelFreqMhz, WifiTxVector txVector,
                               SignalNoiseDbm signalNoise, std::vector<bool> statusPerMpdu)
{
  MpduInfo aMpdu;
  if (psdu->IsAggregate ())
    {
      //Expand A-MPDU
      NS_ASSERT_MSG (txVector.IsAggregation (), "TxVector with aggregate flag expected here according to PSDU");
      aMpdu.mpduRefNumber = ++m_rxMpduReferenceNumber;
      size_t nMpdus = psdu->GetNMpdus ();
      NS_ASSERT_MSG (statusPerMpdu.size () == nMpdus, "Should have one reception status per MPDU");
      aMpdu.type = (psdu->IsSingle ()) ? SINGLE_MPDU: FIRST_MPDU_IN_AGGREGATE;
      for (size_t i = 0; i < nMpdus;)
        {
          if (statusPerMpdu.at (i)) //packet received without error, hand over to sniffer
            {
              m_phyMonitorSniffRxTrace (psdu->GetAmpduSubframe (i), channelFreqMhz, txVector, aMpdu, signalNoise);
            }
          ++i;
          aMpdu.type = (i == (nMpdus - 1)) ? LAST_MPDU_IN_AGGREGATE : MIDDLE_MPDU_IN_AGGREGATE;
        }
    }
  else
    {
      aMpdu.type = NORMAL_MPDU;
      NS_ASSERT_MSG (statusPerMpdu.size () == 1, "Should have one reception status for normal MPDU");
      m_phyMonitorSniffRxTrace (psdu->GetPacket (), channelFreqMhz, txVector, aMpdu, signalNoise);
    }
}

void
WifiPhy::NotifyMonitorSniffTx (Ptr<const WifiPsdu> psdu, uint16_t channelFreqMhz, WifiTxVector txVector)
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
          m_phyMonitorSniffTxTrace (psdu->GetAmpduSubframe (i), channelFreqMhz, txVector, aMpdu);
          ++i;
          aMpdu.type = (i == (nMpdus - 1)) ? LAST_MPDU_IN_AGGREGATE : MIDDLE_MPDU_IN_AGGREGATE;
        }
    }
  else
    {
      aMpdu.type = NORMAL_MPDU;
      m_phyMonitorSniffTxTrace (psdu->GetPacket (), channelFreqMhz, txVector, aMpdu);
    }
}

void
WifiPhy::NotifyEndOfHePreamble (HePreambleParameters params)
{
  m_phyEndOfHePreambleTrace (params);
}

void
WifiPhy::Send (Ptr<const WifiPsdu> psdu, WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << *psdu << txVector);
  /* Transmission can happen if:
   *  - we are syncing on a packet. It is the responsibility of the
   *    MAC layer to avoid doing this but the PHY does nothing to
   *    prevent it.
   *  - we are idle
   */
  NS_ASSERT (!m_state->IsStateTx () && !m_state->IsStateSwitching ());
  NS_ASSERT (m_endTxEvent.IsExpired ());

  if (txVector.GetNss () > GetMaxSupportedTxSpatialStreams ())
    {
      NS_FATAL_ERROR ("Unsupported number of spatial streams!");
    }

  if (m_state->IsStateSleep ())
    {
      NS_LOG_DEBUG ("Dropping packet because in sleep mode");
      NotifyTxDrop (psdu);
      return;
    }

  Time txDuration = CalculateTxDuration (psdu->GetSize (), txVector, GetPhyBand ());
  NS_ASSERT (txDuration.IsStrictlyPositive ());

  if ((m_currentEvent != 0) && (m_currentEvent->GetEndTime () > (Simulator::Now () + m_state->GetDelayUntilIdle ())))
    {
      //that packet will be noise _after_ the transmission.
      MaybeCcaBusyDuration ();
    }

  if (m_currentEvent != 0)
    {
      AbortCurrentReception (RECEPTION_ABORTED_BY_TX);
    }

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
  NotifyTxBegin (psdu, txPowerW);
  m_phyTxPsduBeginTrace (psdu, txVector, txPowerW);
  NotifyMonitorSniffTx (psdu, GetFrequency (), txVector);
  m_state->SwitchToTx (txDuration, psdu->GetPacket (), GetPowerDbm (txVector.GetTxPowerLevel ()), txVector);

  Ptr<WifiPpdu> ppdu = Create<WifiPpdu> (psdu, txVector, txDuration, GetPhyBand ());

  if (m_wifiRadioEnergyModel != 0 && m_wifiRadioEnergyModel->GetMaximumTimeInState (WifiPhyState::TX) < txDuration)
    {
      ppdu->SetTruncatedTx ();
    }

  m_endTxEvent = Simulator::Schedule (txDuration, &WifiPhy::NotifyTxEnd, this, psdu);

  StartTx (ppdu);

  m_channelAccessRequested = false;
  m_powerRestricted = false;
}

void
WifiPhy::StartReceiveHeader (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (!IsStateRx ());
  NS_ASSERT (m_endPhyRxEvent.IsExpired ());
  NS_ASSERT (m_currentEvent != 0);
  NS_ASSERT (event->GetStartTime () == m_currentEvent->GetStartTime ());
  NS_ASSERT (event->GetEndTime () == m_currentEvent->GetEndTime ());

  InterferenceHelper::SnrPer snrPer = m_interference.CalculateNonHtPhyHeaderSnrPer (event);
  double snr = snrPer.snr;
  NS_LOG_DEBUG ("snr(dB)=" << RatioToDb (snrPer.snr) << ", per=" << snrPer.per);

  if (!m_preambleDetectionModel || (m_preambleDetectionModel->IsPreambleDetected (event->GetRxPowerW (), snr, m_channelWidth)))
    {
      NotifyRxBegin (event->GetPsdu ());

      m_timeLastPreambleDetected = Simulator::Now ();
      WifiTxVector txVector = event->GetTxVector ();

      if ((txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HT) && (txVector.GetPreambleType () == WIFI_PREAMBLE_HT_GF))
        {
          //No non-HT PHY header for HT GF
          Time remainingPreambleHeaderDuration = CalculatePhyPreambleAndHeaderDuration (txVector) - GetPreambleDetectionDuration ();
          m_state->SwitchMaybeToCcaBusy (remainingPreambleHeaderDuration);
          m_endPhyRxEvent = Simulator::Schedule (remainingPreambleHeaderDuration, &WifiPhy::StartReceivePayload, this, event);
        }
      else
        {
          //Schedule end of non-HT PHY header
          Time remainingPreambleAndNonHtHeaderDuration = GetPhyPreambleDuration (txVector) + GetPhyHeaderDuration (txVector) - GetPreambleDetectionDuration ();
          m_state->SwitchMaybeToCcaBusy (remainingPreambleAndNonHtHeaderDuration);
          m_endPhyRxEvent = Simulator::Schedule (remainingPreambleAndNonHtHeaderDuration, &WifiPhy::ContinueReceiveHeader, this, event);
        }
    }
  else
    {
      NS_LOG_DEBUG ("Drop packet because PHY preamble detection failed");
      NotifyRxDrop (event->GetPsdu (), PREAMBLE_DETECT_FAILURE);
      m_interference.NotifyRxEnd ();
      m_currentEvent = 0;

      // Like CCA-SD, CCA-ED is governed by the 4μs CCA window to flag CCA-BUSY
      // for any received signal greater than the CCA-ED threshold.
      if (event->GetEndTime () > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          MaybeCcaBusyDuration ();
        }
    }
}

void
WifiPhy::ContinueReceiveHeader (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (m_endPhyRxEvent.IsExpired ());

  InterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculateNonHtPhyHeaderSnrPer (event);

  if (m_random->GetValue () > snrPer.per) //non-HT PHY header reception succeeded
    {
      NS_LOG_DEBUG ("Received non-HT PHY header");
      WifiTxVector txVector = event->GetTxVector ();
      Time remainingRxDuration = event->GetEndTime () - Simulator::Now ();
      m_state->SwitchMaybeToCcaBusy (remainingRxDuration);
      Time remainingPreambleHeaderDuration = CalculatePhyPreambleAndHeaderDuration (txVector) - GetPhyPreambleDuration (txVector) - GetPhyHeaderDuration (txVector);
      m_endPhyRxEvent = Simulator::Schedule (remainingPreambleHeaderDuration, &WifiPhy::StartReceivePayload, this, event);
    }
  else //non-HT PHY header reception failed
    {
      NS_LOG_DEBUG ("Abort reception because non-HT PHY header reception failed");
      AbortCurrentReception (L_SIG_FAILURE);
      if (event->GetEndTime () > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          MaybeCcaBusyDuration ();
        }
    }
}

void
WifiPhy::StartReceivePreamble (Ptr<WifiPpdu> ppdu, double rxPowerW)
{
  NS_LOG_FUNCTION (this << *ppdu << rxPowerW);
  WifiTxVector txVector = ppdu->GetTxVector ();
  Time rxDuration = ppdu->GetTxDuration ();
  Ptr<const WifiPsdu> psdu = ppdu->GetPsdu ();
  Ptr<Event> event = m_interference.Add (ppdu, txVector, rxDuration, rxPowerW);
  Time endRx = Simulator::Now () + rxDuration;

  if (m_state->GetState () == WifiPhyState::OFF)
    {
      NS_LOG_DEBUG ("Cannot start RX because device is OFF");
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          MaybeCcaBusyDuration ();
        }
      return;
    }

  if (ppdu->IsTruncatedTx ())
    {
      NS_LOG_DEBUG ("Packet reception stopped because transmitter has been switched off");
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          MaybeCcaBusyDuration ();
        }
      return;
    }

  if (!txVector.GetModeInitialized ())
    {
      //If SetRate method was not called above when filling in txVector, this means the PHY does support the rate indicated in PHY SIG headers
      NS_LOG_DEBUG ("drop packet because of unsupported RX mode");
      NotifyRxDrop (psdu, UNSUPPORTED_SETTINGS);
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          MaybeCcaBusyDuration ();
        }
      return;
    }

  switch (m_state->GetState ())
    {
    case WifiPhyState::SWITCHING:
      NS_LOG_DEBUG ("drop packet because of channel switching");
      NotifyRxDrop (psdu, CHANNEL_SWITCHING);
      /*
       * Packets received on the upcoming channel are added to the event list
       * during the switching state. This way the medium can be correctly sensed
       * when the device listens to the channel for the first time after the
       * switching e.g. after channel switching, the channel may be sensed as
       * busy due to other devices' transmissions started before the end of
       * the switching.
       */
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          //that packet will be noise _after_ the completion of the channel switching.
          MaybeCcaBusyDuration ();
        }
      break;
    case WifiPhyState::RX:
      NS_ASSERT (m_currentEvent != 0);
      if (m_frameCaptureModel != 0
          && m_frameCaptureModel->IsInCaptureWindow (m_timeLastPreambleDetected)
          && m_frameCaptureModel->CaptureNewFrame (m_currentEvent, event))
        {
          AbortCurrentReception (FRAME_CAPTURE_PACKET_SWITCH);
          NS_LOG_DEBUG ("Switch to new packet");
          StartRx (event, rxPowerW);
        }
      else
        {
          NS_LOG_DEBUG ("Drop packet because already in Rx (power=" <<
                        rxPowerW << "W)");
          NotifyRxDrop (psdu, RXING);
          if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
            {
              //that packet will be noise _after_ the reception of the currently-received packet.
              MaybeCcaBusyDuration ();
            }
        }
      break;
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("Drop packet because already in Tx (power=" <<
                    rxPowerW << "W)");
      NotifyRxDrop (psdu, TXING);
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          //that packet will be noise _after_ the transmission of the currently-transmitted packet.
          MaybeCcaBusyDuration ();
        }
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
              StartRx (event, rxPowerW);
            }
          else
            {
              NS_LOG_DEBUG ("Drop packet because already in Rx (power=" <<
                            rxPowerW << "W)");
              NotifyRxDrop (psdu, RXING);
              if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
                {
                  //that packet will be noise _after_ the reception of the currently-received packet.
                  MaybeCcaBusyDuration ();
                }
            }
        }
      else
        {
          StartRx (event, rxPowerW);
        }
      break;
    case WifiPhyState::IDLE:
      StartRx (event, rxPowerW);
      break;
    case WifiPhyState::SLEEP:
      NS_LOG_DEBUG ("Drop packet because in sleep mode");
      NotifyRxDrop (psdu, SLEEPING);
      if (endRx > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
        {
          //that packet will be noise _after_ the sleep period.
          MaybeCcaBusyDuration ();
        }
      break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
}

void
WifiPhy::MaybeCcaBusyDuration ()
{
  //We are here because we have received the first bit of a packet and we are
  //not going to be able to synchronize on it
  //In this model, CCA becomes busy when the aggregation of all signals as
  //tracked by the InterferenceHelper class is higher than the CcaBusyThreshold

  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW);
  if (!delayUntilCcaEnd.IsZero ())
    {
      m_state->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
    }
}

void
WifiPhy::StartReceivePayload (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (m_endPhyRxEvent.IsExpired ());
  NS_ASSERT (m_endRxEvent.IsExpired ());
  WifiTxVector txVector = event->GetTxVector ();
  WifiMode txMode = txVector.GetMode ();
  bool canReceivePayload;
  if (txMode.GetModulationClass () >= WIFI_MOD_CLASS_HT)
    {
      InterferenceHelper::SnrPer snrPer;
      snrPer = m_interference.CalculateHtPhyHeaderSnrPer (event);
      NS_LOG_DEBUG ("snr(dB)=" << RatioToDb (snrPer.snr) << ", per=" << snrPer.per);
      canReceivePayload = (m_random->GetValue () > snrPer.per);
    }
  else
    {
      //If we are here, this means non-HT PHY header was already successfully received
      canReceivePayload = true;
    }
  Time payloadDuration = event->GetEndTime () - event->GetStartTime () - CalculatePhyPreambleAndHeaderDuration (txVector);
  if (canReceivePayload) //PHY reception succeeded
    {
      if (txVector.GetNss () > GetMaxSupportedRxSpatialStreams ())
        {
          NS_LOG_DEBUG ("Packet reception could not be started because not enough RX antennas");
          NotifyRxDrop (event->GetPsdu (), UNSUPPORTED_SETTINGS);
        }
      else if ((txVector.GetChannelWidth () >= 40) && (txVector.GetChannelWidth () > GetChannelWidth ()))
        {
          NS_LOG_DEBUG ("Packet reception could not be started because not enough channel width");
          NotifyRxDrop (event->GetPsdu (), UNSUPPORTED_SETTINGS);
        }
      else if (!IsModeSupported (txMode) && !IsMcsSupported (txMode))
        {
          NS_LOG_DEBUG ("Drop packet because it was sent using an unsupported mode (" << txMode << ")");
          NotifyRxDrop (event->GetPsdu (), UNSUPPORTED_SETTINGS);
        }
      else
        {
          m_statusPerMpdu.clear();
          if (event->GetPsdu ()->GetNMpdus () > 1)
            {
              ScheduleEndOfMpdus (event);
            }
          m_state->SwitchToRx (payloadDuration);
          m_endRxEvent = Simulator::Schedule (payloadDuration, &WifiPhy::EndReceive, this, event);
          NS_LOG_DEBUG ("Receiving PSDU");
          m_phyRxPayloadBeginTrace (txVector, payloadDuration); //this callback (equivalent to PHY-RXSTART primitive) is triggered only if headers have been correctly decoded and that the mode within is supported
          if (txMode.GetModulationClass () == WIFI_MOD_CLASS_HE)
            {
              HePreambleParameters params;
              params.rssiW = event->GetRxPowerW ();
              params.bssColor = event->GetTxVector ().GetBssColor ();
              NotifyEndOfHePreamble (params);
            }
          return;
        }
    }
  else //PHY reception failed
    {
      NS_LOG_DEBUG ("Drop packet because HT PHY header reception failed");
      NotifyRxDrop (event->GetPsdu (), SIG_A_FAILURE);
    }
  m_endRxEvent = Simulator::Schedule (payloadDuration, &WifiPhy::ResetReceive, this, event);
}

void
WifiPhy::ScheduleEndOfMpdus (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
  WifiTxVector txVector = event->GetTxVector ();
  Time endOfMpduDuration = NanoSeconds (0);
  Time relativeStart = NanoSeconds (0);
  Time psduDuration = ppdu->GetTxDuration () - CalculatePhyPreambleAndHeaderDuration (txVector);
  Time remainingAmpduDuration = psduDuration;
  MpduType mpdutype = FIRST_MPDU_IN_AGGREGATE;
  uint32_t totalAmpduSize = 0;
  double totalAmpduNumSymbols = 0.0;
  Ptr<const WifiPsdu> psdu = event->GetPsdu ();
  size_t nMpdus = psdu->GetNMpdus ();
  auto mpdu = psdu->begin ();
  for (size_t i = 0; i < nMpdus && mpdu != psdu->end (); ++mpdu)
    {
      Time mpduDuration = GetPayloadDuration (psdu->GetAmpduSubframeSize (i), txVector,
                                              GetPhyBand (), mpdutype, true, totalAmpduSize, totalAmpduNumSymbols);

      remainingAmpduDuration -= mpduDuration;
      if (i == (nMpdus - 1) && !remainingAmpduDuration.IsZero ()) //no more MPDU coming
        {
          mpduDuration += remainingAmpduDuration; //apply a correction just in case rounding had induced slight shift
        }

      endOfMpduDuration += mpduDuration;
      m_endOfMpduEvents.push_back (Simulator::Schedule (endOfMpduDuration, &WifiPhy::EndOfMpdu, this, event, Create<WifiPsdu> (*mpdu, false), i, relativeStart, mpduDuration));

      //Prepare next iteration
      ++i;
      relativeStart += mpduDuration;
      mpdutype = (i == (nMpdus - 1)) ? LAST_MPDU_IN_AGGREGATE : MIDDLE_MPDU_IN_AGGREGATE;
    }
}

void
WifiPhy::EndOfMpdu (Ptr<Event> event, Ptr<const WifiPsdu> psdu, size_t mpduIndex, Time relativeStart, Time mpduDuration)
{
  NS_LOG_FUNCTION (this << *event << mpduIndex << relativeStart << mpduDuration);
  Ptr<const WifiPpdu> ppdu = event->GetPpdu ();

  std::pair<bool, SignalNoiseDbm> rxInfo = GetReceptionStatus (psdu, event, relativeStart, mpduDuration);
  NS_LOG_DEBUG ("Extracted MPDU #" << mpduIndex << ": duration: " << mpduDuration.GetNanoSeconds () << "ns" <<
                ", correct reception: " << rxInfo.first << ", Signal/Noise: " << rxInfo.second.signal << "/" << rxInfo.second.noise << "dBm");

  m_signalNoise = rxInfo.second;
  m_statusPerMpdu.push_back (rxInfo.first);

  if (rxInfo.first)
    {
      m_state->ContinueRxNextMpdu (Copy (psdu), m_interference.CalculateSnr (event), event->GetTxVector ());
    }
}

void
WifiPhy::EndReceive (Ptr<Event> event)
{
  Time psduDuration = event->GetEndTime () - event->GetStartTime ();
  NS_LOG_FUNCTION (this << *event << psduDuration);
  NS_ASSERT (GetLastRxEndTime () == Simulator::Now ());
  NS_ASSERT (event->GetEndTime () == Simulator::Now ());

  Ptr<const WifiPsdu> psdu = event->GetPsdu ();
  if (psdu->GetNMpdus () == 1)
    {
      //We do not enter here for A-MPDU since this is done in WifiPhy::EndOfMpdu
      std::pair<bool, SignalNoiseDbm> rxInfo = GetReceptionStatus (psdu, event, NanoSeconds (0), psduDuration);
      m_signalNoise = rxInfo.second;
      m_statusPerMpdu.push_back (rxInfo.first);
    }

  NotifyRxEnd (psdu);
  double snr = m_interference.CalculateSnr (event);
  if (std::count (m_statusPerMpdu.begin (), m_statusPerMpdu.end (), true))
    {
      //At least one MPDU has been successfully received
      WifiTxVector txVector = event->GetTxVector ();
      NotifyMonitorSniffRx (psdu, GetFrequency (), txVector, m_signalNoise, m_statusPerMpdu);
      m_state->SwitchFromRxEndOk (Copy (psdu), snr, txVector, m_statusPerMpdu);
    }
  else
    {
      m_state->SwitchFromRxEndError (Copy (psdu), snr);
    }

  m_interference.NotifyRxEnd ();
  m_currentEvent = 0;
  MaybeCcaBusyDuration ();
}

std::pair<bool, SignalNoiseDbm>
WifiPhy::GetReceptionStatus (Ptr<const WifiPsdu> psdu, Ptr<Event> event, Time relativeMpduStart, Time mpduDuration)
{
  NS_LOG_FUNCTION (this << *psdu << *event << relativeMpduStart << mpduDuration);
  InterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculatePayloadSnrPer (event, std::make_pair (relativeMpduStart, relativeMpduStart + mpduDuration));

  NS_LOG_DEBUG ("mode=" << (event->GetTxVector ().GetMode ().GetDataRate (event->GetTxVector ())) <<
                ", snr(dB)=" << RatioToDb (snrPer.snr) << ", per=" << snrPer.per << ", size=" << psdu->GetSize () <<
                ", relativeStart = " << relativeMpduStart.GetNanoSeconds () << "ns, duration = " << mpduDuration.GetNanoSeconds () << "ns");

  // There are two error checks: PER and receive error model check.
  // PER check models is typical for Wi-Fi and is based on signal modulation;
  // Receive error model is optional, if we have an error model and
  // it indicates that the packet is corrupt, drop the packet.
  SignalNoiseDbm signalNoise;
  signalNoise.signal = WToDbm (event->GetRxPowerW ());
  signalNoise.noise = WToDbm (event->GetRxPowerW () / snrPer.snr);
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
  NS_ASSERT (event->GetEndTime () == Simulator::Now ());
  NS_ASSERT (!IsStateRx ());
  m_interference.NotifyRxEnd ();
  m_currentEvent = 0;
  MaybeCcaBusyDuration ();
}

void
WifiPhy::NotifyChannelAccessRequested (void)
{
  NS_LOG_FUNCTION (this);
  m_channelAccessRequested = true;
}

// Clause 15 rates (DSSS)

WifiMode
WifiPhy::GetDsssRate1Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DsssRate1Mbps",
                                     WIFI_MOD_CLASS_DSSS,
                                     true,
                                     WIFI_CODE_RATE_UNDEFINED,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetDsssRate2Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DsssRate2Mbps",
                                     WIFI_MOD_CLASS_DSSS,
                                     true,
                                     WIFI_CODE_RATE_UNDEFINED,
                                     4);
  return mode;
}


// Clause 18 rates (HR/DSSS)

WifiMode
WifiPhy::GetDsssRate5_5Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DsssRate5_5Mbps",
                                     WIFI_MOD_CLASS_HR_DSSS,
                                     true,
                                     WIFI_CODE_RATE_UNDEFINED,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetDsssRate11Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("DsssRate11Mbps",
                                     WIFI_MOD_CLASS_HR_DSSS,
                                     true,
                                     WIFI_CODE_RATE_UNDEFINED,
                                     256);
  return mode;
}


// Clause 19.5 rates (ERP-OFDM)

WifiMode
WifiPhy::GetErpOfdmRate6Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate6Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate9Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate9Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate12Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate12Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate18Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate18Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate24Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate24Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate36Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate36Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate48Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate48Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

WifiMode
WifiPhy::GetErpOfdmRate54Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("ErpOfdmRate54Mbps",
                                     WIFI_MOD_CLASS_ERP_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}


// Clause 17 rates (OFDM)

WifiMode
WifiPhy::GetOfdmRate6Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate6Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate9Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate9Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate12Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate12Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate18Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate18Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate24Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate24Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate36Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate36Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate48Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate48Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate54Mbps ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate54Mbps",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}


// 10 MHz channel rates

WifiMode
WifiPhy::GetOfdmRate3MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate3MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate4_5MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate4_5MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate6MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate6MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate9MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate9MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate12MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate12MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate18MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate18MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate24MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate24MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate27MbpsBW10MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate27MbpsBW10MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}


// 5 MHz channel rates

WifiMode
WifiPhy::GetOfdmRate1_5MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate1_5MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate2_25MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate2_25MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     2);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate3MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate3MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate4_5MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate4_5MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     4);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate6MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate6MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     true,
                                     WIFI_CODE_RATE_1_2,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate9MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate9MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     16);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate12MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate12MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_2_3,
                                     64);
  return mode;
}

WifiMode
WifiPhy::GetOfdmRate13_5MbpsBW5MHz ()
{
  static WifiMode mode =
    WifiModeFactory::CreateWifiMode ("OfdmRate13_5MbpsBW5MHz",
                                     WIFI_MOD_CLASS_OFDM,
                                     false,
                                     WIFI_CODE_RATE_3_4,
                                     64);
  return mode;
}


// Clause 20

WifiMode
WifiPhy::GetHtMcs0 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs0", 0, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs1 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs1", 1, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs2 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs2", 2, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs3 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs3", 3, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs4 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs4", 4, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs5 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs5", 5, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs6 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs6", 6, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs7 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs7", 7, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs8 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs8", 8, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs9 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs9", 9, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs10 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs10", 10, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs11 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs11", 11, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs12 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs12", 12, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs13 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs13", 13, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs14 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs14", 14, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs15 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs15", 15, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs16 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs16", 16, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs17 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs17", 17, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs18 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs18", 18, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs19 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs19", 19, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs20 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs20", 20, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs21 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs21", 21, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs22 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs22", 22, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs23 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs23", 23, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs24 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs24", 24, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs25 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs25", 25, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs26 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs26", 26, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs27 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs27", 27, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs28 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs28", 28, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs29 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs29", 29, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs30 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs30", 30, WIFI_MOD_CLASS_HT);
  return mcs;
}

WifiMode
WifiPhy::GetHtMcs31 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HtMcs31", 31, WIFI_MOD_CLASS_HT);
  return mcs;
}


// Clause 22

WifiMode
WifiPhy::GetVhtMcs0 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs0", 0, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs1 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs1", 1, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs2 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs2", 2, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs3 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs3", 3, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs4 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs4", 4, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs5 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs5", 5, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs6 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs6", 6, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs7 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs7", 7, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs8 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs8", 8, WIFI_MOD_CLASS_VHT);
  return mcs;
}

WifiMode
WifiPhy::GetVhtMcs9 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("VhtMcs9", 9, WIFI_MOD_CLASS_VHT);
  return mcs;
}

// Clause 26

WifiMode
WifiPhy::GetHeMcs0 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs0", 0, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs1 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs1", 1, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs2 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs2", 2, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs3 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs3", 3, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs4 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs4", 4, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs5 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs5", 5, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs6 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs6", 6, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs7 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs7", 7, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs8 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs8", 8, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs9 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs9", 9, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs10 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs10", 10, WIFI_MOD_CLASS_HE);
  return mcs;
}

WifiMode
WifiPhy::GetHeMcs11 ()
{
  static WifiMode mcs =
    WifiModeFactory::CreateWifiMcs ("HeMcs11", 11, WIFI_MOD_CLASS_HE);
  return mcs;
}

bool
WifiPhy::IsModeSupported (WifiMode mode) const
{
  for (uint8_t i = 0; i < GetNModes (); i++)
    {
      if (mode == GetMode (i))
        {
          return true;
        }
    }
  return false;
}

bool
WifiPhy::IsMcsSupported (WifiMode mcs) const
{
  WifiModulationClass modulation = mcs.GetModulationClass ();
  if (modulation == WIFI_MOD_CLASS_HT || modulation == WIFI_MOD_CLASS_VHT
      || modulation == WIFI_MOD_CLASS_HE)
    {
      return IsMcsSupported (modulation, mcs.GetMcsValue ());
    }
  return false;
}

bool
WifiPhy::IsMcsSupported (WifiModulationClass mc, uint8_t mcs) const
{
  if (m_mcsIndexMap.find (mc) == m_mcsIndexMap.end ())
    {
      return false;
    }
  if (m_mcsIndexMap.at (mc).find (mcs) == m_mcsIndexMap.at (mc).end ())
    {
      return false;
    }
  return true;
}

uint8_t
WifiPhy::GetNModes (void) const
{
  return static_cast<uint8_t> (m_deviceRateSet.size ());
}

WifiMode
WifiPhy::GetMode (uint8_t mode) const
{
  return m_deviceRateSet[mode];
}

uint8_t
WifiPhy::GetNMcs (void) const
{
  return static_cast<uint8_t> (m_deviceMcsSet.size ());
}

WifiMode
WifiPhy::GetMcs (uint8_t mcs) const
{
  return m_deviceMcsSet[mcs];
}

WifiMode
WifiPhy::GetMcs (WifiModulationClass modulation, uint8_t mcs) const
{
  NS_ASSERT_MSG (IsMcsSupported (modulation, mcs), "Unsupported MCS");
  uint8_t index = m_mcsIndexMap.at (modulation).at (mcs);
  NS_ASSERT (index < m_deviceMcsSet.size ());
  WifiMode mode = m_deviceMcsSet[index];
  NS_ASSERT (mode.GetModulationClass () == modulation);
  NS_ASSERT (mode.GetMcsValue () == mcs);
  return mode;
}

WifiMode
WifiPhy::GetHtMcs (uint8_t mcs)
{
  WifiMode mode;
  switch (mcs)
    {
      case 0:
        mode = WifiPhy::GetHtMcs0 ();
        break;
      case 1:
        mode = WifiPhy::GetHtMcs1 ();
        break;
      case 2:
        mode = WifiPhy::GetHtMcs2 ();
        break;
      case 3:
        mode = WifiPhy::GetHtMcs3 ();
        break;
      case 4:
        mode = WifiPhy::GetHtMcs4 ();
        break;
      case 5:
        mode = WifiPhy::GetHtMcs5 ();
        break;
      case 6:
        mode = WifiPhy::GetHtMcs6 ();
        break;
      case 7:
        mode = WifiPhy::GetHtMcs7 ();
        break;
      case 8:
        mode = WifiPhy::GetHtMcs8 ();
        break;
      case 9:
        mode = WifiPhy::GetHtMcs9 ();
        break;
      case 10:
        mode = WifiPhy::GetHtMcs10 ();
        break;
      case 11:
        mode = WifiPhy::GetHtMcs11 ();
        break;
      case 12:
        mode = WifiPhy::GetHtMcs12 ();
        break;
      case 13:
        mode = WifiPhy::GetHtMcs13 ();
        break;
      case 14:
        mode = WifiPhy::GetHtMcs14 ();
        break;
      case 15:
        mode = WifiPhy::GetHtMcs15 ();
        break;
      case 16:
        mode = WifiPhy::GetHtMcs16 ();
        break;
      case 17:
        mode = WifiPhy::GetHtMcs17 ();
        break;
      case 18:
        mode = WifiPhy::GetHtMcs18 ();
        break;
      case 19:
        mode = WifiPhy::GetHtMcs19 ();
        break;
      case 20:
        mode = WifiPhy::GetHtMcs20 ();
        break;
      case 21:
        mode = WifiPhy::GetHtMcs21 ();
        break;
      case 22:
        mode = WifiPhy::GetHtMcs22 ();
        break;
      case 23:
        mode = WifiPhy::GetHtMcs23 ();
        break;
      case 24:
        mode = WifiPhy::GetHtMcs24 ();
        break;
      case 25:
        mode = WifiPhy::GetHtMcs25 ();
        break;
      case 26:
        mode = WifiPhy::GetHtMcs26 ();
        break;
      case 27:
        mode = WifiPhy::GetHtMcs27 ();
        break;
      case 28:
        mode = WifiPhy::GetHtMcs28 ();
        break;
      case 29:
        mode = WifiPhy::GetHtMcs29 ();
        break;
      case 30:
        mode = WifiPhy::GetHtMcs30 ();
        break;
      case 31:
        mode = WifiPhy::GetHtMcs31 ();
        break;
      default:
        NS_ABORT_MSG ("Invalid HT MCS");
        break;
    }
  return mode;
}

WifiMode
WifiPhy::GetVhtMcs (uint8_t mcs)
{
  WifiMode mode;
  switch (mcs)
    {
      case 0:
        mode = WifiPhy::GetVhtMcs0 ();
        break;
      case 1:
        mode = WifiPhy::GetVhtMcs1 ();
        break;
      case 2:
        mode = WifiPhy::GetVhtMcs2 ();
        break;
      case 3:
        mode = WifiPhy::GetVhtMcs3 ();
        break;
      case 4:
        mode = WifiPhy::GetVhtMcs4 ();
        break;
      case 5:
        mode = WifiPhy::GetVhtMcs5 ();
        break;
      case 6:
        mode = WifiPhy::GetVhtMcs6 ();
        break;
      case 7:
        mode = WifiPhy::GetVhtMcs7 ();
        break;
      case 8:
        mode = WifiPhy::GetVhtMcs8 ();
        break;
      case 9:
        mode = WifiPhy::GetVhtMcs9 ();
        break;
      default:
        NS_ABORT_MSG ("Invalid VHT MCS");
        break;
    }
  return mode;
}

WifiMode
WifiPhy::GetHeMcs (uint8_t mcs)
{
  WifiMode mode;
  switch (mcs)
    {
      case 0:
        mode = WifiPhy::GetHeMcs0 ();
        break;
      case 1:
        mode = WifiPhy::GetHeMcs1 ();
        break;
      case 2:
        mode = WifiPhy::GetHeMcs2 ();
        break;
      case 3:
        mode = WifiPhy::GetHeMcs3 ();
        break;
      case 4:
        mode = WifiPhy::GetHeMcs4 ();
        break;
      case 5:
        mode = WifiPhy::GetHeMcs5 ();
        break;
      case 6:
        mode = WifiPhy::GetHeMcs6 ();
        break;
      case 7:
        mode = WifiPhy::GetHeMcs7 ();
        break;
      case 8:
        mode = WifiPhy::GetHeMcs8 ();
        break;
      case 9:
        mode = WifiPhy::GetHeMcs9 ();
        break;
      case 10:
        mode = WifiPhy::GetHeMcs10 ();
        break;
      case 11:
        mode = WifiPhy::GetHeMcs11 ();
        break;
      default:
        NS_ABORT_MSG ("Invalid HE MCS");
        break;
    }
  return mode;
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
WifiPhy::SwitchMaybeToCcaBusy (void)
{
  NS_LOG_FUNCTION (this);
  //We are here because we have received the first bit of a packet and we are
  //not going to be able to synchronize on it
  //In this model, CCA becomes busy when the aggregation of all signals as
  //tracked by the InterferenceHelper class is higher than the CcaBusyThreshold

  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW);
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
  if (m_endPreambleDetectionEvent.IsRunning ())
    {
      m_endPreambleDetectionEvent.Cancel ();
    }
  if (m_endPhyRxEvent.IsRunning ())
    {
      m_endPhyRxEvent.Cancel ();
    }
  if (m_endRxEvent.IsRunning ())
    {
      m_endRxEvent.Cancel ();
    }
  NotifyRxDrop (m_currentEvent->GetPsdu (), reason);
  m_interference.NotifyRxEnd ();
  if (reason == OBSS_PD_CCA_RESET)
    {
      m_state->SwitchFromRxAbort ();
    }
  m_currentEvent = 0;
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
WifiPhy::GetTxPowerForTransmission (WifiTxVector txVector) const
{
  NS_LOG_FUNCTION (this << m_powerRestricted);
  if (!m_powerRestricted)
    {
      return GetPowerDbm (txVector.GetTxPowerLevel ());
    }
  else
    {
      if (txVector.GetNss () > 1)
        {
          return std::min (m_txPowerMaxMimo, GetPowerDbm (txVector.GetTxPowerLevel ()));
        }
      else
        {
          return std::min (m_txPowerMaxSiso, GetPowerDbm (txVector.GetTxPowerLevel ()));
        }
    }
}

void
WifiPhy::StartRx (Ptr<Event> event, double rxPowerW)
{
  NS_LOG_FUNCTION (this << *event << rxPowerW);

  NS_LOG_DEBUG ("sync to signal (power=" << rxPowerW << "W)");
  m_interference.NotifyRxStart (); //We need to notify it now so that it starts recording events

  if (!m_endPreambleDetectionEvent.IsRunning ())
    {
      Time startOfPreambleDuration = GetPreambleDetectionDuration ();
      Time remainingRxDuration = event->GetDuration () - startOfPreambleDuration;
      m_endPreambleDetectionEvent = Simulator::Schedule (startOfPreambleDuration, &WifiPhy::StartReceiveHeader, this, event);
    }
  else if ((m_frameCaptureModel != 0) && (rxPowerW > m_currentEvent->GetRxPowerW ()))
    {
      NS_LOG_DEBUG ("Received a stronger signal during preamble detection: drop current packet and switch to new packet");
      NotifyRxDrop (m_currentEvent->GetPsdu (), PREAMBLE_DETECTION_PACKET_SWITCH);
      m_interference.NotifyRxEnd ();
      m_endPreambleDetectionEvent.Cancel ();
      m_interference.NotifyRxStart ();
      Time startOfPreambleDuration = GetPreambleDetectionDuration ();
      Time remainingRxDuration = event->GetDuration () - startOfPreambleDuration;
      m_endPreambleDetectionEvent = Simulator::Schedule (startOfPreambleDuration, &WifiPhy::StartReceiveHeader, this, event);
    }
  else
    {
      NS_LOG_DEBUG ("Drop packet because RX is already decoding preamble");
      NotifyRxDrop (event->GetPsdu (), BUSY_DECODING_PREAMBLE);
      return;
    }
  m_currentEvent = event;
}

int64_t
WifiPhy::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_random->SetStream (stream);
  return 1;
}

} //namespace ns3

namespace {

/**
 * Constructor class
 */
static class Constructor
{
public:
  Constructor ()
  {
    ns3::WifiPhy::GetDsssRate1Mbps ();
    ns3::WifiPhy::GetDsssRate2Mbps ();
    ns3::WifiPhy::GetDsssRate5_5Mbps ();
    ns3::WifiPhy::GetDsssRate11Mbps ();
    ns3::WifiPhy::GetErpOfdmRate6Mbps ();
    ns3::WifiPhy::GetErpOfdmRate9Mbps ();
    ns3::WifiPhy::GetErpOfdmRate12Mbps ();
    ns3::WifiPhy::GetErpOfdmRate18Mbps ();
    ns3::WifiPhy::GetErpOfdmRate24Mbps ();
    ns3::WifiPhy::GetErpOfdmRate36Mbps ();
    ns3::WifiPhy::GetErpOfdmRate48Mbps ();
    ns3::WifiPhy::GetErpOfdmRate54Mbps ();
    ns3::WifiPhy::GetOfdmRate6Mbps ();
    ns3::WifiPhy::GetOfdmRate9Mbps ();
    ns3::WifiPhy::GetOfdmRate12Mbps ();
    ns3::WifiPhy::GetOfdmRate18Mbps ();
    ns3::WifiPhy::GetOfdmRate24Mbps ();
    ns3::WifiPhy::GetOfdmRate36Mbps ();
    ns3::WifiPhy::GetOfdmRate48Mbps ();
    ns3::WifiPhy::GetOfdmRate54Mbps ();
    ns3::WifiPhy::GetOfdmRate3MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate4_5MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate6MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate9MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate12MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate18MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate24MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate27MbpsBW10MHz ();
    ns3::WifiPhy::GetOfdmRate1_5MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate2_25MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate3MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate4_5MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate6MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate9MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate12MbpsBW5MHz ();
    ns3::WifiPhy::GetOfdmRate13_5MbpsBW5MHz ();
    ns3::WifiPhy::GetHtMcs0 ();
    ns3::WifiPhy::GetHtMcs1 ();
    ns3::WifiPhy::GetHtMcs2 ();
    ns3::WifiPhy::GetHtMcs3 ();
    ns3::WifiPhy::GetHtMcs4 ();
    ns3::WifiPhy::GetHtMcs5 ();
    ns3::WifiPhy::GetHtMcs6 ();
    ns3::WifiPhy::GetHtMcs7 ();
    ns3::WifiPhy::GetHtMcs8 ();
    ns3::WifiPhy::GetHtMcs9 ();
    ns3::WifiPhy::GetHtMcs10 ();
    ns3::WifiPhy::GetHtMcs11 ();
    ns3::WifiPhy::GetHtMcs12 ();
    ns3::WifiPhy::GetHtMcs13 ();
    ns3::WifiPhy::GetHtMcs14 ();
    ns3::WifiPhy::GetHtMcs15 ();
    ns3::WifiPhy::GetHtMcs16 ();
    ns3::WifiPhy::GetHtMcs17 ();
    ns3::WifiPhy::GetHtMcs18 ();
    ns3::WifiPhy::GetHtMcs19 ();
    ns3::WifiPhy::GetHtMcs20 ();
    ns3::WifiPhy::GetHtMcs21 ();
    ns3::WifiPhy::GetHtMcs22 ();
    ns3::WifiPhy::GetHtMcs23 ();
    ns3::WifiPhy::GetHtMcs24 ();
    ns3::WifiPhy::GetHtMcs25 ();
    ns3::WifiPhy::GetHtMcs26 ();
    ns3::WifiPhy::GetHtMcs27 ();
    ns3::WifiPhy::GetHtMcs28 ();
    ns3::WifiPhy::GetHtMcs29 ();
    ns3::WifiPhy::GetHtMcs30 ();
    ns3::WifiPhy::GetHtMcs31 ();
    ns3::WifiPhy::GetVhtMcs0 ();
    ns3::WifiPhy::GetVhtMcs1 ();
    ns3::WifiPhy::GetVhtMcs2 ();
    ns3::WifiPhy::GetVhtMcs3 ();
    ns3::WifiPhy::GetVhtMcs4 ();
    ns3::WifiPhy::GetVhtMcs5 ();
    ns3::WifiPhy::GetVhtMcs6 ();
    ns3::WifiPhy::GetVhtMcs7 ();
    ns3::WifiPhy::GetVhtMcs8 ();
    ns3::WifiPhy::GetVhtMcs9 ();
    ns3::WifiPhy::GetHeMcs0 ();
    ns3::WifiPhy::GetHeMcs1 ();
    ns3::WifiPhy::GetHeMcs2 ();
    ns3::WifiPhy::GetHeMcs3 ();
    ns3::WifiPhy::GetHeMcs4 ();
    ns3::WifiPhy::GetHeMcs5 ();
    ns3::WifiPhy::GetHeMcs6 ();
    ns3::WifiPhy::GetHeMcs7 ();
    ns3::WifiPhy::GetHeMcs8 ();
    ns3::WifiPhy::GetHeMcs9 ();
    ns3::WifiPhy::GetHeMcs10 ();
    ns3::WifiPhy::GetHeMcs11 ();
  }
} g_constructor; ///< the constructor

}
