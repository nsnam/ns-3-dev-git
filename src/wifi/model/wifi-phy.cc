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
#include "ns3/ht-configuration.h"
#include "ns3/he-configuration.h"
#include "mpdu-aggregator.h"
#include "wifi-psdu.h"
#include "wifi-ppdu.h"
#include "ap-wifi-mac.h"
#include "ns3/dsss-phy.h"
#include "ns3/erp-ofdm-phy.h"
#include "ns3/he-phy.h" //includes OFDM, HT, and VHT

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiPhy");

/****************************************************************
 *       The actual WifiPhy class
 ****************************************************************/

NS_OBJECT_ENSURE_REGISTERED (WifiPhy);

const std::set<FrequencyChannelInfo> WifiPhy::m_frequencyChannels =
{
  //2.4 GHz channels
  // 802.11b uses width of 22, while OFDM modes use width of 20
  { std::make_tuple (1, 2412, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (1, 2412, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (2, 2417, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (2, 2417, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (3, 2422, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (3, 2422, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (4, 2427, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (4, 2427, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (5, 2432, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (5, 2432, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (6, 2437, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (6, 2437, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (7, 2442, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (7, 2442, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (8, 2447, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (8, 2447, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (9, 2452, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (9, 2452, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (10, 2457, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (10, 2457, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (11, 2462, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (11, 2462, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (12, 2467, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (12, 2467, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (13, 2472, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (13, 2472, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  // Only defined for 802.11b
  { std::make_tuple (14, 2484, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  // 40 MHz channels
  { std::make_tuple (3, 2422, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (4, 2427, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (5, 2432, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (6, 2437, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (7, 2442, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (8, 2447, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (9, 2452, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (10, 2457, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },
  { std::make_tuple (11, 2462, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ) },

  // Now the 5 GHz channels used for 802.11a/n/ac/ax
  // 20 MHz channels
  { std::make_tuple (36, 5180, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (40, 5200, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (44, 5220, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (48, 5240, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (52, 5260, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (56, 5280, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (60, 5300, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (64, 5320, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (100, 5500, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (104, 5520, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (108, 5540, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (112, 5560, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (116, 5580, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (120, 5600, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (124, 5620, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (128, 5640, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (132, 5660, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (136, 5680, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (140, 5700, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (144, 5720, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (149, 5745, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (153, 5765, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (157, 5785, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (161, 5805, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (165, 5825, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (169, 5845, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (173, 5865, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (177, 5885, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (181, 5905, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  // 40 MHz channels
  { std::make_tuple (38, 5190, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (46, 5230, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (54, 5270, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (62, 5310, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (102, 5510, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (110, 5550, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (118, 5590, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (126, 5630, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (134, 5670, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (142, 5710, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (151, 5755, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (159, 5795, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (167, 5835, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (175, 5875, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  // 80 MHz channels
  { std::make_tuple (42, 5210, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (58, 5290, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (106, 5530, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (122, 5610, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (138, 5690, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (155, 5775, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (171, 5855, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  // 160 MHz channels
  { std::make_tuple (50, 5250, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (114, 5570, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (163, 5815, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ) },

  // 802.11p 10 MHz channels at the 5.855-5.925 band
  { std::make_tuple (172, 5860, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (174, 5870, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (176, 5880, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (178, 5890, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (180, 5900, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (182, 5910, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (184, 5920, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },

  // 802.11p 5 MHz channels at the 5.855-5.925 band (for simplification, we consider the same center frequencies as the 10 MHz channels)
  { std::make_tuple (171, 5860, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (173, 5870, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (175, 5880, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (177, 5890, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (179, 5900, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (181, 5910, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },
  { std::make_tuple (183, 5920, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ) },

  // Now the 6 GHz channels (802.11ax only)
  // 20 MHz channels
  { std::make_tuple (1, 5945, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (5, 5965, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (9, 5985, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (13, 6005, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (17, 6025, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (21, 6045, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (25, 6065, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (29, 6085, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (33, 6105, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (37, 6125, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (41, 6145, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (45, 6165, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (49, 6185, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (53, 6205, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (57, 6225, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (61, 6245, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (65, 6265, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (69, 6285, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (73, 6305, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (77, 6325, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (81, 6345, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (85, 6365, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (89, 6385, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (93, 6405, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (97, 6425, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (101, 6445, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (105, 6465, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (109, 6485, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (113, 6505, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (117, 6525, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (121, 6545, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (125, 6565, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (129, 6585, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (133, 6605, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (137, 6625, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (141, 6645, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (145, 6665, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (149, 6685, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (153, 6705, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (157, 6725, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (161, 6745, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (165, 6765, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (169, 6785, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (173, 6805, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (177, 6825, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (181, 6845, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (185, 6865, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (189, 6885, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (193, 6905, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (197, 6925, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (201, 6945, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (205, 6965, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (209, 6985, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (213, 7005, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (217, 7025, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (221, 7045, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (225, 7065, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (229, 7085, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (233, 7105, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  // 40 MHz channels
  { std::make_tuple (3, 5955, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (11, 5995, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (19, 6035, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (27, 6075, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (35, 6115, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (43, 6155, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (51, 6195, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (59, 6235, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (67, 6275, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (75, 6315, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (83, 6355, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (91, 6395, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (99, 6435, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (107, 6475, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (115, 6515, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (123, 6555, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (131, 6595, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (139, 6635, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (147, 6675, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (155, 6715, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (163, 6755, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (171, 6795, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (179, 6835, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (187, 6875, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (195, 6915, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (203, 6955, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (211, 6995, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (219, 7035, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (227, 7075, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  // 80 MHz channels
  { std::make_tuple (7, 5975, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (23, 6055, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (39, 6135, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (55, 6215, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (71, 6295, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (87, 6375, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (103, 6455, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (119, 6535, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (135, 6615, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (151, 6695, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (167, 6775, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (183, 6855, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (199, 6935, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (215, 7015, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  // 160 MHz channels
  { std::make_tuple (15, 6015, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (47, 6175, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (79, 6335, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (111, 6495, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (143, 6655, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (175, 6815, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) },
  { std::make_tuple (207, 6975, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ) }
};

std::map<WifiModulationClass, Ptr<PhyEntity> > WifiPhy::m_staticPhyEntities; //will be filled by g_constructor_XXX

TypeId
WifiPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiPhy")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddAttribute ("Frequency",
                   "The center frequency (MHz) of the operating channel. "
                   "If the operating channel for this object has not been set yet, the "
                   "value of this attribute is saved and will be used, along with the channel "
                   "number and width configured via other attributes, to set the operating "
                   "channel when the standard and band are configured. The default value of "
                   "this attribute is 0, which means unspecified center frequency. Note that "
                   "if center frequency and channel number are both 0 when the standard and "
                   "band are configured, a default channel (of the configured width, if any, "
                   "or the default width for the current standard and band, otherwise) is set. "
                   "If the operating channel for this object has been already set, the "
                   "specified center frequency must uniquely identify a channel in the "
                   "band being used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&WifiPhy::GetFrequency,
                                         &WifiPhy::SetFrequency),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ChannelNumber",
                   "The channel number of the operating channel. "
                   "If the operating channel for this object has not been set yet, the "
                   "value of this attribute is saved and will be used, along with the center "
                   "frequency and width configured via other attributes, to set the operating "
                   "channel when the standard and band are configured. The default value of "
                   "this attribute is 0, which means unspecified channel number. Note that "
                   "if center frequency and channel number are both 0 when the standard and "
                   "band are configured, a default channel (of the configured width, if any, "
                   "or the default width for the current standard and band, otherwise) is set. "
                   "If the operating channel for this object has been already set, the "
                   "specified channel number must uniquely identify a channel in the "
                   "band being used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&WifiPhy::SetChannelNumber,
                                         &WifiPhy::GetChannelNumber),
                   MakeUintegerChecker<uint8_t> (0, 233))
    .AddAttribute ("ChannelWidth",
                   "The width in MHz of the operating channel (5, 10, 20, 22, 40, 80 or 160). "
                   "If the operating channel for this object has not been set yet, the "
                   "value of this attribute is saved and will be used, along with the center "
                   "frequency and channel number configured via other attributes, to set the "
                   "operating channel when the standard and band are configured. The default value "
                   "of this attribute is 0, which means unspecified channel width. Note that "
                   "if center frequency and channel number are both 0 when the standard and "
                   "band are configured, a default channel (of the configured width, if any, "
                   "or the default width for the current standard and band, otherwise) is set. "
                   "Do not set this attribute when the standard and band of this object have "
                   "been already configured, because it cannot uniquely identify a channel in "
                   "the band being used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&WifiPhy::GetChannelWidth,
                                         &WifiPhy::SetChannelWidth),
                   MakeUintegerChecker<uint16_t> (5, 160))
    .AddAttribute ("Primary20MHzIndex",
                   "The index of the primary 20 MHz channel within the operating channel "
                   "(0 indicates the 20 MHz subchannel with the lowest center frequency). "
                   "This attribute is only valid if the width of the operating channel is "
                   "a multiple of 20 MHz.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&WifiPhy::SetPrimary20Index),
                   MakeUintegerChecker<uint8_t> (0, 7))
    .AddAttribute ("RxSensitivity",
                   "The energy of a received signal should be higher than "
                   "this threshold (dBm) for the PHY to detect the signal. "
                   "This threshold refers to a width of 20 MHz and will be "
                   "scaled to match the width of the received signal.",
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
    m_standard (WIFI_PHY_STANDARD_UNSPECIFIED),
    m_band (WIFI_PHY_BAND_UNSPECIFIED),
    m_initialFrequency (0),
    m_initialChannelNumber (0),
    m_initialChannelWidth (0),
    m_initialPrimary20Index (0),
    m_sifs (Seconds (0)),
    m_slot (Seconds (0)),
    m_pifs (Seconds (0)),
    m_ackTxTime (Seconds (0)),
    m_blockAckTxTime (Seconds (0)),
    m_powerRestricted (false),
    m_channelAccessRequested (false),
    m_txSpatialStreams (0),
    m_rxSpatialStreams (0),
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
  for (auto & phyEntity : m_phyEntities)
    {
      phyEntity.second->CancelAllEvents ();
    }
  m_device = 0;
  m_mobility = 0;
  m_frameCaptureModel = 0;
  m_preambleDetectionModel = 0;
  m_wifiRadioEnergyModel = 0;
  m_postReceptionErrorModel = 0;
  m_supportedChannelWidthSet.clear ();
  m_random = 0;
  m_state = 0;
  m_currentEvent = 0;
  for (auto & preambleEvent : m_currentPreambleEvents)
    {
      preambleEvent.second = 0;
    }
  m_currentPreambleEvents.clear ();

  for (auto & phyEntity : m_phyEntities)
    {
      phyEntity.second = 0;
    }
  m_phyEntities.clear ();
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
WifiPhy::CalculateSnr (const WifiTxVector& txVector, double ber) const
{
  return m_interference.GetErrorRateModel ()->CalculateSnr (txVector, ber);
}

const Ptr<const PhyEntity>
WifiPhy::GetStaticPhyEntity (WifiModulationClass modulation)
{
  const auto it = m_staticPhyEntities.find (modulation);
  NS_ABORT_MSG_IF (it == m_staticPhyEntities.end (), "Unimplemented Wi-Fi modulation class");
  return it->second;
}

Ptr<PhyEntity>
WifiPhy::GetPhyEntity (WifiModulationClass modulation) const
{
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
  phyEntity->SetOwner (this);
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

void
WifiPhy::ConfigureStandardAndBand (WifiPhyStandard standard, WifiPhyBand band)
{
  NS_LOG_FUNCTION (this << standard << band);
  m_standard = standard;
  m_band = band;

  if (m_initialFrequency == 0 && m_initialChannelNumber == 0)
    {
      // set a default channel if the user did not specify anything
      if (m_initialChannelWidth == 0)
        {
          // set a default channel width
          m_initialChannelWidth = GetDefaultChannelWidth (m_standard, m_band);
        }

      m_operatingChannel.SetDefault (m_initialChannelWidth, m_standard, m_band);
    }
  else
    {
      m_operatingChannel.Set (m_initialChannelNumber, m_initialFrequency, m_initialChannelWidth,
                              m_standard, m_band);
    }
  m_operatingChannel.SetPrimary20Index (m_initialPrimary20Index);

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

const WifiPhyOperatingChannel&
WifiPhy::GetOperatingChannel (void) const
{
  return m_operatingChannel;
}

void
WifiPhy::SetFrequency (uint16_t frequency)
{
  NS_LOG_FUNCTION (this << frequency);

  if (!m_operatingChannel.IsSet ())
    {
      // ConfigureStandardAndBand has not been called yet, so store the frequency
      // into m_initialFrequency
      NS_LOG_DEBUG ("Saving frequency configuration for initialization");
      m_initialFrequency = frequency;
      return;
    }

  if (GetFrequency () == frequency)
    {
      NS_LOG_DEBUG ("No frequency change requested");
      return;
    }

  // if the frequency does not uniquely identify an operating channel,
  // the simulation aborts
  SetOperatingChannel (0, frequency, 0);
}

uint16_t
WifiPhy::GetFrequency (void) const
{
  return m_operatingChannel.GetFrequency ();
}

void
WifiPhy::SetChannelNumber (uint8_t nch)
{
  NS_LOG_FUNCTION (this << +nch);

  if (!m_operatingChannel.IsSet ())
    {
      // ConfigureStandardAndBand has not been called yet, so store the channel
      // into m_initialChannelNumber
      NS_LOG_DEBUG ("Saving channel number configuration for initialization");
      m_initialChannelNumber = nch;
      return;
    }

  if (GetChannelNumber () == nch)
    {
      NS_LOG_DEBUG ("No channel change requested");
      return;
    }

  // if the channel number does not uniquely identify an operating channel,
  // the simulation aborts
  SetOperatingChannel (nch, 0, 0);
}

uint8_t
WifiPhy::GetChannelNumber (void) const
{
  return m_operatingChannel.GetNumber ();
}

void
WifiPhy::SetChannelWidth (uint16_t channelWidth)
{
  NS_LOG_FUNCTION (this << channelWidth);

  if (channelWidth != 0)
    {
      AddSupportedChannelWidth (channelWidth);
    }

  if (!m_operatingChannel.IsSet ())
    {
      // ConfigureStandardAndBand has not been called yet, so store the channel width
      // into m_initialChannelWidth
      NS_LOG_DEBUG ("Saving channel width configuration for initialization");
      m_initialChannelWidth = channelWidth;
      return;
    }

  if (GetChannelWidth () == channelWidth)
    {
      NS_LOG_DEBUG ("No channel width change requested");
      return;
    }

  NS_ABORT_MSG ("The channel width does not uniquely identify an operating channel.");
}

uint16_t
WifiPhy::GetChannelWidth (void) const
{
  return m_operatingChannel.GetWidth ();
}

void
WifiPhy::SetPrimary20Index (uint8_t index)
{
  NS_LOG_FUNCTION (this << +index);

  if (!m_operatingChannel.IsSet ())
    {
      // ConfigureStandardAndBand has not been called yet, so store the primary20
      // index into m_initialPrimary20Index
      NS_LOG_DEBUG ("Saving primary20 index configuration for initialization");
      m_initialPrimary20Index = index;
      return;
    }

  m_operatingChannel.SetPrimary20Index (index);
}

void
WifiPhy::SetOperatingChannel (uint8_t number, uint16_t frequency, uint16_t width)
{
  Time delay = Seconds (0);

  if (IsInitialized ())
    {
      delay = DoChannelSwitch ();
    }

  if (delay.IsStrictlyNegative ())
    {
      // switching channel is not possible now
      return;
    }
  if (delay.IsStrictlyPositive ())
    {
      // switching channel has been postponed
      Simulator::Schedule (delay, &WifiPhy::SetOperatingChannel, this, number, frequency, width);
      return;
    }

  // channel can be switched now.
  uint16_t prevChannelWidth = 0;
  if (m_operatingChannel.IsSet ())
    {
      prevChannelWidth = GetChannelWidth ();
    }

  m_operatingChannel.Set (number, frequency, width, m_standard, m_band);

  if (GetChannelWidth () != prevChannelWidth)
    {
      AddSupportedChannelWidth (GetChannelWidth ());

      // If channel width changed after initialization, invoke the capabilities changed callback
      if (IsInitialized () && !m_capabilitiesChangedCallback.IsNull ())
        {
          m_capabilitiesChangedCallback ();
        }
    }
}

Time
WifiPhy::DoChannelSwitch (void)
{
  m_powerRestricted = false;
  m_channelAccessRequested = false;
  m_currentEvent = 0;
  m_currentPreambleEvents.clear ();
  if (!IsInitialized ())
    {
      //this is not channel switch, this is initialization
      NS_LOG_DEBUG ("Before initialization, nothing to do");
      return Seconds (0);
    }

  Time delay = Seconds (0);

  NS_ASSERT (!IsStateSwitching ());
  switch (m_state->GetState ())
    {
    case WifiPhyState::RX:
      NS_LOG_DEBUG ("drop packet because of channel switching while reception");
      m_endPhyRxEvent.Cancel ();
      for (auto & phyEntity : m_phyEntities)
        {
          phyEntity.second->CancelAllEvents ();
        }
      break;
    case WifiPhyState::TX:
      NS_LOG_DEBUG ("channel switching postponed until end of current transmission");
      delay = GetDelayUntilIdle ();
      break;
    case WifiPhyState::CCA_BUSY:
    case WifiPhyState::IDLE:
      for (auto & phyEntity : m_phyEntities)
        {
          phyEntity.second->CancelAllEvents ();
        }
      break;
    case WifiPhyState::SLEEP:
      NS_LOG_DEBUG ("channel switching ignored in sleep mode");
      delay = Seconds (-1);  // negative value to indicate switching not possible
      break;
    default:
      NS_ASSERT (false);
      break;
    }

  if (delay.IsZero ())
    {
      // channel switch can be done now
      NS_LOG_DEBUG ("switching channel");
      m_state->SwitchToChannelSwitching (GetChannelSwitchDelay ());
      m_interference.EraseEvents ();
      /*
      * Needed here to be able to correctly sensed the medium for the first
      * time after the switching. The actual switching is not performed until
      * after m_channelSwitchDelay. Packets received during the switching
      * state are added to the event list and are employed later to figure
      * out the state of the medium after the switching.
      */
    }

  return delay;
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
  for (auto & phyEntity : m_phyEntities)
    {
      phyEntity.second->CancelAllEvents ();
    }
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
        Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW, GetPrimaryBand (GetMeasurementChannelWidth (nullptr)));
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
        Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW, GetPrimaryBand (GetMeasurementChannelWidth (nullptr)));
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
WifiPhy::GetStartOfPacketDuration (const WifiTxVector& txVector)
{
  return MicroSeconds (4);
}

Time
WifiPhy::GetPayloadDuration (uint32_t size, const WifiTxVector& txVector, WifiPhyBand band, MpduType mpdutype, uint16_t staId)
{
  uint32_t totalAmpduSize;
  double totalAmpduNumSymbols;
  return GetPayloadDuration (size, txVector, band, mpdutype, false, totalAmpduSize, totalAmpduNumSymbols, staId);
}

Time
WifiPhy::GetPayloadDuration (uint32_t size, const WifiTxVector& txVector, WifiPhyBand band, MpduType mpdutype,
                             bool incFlag, uint32_t &totalAmpduSize, double &totalAmpduNumSymbols,
                             uint16_t staId)
{
  return GetStaticPhyEntity (txVector.GetModulationClass ())->GetPayloadDuration (size, txVector, band, mpdutype,
                                                                                  incFlag, totalAmpduSize, totalAmpduNumSymbols,
                                                                                  staId);
}

Time
WifiPhy::CalculatePhyPreambleAndHeaderDuration (const WifiTxVector& txVector)
{
  return GetStaticPhyEntity (txVector.GetModulationClass ())->CalculatePhyPreambleAndHeaderDuration (txVector);
}

Time
WifiPhy::CalculateTxDuration (uint32_t size, const WifiTxVector& txVector, WifiPhyBand band, uint16_t staId)
{
  Time duration = CalculatePhyPreambleAndHeaderDuration (txVector)
    + GetPayloadDuration (size, txVector, band, NORMAL_MPDU, staId);
  NS_ASSERT (duration.IsStrictlyPositive ());
  return duration;
}

Time
WifiPhy::CalculateTxDuration (Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, WifiPhyBand band)
{
  return CalculateTxDuration (GetWifiConstPsduMap (psdu, txVector), txVector, band);
}

Time
WifiPhy::CalculateTxDuration (WifiConstPsduMap psduMap, const WifiTxVector& txVector, WifiPhyBand band)
{
  return GetStaticPhyEntity (txVector.GetModulationClass ())->CalculateTxDuration (psduMap, txVector, band);
}

uint32_t
WifiPhy::GetMaxPsduSize (WifiModulationClass modulation)
{
  return GetStaticPhyEntity (modulation)->GetMaxPsduSize ();
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
WifiPhy::NotifyRxBegin (Ptr<const WifiPsdu> psdu, const RxPowerWattPerChannelBand& rxPowersW)
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

WifiConstPsduMap
WifiPhy::GetWifiConstPsduMap (Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
  return GetStaticPhyEntity (txVector.GetModulationClass ())->GetWifiConstPsduMap (psdu, txVector);
}

void
WifiPhy::Send (Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
  NS_LOG_FUNCTION (this << *psdu << txVector);
  Send (GetWifiConstPsduMap (psdu, txVector), txVector);
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
  
  // Set RU PHY indices
  if (txVector.IsMu ())
    {
      for (auto& heMuUserInfo : txVector.GetHeMuUserInfoMap ())
        {
          heMuUserInfo.second.ru.SetPhyIndex (txVector.GetChannelWidth (),
                                              m_operatingChannel.GetPrimaryChannelIndex (20));
        }
    }

  Time txDuration = CalculateTxDuration (psdus, txVector, GetPhyBand ());

  bool noEndPreambleDetectionEvent = true;
  for (const auto & it : m_phyEntities)
    {
      noEndPreambleDetectionEvent &= it.second->NoEndPreambleDetectionEvents ();
    }
  if (!noEndPreambleDetectionEvent || ((m_currentEvent != 0) && (m_currentEvent->GetEndTime () > (Simulator::Now () + m_state->GetDelayUntilIdle ()))))
    {
      AbortCurrentReception (RECEPTION_ABORTED_BY_TX);
      //that packet will be noise _after_ the transmission.
      MaybeCcaBusyDuration (GetMeasurementChannelWidth (m_currentEvent != 0 ? m_currentEvent->GetPpdu () : nullptr));
    }

  for (auto & it : m_phyEntities)
    {
      it.second->CancelRunningEndPreambleDetectionEvents ();
    }
  m_currentPreambleEvents.clear ();
  m_endPhyRxEvent.Cancel ();

  if (m_powerRestricted)
    {
      NS_LOG_DEBUG ("Transmitting with power restriction for " << txDuration.As (Time::NS));
    }
  else
    {
      NS_LOG_DEBUG ("Transmitting without power restriction for " << txDuration.As (Time::NS));
    }

  if (m_state->GetState () == WifiPhyState::OFF)
    {
      NS_LOG_DEBUG ("Transmission canceled because device is OFF");
      return;
    }

  Ptr<WifiPpdu> ppdu = GetPhyEntity (txVector.GetModulationClass ())->BuildPpdu (psdus, txVector, txDuration);
  m_previouslyRxPpduUid = UINT64_MAX; //reset (after creation of PPDU) to use it only once

  double txPowerW = DbmToW (GetTxPowerForTransmission (ppdu) + GetTxGain ());
  NotifyTxBegin (psdus, txPowerW);
  m_phyTxPsduBeginTrace (psdus, txVector, txPowerW);
  for (auto const& psdu : psdus)
    {
      NotifyMonitorSniffTx (psdu.second, GetFrequency (), txVector, psdu.first);
    }
  m_state->SwitchToTx (txDuration, psdus, GetPowerDbm (txVector.GetTxPowerLevel ()), txVector);

  if (m_wifiRadioEnergyModel != 0 && m_wifiRadioEnergyModel->GetMaximumTimeInState (WifiPhyState::TX) < txDuration)
    {
      ppdu->SetTruncatedTx ();
    }

  m_endTxEvent = Simulator::Schedule (txDuration, &WifiPhy::NotifyTxEnd, this, psdus); //TODO: fix for MU

  StartTx (ppdu);

  m_channelAccessRequested = false;
  m_powerRestricted = false;

  Simulator::Schedule (txDuration, &WifiPhy::Reset, this);
}

uint64_t
WifiPhy::GetPreviouslyRxPpduUid (void) const
{
  return m_previouslyRxPpduUid;
}

void
WifiPhy::Reset (void)
{
  NS_LOG_FUNCTION (this);
  m_currentPreambleEvents.clear ();
  m_currentEvent = 0;
  for (auto & phyEntity : m_phyEntities)
    {
      phyEntity.second->CancelAllEvents ();
    }
}

void
WifiPhy::StartReceivePreamble (Ptr<WifiPpdu> ppdu, RxPowerWattPerChannelBand& rxPowersW, Time rxDuration)
{
  WifiModulationClass modulation = ppdu->GetTxVector ().GetModulationClass ();
  auto it = m_phyEntities.find (modulation);
  if (it != m_phyEntities.end ())
    {
      it->second->StartReceivePreamble (ppdu, rxPowersW, rxDuration);
    }
  else
    {
      //TODO find a fallback PHY for receiving the PPDU (e.g. 11a for 11ax due to preamble structure)
      NS_LOG_DEBUG ("Unsupported modulation received (" << modulation << "), consider as noise");
      if (ppdu->GetTxDuration () > m_state->GetDelayUntilIdle ())
        {
          MaybeCcaBusyDuration (GetMeasurementChannelWidth (nullptr));
        }
    }
}

void
WifiPhy::MaybeCcaBusyDuration (uint16_t channelWidth)
{
  //We are here because we have received the first bit of a packet and we are
  //not going to be able to synchronize on it
  //In this model, CCA becomes busy when the aggregation of all signals as
  //tracked by the InterferenceHelper class is higher than the CcaBusyThreshold
  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW, GetPrimaryBand (channelWidth));
  if (!delayUntilCcaEnd.IsZero ())
    {
      m_state->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
    }
}

WifiSpectrumBand
WifiPhy::ConvertHeRuSubcarriers (uint16_t bandWidth, uint16_t guardBandwidth,
                                 HeRu::SubcarrierRange range, uint8_t bandIndex) const
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
  NS_ASSERT (!IsStateRx ());
  m_interference.NotifyRxEnd (Simulator::Now ());
  m_currentEvent = 0;
  m_currentPreambleEvents.clear ();
  MaybeCcaBusyDuration (GetMeasurementChannelWidth (event->GetPpdu ()));
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
  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaEdThresholdW, GetPrimaryBand (channelWidth));
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
  if (reason != OBSS_PD_CCA_RESET || m_currentEvent) //Otherwise abort has already been called previously
    {
      for (auto & phyEntity : m_phyEntities)
        {
          phyEntity.second->CancelAllEvents ();
        }
      if (m_endPhyRxEvent.IsRunning ())
        {
          m_endPhyRxEvent.Cancel ();
        }
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
}

void
WifiPhy::ResetCca (bool powerRestricted, double txPowerMaxSiso, double txPowerMaxMimo)
{
  NS_LOG_FUNCTION (this << powerRestricted << txPowerMaxSiso << txPowerMaxMimo);
  // This method might be called multiple times when receiving TB PPDUs with a BSS color
  // different than the one of the receiver. The first time this method is called, the call
  // to AbortCurrentReception sets m_currentEvent to 0. Therefore, we need to check whether
  // m_currentEvent is not 0 before executing the instructions below.
  if (m_currentEvent != 0)
    {
      m_powerRestricted = powerRestricted;
      m_txPowerMaxSiso = txPowerMaxSiso;
      m_txPowerMaxMimo = txPowerMaxMimo;
      NS_ASSERT ((m_currentEvent->GetEndTime () - Simulator::Now ()).IsPositive ());
      Simulator::Schedule (m_currentEvent->GetEndTime () - Simulator::Now (), &WifiPhy::EndReceiveInterBss, this);
      Simulator::ScheduleNow (&WifiPhy::AbortCurrentReception, this, OBSS_PD_CCA_RESET); //finish processing field first
    }
}

double
WifiPhy::GetTxPowerForTransmission (Ptr<const WifiPpdu> ppdu) const
{
  NS_LOG_FUNCTION (this << m_powerRestricted << ppdu);
  const WifiTxVector& txVector = ppdu->GetTxVector ();
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
  uint16_t channelWidth = ppdu->GetTransmissionChannelWidth ();
  double txPowerDbmPerMhz = (txPowerDbm + GetTxGain ()) - RatioToDb (channelWidth); //account for antenna gain since EIRP
  NS_LOG_INFO ("txPowerDbm=" << txPowerDbm << " with txPowerDbmPerMhz=" << txPowerDbmPerMhz << " over " << channelWidth << " MHz");
  txPowerDbm = std::min (txPowerDbmPerMhz, m_powerDensityLimit) + RatioToDb (channelWidth);
  txPowerDbm -= GetTxGain (); //remove antenna gain since will be added right afterwards
  NS_LOG_INFO ("txPowerDbm=" << txPowerDbm << " after applying m_powerDensityLimit=" << m_powerDensityLimit);
  return txPowerDbm;
}

Ptr<const WifiPsdu>
WifiPhy::GetAddressedPsduInPpdu (Ptr<const WifiPpdu> ppdu) const
{
  //TODO: wrapper. See if still needed
  return GetPhyEntity (ppdu->GetModulation ())->GetAddressedPsduInPpdu (ppdu);
}

uint16_t
WifiPhy::GetMeasurementChannelWidth (const Ptr<const WifiPpdu> ppdu) const
{
  if (ppdu == nullptr)
    {
      // Here because PHY was not receiving anything (e.g. resuming from OFF) nor expecting anything (e.g. sleep)
      // nor processing a Wi-Fi signal.
      return GetChannelWidth () >= 40 ? 20 : GetChannelWidth ();
    }
  return GetPhyEntity (ppdu->GetModulation ())->GetMeasurementChannelWidth (ppdu);
}

WifiSpectrumBand
WifiPhy::GetBand (uint16_t /*bandWidth*/, uint8_t /*bandIndex*/)
{
  WifiSpectrumBand band;
  band.first = 0;
  band.second = 0;
  return band;
}

WifiSpectrumBand
WifiPhy::GetPrimaryBand (uint16_t bandWidth)
{
  if (GetChannelWidth () % 20 != 0)
    {
      return GetBand (bandWidth);
    }

  return GetBand (bandWidth, m_operatingChannel.GetPrimaryChannelIndex (bandWidth));
}

int64_t
WifiPhy::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  int64_t currentStream = stream;
  m_random->SetStream (currentStream++);
  currentStream += m_interference.GetErrorRateModel ()->AssignStreams (currentStream);
  return (currentStream - stream);
}

std::ostream& operator<< (std::ostream& os, RxSignalInfo rxSignalInfo)
{
  os << "SNR:" << RatioToDb (rxSignalInfo.snr) << " dB"
     << ", RSSI:" << rxSignalInfo.rssi << " dBm";
  return os;
}

} //namespace ns3
