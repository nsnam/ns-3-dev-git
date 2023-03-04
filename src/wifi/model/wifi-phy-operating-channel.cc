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
 * Authors: Stefano Avallone <stavallo@unina.it>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-phy-operating-channel.h"

#include "wifi-phy-common.h"

#include "ns3/assert.h"
#include "ns3/log.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiPhyOperatingChannel");

const std::set<FrequencyChannelInfo> WifiPhyOperatingChannel::m_frequencyChannels = {
    // 2.4 GHz channels
    //  802.11b uses width of 22, while OFDM modes use width of 20
    {std::make_tuple(1, 2412, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(1, 2412, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(2, 2417, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(2, 2417, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(3, 2422, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(3, 2422, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(4, 2427, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(4, 2427, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(5, 2432, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(5, 2432, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(6, 2437, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(6, 2437, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(7, 2442, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(7, 2442, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(8, 2447, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(8, 2447, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(9, 2452, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(9, 2452, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(10, 2457, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(10, 2457, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(11, 2462, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(11, 2462, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(12, 2467, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(12, 2467, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(13, 2472, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(13, 2472, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    // Only defined for 802.11b
    {std::make_tuple(14, 2484, 22, WIFI_PHY_DSSS_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    // 40 MHz channels
    {std::make_tuple(3, 2422, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(4, 2427, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(5, 2432, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(6, 2437, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(7, 2442, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(8, 2447, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(9, 2452, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(10, 2457, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},
    {std::make_tuple(11, 2462, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_2_4GHZ)},

    // Now the 5 GHz channels used for 802.11a/n/ac/ax/be
    // 20 MHz channels
    {std::make_tuple(36, 5180, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(40, 5200, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(44, 5220, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(48, 5240, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(52, 5260, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(56, 5280, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(60, 5300, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(64, 5320, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(100, 5500, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(104, 5520, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(108, 5540, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(112, 5560, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(116, 5580, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(120, 5600, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(124, 5620, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(128, 5640, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(132, 5660, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(136, 5680, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(140, 5700, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(144, 5720, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(149, 5745, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(153, 5765, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(157, 5785, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(161, 5805, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(165, 5825, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(169, 5845, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(173, 5865, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(177, 5885, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(181, 5905, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    // 40 MHz channels
    {std::make_tuple(38, 5190, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(46, 5230, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(54, 5270, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(62, 5310, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(102, 5510, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(110, 5550, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(118, 5590, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(126, 5630, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(134, 5670, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(142, 5710, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(151, 5755, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(159, 5795, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(167, 5835, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(175, 5875, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    // 80 MHz channels
    {std::make_tuple(42, 5210, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(58, 5290, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(106, 5530, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(122, 5610, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(138, 5690, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(155, 5775, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(171, 5855, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    // 160 MHz channels
    {std::make_tuple(50, 5250, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(114, 5570, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(163, 5815, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_5GHZ)},

    // 802.11p 10 MHz channels at the 5.855-5.925 band
    {std::make_tuple(172, 5860, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(174, 5870, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(176, 5880, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(178, 5890, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(180, 5900, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(182, 5910, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(184, 5920, 10, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},

    // 802.11p 5 MHz channels at the 5.855-5.925 band (for simplification, we consider the same
    // center frequencies as the 10 MHz channels)
    {std::make_tuple(171, 5860, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(173, 5870, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(175, 5880, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(177, 5890, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(179, 5900, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(181, 5910, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},
    {std::make_tuple(183, 5920, 5, WIFI_PHY_80211p_CHANNEL, WIFI_PHY_BAND_5GHZ)},

    // Now the 6 GHz channels for 802.11ax/be
    // 20 MHz channels
    {std::make_tuple(1, 5955, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(5, 5975, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(9, 5995, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(13, 6015, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(17, 6035, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(21, 6055, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(25, 6075, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(29, 6095, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(33, 6115, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(37, 6135, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(41, 6155, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(45, 6175, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(49, 6195, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(53, 6215, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(57, 6235, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(61, 6255, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(65, 6275, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(69, 6295, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(73, 6315, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(77, 6335, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(81, 6355, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(85, 6375, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(89, 6395, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(93, 6415, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(97, 6435, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(101, 6455, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(105, 6475, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(109, 6495, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(113, 6515, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(117, 6535, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(121, 6555, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(125, 6575, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(129, 6595, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(133, 6615, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(137, 6635, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(141, 6655, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(145, 6675, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(149, 6695, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(153, 6715, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(157, 6735, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(161, 6755, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(165, 6775, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(169, 6795, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(173, 6815, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(177, 6835, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(181, 6855, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(185, 6875, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(189, 6895, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(193, 6915, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(197, 6935, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(201, 6955, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(205, 6975, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(209, 6995, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(213, 7015, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(217, 7035, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(221, 7055, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(225, 7075, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(229, 7095, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(233, 7115, 20, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    // 40 MHz channels
    {std::make_tuple(3, 5965, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(11, 6005, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(19, 6045, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(27, 6085, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(35, 6125, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(43, 6165, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(51, 6205, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(59, 6245, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(67, 6285, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(75, 6325, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(83, 6365, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(91, 6405, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(99, 6445, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(107, 6485, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(115, 6525, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(123, 6565, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(131, 6605, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(139, 6645, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(147, 6685, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(155, 6725, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(163, 6765, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(171, 6805, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(179, 6845, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(187, 6885, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(195, 6925, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(203, 6965, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(211, 7005, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(219, 7045, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(227, 7085, 40, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    // 80 MHz channels
    {std::make_tuple(7, 5985, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(23, 6065, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(39, 6145, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(55, 6225, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(71, 6305, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(87, 6385, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(103, 6465, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(119, 6585, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(135, 6625, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(151, 6705, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(167, 6785, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(183, 6865, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(199, 6945, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(215, 7025, 80, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    // 160 MHz channels
    {std::make_tuple(15, 6025, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(47, 6185, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(79, 6345, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(111, 6505, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(143, 6665, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(175, 6825, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
    {std::make_tuple(207, 6985, 160, WIFI_PHY_OFDM_CHANNEL, WIFI_PHY_BAND_6GHZ)},
};

WifiPhyOperatingChannel::WifiPhyOperatingChannel()
    : WifiPhyOperatingChannel(m_frequencyChannels.end())
{
}

WifiPhyOperatingChannel::WifiPhyOperatingChannel(ConstIterator it)
    : m_channelIt(it),
      m_primary20Index(0)
{
    NS_LOG_FUNCTION(this);
}

WifiPhyOperatingChannel::~WifiPhyOperatingChannel()
{
    NS_LOG_FUNCTION_NOARGS();
}

bool
WifiPhyOperatingChannel::IsSet() const
{
    return m_channelIt != m_frequencyChannels.end();
}

void
WifiPhyOperatingChannel::Set(uint8_t number,
                             uint16_t frequency,
                             uint16_t width,
                             WifiStandard standard,
                             WifiPhyBand band)
{
    NS_LOG_FUNCTION(this << +number << frequency << width << standard << band);

    auto channelIt = FindFirst(number, frequency, width, standard, band);

    if (channelIt != m_frequencyChannels.end() &&
        FindFirst(number, frequency, width, standard, band, std::next(channelIt)) ==
            m_frequencyChannels.end())
    {
        // a unique channel matches the specified criteria
        m_channelIt = channelIt;
        m_primary20Index = 0;
        return;
    }

    // if a unique channel was not found, throw an exception (mainly for unit testing this code)
    throw std::runtime_error(
        "WifiPhyOperatingChannel: No unique channel found given the specified criteria");
}

void
WifiPhyOperatingChannel::SetDefault(uint16_t width, WifiStandard standard, WifiPhyBand band)
{
    NS_LOG_FUNCTION(this << width << standard << band);

    Set(GetDefaultChannelNumber(width, standard, band), 0, width, standard, band);
}

uint8_t
WifiPhyOperatingChannel::GetDefaultChannelNumber(uint16_t width,
                                                 WifiStandard standard,
                                                 WifiPhyBand band)
{
    auto channelIt = FindFirst(0, 0, width, standard, band);

    if (channelIt != m_frequencyChannels.end())
    {
        // a channel matches the specified criteria
        return std::get<0>(*channelIt);
    }

    // if a default channel was not found, throw an exception (mainly for unit testing this code)
    throw std::runtime_error("WifiPhyOperatingChannel: No default channel found of the given width "
                             "and for the given PHY standard and band");
}

WifiPhyOperatingChannel::ConstIterator
WifiPhyOperatingChannel::FindFirst(uint8_t number,
                                   uint16_t frequency,
                                   uint16_t width,
                                   WifiStandard standard,
                                   WifiPhyBand band,
                                   ConstIterator start)
{
    // lambda used to match channels against the specified criteria
    auto predicate = [&](const FrequencyChannelInfo& channel) {
        if (number != 0 && std::get<0>(channel) != number)
        {
            return false;
        }
        if (frequency != 0 && std::get<1>(channel) != frequency)
        {
            return false;
        }
        if (width != 0 && std::get<2>(channel) != width)
        {
            return false;
        }
        if (standard != WIFI_STANDARD_UNSPECIFIED &&
            std::get<3>(channel) != GetFrequencyChannelType(standard))
        {
            return false;
        }
        if (std::get<4>(channel) != band)
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
WifiPhyOperatingChannel::GetNumber() const
{
    NS_ASSERT(IsSet());
    return std::get<0>(*m_channelIt);
}

uint16_t
WifiPhyOperatingChannel::GetFrequency() const
{
    NS_ASSERT(IsSet());
    return std::get<1>(*m_channelIt);
}

uint16_t
WifiPhyOperatingChannel::GetWidth() const
{
    NS_ASSERT(IsSet());
    return std::get<2>(*m_channelIt);
}

WifiPhyBand
WifiPhyOperatingChannel::GetPhyBand() const
{
    NS_ASSERT(IsSet());
    return std::get<4>(*m_channelIt);
}

bool
WifiPhyOperatingChannel::IsOfdm() const
{
    NS_ASSERT(IsSet());
    return std::get<FrequencyChannelType>(*m_channelIt) == WIFI_PHY_OFDM_CHANNEL;
}

bool
WifiPhyOperatingChannel::IsDsss() const
{
    NS_ASSERT(IsSet());
    return std::get<FrequencyChannelType>(*m_channelIt) == WIFI_PHY_DSSS_CHANNEL;
}

bool
WifiPhyOperatingChannel::Is80211p() const
{
    NS_ASSERT(IsSet());
    return std::get<FrequencyChannelType>(*m_channelIt) == WIFI_PHY_80211p_CHANNEL;
}

uint8_t
WifiPhyOperatingChannel::GetPrimaryChannelIndex(uint16_t primaryChannelWidth) const
{
    NS_LOG_FUNCTION(this << primaryChannelWidth);

    if (primaryChannelWidth % 20 != 0)
    {
        NS_LOG_DEBUG("The operating channel width is not a multiple of 20 MHz; return 0");
        return 0;
    }

    NS_ASSERT(primaryChannelWidth <= GetWidth());

    // the index of primary40 is half the index of primary20; the index of
    // primary80 is half the index of primary40, ...
    uint16_t width = 20;
    uint8_t index = m_primary20Index;

    while (width < primaryChannelWidth)
    {
        index /= 2;
        width *= 2;
    }
    NS_LOG_LOGIC("Return primary index " << +index);
    return index;
}

uint8_t
WifiPhyOperatingChannel::GetSecondaryChannelIndex(uint16_t secondaryChannelWidth) const
{
    NS_LOG_FUNCTION(this << secondaryChannelWidth);
    const uint8_t primaryIndex = GetPrimaryChannelIndex(secondaryChannelWidth);
    const uint8_t secondaryIndex =
        (primaryIndex % 2 == 0) ? (primaryIndex + 1) : (primaryIndex - 1);
    NS_LOG_LOGIC("Return secondary index " << +secondaryIndex);
    return secondaryIndex;
}

void
WifiPhyOperatingChannel::SetPrimary20Index(uint8_t index)
{
    NS_LOG_FUNCTION(this << +index);

    NS_ABORT_MSG_IF(index > 0 && index >= GetWidth() / 20, "Primary20 index out of range");
    m_primary20Index = index;
}

uint16_t
WifiPhyOperatingChannel::GetPrimaryChannelCenterFrequency(uint16_t primaryChannelWidth) const
{
    uint16_t freq = GetFrequency() - GetWidth() / 2. +
                    (GetPrimaryChannelIndex(primaryChannelWidth) + 0.5) * primaryChannelWidth;

    NS_LOG_FUNCTION(this << primaryChannelWidth << freq);
    return freq;
}

uint16_t
WifiPhyOperatingChannel::GetSecondaryChannelCenterFrequency(uint16_t secondaryChannelWidth) const
{
    const uint8_t primaryIndex = GetPrimaryChannelIndex(secondaryChannelWidth);
    const uint16_t primaryCenterFrequency = GetPrimaryChannelCenterFrequency(secondaryChannelWidth);
    return (primaryIndex % 2 == 0) ? (primaryCenterFrequency + secondaryChannelWidth)
                                   : (primaryCenterFrequency - secondaryChannelWidth);
}

uint8_t
WifiPhyOperatingChannel::GetPrimaryChannelNumber(uint16_t primaryChannelWidth,
                                                 WifiStandard standard) const
{
    auto frequency = GetPrimaryChannelCenterFrequency(primaryChannelWidth);
    NS_ASSERT_MSG(IsSet(), "No channel set");
    auto& [chanNumber, centerFreq, channelWidth, channelType, band] = *m_channelIt;
    auto primaryChanIt = FindFirst(0, frequency, primaryChannelWidth, standard, band);
    NS_ASSERT_MSG(primaryChanIt != m_frequencyChannels.end(), "Primary channel number not found");
    return std::get<0>(*primaryChanIt);
}

std::set<uint8_t>
WifiPhyOperatingChannel::GetAll20MHzChannelIndicesInPrimary(uint16_t width) const
{
    if (width > GetWidth())
    {
        // a primary channel of the given width does not exist
        return {};
    }

    uint16_t currWidth = 20; // MHz
    std::set<uint8_t> indices;
    indices.insert(m_primary20Index);

    while (currWidth < width)
    {
        indices.merge(GetAll20MHzChannelIndicesInSecondary(indices));
        currWidth <<= 1;
    }

    return indices;
}

std::set<uint8_t>
WifiPhyOperatingChannel::GetAll20MHzChannelIndicesInSecondary(uint16_t width) const
{
    return GetAll20MHzChannelIndicesInSecondary(GetAll20MHzChannelIndicesInPrimary(width));
}

std::set<uint8_t>
WifiPhyOperatingChannel::GetAll20MHzChannelIndicesInSecondary(
    const std::set<uint8_t>& primaryIndices) const
{
    if (primaryIndices.empty() || GetWidth() == 20)
    {
        return {};
    }

    uint8_t size = 1;
    uint16_t primaryWidth = 20; // MHz

    // find the width of the primary channel corresponding to the size of the given set
    while (size != primaryIndices.size())
    {
        size <<= 1;
        primaryWidth <<= 1;

        if (primaryWidth >= GetWidth())
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
WifiPhyOperatingChannel::Get20MHzIndicesCoveringRu(HeRu::RuSpec ru, uint16_t width) const
{
    auto ruType = ru.GetRuType();

    NS_ASSERT_MSG(HeRu::GetBandwidth(ruType) <= width,
                  "No RU of type " << ruType << " is contained in a " << width << " MHz channel");
    NS_ASSERT_MSG(width <= GetWidth(),
                  "The given width (" << width << " MHz) exceeds the operational width ("
                                      << GetWidth() << " MHz)");

    // trivial case: 2x996-tone RU
    if (ruType == HeRu::RU_2x996_TONE)
    {
        return {0, 1, 2, 3, 4, 5, 6, 7};
    }

    // handle first the special case of center 26-tone RUs
    if (ruType == HeRu::RU_26_TONE && ru.GetIndex() == 19)
    {
        NS_ASSERT_MSG(width >= 80,
                      "26-tone RU with index 19 is only present in channels of at least 80 MHz");
        // the center 26-tone RU in an 80 MHz channel is not fully covered by
        // any 20 MHz channel, but by the two central 20 MHz channels in the 80 MHz channel
        auto indices = ru.GetPrimary80MHz() ? GetAll20MHzChannelIndicesInPrimary(80)
                                            : GetAll20MHzChannelIndicesInSecondary(80);
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
    if (width == 160)
    {
        bool primary80IsLower80 = (m_primary20Index < 4);
        if (primary80IsLower80 != ru.GetPrimary80MHz())
        {
            auto nRusIn80MHz = HeRu::GetNRus(80, ruType);
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

    auto nRusInCoveringChannel = HeRu::GetNRus(n20MHzChannels * 20, ruType);
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
    auto offset = GetPrimaryChannelIndex(width) * width / 20;

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

} // namespace ns3
