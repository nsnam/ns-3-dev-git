/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "eht-ru.h"

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/wifi-ru.h"

#include <algorithm>
#include <optional>
#include <set>
#include <tuple>

namespace ns3
{

const SubcarrierGroups EhtRu::m_ruSubcarrierGroups{
    // TODO: cleanup code duplication with HeRu
    // RUs in a 20 MHz HE PPDU (Table 27-7 IEEE802.11ax-2021)
    {{MHz_u{20}, RuType::RU_26_TONE},
     {/* 1 */ {{-121, -96}},
      /* 2 */ {{-95, -70}},
      /* 3 */ {{-68, -43}},
      /* 4 */ {{-42, -17}},
      /* 5 */ {{-16, -4}, {4, 16}},
      /* 6 */ {{17, 42}},
      /* 7 */ {{43, 68}},
      /* 8 */ {{70, 95}},
      /* 9 */ {{96, 121}}}},
    {{MHz_u{20}, RuType::RU_52_TONE},
     {/* 1 */ {{-121, -70}},
      /* 2 */ {{-68, -17}},
      /* 3 */ {{17, 68}},
      /* 4 */ {{70, 121}}}},
    {{MHz_u{20}, RuType::RU_106_TONE},
     {/* 1 */ {{-122, -17}},
      /* 2 */ {{17, 122}}}},
    {{MHz_u{20}, RuType::RU_242_TONE}, {/* 1 */ {{-122, -2}, {2, 122}}}},
    // RUs in a 40 MHz HE PPDU (Table 27-8 IEEE802.11ax-2021)
    {{MHz_u{40}, RuType::RU_26_TONE},
     {/* 1 */ {{-243, -218}},
      /* 2 */ {{-217, -192}},
      /* 3 */ {{-189, -164}},
      /* 4 */ {{-163, -138}},
      /* 5 */ {{-136, -111}},
      /* 6 */ {{-109, -84}},
      /* 7 */ {{-83, -58}},
      /* 8 */ {{-55, -30}},
      /* 9 */ {{-29, -4}},
      /* 10 */ {{4, 29}},
      /* 11 */ {{30, 55}},
      /* 12 */ {{58, 83}},
      /* 13 */ {{84, 109}},
      /* 14 */ {{111, 136}},
      /* 15 */ {{138, 163}},
      /* 16 */ {{164, 189}},
      /* 17 */ {{192, 217}},
      /* 18 */ {{218, 243}}}},
    {{MHz_u{40}, RuType::RU_52_TONE},
     {/* 1 */ {{-243, -192}},
      /* 2 */ {{-189, -138}},
      /* 3 */ {{-109, -58}},
      /* 4 */ {{-55, -4}},
      /* 5 */ {{4, 55}},
      /* 6 */ {{58, 109}},
      /* 7 */ {{138, 189}},
      /* 8 */ {{192, 243}}}},
    {{MHz_u{40}, RuType::RU_106_TONE},
     {/* 1 */ {{-243, -138}},
      /* 2 */ {{-109, -4}},
      /* 3 */ {{4, 109}},
      /* 4 */ {{138, 243}}}},
    {{MHz_u{40}, RuType::RU_242_TONE},
     {/* 1 */ {{-244, -3}},
      /* 2 */ {{3, 244}}}},
    {{MHz_u{40}, RuType::RU_484_TONE}, {/* 1 */ {{-244, -3}, {3, 244}}}},
    // RUs in an 80 MHz EHT PPDU (Table 36-5 IEEE802.11be-D7.0)
    {{MHz_u{80}, RuType::RU_26_TONE},
     {/* 1 */ {{-499, -474}},
      /* 2 */ {{-473, -448}},
      /* 3 */ {{-445, -420}},
      /* 4 */ {{-419, -394}},
      /* 5 */ {{-392, -367}},
      /* 6 */ {{-365, -340}},
      /* 7 */ {{-339, -314}},
      /* 8 */ {{-311, -286}},
      /* 9 */ {{-285, -260}},
      /* 10 */ {{-252, -227}},
      /* 11 */ {{-226, -201}},
      /* 12 */ {{-198, -173}},
      /* 13 */ {{-172, -147}},
      /* 14 */ {{-145, -120}},
      /* 15 */ {{-118, -93}},
      /* 16 */ {{-92, -67}},
      /* 17 */ {{-64, -39}},
      /* 18 */ {{-38, -13}},
      /* 19 not defined */ {},
      /* 20 */ {{13, 38}},
      /* 21 */ {{39, 64}},
      /* 22 */ {{67, 92}},
      /* 23 */ {{93, 118}},
      /* 24 */ {{120, 145}},
      /* 25 */ {{147, 172}},
      /* 26 */ {{173, 198}},
      /* 27 */ {{201, 226}},
      /* 28 */ {{227, 252}},
      /* 29 */ {{260, 285}},
      /* 30 */ {{286, 311}},
      /* 31 */ {{314, 339}},
      /* 32 */ {{340, 365}},
      /* 33 */ {{367, 392}},
      /* 34 */ {{394, 419}},
      /* 35 */ {{420, 445}},
      /* 36 */ {{448, 473}},
      /* 37 */ {{474, 499}}}},
    {{MHz_u{80}, RuType::RU_52_TONE},
     {/* 1 */ {{-499, -448}},
      /* 2 */ {{-445, -394}},
      /* 3 */ {{-365, -314}},
      /* 4 */ {{-311, -260}},
      /* 5 */ {{-252, -201}},
      /* 6 */ {{-198, -147}},
      /* 7 */ {{-118, -67}},
      /* 8 */ {{-64, -13}},
      /* 9 */ {{13, 64}},
      /* 10 */ {{67, 118}},
      /* 11 */ {{147, 198}},
      /* 12 */ {{201, 252}},
      /* 13 */ {{260, 311}},
      /* 14 */ {{314, 365}},
      /* 15 */ {{394, 445}},
      /* 16 */ {{448, 499}}}},
    {{MHz_u{80}, RuType::RU_106_TONE},
     {/* 1 */ {{-499, -394}},
      /* 2 */ {{-365, -260}},
      /* 3 */ {{-252, -147}},
      /* 4 */ {{-118, -13}},
      /* 5 */ {{13, 118}},
      /* 6 */ {{147, 252}},
      /* 7 */ {{260, 365}},
      /* 8 */ {{394, 499}}}},
    {{MHz_u{80}, RuType::RU_242_TONE},
     {/* 1 */ {{-500, -259}},
      /* 2 */ {{-253, -12}},
      /* 3 */ {{12, 253}},
      /* 4 */ {{259, 500}}}},
    {{MHz_u{80}, RuType::RU_484_TONE},
     {/* 1 */ {{-500, -259}, {-253, -12}},
      /* 2 */ {{12, 253}, {259, 500}}}},
    {{MHz_u{80}, RuType::RU_996_TONE}, {/* 1 */ {{-500, -3}, {3, 500}}}},
    // RUs in an 160 MHz EHT PPDU (Table 36-6 IEEE802.11be-D7.0)
    {{MHz_u{160}, RuType::RU_26_TONE},
     {/* 1 */ {{-1011, -986}},
      /* 2 */ {{-985, -960}},
      /* 3 */ {{-957, -932}},
      /* 4 */ {{-931, -906}},
      /* 5 */ {{-904, -879}},
      /* 6 */ {{-877, -852}},
      /* 7 */ {{-851, -826}},
      /* 8 */ {{-823, -798}},
      /* 9 */ {{-797, -772}},
      /* 10 */ {{-764, -739}},
      /* 11 */ {{-738, -713}},
      /* 12 */ {{-710, -685}},
      /* 13 */ {{-684, -659}},
      /* 14 */ {{-657, -632}},
      /* 15 */ {{-630, -605}},
      /* 16 */ {{-604, -579}},
      /* 17 */ {{-576, -551}},
      /* 18 */ {{-550, -525}},
      /* 19 not defined */ {},
      /* 20 */ {{-499, -474}},
      /* 21 */ {{-473, -448}},
      /* 22 */ {{-445, -420}},
      /* 23 */ {{-419, -394}},
      /* 24 */ {{-392, -367}},
      /* 25 */ {{-365, -340}},
      /* 26 */ {{-339, -314}},
      /* 27 */ {{-311, -286}},
      /* 28 */ {{-285, -260}},
      /* 29 */ {{-252, -227}},
      /* 30 */ {{-226, -201}},
      /* 31 */ {{-198, -173}},
      /* 32 */ {{-172, -147}},
      /* 33 */ {{-145, -120}},
      /* 34 */ {{-118, -93}},
      /* 35 */ {{-92, -67}},
      /* 36 */ {{-64, -39}},
      /* 37 */ {{-38, -13}},
      /* 38 */ {{13, 38}},
      /* 39 */ {{39, 64}},
      /* 40 */ {{67, 92}},
      /* 41 */ {{93, 118}},
      /* 42 */ {{120, 145}},
      /* 43 */ {{147, 172}},
      /* 44 */ {{173, 198}},
      /* 45 */ {{201, 226}},
      /* 46 */ {{227, 252}},
      /* 47 */ {{260, 285}},
      /* 48 */ {{286, 311}},
      /* 49 */ {{314, 339}},
      /* 50 */ {{340, 365}},
      /* 51 */ {{367, 392}},
      /* 52 */ {{394, 419}},
      /* 53 */ {{420, 445}},
      /* 54 */ {{448, 473}},
      /* 55 */ {{474, 499}},
      /* 56 not defined */ {},
      /* 57 */ {{525, 550}},
      /* 58 */ {{551, 576}},
      /* 59 */ {{579, 604}},
      /* 60 */ {{605, 630}},
      /* 61 */ {{632, 657}},
      /* 62 */ {{659, 684}},
      /* 63 */ {{685, 710}},
      /* 64 */ {{713, 738}},
      /* 65 */ {{739, 764}},
      /* 66 */ {{772, 797}},
      /* 67 */ {{798, 823}},
      /* 68 */ {{826, 851}},
      /* 69 */ {{852, 877}},
      /* 70 */ {{879, 904}},
      /* 71 */ {{906, 931}},
      /* 72 */ {{932, 957}},
      /* 73 */ {{960, 985}},
      /* 74 */ {{986, 1011}}}},
    {{MHz_u{160}, RuType::RU_52_TONE},
     {/* 1 */ {{-1011, -960}},
      /* 2 */ {{-957, -906}},
      /* 3 */ {{-877, -826}},
      /* 4 */ {{-823, -772}},
      /* 5 */ {{-764, -713}},
      /* 6 */ {{-710, -659}},
      /* 7 */ {{-630, -579}},
      /* 8 */ {{-576, -525}},
      /* 9 */ {{-499, -448}},
      /* 10 */ {{-445, -394}},
      /* 11 */ {{-365, -314}},
      /* 12 */ {{-311, -260}},
      /* 13 */ {{-252, -201}},
      /* 14 */ {{-198, -147}},
      /* 15 */ {{-118, -67}},
      /* 16 */ {{-64, -13}},
      /* 17 */ {{13, 64}},
      /* 18 */ {{67, 118}},
      /* 19 */ {{147, 198}},
      /* 20 */ {{201, 252}},
      /* 21 */ {{260, 311}},
      /* 22 */ {{314, 365}},
      /* 23 */ {{394, 445}},
      /* 24 */ {{448, 499}},
      /* 25 */ {{525, 576}},
      /* 26 */ {{579, 630}},
      /* 27 */ {{659, 710}},
      /* 28 */ {{713, 764}},
      /* 29 */ {{772, 823}},
      /* 30 */ {{826, 877}},
      /* 31 */ {{906, 957}},
      /* 32 */ {{960, 1011}}}},
    {{MHz_u{160}, RuType::RU_106_TONE},
     {/* 1 */ {{-1011, -906}},
      /* 2 */ {{-877, -772}},
      /* 3 */ {{-764, -659}},
      /* 4 */ {{-630, -525}},
      /* 5 */ {{-499, -394}},
      /* 6 */ {{-365, -260}},
      /* 7 */ {{-252, -147}},
      /* 8 */ {{-118, -13}},
      /* 9 */ {{13, 118}},
      /* 10 */ {{147, 252}},
      /* 11 */ {{260, 365}},
      /* 12 */ {{394, 499}},
      /* 13 */ {{525, 630}},
      /* 14 */ {{659, 764}},
      /* 15 */ {{772, 877}},
      /* 16 */ {{906, 1011}}}},
    {{MHz_u{160}, RuType::RU_242_TONE},
     {/* 1 */ {{-1012, -771}},
      /* 2 */ {{-765, -524}},
      /* 3 */ {{-500, -259}},
      /* 4 */ {{-253, -12}},
      /* 5 */ {{12, 253}},
      /* 6 */ {{259, 500}},
      /* 7 */ {{524, 765}},
      /* 8 */ {{771, 1012}}}},
    {{MHz_u{160}, RuType::RU_484_TONE},
     {/* 1 */ {{-1012, -771}, {-765, -524}},
      /* 2 */ {{-500, -259}, {-253, -12}},
      /* 3 */ {{12, 253}, {259, 500}},
      /* 4 */ {{524, 765}, {771, 1012}}}},
    {{MHz_u{160}, RuType::RU_996_TONE},
     {/* 1 */ {{-1012, -515}, {-509, -12}},
      /* 2 */ {{12, 509}, {515, 1012}}}},
    {{MHz_u{160}, RuType::RU_2x996_TONE},
     {/* 1 */ {{-1012, -515}, {-509, -12}, {12, 509}, {515, 1012}}}},
    // RUs in an 320 MHz EHT PPDU (Table 36-7 IEEE802.11be-D7.0)
    {{MHz_u{320}, RuType::RU_26_TONE},
     {/* 1 */ {{-2035, -2010}},
      /* 2 */ {{-2009, -1984}},
      /* 3 */ {{-1981, -1956}},
      /* 4 */ {{-1955, -1930}},
      /* 5 */ {{-1928, -1903}},
      /* 6 */ {{-1901, -1876}},
      /* 7 */ {{-1875, -1850}},
      /* 8 */ {{-1847, -1822}},
      /* 9 */ {{-1821, -1796}},
      /* 10 */ {{-1788, -1763}},
      /* 11 */ {{-1762, -1737}},
      /* 12 */ {{-1734, -1709}},
      /* 13 */ {{-1708, -1683}},
      /* 14 */ {{-1681, -1656}},
      /* 15 */ {{-1654, -1629}},
      /* 16 */ {{-1628, -1603}},
      /* 17 */ {{-1600, -1575}},
      /* 18 */ {{-1574, -1549}},
      /* 19 not defined */ {},
      /* 20 */ {{-1523, -1498}},
      /* 21 */ {{-1497, -1472}},
      /* 22 */ {{-1469, -1444}},
      /* 23 */ {{-1443, -1418}},
      /* 24 */ {{-1416, -1391}},
      /* 25 */ {{-1389, -1364}},
      /* 26 */ {{-1363, -1338}},
      /* 27 */ {{-1335, -1310}},
      /* 28 */ {{-1309, -1284}},
      /* 29 */ {{-1276, -1251}},
      /* 30 */ {{-1250, -1225}},
      /* 31 */ {{-1222, -1197}},
      /* 32 */ {{-1196, -1171}},
      /* 33 */ {{-1169, -1144}},
      /* 34 */ {{-1142, -1117}},
      /* 35 */ {{-1116, -1091}},
      /* 36 */ {{-1088, -1063}},
      /* 37 */ {{-1062, -1037}},
      /* 38 */ {{-1011, -986}},
      /* 39 */ {{-985, -960}},
      /* 40 */ {{-957, -932}},
      /* 41 */ {{-931, -906}},
      /* 42 */ {{-904, -879}},
      /* 43 */ {{-877, -852}},
      /* 44 */ {{-851, -826}},
      /* 45 */ {{-823, -798}},
      /* 46 */ {{-797, -772}},
      /* 47 */ {{-764, -739}},
      /* 48 */ {{-738, -713}},
      /* 49 */ {{-710, -685}},
      /* 50 */ {{-684, -659}},
      /* 51 */ {{-657, -632}},
      /* 52 */ {{-630, -605}},
      /* 53 */ {{-604, -579}},
      /* 54 */ {{-576, -551}},
      /* 55 */ {{-550, -525}},
      /* 56 not defined */ {},
      /* 57 */ {{-499, -474}},
      /* 58 */ {{-473, -448}},
      /* 59 */ {{-445, -420}},
      /* 60 */ {{-419, -394}},
      /* 61 */ {{-392, -367}},
      /* 62 */ {{-365, -340}},
      /* 63 */ {{-339, -314}},
      /* 64 */ {{-311, -286}},
      /* 65 */ {{-285, -260}},
      /* 66 */ {{-252, -227}},
      /* 67 */ {{-226, -201}},
      /* 68 */ {{-198, -173}},
      /* 69 */ {{-172, -147}},
      /* 70 */ {{-145, -120}},
      /* 71 */ {{-118, -93}},
      /* 72 */ {{-92, -67}},
      /* 73 */ {{-64, -39}},
      /* 74 */ {{-38, -13}},
      /* 75 */ {{13, 38}},
      /* 76 */ {{39, 64}},
      /* 77 */ {{67, 92}},
      /* 78 */ {{93, 118}},
      /* 79 */ {{120, 145}},
      /* 80 */ {{147, 172}},
      /* 81 */ {{173, 198}},
      /* 82 */ {{201, 226}},
      /* 83 */ {{227, 252}},
      /* 84 */ {{260, 285}},
      /* 85 */ {{286, 311}},
      /* 86 */ {{314, 339}},
      /* 87 */ {{340, 365}},
      /* 88 */ {{367, 392}},
      /* 89 */ {{394, 419}},
      /* 90 */ {{420, 445}},
      /* 91 */ {{448, 473}},
      /* 92 */ {{474, 499}},
      /* 93 not defined */ {},
      /* 94 */ {{525, 550}},
      /* 95 */ {{551, 576}},
      /* 96 */ {{579, 604}},
      /* 97 */ {{605, 630}},
      /* 98 */ {{632, 657}},
      /* 99 */ {{659, 684}},
      /* 100 */ {{685, 710}},
      /* 101 */ {{713, 738}},
      /* 102 */ {{739, 764}},
      /* 103 */ {{772, 797}},
      /* 104 */ {{798, 823}},
      /* 105 */ {{826, 851}},
      /* 106 */ {{852, 877}},
      /* 107 */ {{879, 904}},
      /* 108 */ {{906, 931}},
      /* 109 */ {{932, 957}},
      /* 110 */ {{960, 985}},
      /* 111 */ {{986, 1011}},
      /* 112 */ {{1037, 1062}},
      /* 113 */ {{1063, 1088}},
      /* 114 */ {{1091, 1116}},
      /* 115 */ {{1117, 1142}},
      /* 116 */ {{1144, 1169}},
      /* 117 */ {{1171, 1196}},
      /* 118 */ {{1197, 1222}},
      /* 119 */ {{1225, 1250}},
      /* 120 */ {{1251, 1276}},
      /* 121 */ {{1284, 1309}},
      /* 122 */ {{1310, 1335}},
      /* 123 */ {{1338, 1363}},
      /* 124 */ {{1364, 1389}},
      /* 125 */ {{1391, 1416}},
      /* 126 */ {{1418, 1443}},
      /* 127 */ {{1444, 1469}},
      /* 128 */ {{1472, 1497}},
      /* 129 */ {{1498, 1523}},
      /* 130 not defined */ {},
      /* 131 */ {{1549, 1574}},
      /* 132 */ {{1575, 1600}},
      /* 133 */ {{1603, 1628}},
      /* 134 */ {{1629, 1654}},
      /* 135 */ {{1656, 1681}},
      /* 136 */ {{1683, 1708}},
      /* 137 */ {{1709, 1734}},
      /* 138 */ {{1737, 1762}},
      /* 139 */ {{1763, 1788}},
      /* 140 */ {{1796, 1821}},
      /* 141 */ {{1822, 1847}},
      /* 142 */ {{1850, 1875}},
      /* 143 */ {{1876, 1901}},
      /* 144 */ {{1903, 1928}},
      /* 145 */ {{1930, 1955}},
      /* 146 */ {{1956, 1981}},
      /* 147 */ {{1984, 2009}},
      /* 148 */ {{2010, 2035}}}},
    {{MHz_u{320}, RuType::RU_52_TONE},
     {/* 1 */ {{-2035, -1984}},
      /* 2 */ {{-1981, -1930}},
      /* 3 */ {{-1901, -1850}},
      /* 4 */ {{-1847, -1796}},
      /* 5 */ {{-1788, -1737}},
      /* 6 */ {{-1734, -1683}},
      /* 7 */ {{-1654, -1603}},
      /* 8 */ {{-1600, -1549}},
      /* 9 */ {{-1523, -1472}},
      /* 10 */ {{-1469, -1418}},
      /* 11 */ {{-1389, -1338}},
      /* 12 */ {{-1335, -1284}},
      /* 13 */ {{-1276, -1225}},
      /* 14 */ {{-1222, -1171}},
      /* 15 */ {{-1142, -1091}},
      /* 16 */ {{-1088, -1037}},
      /* 17 */ {{-1011, -960}},
      /* 18 */ {{-957, -906}},
      /* 19 */ {{-877, -826}},
      /* 20 */ {{-823, -772}},
      /* 21 */ {{-764, -713}},
      /* 22 */ {{-710, -659}},
      /* 23 */ {{-630, -579}},
      /* 24 */ {{-576, -525}},
      /* 25 */ {{-499, -448}},
      /* 26 */ {{-445, -394}},
      /* 27 */ {{-365, -314}},
      /* 28 */ {{-311, -260}},
      /* 29 */ {{-252, -201}},
      /* 30 */ {{-198, -147}},
      /* 31 */ {{-118, -67}},
      /* 32 */ {{-64, -13}},
      /* 33 */ {{13, 64}},
      /* 34 */ {{67, 118}},
      /* 35 */ {{147, 198}},
      /* 36 */ {{201, 252}},
      /* 37 */ {{260, 311}},
      /* 38 */ {{314, 365}},
      /* 39 */ {{394, 445}},
      /* 40 */ {{448, 499}},
      /* 41 */ {{525, 576}},
      /* 42 */ {{579, 630}},
      /* 43 */ {{659, 710}},
      /* 44 */ {{713, 764}},
      /* 45 */ {{772, 823}},
      /* 46 */ {{826, 877}},
      /* 47 */ {{906, 957}},
      /* 48 */ {{960, 1011}},
      /* 49 */ {{1037, 1088}},
      /* 50 */ {{1091, 1142}},
      /* 51 */ {{1171, 1222}},
      /* 52 */ {{1225, 1276}},
      /* 53 */ {{1284, 1335}},
      /* 54 */ {{1338, 1389}},
      /* 55 */ {{1418, 1469}},
      /* 56 */ {{1472, 1523}},
      /* 57 */ {{1549, 1600}},
      /* 58 */ {{1603, 1654}},
      /* 59 */ {{1683, 1734}},
      /* 60 */ {{1737, 1788}},
      /* 61 */ {{1796, 1847}},
      /* 62 */ {{1850, 1901}},
      /* 63 */ {{1930, 1981}},
      /* 64 */ {{1984, 2035}}}},
    {{MHz_u{320}, RuType::RU_106_TONE},
     {/* 1 */ {{-2035, -1930}},
      /* 2 */ {{-1901, -1796}},
      /* 3 */ {{-1788, -1683}},
      /* 4 */ {{-1654, -1549}},
      /* 5 */ {{-1523, -1418}},
      /* 6 */ {{-1389, -1284}},
      /* 7 */ {{-1276, -1171}},
      /* 8 */ {{-1142, -1037}},
      /* 9 */ {{-1011, -906}},
      /* 10 */ {{-877, -772}},
      /* 11 */ {{-764, -659}},
      /* 12 */ {{-630, -525}},
      /* 13 */ {{-499, -394}},
      /* 14 */ {{-365, -260}},
      /* 15 */ {{-252, -147}},
      /* 16 */ {{-118, -13}},
      /* 17 */ {{13, 118}},
      /* 18 */ {{147, 252}},
      /* 19 */ {{260, 365}},
      /* 20 */ {{394, 499}},
      /* 21 */ {{525, 630}},
      /* 22 */ {{659, 764}},
      /* 23 */ {{772, 877}},
      /* 24 */ {{906, 1011}},
      /* 25 */ {{1037, 1142}},
      /* 26 */ {{1171, 1276}},
      /* 27 */ {{1284, 1389}},
      /* 28 */ {{1418, 1523}},
      /* 29 */ {{1549, 1654}},
      /* 30 */ {{1683, 1788}},
      /* 31 */ {{1796, 1901}},
      /* 32 */ {{1930, 2035}}}},
    {{MHz_u{320}, RuType::RU_242_TONE},
     {/* 1 */ {{-2036, -1795}},
      /* 2 */ {{-1789, -1548}},
      /* 3 */ {{-1524, -1283}},
      /* 4 */ {{-1277, -1036}},
      /* 5 */ {{-1012, -771}},
      /* 6 */ {{-765, -524}},
      /* 7 */ {{-500, -259}},
      /* 8 */ {{-253, -12}},
      /* 9 */ {{12, 253}},
      /* 10 */ {{259, 500}},
      /* 11 */ {{524, 765}},
      /* 12 */ {{771, 1012}},
      /* 13 */ {{1036, 1277}},
      /* 14 */ {{1283, 1524}},
      /* 15 */ {{1548, 1789}},
      /* 16 */ {{1795, 2036}}}},
    {{MHz_u{320}, RuType::RU_484_TONE},
     {/* 1 */ {{-2036, -1795}, {-1789, -1548}},
      /* 2 */ {{-1524, -1283}, {-1277, -1036}},
      /* 3 */ {{-1012, -771}, {-765, -524}},
      /* 4 */ {{-500, -259}, {-253, -12}},
      /* 5 */ {{12, 253}, {259, 500}},
      /* 6 */ {{524, 765}, {771, 1012}},
      /* 7 */ {{1036, 1277}, {1283, 1524}},
      /* 8 */ {{1548, 1789}, {1795, 2036}}}},
    {{MHz_u{320}, RuType::RU_996_TONE},
     {/* 1 */ {{-2036, -1539}, {-1533, -1036}},
      /* 2 */ {{-1012, -515}, {-509, -12}},
      /* 3 */ {{12, 509}, {515, 1012}},
      /* 4 */ {{1036, 1533}, {1539, 2036}}}},
    {{MHz_u{320}, RuType::RU_2x996_TONE},
     {/* 1 */ {{-2036, -1539}, {-1533, -1036}, {-1012, -515}, {-509, -12}},
      /* 2 */ {{12, 509}, {515, 1012}, {1036, 1533}, {1539, 2036}}}},
    {{MHz_u{320}, RuType::RU_4x996_TONE},
     {/* 1 */ {{-2036, -1539},
               {-1533, -1036},
               {-1012, -515},
               {-509, -12},
               {12, 509},
               {515, 1012},
               {1036, 1533},
               {1539, 2036}}}},
};

// Table 36-34 IEEE802.11be-D7.0
const EhtRu::RuAllocationMap EhtRu::m_ruAllocations = {
    // clang-format off
    {0,
     {EhtRu::RuSpec{RuType::RU_26_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 4, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 6, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 7, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 8, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 9, true, true}}},
    {1,
     {EhtRu::RuSpec{RuType::RU_26_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 4, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 6, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 7, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 4, true, true}}},
    {2,
     {EhtRu::RuSpec{RuType::RU_26_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 4, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 8, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 9, true, true}}},
    {3,
     {EhtRu::RuSpec{RuType::RU_26_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 4, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 4, true, true}}},
    {4,
     {EhtRu::RuSpec{RuType::RU_26_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 6, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 7, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 8, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 9, true, true}}},
    {5,
     {EhtRu::RuSpec{RuType::RU_26_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 6, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 7, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 4, true, true}}},
    {6,
     {EhtRu::RuSpec{RuType::RU_26_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 8, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 9, true, true}}},
    {7,
     {EhtRu::RuSpec{RuType::RU_26_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 4, true, true}}},
    {8,
     {EhtRu::RuSpec{RuType::RU_52_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 4, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 6, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 7, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 8, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 9, true, true}}},
    {9,
     {EhtRu::RuSpec{RuType::RU_52_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 4, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 6, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 7, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 4, true, true}}},
    {10,
     {EhtRu::RuSpec{RuType::RU_52_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 4, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 8, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 9, true, true}}},
    {11,
     {EhtRu::RuSpec{RuType::RU_52_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 4, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 4, true, true}}},
    {12,
     {EhtRu::RuSpec{RuType::RU_52_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 6, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 7, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 8, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 9, true, true}}},
    {13,
     {EhtRu::RuSpec{RuType::RU_52_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 6, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 7, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 4, true, true}}},
    {14,
     {EhtRu::RuSpec{RuType::RU_52_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 8, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 9, true, true}}},
    {15,
     {EhtRu::RuSpec{RuType::RU_52_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 4, true, true}}},
    {16,
     {EhtRu::RuSpec{RuType::RU_26_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 4, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_106_TONE, 2, true, true}}},
    {17,
     {EhtRu::RuSpec{RuType::RU_26_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_106_TONE, 2, true, true}}},
    {18,
     {EhtRu::RuSpec{RuType::RU_52_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 4, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_106_TONE, 2, true, true}}},
    {19,
     {EhtRu::RuSpec{RuType::RU_52_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_106_TONE, 2, true, true}}},
    {20,
     {EhtRu::RuSpec{RuType::RU_106_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 6, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 7, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 8, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 9, true, true}}},
    {21,
     {EhtRu::RuSpec{RuType::RU_106_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 6, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 7, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 4, true, true}}},
    {22,
     {EhtRu::RuSpec{RuType::RU_106_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 8, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 9, true, true}}},
    {23,
     {EhtRu::RuSpec{RuType::RU_106_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 4, true, true}}},
    {24,
     {EhtRu::RuSpec{RuType::RU_52_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 2, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 3, true, true},
      EhtRu::RuSpec{RuType::RU_52_TONE, 4, true, true}}},
    {25,
     {EhtRu::RuSpec{RuType::RU_106_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_26_TONE, 5, true, true},
      EhtRu::RuSpec{RuType::RU_106_TONE, 2, true, true}}},
    {48, // FIXME: map 106/106 to 106+26/106 as long as MRU is not supported
     {EhtRu::RuSpec{RuType::RU_106_TONE, 1, true, true},
      EhtRu::RuSpec{RuType::RU_106_TONE, 2, true, true}}},
    {64,
      {EhtRu::RuSpec{RuType::RU_242_TONE, 1, true, true}}},
    {72,
      {EhtRu::RuSpec{RuType::RU_484_TONE, 1, true, true}}},
    {80,
      {EhtRu::RuSpec{RuType::RU_996_TONE, 1, true, true}}},
    {88,
      {EhtRu::RuSpec{RuType::RU_2x996_TONE, 1, true, true}}},
    // clang-format on
};

std::vector<EhtRu::RuSpec>
EhtRu::GetRuSpecs(uint16_t ruAllocation)
{
    std::optional<std::size_t> idx;
    if ((ruAllocation <= 25) || (ruAllocation == 48))
    {
        idx = ruAllocation;
    }
    else if ((ruAllocation >= 26) && (ruAllocation <= 31))
    {
        // unassigned RU, punctured RU or contributes to zero User fields
    }
    else if (((ruAllocation >= 64) && (ruAllocation <= 95)))
    {
        idx = ruAllocation & 0x1F8;
    }
    else
    {
        NS_FATAL_ERROR("Unsupported RU allocation " << ruAllocation);
    }
    return idx.has_value() ? m_ruAllocations.at(idx.value()) : std::vector<EhtRu::RuSpec>{};
}

uint16_t
EhtRu::GetEqualizedRuAllocation(RuType ruType, bool isOdd, bool hasUsers)
{
    switch (ruType)
    {
    case RuType::RU_26_TONE:
        return 0;
    case RuType::RU_52_TONE:
        return isOdd ? 15 : 24;
    case RuType::RU_106_TONE:
        // FIXME: map 106/106 to 106+26/106 as long as MRU is not supported
        return isOdd ? 25 : 48;
    case RuType::RU_242_TONE:
        return hasUsers ? 64 : 28;
    case RuType::RU_484_TONE:
        return hasUsers ? 72 : 29;
    default:
        const auto ruAlloc = (ruType == RuType::RU_2x996_TONE) ? 88 : 80;
        return hasUsers ? ruAlloc : 30;
    }
}

EhtRu::RuSpec::RuSpec(RuType ruType,
                      std::size_t index,
                      bool primary160MHz,
                      bool primary80MHzOrLower80MHz)
    : m_ruType{ruType},
      m_index{index},
      m_primary160MHz{primary160MHz},
      m_primary80MHzOrLower80MHz{primary80MHzOrLower80MHz}
{
    NS_ABORT_MSG_IF(index == 0, "Index cannot be zero");
}

RuType
EhtRu::RuSpec::GetRuType() const
{
    NS_ABORT_MSG_IF(m_index == 0, "Undefined RU");
    return m_ruType;
}

std::size_t
EhtRu::RuSpec::GetIndex() const
{
    NS_ABORT_MSG_IF(m_index == 0, "Undefined RU");
    return m_index;
}

bool
EhtRu::RuSpec::GetPrimary160MHz() const
{
    NS_ABORT_MSG_IF(m_index == 0, "Undefined RU");
    return m_primary160MHz;
}

bool
EhtRu::RuSpec::GetPrimary80MHzOrLower80MHz() const
{
    NS_ABORT_MSG_IF(m_index == 0, "Undefined RU");
    return m_primary80MHzOrLower80MHz;
}

std::size_t
EhtRu::RuSpec::GetPhyIndex(MHz_u bw, uint8_t p20Index) const
{
    auto phyIndex{m_index};
    const auto primary160IsLower160 = (p20Index < (MHz_u{320} / MHz_u{20}) / 2);
    if ((bw > MHz_u{160}) && (m_ruType == RuType::RU_2x996_TONE) &&
        ((primary160IsLower160 && !m_primary160MHz) || (!primary160IsLower160 && m_primary160MHz)))
    {
        phyIndex += 1;
    }
    const auto indicesPer80MHz = GetNRus(MHz_u{80}, m_ruType, true);
    if ((bw > MHz_u{160}) && (m_ruType < RuType::RU_2x996_TONE) &&
        ((primary160IsLower160 && !m_primary160MHz) || (!primary160IsLower160 && m_primary160MHz)))
    {
        phyIndex += 2 * indicesPer80MHz;
    }
    const uint8_t num20MHzSubchannelsIn160 = MHz_u{160} / MHz_u{20};
    const auto primary80IsLower80 =
        ((p20Index % num20MHzSubchannelsIn160) < (num20MHzSubchannelsIn160 / 2));
    if ((bw > MHz_u{80}) && (m_ruType < RuType::RU_4x996_TONE) &&
        ((m_primary160MHz && ((primary80IsLower80 && !m_primary80MHzOrLower80MHz) ||
                              (!primary80IsLower80 && m_primary80MHzOrLower80MHz))) ||
         (!m_primary160MHz && !m_primary80MHzOrLower80MHz)))
    {
        phyIndex += indicesPer80MHz;
    }
    return phyIndex;
}

std::pair<bool, bool>
EhtRu::GetPrimaryFlags(MHz_u bw, RuType ruType, std::size_t phyIndex, uint8_t p20Index)
{
    const auto nRus = GetNRus(bw, ruType);
    const auto ruWidth = WifiRu::GetBandwidth(ruType);
    const auto indicesPer80MHz = (ruWidth <= MHz_u{80}) ? GetNRus(MHz_u{80}, ruType, true) : 1;
    const auto undefinedRusPer80MHz =
        (ruWidth <= MHz_u{80}) ? (indicesPer80MHz - GetNRus(MHz_u{80}, ruType)) : 0;
    const auto primary160IsLower160 = (p20Index < (MHz_u{320} / MHz_u{20}) / 2);
    const auto primary160 =
        (bw < MHz_u{320} || ruType == RuType::RU_4x996_TONE ||
         (primary160IsLower160 && phyIndex <= (nRus / 2) + (2 * undefinedRusPer80MHz)) ||
         (!primary160IsLower160 && phyIndex > (nRus / 2) + (2 * undefinedRusPer80MHz)));
    bool primary80OrLow80;
    if (primary160)
    {
        const uint8_t num20MHzSubchannelsIn160 = MHz_u{160} / MHz_u{20};
        const auto primary80IsLower80 =
            ((p20Index % num20MHzSubchannelsIn160) < (num20MHzSubchannelsIn160 / 2));
        const auto threshold =
            ((bw < MHz_u{320}) || primary160IsLower160) ? indicesPer80MHz : 3 * indicesPer80MHz;
        primary80OrLow80 = (bw < MHz_u{160}) || (ruType >= RuType::RU_2x996_TONE) ||
                           (primary80IsLower80 && phyIndex <= threshold) ||
                           (!primary80IsLower80 && phyIndex > threshold);
    }
    else
    {
        primary80OrLow80 = (bw < MHz_u{160}) || (ruType >= RuType::RU_2x996_TONE) ||
                           ((((phyIndex - 1) / indicesPer80MHz) % 2) == 0);
    }
    return {primary160, primary80OrLow80};
}

std::size_t
EhtRu::GetIndexIn80MHzSegment(MHz_u bw, RuType ruType, std::size_t phyIndex)
{
    if (WifiRu::GetBandwidth(ruType) > MHz_u{80})
    {
        return 1;
    }

    if (const auto indicesPer80MHz = GetNRus(MHz_u{80}, ruType, true);
        bw > MHz_u{80} && phyIndex > indicesPer80MHz)
    {
        return (((phyIndex - 1) % indicesPer80MHz) + 1);
    }

    return phyIndex;
}

std::size_t
EhtRu::GetNRus(MHz_u bw, RuType ruType, bool includeUndefinedRus /* = false */)
{
    if (WifiRu::GetBandwidth(ruType) >= MHz_u{20})
    {
        return bw / WifiRu::GetBandwidth(ruType);
    }

    const auto it = m_ruSubcarrierGroups.find({bw, ruType});
    if (it == m_ruSubcarrierGroups.end())
    {
        return 0;
    }

    auto nRus = it->second.size();
    if (!includeUndefinedRus)
    {
        const auto num80MHz = std::max<uint16_t>(bw / MHz_u{80}, 1);
        const auto undefinedRus =
            ((ruType == RuType::RU_26_TONE) && (bw >= MHz_u{80})) ? num80MHz : 0;
        nRus -= undefinedRus;
    }
    return nRus;
}

std::vector<EhtRu::RuSpec>
EhtRu::GetRusOfType(MHz_u bw, RuType ruType)
{
    if (GetNRus(bw, ruType) == 0)
    {
        return {};
    }

    if (WifiRu::GetBandwidth(ruType) == bw)
    {
        return {{ruType, 1, true, true}};
    }

    if (ruType == RuType::RU_2x996_TONE)
    {
        NS_ASSERT(bw >= MHz_u{160});
        return {{ruType, 1, true, true}, {ruType, 1, false, true}};
    }

    std::vector<EhtRu::RuSpec> ret;
    const auto subcarrierGroup = EhtRu::m_ruSubcarrierGroups.at({bw, ruType});
    const auto indices = GetNRus(std::min(bw, MHz_u{80}), ruType, true);
    for (uint8_t idx80MHz = 0; idx80MHz < (bw / MHz_u{80}); ++idx80MHz)
    {
        const auto p160 = (idx80MHz < 2);
        const auto p80orLow80 = ((idx80MHz % 2) == 0);
        for (std::size_t ruIndex = 1; ruIndex <= indices; ++ruIndex)
        {
            if (subcarrierGroup.at(ruIndex - 1).empty())
            {
                // undefined RU
                continue;
            }
            ret.emplace_back(ruType, ruIndex, p160, p80orLow80);
        }
    }
    return ret;
}

std::vector<EhtRu::RuSpec>
EhtRu::GetCentral26TonesRus(MHz_u bw, RuType ruType)
{
    if ((ruType == RuType::RU_26_TONE) || (ruType >= RuType::RU_242_TONE))
    {
        return {};
    }

    std::vector<std::size_t> indices;
    if (bw >= MHz_u{20})
    {
        indices.push_back(5);
    }
    if (bw >= MHz_u{40})
    {
        indices.push_back(14);
    }
    if (bw >= MHz_u{80})
    {
        indices.insert(indices.end(), {24, 33});
    }

    std::vector<EhtRu::RuSpec> ret;
    for (uint8_t idx80MHz = 0; idx80MHz < (bw / MHz_u{80}); ++idx80MHz)
    {
        const auto p160 = (idx80MHz < 2);
        const auto p80orLow80 = ((idx80MHz % 2) == 0);
        std::transform(indices.cbegin(),
                       indices.cend(),
                       std::back_inserter(ret),
                       [p160, p80orLow80](const auto indice) {
                           return EhtRu::RuSpec{RuType::RU_26_TONE, indice, p160, p80orLow80};
                       });
    }
    return ret;
}

SubcarrierGroup
EhtRu::GetSubcarrierGroup(MHz_u bw, RuType ruType, std::size_t phyIndex)
{
    const auto it = m_ruSubcarrierGroups.find({bw, ruType});
    NS_ABORT_MSG_IF(it == m_ruSubcarrierGroups.cend(), "RU not found");
    NS_ABORT_MSG_IF(phyIndex == 0 || phyIndex > it->second.size(),
                    "Invalid PHY index " << phyIndex << " for RU type " << ruType
                                         << " and bandwidth " << bw);
    return it->second.at(phyIndex - 1);
}

bool
EhtRu::DoesOverlap(MHz_u bw, EhtRu::RuSpec ru, const std::vector<EhtRu::RuSpec>& v)
{
    // A 4x996-tone RU spans 320 MHz, hence it overlaps with any other RU
    if (bw == MHz_u{320} && ru.GetRuType() == RuType::RU_4x996_TONE && !v.empty())
    {
        return true;
    }

    // This function may be called by the MAC layer, hence the PHY index may have
    // not been set yet. Hence, we pass the "MAC" index to GetSubcarrierGroup instead
    // of the PHY index. This is fine because we compare the primary 80 MHz bands of
    // the two RUs below.
    const auto rangesRu = GetSubcarrierGroup(bw, ru.GetRuType(), ru.GetIndex());
    const auto ruBw = WifiRu::GetBandwidth(ru.GetRuType());
    for (auto& p : v)
    {
        // A 4x996-tone RU spans 160 MHz, hence it overlaps
        if (bw == MHz_u{320} && p.GetRuType() == RuType::RU_4x996_TONE)
        {
            return true;
        }
        if ((ru.GetPrimary160MHz() != p.GetPrimary160MHz()))
        {
            // the two RUs are located in distinct 160MHz bands
            continue;
        }
        if (const auto otherRuBw = WifiRu::GetBandwidth(p.GetRuType());
            (ruBw <= MHz_u{80}) && (otherRuBw <= MHz_u{80}) &&
            (ru.GetPrimary80MHzOrLower80MHz() != p.GetPrimary80MHzOrLower80MHz()))
        {
            // the two RUs are located in distinct 80MHz bands
            continue;
        }
        for (const auto& rangeRu : rangesRu)
        {
            SubcarrierGroup rangesP = GetSubcarrierGroup(bw, p.GetRuType(), p.GetIndex());
            for (auto& rangeP : rangesP)
            {
                if (rangeP.second >= rangeRu.first && rangeRu.second >= rangeP.first)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

EhtRu::RuSpec
EhtRu::FindOverlappingRu(MHz_u bw, EhtRu::RuSpec referenceRu, RuType searchedRuType)
{
    const auto numRus = EhtRu::GetNRus(bw, searchedRuType);
    for (uint8_t idx80MHz = 0; idx80MHz < (bw / MHz_u{80}); ++idx80MHz)
    {
        const auto p160 = (idx80MHz < 2);
        const auto p80orLow80 = ((idx80MHz % 2) == 0);
        for (std::size_t index = 1; index <= numRus; ++index)
        {
            EhtRu::RuSpec searchedRu(searchedRuType, index, p160, p80orLow80);
            if (DoesOverlap(bw, referenceRu, {searchedRu}))
            {
                return searchedRu;
            }
        }
    }
    NS_ABORT_MSG("The searched RU type " << searchedRuType << " was not found for bw=" << bw
                                         << " and referenceRu=" << referenceRu);
    return EhtRu::RuSpec();
}

RuType
EhtRu::GetEqualSizedRusForStations(MHz_u bandwidth,
                                   std::size_t& nStations,
                                   std::size_t& nCentral26TonesRus)
{
    RuType ruType{RuType::RU_TYPE_MAX};
    for (int i = static_cast<int>(RuType::RU_4x996_TONE); i >= static_cast<int>(RuType::RU_26_TONE);
         i--)
    {
        if (WifiRu::GetBandwidth(static_cast<RuType>(i)) > bandwidth)
        {
            continue;
        }
        if (GetNRus(bandwidth, static_cast<RuType>(i)) > nStations)
        {
            /* the number of RUs that can be allocated using that RU type
             * is larger than the number of stations to allocate, the RU type
             * to return is hence the previous one and we can stop here */
            break;
        }
        ruType = static_cast<RuType>(i);
    }
    NS_ASSERT_MSG(ruType != RuType::RU_TYPE_MAX, "Cannot find equal size RUs");
    nStations = GetNRus(bandwidth, ruType);
    nCentral26TonesRus = GetNumCentral26TonesRus(bandwidth, ruType);
    return ruType;
}

uint8_t
EhtRu::GetNumCentral26TonesRus(MHz_u bandwidth, RuType ruType)
{
    return ((ruType == RuType::RU_52_TONE) || (ruType == RuType::RU_106_TONE))
               ? (bandwidth / MHz_u{20})
               : 0;
}

bool
EhtRu::RuSpec::operator==(const EhtRu::RuSpec& other) const
{
    // we do not compare the RU PHY indices because they may be uninitialized for
    // one of the compared RUs. This event should not cause the comparison to evaluate
    // to false
    return std::tie(m_ruType, m_index, m_primary160MHz, m_primary80MHzOrLower80MHz) ==
           std::tie(other.m_ruType, other.m_index, m_primary160MHz, m_primary80MHzOrLower80MHz);
}

bool
EhtRu::RuSpec::operator!=(const EhtRu::RuSpec& other) const
{
    return !(*this == other);
}

bool
EhtRu::RuSpec::operator<(const EhtRu::RuSpec& other) const
{
    // we do not compare the RU PHY indices because they may be uninitialized for
    // one of the compared RUs. This event should not cause the comparison to evaluate
    // to false
    return std::tie(m_ruType, m_index, m_primary160MHz, m_primary80MHzOrLower80MHz) <
           std::tie(other.m_ruType, other.m_index, m_primary160MHz, m_primary80MHzOrLower80MHz);
}

std::ostream&
operator<<(std::ostream& os, const EhtRu::RuSpec& ru)
{
    os << "RU{" << ru.GetRuType() << "/" << ru.GetIndex() << "/"
       << (ru.GetPrimary160MHz() ? "primary160MHz" : "secondary160MHz") << "/";
    if (ru.GetPrimary160MHz())
    {
        os << (ru.GetPrimary80MHzOrLower80MHz() ? "primary80MHz" : "secondary80MHz");
    }
    else
    {
        os << (ru.GetPrimary80MHzOrLower80MHz() ? "Lower80MHz" : "high80MHz");
    }
    os << "}";
    return os;
}

} // namespace ns3
