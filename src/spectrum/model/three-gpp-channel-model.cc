/*
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 * Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering,
 * New York University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "three-gpp-channel-model.h"

#include "ns3/double.h"
#include "ns3/geocentric-constant-position-mobility-model.h"
#include "ns3/integer.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/node.h"
#include "ns3/phased-array-model.h"
#include "ns3/pointer.h"
#include "ns3/shuffle.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

#include <algorithm>
#include <array>
#include <map>
#include <random>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ThreeGppChannelModel");

NS_OBJECT_ENSURE_REGISTERED(ThreeGppChannelModel);

/// Conversion factor: degrees to radians
static constexpr double DEG2RAD = M_PI / 180.0;

/// The ray offset angles within a cluster, given for rms angle spread normalized to 1.
/// (Table 7.5-3)
static constexpr std::array<double, 20> offSetAlpha = {
    0.0447, -0.0447, 0.1413, -0.1413, 0.2492, -0.2492, 0.3715, -0.3715, 0.5129, -0.5129,
    0.6797, -0.6797, 0.8844, -0.8844, 1.1481, -1.1481, 1.5195, -1.5195, 2.1551, -2.1551,
};

/**
 * The square root matrix for <em>RMa LOS</em>, which is generated using the
 * Cholesky decomposition according to table 7.5-6 Part 2 and follows the order
 * of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 7>, 7> sqrtC_RMa_LOS = {{
    {1, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0},
    {-0.5, 0, 0.866025, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0},
    {0.01, 0, -0.0519615, 0.73, -0.2, 0.651383, 0},
    {-0.17, -0.02, 0.21362, -0.14, 0.24, 0.142773, 0.909661},
}};

/**
 * The square root matrix for <em>RMa NLOS</em>, which is generated using the
 * Cholesky decomposition according to table 7.5-6 Part 2 and follows the order
 * of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 * The parameter K is ignored.
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 6>, 6> sqrtC_RMa_NLOS = {{
    {1, 0, 0, 0, 0, 0},
    {-0.5, 0.866025, 0, 0, 0, 0},
    {0.6, -0.11547, 0.791623, 0, 0, 0},
    {0, 0, 0, 1, 0, 0},
    {-0.04, -0.138564, 0.540662, -0.18, 0.809003, 0},
    {-0.25, -0.606218, -0.240013, 0.26, -0.231685, 0.625392},
}};

/**
 * The square root matrix for <em>RMa O2I</em>, which is generated using the
 * Cholesky decomposition according to table 7.5-6 Part 2 and follows the order
 * of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 6>, 6> sqrtC_RMa_O2I = {{
    {1, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0},
    {0, 0, 1, 0, 0, 0},
    {0, 0, -0.7, 0.714143, 0, 0},
    {0, 0, 0.66, -0.123225, 0.741091, 0},
    {0, 0, 0.47, 0.152631, -0.393194, 0.775373},
}};

/**
 * The square root matrix for <em>UMa LOS</em>, which is generated using the
 * Cholesky decomposition according to table 7.5-6 Part 1 and follows the order
 * of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 7>, 7> sqrtC_UMa_LOS = {{
    {1, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0},
    {-0.4, -0.4, 0.824621, 0, 0, 0, 0},
    {-0.5, 0, 0.242536, 0.83137, 0, 0, 0},
    {-0.5, -0.2, 0.630593, -0.484671, 0.278293, 0, 0},
    {0, 0, -0.242536, 0.672172, 0.642214, 0.27735, 0},
    {-0.8, 0, -0.388057, -0.367926, 0.238537, -3.58949e-15, 0.130931},
}};

/**
 * The square root matrix for <em>UMa NLOS</em>, which is generated using the
 * Cholesky decomposition according to table 7.5-6 Part 1 and follows the order
 * of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 * The parameter K is ignored.
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 6>, 6> sqrtC_UMa_NLOS = {{
    {1, 0, 0, 0, 0, 0},
    {-0.4, 0.916515, 0, 0, 0, 0},
    {-0.6, 0.174574, 0.78072, 0, 0, 0},
    {0, 0.654654, 0.365963, 0.661438, 0, 0},
    {0, -0.545545, 0.762422, 0.118114, 0.327327, 0},
    {-0.4, -0.174574, -0.396459, 0.392138, 0.49099, 0.507445},
}};

/**
 * The square root matrix for <em>UMa O2I</em>, which is generated using the
 * Cholesky decomposition according to table 7.5-6 Part 1 and follows the order
 * of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 6>, 6> sqrtC_UMa_O2I = {{
    {1, 0, 0, 0, 0, 0},
    {-0.5, 0.866025, 0, 0, 0, 0},
    {0.2, 0.57735, 0.791623, 0, 0, 0},
    {0, 0.46188, -0.336861, 0.820482, 0, 0},
    {0, -0.69282, 0.252646, 0.493742, 0.460857, 0},
    {0, -0.23094, 0.16843, 0.808554, -0.220827, 0.464515},
}};

/**
 * The square root matrix for <em>UMi LOS</em>, which is generated using the
 * Cholesky decomposition according to table 7.5-6 Part 1 and follows the order
 * of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 7>, 7> sqrtC_UMi_LOS = {{
    {1, 0, 0, 0, 0, 0, 0},
    {0.5, 0.866025, 0, 0, 0, 0, 0},
    {-0.4, -0.57735, 0.711805, 0, 0, 0, 0},
    {-0.5, 0.057735, 0.468293, 0.726201, 0, 0, 0},
    {-0.4, -0.11547, 0.805464, -0.23482, 0.350363, 0, 0},
    {0, 0, 0, 0.688514, 0.461454, 0.559471, 0},
    {0, 0, 0.280976, 0.231921, -0.490509, 0.11916, 0.782603},
}};

/**
 * The square root matrix for <em>UMi NLOS</em>, which is generated using the
 * Cholesky decomposition according to table 7.5-6 Part 1 and follows the order
 * of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 * The parameter K is ignored.
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 6>, 6> sqrtC_UMi_NLOS = {{
    {1, 0, 0, 0, 0, 0},
    {-0.7, 0.714143, 0, 0, 0, 0},
    {0, 0, 1, 0, 0, 0},
    {-0.4, 0.168034, 0, 0.90098, 0, 0},
    {0, -0.70014, 0.5, 0.130577, 0.4927, 0},
    {0, 0, 0.5, 0.221981, -0.566238, 0.616522},
}};

/**
 * The square root matrix for <em>UMi O2I</em>, which is generated using the
 * Cholesky decomposition according to table 7.5-6 Part 1 and follows the order
 * of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 6>, 6> sqrtC_UMi_O2I = {{
    {1, 0, 0, 0, 0, 0},
    {-0.5, 0.866025, 0, 0, 0, 0},
    {0.2, 0.57735, 0.791623, 0, 0, 0},
    {0, 0.46188, -0.336861, 0.820482, 0, 0},
    {0, -0.69282, 0.252646, 0.493742, 0.460857, 0},
    {0, -0.23094, 0.16843, 0.808554, -0.220827, 0.464515},
}};

/**
 * The square root matrix for <em>Indoor-Office LOS</em>, which is generated
 * using the Cholesky decomposition according to table 7.5-6 Part 2 and follows
 * the order of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 7>, 7> sqrtC_office_LOS = {{
    {1, 0, 0, 0, 0, 0, 0},
    {0.5, 0.866025, 0, 0, 0, 0, 0},
    {-0.8, -0.11547, 0.588784, 0, 0, 0, 0},
    {-0.4, 0.23094, 0.520847, 0.717903, 0, 0, 0},
    {-0.5, 0.288675, 0.73598, -0.348236, 0.0610847, 0, 0},
    {0.2, -0.11547, 0.418943, 0.541106, 0.219905, 0.655744, 0},
    {0.3, -0.057735, 0.73598, -0.348236, 0.0610847, -0.304997, 0.383375},
}};

/**
 * The square root matrix for <em>Indoor-Office NLOS</em>, which is generated
 * using the Cholesky decomposition according to table 7.5-6 Part 2 and follows
 * the order of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 * The parameter K is ignored.
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 6>, 6> sqrtC_office_NLOS = {{
    {1, 0, 0, 0, 0, 0},
    {-0.5, 0.866025, 0, 0, 0, 0},
    {0, 0.46188, 0.886942, 0, 0, 0},
    {-0.4, -0.23094, 0.120263, 0.878751, 0, 0},
    {0, -0.311769, 0.55697, -0.249198, 0.728344, 0},
    {0, -0.069282, 0.295397, 0.430696, 0.468462, 0.709214},
}};

/**
 * The square root matrix for <em>NTN Dense Urban LOS</em>, which is generated
 * using the Cholesky decomposition according to 3GPP TR 38.811 v15.4.0 table 6.7.2-1 and follows
 * the order of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 7>, 7> sqrtC_NTN_DenseUrban_LOS = {{
    {1, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0},
    {-0.4, -0.4, 0.824621, 0, 0, 0, 0},
    {-0.5, 0, 0.242536, 0.83137, 0, 0, 0},
    {-0.5, -0.2, 0.630593, -0.484671, 0.278293, 0, 0},
    {0, 0, -0.242536, 0.672172, 0.642214, 0.27735, 0},
    {-0.8, 0, -0.388057, -0.367926, 0.238537, -4.09997e-15, 0.130931},
}};

/**
 * The square root matrix for <em>NTN Dense Urban NLOS</em>, which is generated
 * using the Cholesky decomposition according to 3GPP TR 38.811 v15.4.0 table 6.7.2-2 and follows
 * the order of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 6>, 6> sqrtC_NTN_DenseUrban_NLOS = {{
    {1, 0, 0, 0, 0, 0},
    {-0.4, 0.916515, 0, 0, 0, 0},
    {-0.6, 0.174574, 0.78072, 0, 0, 0},
    {0, 0.654654, 0.365963, 0.661438, 0, 0},
    {0, -0.545545, 0.762422, 0.118114, 0.327327, 0},
    {-0.4, -0.174574, -0.396459, 0.392138, 0.49099, 0.507445},
}};

/**
 * The square root matrix for <em>NTN Urban LOS</em>, which is generated
 * using the Cholesky decomposition according to 3GPP TR 38.811 v15.4.0 table 6.7.2-3 and follows
 * the order of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 7>, 7> sqrtC_NTN_Urban_LOS = {{
    {1, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0},
    {-0.4, -0.4, 0.824621, 0, 0, 0, 0},
    {-0.5, 0, 0.242536, 0.83137, 0, 0, 0},
    {-0.5, -0.2, 0.630593, -0.484671, 0.278293, 0, 0},
    {0, 0, -0.242536, 0.672172, 0.642214, 0.27735, 0},
    {-0.8, 0, -0.388057, -0.367926, 0.238537, -4.09997e-15, 0.130931},
}};

/**
 * The square root matrix for <em>NTN Urban NLOS</em>, which is generated
 * using the Cholesky decomposition according to 3GPP TR 38.811 v15.4.0 table 6.7.2-4 and follows
 * the order of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The square root matrix is dependent on the elevation angle, thus requiring a map.
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static const std::map<int, std::array<std::array<double, 6>, 6>> sqrtC_NTN_Urban_NLOS{
    {10,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.21, 0.977701, 0, 0, 0, 0},
         {-0.48, 0.459445, 0.747335, 0, 0, 0},
         {-0.05, 0.377927, 0.28416, 0.879729, 0, 0},
         {-0.02, 0.691213, 0.258017, 0.073265, 0.670734, 0},
         {-0.31, -0.00521632, -0.115615, 0.0788023, 0.00218104, 0.940368},
     }}},
    {20,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.25, 0.968246, 0, 0, 0, 0},
         {-0.52, 0.35115, 0.778648, 0, 0, 0},
         {-0.04, 0.371806, 0.345008, 0.860889, 0, 0},
         {0, 0.743613, 0.281102, 0.0424415, 0.605161, 0},
         {-0.32, 0.0206559, -0.0689057, 0.154832, 0.061865, 0.929852},
     }}},
    {30,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.21, 0.977701, 0, 0, 0, 0},
         {-0.52, 0.450853, 0.725487, 0, 0, 0},
         {-0.04, 0.288023, 0.260989, 0.920504, 0, 0},
         {0.01, 0.697657, 0.386856, 0.0418183, 0.601472, 0},
         {-0.33, 0.0416283, -0.0694268, 0.166137, 0.139937, 0.915075},
     }}},
    {40,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.26, 0.965609, 0, 0, 0, 0},
         {-0.53, 0.395813, 0.749955, 0, 0, 0},
         {-0.04, 0.299914, 0.320139, 0.897754, 0, 0},
         {0.01, 0.696556, 0.372815, 0.0580784, 0.610202, 0},
         {-0.33, 0.0457742, -0.0173584, 0.154417, 0.129332, 0.920941},
     }}},
    {50,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.25, 0.968246, 0, 0, 0, 0},
         {-0.57, 0.420864, 0.705672, 0, 0, 0},
         {-0.03, 0.229797, 0.235501, 0.943839, 0, 0},
         {0.03, 0.679063, 0.384466, 0.0681379, 0.6209, 0},
         {-0.41, -0.147173, -0.229228, 0.270707, 0.293002, 0.773668},
     }}},
    {60,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.2, 0.979796, 0, 0, 0, 0},
         {-0.53, 0.473568, 0.703444, 0, 0, 0},
         {-0.05, 0.204124, 0.109225, 0.971547, 0, 0},
         {0.03, 0.68994, 0.411073, 0.0676935, 0.591202, 0},
         {-0.4, -0.224537, -0.292371, 0.275609, 0.301835, 0.732828},
     }}},
    {70,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.19, 0.981784, 0, 0, 0, 0},
         {-0.5, 0.524555, 0.689088, 0, 0, 0},
         {-0.03, 0.228462, 0.18163, 0.955989, 0, 0},
         {-0.02, 0.637818, 0.428725, 0.00608114, 0.639489, 0},
         {-0.36, -0.18171, -0.282523, 0.106726, 0.123808, 0.854894},
     }}},
    {80,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.2, 0.979796, 0, 0, 0, 0},
         {-0.49, 0.502145, 0.712566, 0, 0, 0},
         {-0.01, 0.232702, 0.151916, 0.960558, 0, 0},
         {-0.05, 0.612372, 0.376106, 0.0206792, 0.693265, 0},
         {-0.37, -0.320475, -0.365405, -0.00376264, 0.0364343, 0.790907},
     }}},
    {90,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.19, 0.981784, 0, 0, 0, 0},
         {-0.38, 0.58852, 0.713613, 0, 0, 0},
         {-0.03, 0.360874, 0.12082, 0.924269, 0, 0},
         {-0.12, 0.526796, 0.34244, 0.0594196, 0.766348, 0},
         {-0.33, -0.257389, -0.24372, -0.257035, -0.176521, 0.817451},
     }}},
};

/**
 * The square root matrix for <em>NTN Suburban LOS</em>, which is generated
 * using the Cholesky decomposition according to 3GPP TR 38.811 v15.4.0 table 6.7.2-5 and follows
 * the order of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 7>, 7> sqrtC_NTN_Suburban_LOS = {{
    {1, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0},
    {-0.4, -0.4, 0.824621, 0, 0, 0, 0},
    {-0.5, 0, 0.242536, 0.83137, 0, 0, 0},
    {-0.5, -0.2, 0.630593, -0.484671, 0.278293, 0, 0},
    {0, 0, -0.242536, 0.672172, 0.642214, 0.27735, 0},
    {-0.8, 0, -0.388057, -0.367926, 0.238537, -4.09997e-15, 0.130931},
}};

/**
 * The square root matrix for <em>NTN Suburban NLOS</em>, which is generated
 * using the Cholesky decomposition according to 3GPP TR 38.811 v15.4.0 table 6.7.2-6 and follows
 * the order of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 6>, 6> sqrtC_NTN_Suburban_NLOS = {{
    {1, 0, 0, 0, 0, 0},
    {-0.4, 0.916515, 0, 0, 0, 0},
    {-0.6, 0.174574, 0.78072, 0, 0, 0},
    {0, 0.654654, 0.365963, 0.661438, 0, 0},
    {0, -0.545545, 0.762422, 0.118114, 0.327327, 0},
    {-0.4, -0.174574, -0.396459, 0.392138, 0.49099, 0.507445},
}};

/**
 * The square root matrix for <em>NTN Rural LOS</em>, which is generated
 * using the Cholesky decomposition according to 3GPP TR 38.811 v15.4.0 table 6.7.2-7 and follows
 * the order of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static constexpr std::array<std::array<double, 7>, 7> sqrtC_NTN_Rural_LOS = {{
    {1, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0},
    {-0.5, 0, 0.866025, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0},
    {0.01, 0, -0.0519615, 0.73, -0.2, 0.651383, 0},
    {-0.17, -0.02, 0.21362, -0.14, 0.24, 0.142773, 0.909661},
}};

/**
 * The square root matrix for <em>NTN Rural NLOS S Band</em>, which is generated
 * using the Cholesky decomposition according to 3GPP TR 38.811 v15.4.0 table 6.7.2-8a and follows
 * the order of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The square root matrix is dependent on the elevation angle, thus requiring a map.
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static const std::map<int, std::array<std::array<double, 6>, 6>> sqrtC_NTN_Rural_NLOS_S{
    {10,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.36, 0.932952, 0, 0, 0, 0},
         {0.45, 0.516639, 0.728412, 0, 0, 0},
         {0.02, 0.329277, 0.371881, 0.867687, 0, 0},
         {-0.06, 0.59853, 0.436258, -0.0324062, 0.668424, 0},
         {-0.07, 0.0373009, 0.305087, -0.0280496, -0.225204, 0.921481},
     }}},
    {20,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.39, 0.920815, 0, 0, 0, 0},
         {0.52, 0.426579, 0.740021, 0, 0, 0},
         {0, 0.347518, -0.0381664, 0.936896, 0, 0},
         {-0.04, 0.710675, 0.172483, 0.116993, 0.670748, 0},
         {-0.17, -0.0394216, 0.115154, 0.243458, -0.0702635, 0.944498},
     }}},
    {30,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.41, 0.912086, 0, 0, 0, 0},
         {0.54, 0.49491, 0.680782, 0, 0, 0},
         {0, 0.350844, -0.152231, 0.923977, 0, 0},
         {-0.04, 0.694672, 0.0702137, 0.0832998, 0.709903, 0},
         {-0.19, -0.0854087, 0.0805978, 0.283811, -0.137441, 0.922318},
     }}},
    {40,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.37, 0.929032, 0, 0, 0, 0},
         {0.53, 0.480177, 0.698949, 0, 0, 0},
         {0.01, 0.434538, 0.00864797, 0.900556, 0, 0},
         {-0.05, 0.765851, -0.0303947, 0.0421641, 0.63896, 0},
         {-0.17, -0.16458, 0.0989022, 0.158081, -0.150425, 0.941602},
     }}},
    {50,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.4, 0.916515, 0, 0, 0, 0},
         {0.55, 0.403703, 0.731111, 0, 0, 0},
         {0.02, 0.499719, -0.0721341, 0.862947, 0, 0},
         {-0.06, 0.835775, -0.156481, 0.0373835, 0.521534, 0},
         {-0.19, -0.301141, 0.145082, 0.144564, -0.0238067, 0.911427},
     }}},
    {60,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.41, 0.912086, 0, 0, 0, 0},
         {0.56, 0.339442, 0.755764, 0, 0, 0},
         {0.02, 0.436582, -0.0256617, 0.899076, 0, 0},
         {-0.07, 0.856608, -0.12116, 0.0715303, 0.491453, 0},
         {-0.2, -0.331109, 0.15136, 0.036082, 0.031313, 0.908391},
     }}},
    {70,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.4, 0.916515, 0, 0, 0, 0},
         {0.56, 0.386246, 0.732949, 0, 0, 0},
         {0.04, 0.573913, -0.0601289, 0.815726, 0, 0},
         {-0.11, 0.813953, -0.0720183, 0.0281118, 0.565158, 0},
         {-0.19, -0.432071, 0.236423, -0.0247788, -0.0557206, 0.847113},
     }}},
    {80,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.46, 0.887919, 0, 0, 0, 0},
         {0.58, 0.469412, 0.665772, 0, 0, 0},
         {0.01, 0.309262, -0.286842, 0.90663, 0, 0},
         {-0.05, 0.762457, -0.268721, -0.0467443, 0.584605, 0},
         {-0.23, -0.580909, 0.399665, 0.0403629, 0.326208, 0.584698},
     }}},
    {90,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.3, 0.953939, 0, 0, 0, 0},
         {0.47, 0.81871, 0.329868, 0, 0, 0},
         {0.06, 0.0712834, -0.595875, 0.797654, 0, 0},
         {-0.1, 0.408831, -0.0233859, 0.0412736, 0.905873, 0},
         {-0.13, -0.407783, 0.439436, -0.0768289, -0.212875, 0.756631},
     }}},
};

/**
 * The square root matrix for <em>NTN Rural NLOS Ka Band</em>, which is generated
 * using the Cholesky decomposition according to 3GPP TR 38.811 v15.4.0 table 6.7.2-8b and follows
 * the order of [SF, K, DS, ASD, ASA, ZSD, ZSA].
 *
 * The square root matrix is dependent on the elevation angle, which acts as the corresponding map's
 * key..
 *
 * The Matlab file to generate the matrices can be found in
 * https://github.com/nyuwireless-unipd/ns3-mmwave/blob/master/src/mmwave/model/BeamFormingMatrix/SqrtMatrix.m
 */
static const std::map<int, std::array<std::array<double, 6>, 6>> sqrtC_NTN_Rural_NLOS_Ka{
    {10,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.36, 0.932952, 0, 0, 0, 0},
         {0.45, 0.527358, 0.72069, 0, 0, 0},
         {0.02, 0.350715, 0.355282, 0.866241, 0, 0},
         {-0.07, 0.562515, 0.478504, 0.0162932, 0.670406, 0},
         {-0.06, 0.0411597, 0.270982, 0.0121094, -0.159927, 0.946336},
     }}},
    {20,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.38, 0.924986, 0, 0, 0, 0},
         {0.52, 0.473088, 0.711188, 0, 0, 0},
         {0, 0.367573, -0.0617198, 0.927944, 0, 0},
         {-0.04, 0.68628, 0.149228, 0.115257, 0.701332, 0},
         {-0.16, -0.0441088, 0.118207, 0.251641, -0.0752458, 0.943131},
     }}},
    {30,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.42, 0.907524, 0, 0, 0, 0},
         {0.54, 0.48131, 0.690464, 0, 0, 0},
         {0, 0.363627, -0.137613, 0.921324, 0, 0},
         {-0.04, 0.686704, 0.117433, 0.104693, 0.708581, 0},
         {-0.19, -0.0438556, 0.0922685, 0.269877, -0.136292, 0.928469},
     }}},
    {40,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.36, 0.932952, 0, 0, 0, 0},
         {0.53, 0.483197, 0.696865, 0, 0, 0},
         {0.01, 0.464761, -0.0285153, 0.88492, 0, 0},
         {-0.05, 0.763169, 0.140255, 0.0562856, 0.626286, 0},
         {-0.16, -0.126051, 0.0942905, 0.195354, -0.217188, 0.92967},
     }}},
    {50,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.39, 0.920815, 0, 0, 0, 0},
         {0.55, 0.406705, 0.729446, 0, 0, 0},
         {0.01, 0.503793, -0.123923, 0.854831, 0, 0},
         {-0.06, 0.821664, -0.207246, 0.0245302, 0.526988, 0},
         {-0.19, -0.254231, 0.10679, 0.190931, -0.0665276, 0.920316},
     }}},
    {60,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.42, 0.907524, 0, 0, 0, 0},
         {0.56, 0.391395, 0.730213, 0, 0, 0},
         {0.02, 0.427978, -0.0393147, 0.902712, 0, 0},
         {-0.06, 0.820694, -0.119986, 0.105509, 0.545281, 0},
         {-0.2, -0.279882, 0.180145, 0.0563477, -0.0121631, 0.919723},
     }}},
    {70,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.36, 0.932952, 0, 0, 0, 0},
         {0.54, 0.519212, 0.662434, 0, 0, 0},
         {0.04, 0.412025, -0.0234416, 0.909992, 0, 0},
         {-0.09, 0.758452, -0.0682296, 0.0214276, 0.64151, 0},
         {-0.17, -0.387158, 0.306169, -0.0291255, -0.109344, 0.845378},
     }}},
    {80,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.44, 0.897998, 0, 0, 0, 0},
         {0.57, 0.43519, 0.696928, 0, 0, 0},
         {0.01, 0.316705, -0.248988, 0.915207, 0, 0},
         {-0.06, 0.805793, -0.296262, -0.0419182, 0.507514, 0},
         {-0.22, -0.497551, 0.289742, 0.0785823, 0.328773, 0.711214},
     }}},
    {90,
     {{
         {1, 0, 0, 0, 0, 0},
         {-0.27, 0.96286, 0, 0, 0, 0},
         {0.46, 0.741748, 0.488067, 0, 0, 0},
         {0.04, 0.0735309, -0.374828, 0.923308, 0, 0},
         {-0.08, 0.517624, 0.128779, 0.0795063, 0.838308, 0},
         {-0.11, -0.321646, 0.0802763, -0.131981, -0.193429, 0.907285},
     }}},
};

/**
 * The enumerator used for code clarity when performing parameter assignment in GetThreeGppTable
 */
enum Table3gppParams
{
    uLgDS,
    sigLgDS,
    uLgASD,
    sigLgASD,
    uLgASA,
    sigLgASA,
    uLgZSA,
    sigLgZSA,
    uLgZSD,
    sigLgZSD,
    uK,
    sigK,
    rTau,
    uXpr,
    sigXpr,
    numOfCluster,
    raysPerCluster,
    cDS,
    cASD,
    cASA,
    cZSA,
    perClusterShadowingStd
};

/**
 * The nested map containing the 3GPP value tables for the NTN Dense Urban LOS scenario.
 * The outer key specifies the band (either "S" or "Ka"), while the inner key represents
 * the quantized elevation angle.
 * The inner vector collects the table3gpp values.
 */
static const std::map<std::string, std::map<int, std::array<float, 22>>> NTNDenseUrbanLOS{
    {"S",
     {
         {10, {-7.12, 0.8, -3.06, 0.48, 0.94, 0.7,  0.82, 0.03, -2.52, 0.5, 4.4,
               3.3,   2.5, 24.4,  3.8,  3.0,  20.0, 3.9,  0.0,  11.0,  7.0, 3.0}},
         {20, {-7.28, 0.67, -2.68, 0.36, 0.87, 0.66, 0.5, 0.09, -2.29, 0.53, 9.0,
               6.6,   2.5,  23.6,  4.7,  3.0,  20.0, 3.9, 0.0,  11.0,  7.0,  3.0}},
         {30, {-7.45, 0.68, -2.51, 0.38, 0.92, 0.68, 0.82, 0.05, -2.19, 0.58, 9.3,
               6.1,   2.5,  23.2,  4.6,  3.0,  20.0, 3.9,  0.0,  11.0,  7.0,  3.0}},
         {40, {-7.73, 0.66, -2.4, 0.32, 0.79, 0.64, 1.23, 0.03, -2.24, 0.51, 7.9,
               4.0,   2.5,  22.6, 4.9,  3.0,  20.0, 3.9,  0.0,  11.0,  7.0,  3.0}},
         {50, {-7.91, 0.62, -2.31, 0.33, 0.72, 0.63, 1.43, 0.06, -2.3, 0.46, 7.4,
               3.0,   2.5,  21.8,  5.7,  3.0,  20.0, 3.9,  0.0,  11.0, 7.0,  3.0}},
         {60, {-8.14, 0.51, -2.2, 0.39, 0.6, 0.54, 1.56, 0.05, -2.48, 0.35, 7.0,
               2.6,   2.5,  20.5, 6.9,  3.0, 20.0, 3.9,  0.0,  11.0,  7.0,  3.0}},
         {70, {-8.23, 0.45, -2.0, 0.4, 0.55, 0.52, 1.66, 0.05, -2.64, 0.31, 6.9,
               2.2,   2.5,  19.3, 8.1, 3.0,  20.0, 3.9,  0.0,  11.0,  7.0,  3.0}},
         {80, {-8.28, 0.31, -1.64, 0.32, 0.71, 0.53, 1.73, 0.02, -2.68, 0.39, 6.5,
               2.1,   2.5,  17.4,  10.3, 3.0,  20.0, 3.9,  0.0,  11.0,  7.0,  3.0}},
         {90, {-8.36, 0.08, -0.63, 0.53, 0.81, 0.62, 1.79, 0.01, -2.61, 0.28, 6.8,
               1.9,   2.5,  12.3,  15.2, 3.0,  20.0, 3.9,  0.0,  11.0,  7.0,  3.0}},
     }},
    {"Ka",
     {
         {10, {-7.43, 0.9, -3.43, 0.54, 0.65, 0.82, 0.82, 0.05, -2.75, 0.55, 6.1,
               2.6,   2.5, 24.7,  2.1,  3.0,  20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
         {20, {-7.62, 0.78, -3.06, 0.41, 0.53, 0.78, 0.47, 0.11, -2.64, 0.64, 13.7,
               6.8,   2.5,  24.4,  2.8,  3.0,  20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
         {30, {-7.76, 0.8, -2.91, 0.42, 0.6, 0.83, 0.8, 0.05, -2.49, 0.69, 12.9,
               6.0,   2.5, 24.4,  2.7,  3.0, 20.0, 1.6, 0.0,  11.0,  7.0,  3.0}},
         {40, {-8.02, 0.72, -2.81, 0.34, 0.43, 0.78, 1.23, 0.04, -2.51, 0.57, 10.3,
               3.3,   2.5,  24.2,  2.7,  3.0,  20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
         {50, {-8.13, 0.61, -2.74, 0.34, 0.36, 0.77, 1.42, 0.1, -2.54, 0.5, 9.2,
               2.2,   2.5,  23.9,  3.1,  3.0,  20.0, 1.6,  0.0, 11.0,  7.0, 3.0}},
         {60, {-8.3, 0.47, -2.72, 0.7, 0.16, 0.84, 1.56, 0.06, -2.71, 0.37, 8.4,
               1.9,  2.5,  23.3,  3.9, 3.0,  20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
         {70, {-8.34, 0.39, -2.46, 0.4, 0.18, 0.64, 1.65, 0.07, -2.85, 0.31, 8.0,
               1.5,   2.5,  22.6,  4.8, 3.0,  20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
         {80, {-8.39, 0.26, -2.3, 0.78, 0.24, 0.81, 1.73, 0.02, -3.01, 0.45, 7.4,
               1.6,   2.5,  21.2, 6.8,  3.0,  20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
         {90, {-8.45, 0.01, -1.11, 0.51, 0.36, 0.65, 1.79, 0.01, -3.08, 0.27, 7.6,
               1.3,   2.5,  17.6,  12.7, 3.0,  20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
     }},
};

/**
 * The nested map containing the 3GPP value tables for the NTN Dense Urban NLOS scenario.
 * The outer key specifies the band (either "S" or "Ka"), while the inner key represents
 * the quantized elevation angle.
 * The inner vector collects the table3gpp values.
 */
static const std::map<std::string, std::map<int, std::array<float, 22>>> NTNDenseUrbanNLOS{
    {"S",
     {
         {10, {-6.84, 0.82, -2.08, 0.87, 1.0, 1.6,  1.0, 0.63, -2.08, 0.58, 0.0,
               0.0,   2.3,  23.8,  4.4,  4.0, 20.0, 3.9, 0.0,  15.0,  7.0,  3.0}},
         {20, {-6.81, 0.61, -1.68, 0.73, 1.44, 0.87, 0.94, 0.65, -1.66, 0.5, 0.0,
               0.0,   2.3,  21.9,  6.3,  4.0,  20.0, 3.9,  0.0,  15.0,  7.0, 3.0}},
         {30, {-6.94, 0.49, -1.46, 0.53, 1.54, 0.64, 1.15, 0.42, -1.48, 0.4, 0.0,
               0.0,   2.3,  19.7,  8.1,  4.0,  20.0, 3.9,  0.0,  15.0,  7.0, 3.0}},
         {40, {-7.14, 0.49, -1.43, 0.5, 1.53, 0.56, 1.35, 0.28, -1.46, 0.37, 0.0,
               0.0,   2.3,  18.1,  9.3, 4.0,  20.0, 3.9,  0.0,  15.0,  7.0,  3.0}},
         {50, {-7.34, 0.51, -1.44, 0.58, 1.48, 0.54, 1.44, 0.25, -1.53, 0.47, 0.0,
               0.0,   2.3,  16.3,  11.5, 4.0,  20.0, 3.9,  0.0,  15.0,  7.0,  3.0}},
         {60, {-7.53, 0.47, -1.33, 0.49, 1.39, 0.68, 1.56, 0.16, -1.61, 0.43, 0.0,
               0.0,   2.3,  14.0,  13.3, 4.0,  20.0, 3.9,  0.0,  15.0,  7.0,  3.0}},
         {70, {-7.67, 0.44, -1.31, 0.65, 1.42, 0.55, 1.64, 0.18, -1.77, 0.5, 0.0,
               0.0,   2.3,  12.1,  14.9, 4.0,  20.0, 3.9,  0.0,  15.0,  7.0, 3.0}},
         {80, {-7.82, 0.42, -1.11, 0.69, 1.38, 0.6,  1.7, 0.09, -1.9, 0.42, 0.0,
               0.0,   2.3,  8.7,   17.0, 4.0,  20.0, 3.9, 0.0,  15.0, 7.0,  3.0}},
         {90, {-7.84, 0.55, -0.11, 0.53, 1.23, 0.6,  1.7, 0.17, -1.99, 0.5, 0.0,
               0.0,   2.3,  6.4,   12.3, 4.0,  20.0, 3.9, 0.0,  15.0,  7.0, 3.0}},
     }},
    {"Ka",
     {
         {10, {-6.86, 0.81, -2.12, 0.94, 1.02, 1.44, 1.01, 0.56, -2.11, 0.59, 0.0,
               0.0,   2.3,  23.7,  4.5,  4.0,  20.0, 3.9,  0.0,  15.0,  7.0,  3.0}},
         {20, {-6.84, 0.61, -1.74, 0.79, 1.44, 0.77, 0.96, 0.55, -1.69, 0.51, 0.0,
               0.0,   2.3,  21.8,  6.3,  4.0,  20.0, 3.9,  0.0,  15.0,  7.0,  3.0}},
         {30, {-7.0, 0.56, -1.56, 0.66, 1.48, 0.7,  1.13, 0.43, -1.52, 0.46, 0.0,
               0.0,  2.3,  19.6,  8.2,  4.0,  20.0, 3.9,  0.0,  15.0,  7.0,  3.0}},
         {40, {-7.21, 0.56, -1.54, 0.63, 1.46, 0.6,  1.3, 0.37, -1.51, 0.43, 0.0,
               0.0,   2.3,  18.0,  9.4,  4.0,  20.0, 3.9, 0.0,  15.0,  7.0,  3.0}},
         {50, {-7.42, 0.57, -1.45, 0.56, 1.4, 0.59, 1.4, 0.32, -1.54, 0.45, 0.0,
               0.0,   2.3,  16.3,  11.5, 4.0, 20.0, 3.9, 0.0,  15.0,  7.0,  3.0}},
         {60, {-7.86, 0.55, -1.64, 0.78, 0.97, 1.27, 1.41, 0.45, -1.84, 0.63, 0.0,
               0.0,   2.3,  15.9,  12.4, 4.0,  20.0, 3.9,  0.0,  15.0,  7.0,  3.0}},
         {70, {-7.76, 0.47, -1.37, 0.56, 1.33, 0.56, 1.63, 0.17, -1.86, 0.51, 0.0,
               0.0,   2.3,  12.3,  15.0, 4.0,  20.0, 3.9,  0.0,  15.0,  7.0,  3.0}},
         {80, {-8.07, 0.42, -1.29, 0.76, 1.12, 1.04, 1.68, 0.14, -2.16, 0.74, 0.0,
               0.0,   2.3,  10.5,  15.7, 4.0,  20.0, 3.9,  0.0,  15.0,  7.0,  3.0}},
         {90, {-7.95, 0.59, -0.41, 0.59, 1.04, 0.63, 1.7, 0.17, -2.21, 0.61, 0.0,
               0.0,   2.3,  10.5,  15.7, 4.0,  20.0, 3.9, 0.0,  15.0,  7.0,  3.0}},
     }},
};

/**
 * The nested map containing the 3GPP value tables for the NTN Urban LOS scenario.
 * The outer key specifies the band (either "S" or "Ka"), while the inner key represents
 * the quantized elevation angle.
 * The inner vector collects the table3gpp values.
 */
static const std::map<std::string, std::map<int, std::array<float, 22>>> NTNUrbanLOS{
    {"S",
     {
         {10, {-7.97, 1.0, -2.6, 0.79, 0.18, 0.74, -0.63, 2.6,  -2.54, 2.62, 31.83,
               13.84, 2.5, 8.0,  4.0,  4.0,  20.0, 3.9,   0.09, 12.55, 1.25, 3.0}},
         {20, {-8.12, 0.83, -2.48, 0.8, 0.42, 0.9,  -0.15, 3.31, -2.67, 2.96, 18.78,
               13.78, 2.5,  8.0,   4.0, 3.0,  20.0, 3.9,   0.09, 12.76, 3.23, 3.0}},
         {30, {-8.21, 0.68, -2.44, 0.91, 0.41, 1.3,  0.54, 1.1,  -2.03, 0.86, 10.49,
               10.42, 2.5,  8.0,   4.0,  3.0,  20.0, 3.9,  0.12, 14.36, 4.39, 3.0}},
         {40, {-8.31, 0.48, -2.6, 1.02, 0.18, 1.69, 0.35, 1.59, -2.28, 1.19, 7.46,
               8.01,  2.5,  8.0,  4.0,  3.0,  20.0, 3.9,  0.16, 16.42, 5.72, 3.0}},
         {50, {-8.37, 0.38, -2.71, 1.17, -0.07, 2.04, 0.27, 1.62, -2.48, 1.4,  6.52,
               8.27,  2.5,  8.0,   4.0,  3.0,   20.0, 3.9,  0.2,  17.13, 6.17, 3.0}},
         {60, {-8.39, 0.24, -2.76, 1.17, -0.43, 2.54, 0.26, 0.97, -2.56, 0.85, 5.47,
               7.26,  2.5,  8.0,   4.0,  3.0,   20.0, 3.9,  0.28, 19.01, 7.36, 3.0}},
         {70, {-8.38, 0.18, -2.78, 1.2, -0.64, 2.47, -0.12, 1.99, -2.96, 1.61, 4.54,
               5.53,  2.5,  8.0,   4.0, 3.0,   20.0, 3.9,   0.44, 19.31, 7.3,  3.0}},
         {80, {-8.35, 0.13, -2.65, 1.45, -0.91, 2.69, -0.21, 1.82, -3.08, 1.49, 4.03,
               4.49,  2.5,  8.0,   4.0,  3.0,   20.0, 3.9,   0.9,  22.39, 7.7,  3.0}},
         {90, {-8.34, 0.09, -2.27, 1.85, -0.54, 1.66, -0.07, 1.43, -3.0, 1.09, 3.68,
               3.14,  2.5,  8.0,   4.0,  3.0,   20.0, 3.9,   2.87, 27.8, 9.25, 3.0}},
     }},
    {"Ka",
     {
         {10, {-8.52, 0.92, -3.18, 0.79, -0.4, 0.77, -0.67, 2.22, -2.61, 2.41, 40.18,
               16.99, 2.5,  8.0,   4.0,  4.0,  20.0, 1.6,   0.09, 11.8,  1.14, 3.0}},
         {20, {-8.59, 0.79, -3.05, 0.87, -0.15, 0.97, -0.34, 3.04, -2.82, 2.59, 23.62,
               18.96, 2.5,  8.0,   4.0,  3.0,   20.0, 1.6,   0.09, 11.6,  2.78, 3.0}},
         {30, {-8.51, 0.65, -2.98, 1.04, -0.18, 1.58, 0.07, 1.33, -2.48, 1.02, 12.48,
               14.23, 2.5,  8.0,   4.0,  3.0,   20.0, 1.6,  0.11, 13.05, 3.87, 3.0}},
         {40, {-8.49, 0.48, -3.11, 1.06, -0.31, 1.69, -0.08, 1.45, -2.76, 1.27, 8.56,
               11.06, 2.5,  8.0,   4.0,  3.0,   20.0, 1.6,   0.15, 14.56, 4.94, 3.0}},
         {50, {-8.48, 0.46, -3.19, 1.12, -0.58, 2.13, -0.21, 1.62, -2.93, 1.38, 7.42,
               11.21, 2.5,  8.0,   4.0,  3.0,   20.0, 1.6,   0.18, 15.35, 5.41, 3.0}},
         {60, {-8.44, 0.34, -3.25, 1.14, -0.9, 2.51, -0.25, 1.06, -3.05, 0.96, 5.97,
               9.47,  2.5,  8.0,   4.0,  3.0,  20.0, 1.6,   0.27, 16.97, 6.31, 3.0}},
         {70, {-8.4, 0.27, -3.33, 1.25, -1.16, 2.47, -0.61, 1.88, -3.45, 1.51, 4.88,
               7.24, 2.5,  8.0,   4.0,  3.0,   20.0, 1.6,   0.42, 17.96, 6.66, 3.0}},
         {80, {-8.37, 0.19, -3.22, 1.35, -1.48, 2.61, -0.79, 1.87, -3.66, 1.49, 4.22,
               5.79,  2.5,  8.0,   4.0,  3.0,   20.0, 1.6,   0.86, 20.68, 7.31, 3.0}},
         {90, {-8.35, 0.14, -2.83, 1.62, -1.14, 1.7,  -0.58, 1.19, -3.56, 0.89, 3.81,
               4.25,  2.5,  8.0,   4.0,  3.0,   20.0, 1.6,   2.55, 25.08, 9.23, 3.0}},
     }},
};

/**
 * The nested map containing the 3GPP value tables for the NTN Urban NLOS scenario.
 * The outer key specifies the band (either "S" or "Ka"), while the inner key represents
 * the quantized elevation angle.
 * The inner vector collects the table3gpp values.
 */
static const std::map<std::string, std::map<int, std::array<float, 22>>> NTNUrbanNLOS{
    {"S",
     {
         {10, {-7.24, 1.26, -1.58, 0.89, 0.13, 2.99, -1.13, 2.66, -2.87, 2.76, 0.0,
               0.0,   2.3,  7.0,   3.0,  3.0,  20.0, 1.6,   0.08, 14.72, 1.57, 3.0}},
         {20, {-7.7, 0.99, -1.67, 0.89, 0.19, 3.12, 0.49, 2.03, -2.68, 2.76, 0.0,
               0.0,  2.3,  7.0,   3.0,  3.0,  20.0, 1.6,  0.1,  14.62, 4.3,  3.0}},
         {30, {-7.82, 0.86, -1.84, 1.3, 0.44, 2.69, 0.95, 1.54, -2.12, 1.54, 0.0,
               0.0,   2.3,  7.0,   3.0, 3.0,  20.0, 1.6,  0.14, 16.4,  6.64, 3.0}},
         {40, {-8.04, 0.75, -2.02, 1.15, 0.48, 2.45, 1.15, 1.02, -2.27, 1.77, 0.0,
               0.0,   2.3,  7.0,   3.0,  3.0,  20.0, 1.6,  0.22, 17.86, 9.21, 3.0}},
         {50, {-8.08, 0.77, -2.06, 1.23, 0.56, 2.17, 1.14, 1.61, -2.5,  2.36,  0.0,
               0.0,   2.3,  7.0,   3.0,  3.0,  20.0, 1.6,  0.31, 19.74, 10.32, 3.0}},
         {60, {-8.1, 0.76, -1.99, 1.02, 0.55, 1.93, 1.13, 1.84, -2.47, 2.33, 0.0,
               0.0,  2.3,  7.0,   3.0,  3.0,  20.0, 1.6,  0.49, 19.73, 10.3, 3.0}},
         {70, {-8.16, 0.73, -2.19, 1.78, 0.48, 1.72, 1.16, 1.81, -2.83, 2.84, 0.0,
               0.0,   2.3,  7.0,   3.0,  2.0,  20.0, 1.6,  0.97, 20.5,  10.2, 3.0}},
         {80, {-8.03, 0.79, -1.88, 1.55, 0.53, 1.51, 1.28, 1.35, -2.82, 2.87,  0.0,
               0.0,   2.3,  7.0,   3.0,  2.0,  20.0, 1.6,  1.52, 26.16, 12.27, 3.0}},
         {90, {-8.33, 0.7, -2.0, 1.4, 0.32, 1.2,  1.42, 0.6,  -4.55, 4.27,  0.0,
               0.0,   2.3, 7.0,  3.0, 2.0,  20.0, 1.6,  5.36, 25.83, 12.75, 3.0}},
     }},
    {"Ka",
     {
         {10, {-7.24, 1.26, -1.58, 0.89, 0.13, 2.99, -1.13, 2.66, -2.87, 2.76, 0.0,
               0.0,   2.3,  7.0,   3.0,  3.0,  20.0, 1.6,   0.08, 14.72, 1.57, 3.0}},
         {20, {-7.7, 0.99, -1.67, 0.89, 0.19, 3.12, 0.49, 2.03, -2.68, 2.76, 0.0,
               0.0,  2.3,  7.0,   3.0,  3.0,  20.0, 1.6,  0.1,  14.62, 4.3,  3.0}},
         {30, {-7.82, 0.86, -1.84, 1.3, 0.44, 2.69, 0.95, 1.54, -2.12, 1.54, 0.0,
               0.0,   2.3,  7.0,   3.0, 3.0,  20.0, 1.6,  0.14, 16.4,  6.64, 3.0}},
         {40, {-8.04, 0.75, -2.02, 1.15, 0.48, 2.45, 1.15, 1.02, -2.27, 1.77, 0.0,
               0.0,   2.3,  7.0,   3.0,  3.0,  20.0, 1.6,  0.22, 17.86, 9.21, 3.0}},
         {50, {-8.08, 0.77, -2.06, 1.23, 0.56, 2.17, 1.14, 1.61, -2.5,  2.36,  0.0,
               0.0,   2.3,  7.0,   3.0,  3.0,  20.0, 1.6,  0.31, 19.74, 10.32, 3.0}},
         {60, {-8.1, 0.76, -1.99, 1.02, 0.55, 1.93, 1.13, 1.84, -2.47, 2.33, 0.0,
               0.0,  2.3,  7.0,   3.0,  3.0,  20.0, 1.6,  0.49, 19.73, 10.3, 3.0}},
         {70, {-8.16, 0.73, -2.19, 1.78, 0.48, 1.72, 1.16, 1.81, -2.83, 2.84, 0.0,
               0.0,   2.3,  7.0,   3.0,  2.0,  20.0, 1.6,  0.97, 20.5,  10.2, 3.0}},
         {80, {-8.03, 0.79, -1.88, 1.55, 0.53, 1.51, 1.28, 1.35, -2.82, 2.87,  0.0,
               0.0,   2.3,  7.0,   3.0,  2.0,  20.0, 1.6,  1.52, 26.16, 12.27, 3.0}},
         {90, {-8.33, 0.7, -2.0, 1.4, 0.32, 1.2,  1.42, 0.6,  -4.55, 4.27,  0.0,
               0.0,   2.3, 7.0,  3.0, 2.0,  20.0, 1.6,  5.36, 25.83, 12.75, 3.0}},
     }},
};

/**
 * The nested map containing the 3GPP value tables for the NTN Suburban LOS scenario.
 * The outer key specifies the band (either "S" or "Ka"), while the inner key represents
 * the quantized elevation angle.
 * The inner vector collects the table3gpp values.
 */
static const std::map<std::string, std::map<int, std::array<float, 22>>> NTNSuburbanLOS{
    {"S",
     {
         {10, {-8.16, 0.99, -3.57, 1.62, 0.05, 1.84, -1.78, 0.62, -1.06, 0.96, 11.4,
               6.26,  2.2,  21.3,  7.6,  3.0,  20.0, 1.6,   0.0,  11.0,  7.0,  3.0}},
         {20, {-8.56, 0.96, -3.8, 1.74, -0.38, 1.94, -1.84, 0.81, -1.21, 0.95, 19.45,
               10.32, 3.36, 21.0, 8.9,  3.0,   20.0, 1.6,   0.0,  11.0,  7.0,  3.0}},
         {30, {-8.72, 0.79, -3.77, 1.72, -0.56, 1.75, -1.67, 0.57, -1.28, 0.49, 20.8,
               16.34, 3.5,  21.2,  8.5,  3.0,   20.0, 1.6,   0.0,  11.0,  7.0,  3.0}},
         {40, {-8.71, 0.81, -3.57, 1.6, -0.59, 1.82, -1.59, 0.86, -1.32, 0.79, 21.2,
               15.63, 2.81, 21.1,  8.4, 3.0,   20.0, 1.6,   0.0,  11.0,  7.0,  3.0}},
         {50, {-8.72, 1.12, -3.42, 1.49, -0.58, 1.87, -1.55, 1.05, -1.39, 0.97, 21.6,
               14.22, 2.39, 20.7,  9.2,  3.0,   20.0, 1.6,   0.0,  11.0,  7.0,  3.0}},
         {60, {-8.66, 1.23, -3.27, 1.43, -0.55, 1.92, -1.51, 1.23, -1.36, 1.17, 19.75,
               14.19, 2.73, 20.6,  9.8,  3.0,   20.0, 1.6,   0.0,  11.0,  7.0,  3.0}},
         {70, {-8.38, 0.55, -3.08, 1.36, -0.28, 1.16, -1.27, 0.54, -1.08, 0.62, 12.0,
               5.7,   2.07, 20.3,  10.8, 2.0,   20.0, 1.6,   0.0,  11.0,  7.0,  3.0}},
         {80, {-8.34, 0.63, -2.75, 1.26, -0.17, 1.09, -1.28, 0.67, -1.31, 0.76, 12.85,
               9.91,  2.04, 19.8,  12.2, 2.0,   20.0, 1.6,   0.0,  11.0,  7.0,  3.0}},
         {90, {-8.34, 0.63, -2.75, 1.26, -0.17, 1.09, -1.28, 0.67, -1.31, 0.76, 12.85,
               9.91,  2.04, 19.1,  13.0, 2.0,   20.0, 1.6,   0.0,  11.0,  7.0,  3.0}},
     }},
    {"Ka",
     {
         {10, {-8.07, 0.46, -3.55, 0.48, 0.89, 0.67, 0.63, 0.35, -3.37, 0.28, 8.9,
               4.4,   2.5,  23.2,  5.0,  3.0,  20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
         {20, {-8.61, 0.45, -3.69, 0.41, 0.31, 0.78, 0.76, 0.3, -3.28, 0.27, 14.0,
               4.6,   2.5,  23.6,  4.5,  3.0,  20.0, 1.6,  0.0, 11.0,  7.0,  3.0}},
         {30, {-8.72, 0.28, -3.59, 0.41, 0.02, 0.75, 1.11, 0.28, -3.04, 0.26, 11.3,
               3.7,   2.5,  23.5,  4.7,  3.0,  20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
         {40, {-8.63, 0.17, -3.38, 0.35, -0.1, 0.65, 1.37, 0.23, -2.88, 0.21, 9.0,
               3.5,   2.5,  23.4,  5.2,  3.0,  20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
         {50, {-8.54, 0.14, -3.23, 0.35, -0.19, 0.55, 1.53, 0.23, -2.83, 0.18, 7.5,
               3.0,   2.5,  23.2,  5.7,  3.0,   20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
         {60, {-8.48, 0.15, -3.19, 0.43, -0.54, 0.96, 1.65, 0.17, -2.86, 0.17, 6.6,
               2.6,   2.5,  23.3,  5.9,  3.0,   20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
         {70, {-8.42, 0.09, -2.83, 0.33, -0.24, 0.43, 1.74, 0.11, -2.95, 0.1, 5.9,
               1.7,   2.5,  23.4,  6.2,  2.0,   20.0, 1.6,  0.0,  11.0,  7.0, 3.0}},
         {80, {-8.39, 0.05, -2.66, 0.44, -0.52, 0.93, 1.82, 0.05, -3.21, 0.07, 5.5,
               0.7,   2.5,  23.2,  7.0,  2.0,   20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
         {90, {-8.37, 0.02, -1.22, 0.31, -0.15, 0.44, 1.87, 0.02, -3.49, 0.24, 5.4,
               0.3,   2.5,  23.1,  7.6,  2.0,   20.0, 1.6,  0.0,  11.0,  7.0,  3.0}},
     }},
};

/**
 * The nested map containing the 3GPP value tables for the NTN Suburban NLOS scenario.
 * The outer key specifies the band (either "S" or "Ka"), while the inner key represents
 * the quantized elevation angle.
 * The inner vector collects the table3gpp values.
 */
static const std::map<std::string, std::map<int, std::array<float, 22>>> NTNSuburbanNLOS{
    {"S",
     {
         {10, {-7.43, 0.5, -2.89, 0.41, 1.49, 0.4,  0.81, 0.36, -3.09, 0.32, 0.0,
               0.0,   2.3, 22.5,  5.0,  4.0,  20.0, 1.6,  0.0,  15.0,  7.0,  3.0}},
         {20, {-7.63, 0.61, -2.76, 0.41, 1.24, 0.82, 1.06, 0.41, -2.93, 0.47, 0.0,
               0.0,   2.3,  19.4,  8.5,  4.0,  20.0, 1.6,  0.0,  15.0,  7.0,  3.0}},
         {30, {-7.86, 0.56, -2.64, 0.41, 1.06, 0.71, 1.12, 0.4, -2.91, 0.46, 0.0,
               0.0,   2.3,  15.5,  10.0, 4.0,  20.0, 1.6,  0.0, 15.0,  7.0,  3.0}},
         {40, {-7.96, 0.58, -2.41, 0.52, 0.91, 0.55, 1.14, 0.39, -2.78, 0.54, 0.0,
               0.0,   2.3,  13.9,  10.6, 4.0,  20.0, 1.6,  0.0,  15.0,  7.0,  3.0}},
         {50, {-7.98, 0.59, -2.42, 0.7,  0.98, 0.58, 1.29, 0.35, -2.7, 0.45, 0.0,
               0.0,   2.3,  11.7,  10.0, 4.0,  20.0, 1.6,  0.0,  15.0, 7.0,  3.0}},
         {60, {-8.45, 0.47, -2.53, 0.5, 0.49, 1.37, 1.38, 0.36, -3.03, 0.36, 0.0,
               0.0,   2.3,  9.8,   9.1, 3.0,  20.0, 1.6,  0.0,  15.0,  7.0,  3.0}},
         {70, {-8.21, 0.36, -2.35, 0.58, 0.73, 0.49, 1.36, 0.29, -2.9, 0.42, 0.0,
               0.0,   2.3,  10.3,  9.1,  3.0,  20.0, 1.6,  0.0,  15.0, 7.0,  3.0}},
         {80, {-8.69, 0.29, -2.31, 0.73, -0.04, 1.48, 1.38, 0.2, -3.2, 0.3, 0.0,
               0.0,   2.3,  15.6,  9.1,  3.0,   20.0, 1.6,  0.0, 15.0, 7.0, 3.0}},
         {90, {-8.69, 0.29, -2.31, 0.73, -0.04, 1.48, 1.38, 0.2, -3.2, 0.3, 0.0,
               0.0,   2.3,  15.6,  9.1,  3.0,   20.0, 1.6,  0.0, 15.0, 7.0, 3.0}},
     }},
    {"Ka",
     {
         {10, {-7.43, 0.5, -2.89, 0.41, 1.49, 0.4,  0.81, 0.36, -3.09, 0.32, 0.0,
               0.0,   2.3, 22.5,  5.0,  4.0,  20.0, 1.6,  0.0,  15.0,  7.0,  3.0}},
         {20, {-7.63, 0.61, -2.76, 0.41, 1.24, 0.82, 1.06, 0.41, -2.93, 0.47, 0.0,
               0.0,   2.3,  19.4,  8.5,  4.0,  20.0, 1.6,  0.0,  15.0,  7.0,  3.0}},
         {30, {-7.86, 0.56, -2.64, 0.41, 1.06, 0.71, 1.12, 0.4, -2.91, 0.46, 0.0,
               0.0,   2.3,  15.5,  10.0, 4.0,  20.0, 1.6,  0.0, 15.0,  7.0,  3.0}},
         {40, {-7.96, 0.58, -2.41, 0.52, 0.91, 0.55, 1.14, 0.39, -2.78, 0.54, 0.0,
               0.0,   2.3,  13.9,  10.6, 4.0,  20.0, 1.6,  0.0,  15.0,  7.0,  3.0}},
         {50, {-7.98, 0.59, -2.42, 0.7,  0.98, 0.58, 1.29, 0.35, -2.7, 0.45, 0.0,
               0.0,   2.3,  11.7,  10.0, 4.0,  20.0, 1.6,  0.0,  15.0, 7.0,  3.0}},
         {60, {-8.45, 0.47, -2.53, 0.5, 0.49, 1.37, 1.38, 0.36, -3.03, 0.36, 0.0,
               0.0,   2.3,  9.8,   9.1, 3.0,  20.0, 1.6,  0.0,  15.0,  7.0,  3.0}},
         {70, {-8.21, 0.36, -2.35, 0.58, 0.73, 0.49, 1.36, 0.29, -2.9, 0.42, 0.0,
               0.0,   2.3,  10.3,  9.1,  3.0,  20.0, 1.6,  0.0,  15.0, 7.0,  3.0}},
         {80, {-8.69, 0.29, -2.31, 0.73, -0.04, 1.48, 1.38, 0.2, -3.2, 0.3, 0.0,
               0.0,   2.3,  15.6,  9.1,  3.0,   20.0, 1.6,  0.0, 15.0, 7.0, 3.0}},
         {90, {-8.69, 0.29, -2.31, 0.73, -0.04, 1.48, 1.38, 0.2, -3.2, 0.3, 0.0,
               0.0,   2.3,  15.6,  9.1,  3.0,   20.0, 1.6,  0.0, 15.0, 7.0, 3.0}},
     }},
};

/**
 * The nested map containing the 3GPP value tables for the NTN Rural LOS scenario.
 * The outer key specifies the band (either "S" or "Ka"), while the inner key represents
 * the quantized elevation angle.
 * The inner vector collects the table3gpp values.
 */
static const std::map<std::string, std::map<int, std::array<float, 22>>> NTNRuralLOS{
    {"S",
     {
         {10, {-9.55, 0.66, -3.42, 0.89, -9.45, 7.83, -4.2, 6.3,  -6.03, 5.19, 24.72,
               5.07,  3.8,  12.0,  4.0,  2.0,   20.0, 0.0,  0.39, 10.81, 1.94, 3.0}},
         {20, {-8.68, 0.44, -3.0, 0.63, -4.45, 6.86, -2.31, 5.04, -4.31, 4.18, 12.31,
               5.75,  3.8,  12.0, 4.0,  2.0,   20.0, 0.0,   0.31, 8.09,  1.83, 3.0}},
         {30, {-8.46, 0.28, -2.86, 0.52, -2.39, 5.14, -0.28, 0.81, -2.57, 0.61, 8.05,
               5.46,  3.8,  12.0,  4.0,  2.0,   20.0, 0.0,   0.29, 13.7,  2.28, 3.0}},
         {40, {-8.36, 0.19, -2.78, 0.45, -1.28, 3.44, -0.38, 1.16, -2.59, 0.79, 6.21,
               5.23,  3.8,  12.0,  4.0,  2.0,   20.0, 0.0,   0.37, 20.05, 2.93, 3.0}},
         {50, {-8.29, 0.14, -2.7, 0.42, -0.99, 2.59, -0.38, 0.82, -2.59, 0.65, 5.04,
               3.95,  3.8,  12.0, 4.0,  2.0,   20.0, 0.0,   0.61, 24.51, 2.84, 3.0}},
         {60, {-8.26, 0.1, -2.66, 0.41, -1.05, 2.42, -0.46, 0.67, -2.65, 0.52, 4.42,
               3.75,  3.8, 12.0,  4.0,  2.0,   20.0, 0.0,   0.9,  26.35, 3.17, 3.0}},
         {70, {-8.22, 0.1, -2.53, 0.42, -0.9, 1.78, -0.49, 1.0,  -2.69, 0.78, 3.92,
               2.56,  3.8, 12.0,  4.0,  2.0,  20.0, 0.0,   1.43, 31.84, 3.88, 3.0}},
         {80, {-8.2, 0.05, -2.21, 0.5, -0.89, 1.65, -0.53, 1.18, -2.65, 1.01, 3.65,
               1.77, 3.8,  12.0,  4.0, 2.0,   20.0, 0.0,   2.87, 36.62, 4.17, 3.0}},
         {90, {-8.19, 0.06, -1.78, 0.91, -0.81, 1.26, -0.46, 0.91, -2.65, 0.71, 3.59,
               1.77,  3.8,  12.0,  4.0,  2.0,   20.0, 0.0,   5.48, 36.77, 4.29, 3.0}},
     }},
    {"Ka",
     {
         {10, {-9.68, 0.46, -4.03, 0.91, -9.74, 7.52, -5.85, 6.51, -7.45, 5.3,  25.43,
               7.04,  3.8,  12.0,  4.0,  2.0,   20.0, 0.0,   0.36, 4.63,  0.75, 3.0}},
         {20, {-8.86, 0.29, -3.55, 0.7, -4.88, 6.67, -3.27, 5.36, -5.25, 4.42, 12.72,
               7.47,  3.8,  12.0,  4.0, 2.0,   20.0, 0.0,   0.3,  6.83,  1.25, 3.0}},
         {30, {-8.59, 0.18, -3.45, 0.55, -2.6, 4.63, -0.88, 0.93, -3.16, 0.68, 8.4,
               7.18,  3.8,  12.0,  4.0,  2.0,  20.0, 0.0,   0.25, 12.91, 1.93, 3.0}},
         {40, {-8.46, 0.19, -3.38, 0.52, -1.92, 3.45, -0.93, 0.96, -3.15, 0.73, 6.52,
               6.88,  3.8,  12.0,  4.0,  2.0,   20.0, 0.0,   0.35, 18.9,  2.37, 3.0}},
         {50, {-8.36, 0.14, -3.33, 0.46, -1.56, 2.44, -0.99, 0.97, -3.2,  0.77, 5.24,
               5.28,  3.8,  12.0,  4.0,  2.0,   20.0, 0.0,   0.53, 22.44, 2.66, 3.0}},
         {60, {-8.3, 0.15, -3.29, 0.43, -1.66, 2.38, -1.04, 0.83, -3.27, 0.61, 4.57,
               4.92, 3.8,  12.0,  4.0,  2.0,   20.0, 0.0,   0.88, 25.69, 3.23, 3.0}},
         {70, {-8.26, 0.13, -3.24, 0.46, -1.59, 1.67, -1.17, 1.01, -3.42, 0.74, 4.02,
               3.4,   3.8,  12.0,  4.0,  2.0,   20.0, 0.0,   1.39, 27.95, 3.71, 3.0}},
         {80, {-8.22, 0.03, -2.9, 0.44, -1.58, 1.44, -1.19, 1.01, -3.36, 0.79, 3.7,
               2.22,  3.8,  12.0, 4.0,  2.0,   20.0, 0.0,   2.7,  31.45, 4.17, 3.0}},
         {90, {-8.21, 0.07, -2.5, 0.82, -1.51, 1.13, -1.13, 0.85, -3.35, 0.65, 3.62,
               2.28,  3.8,  12.0, 4.0,  2.0,   20.0, 0.0,   4.97, 28.01, 4.14, 3.0}},
     }},
};

/**
 * The nested map containing the 3GPP value tables for the NTN Rural NLOS scenario
 * The outer key specifies the band (either "S" or "Ka"), while the inner key represents
 * the quantized elevation angle.
 * The inner vector collects the table3gpp values.
 */
static const std::map<std::string, std::map<int, std::array<float, 22>>> NTNRuralNLOS{
    {"S",
     {
         {10, {-9.01, 1.59, -2.9, 1.34, -3.33, 6.22, -0.88, 3.26, -4.92, 3.96, 0.0,
               0.0,   1.7,  7.0,  3.0,  3.0,   20.0, 0.0,   0.03, 18.16, 2.32, 3.0}},
         {20, {-8.37, 0.95, -2.5, 1.18, -0.74, 4.22, -0.07, 3.29, -4.06, 4.07, 0.0,
               0.0,   1.7,  7.0,  3.0,  3.0,   20.0, 0.0,   0.05, 26.82, 7.34, 3.0}},
         {30, {-8.05, 0.92, -2.12, 1.08, 0.08, 3.02, 0.75, 1.92, -2.33, 1.7,  0.0,
               0.0,   1.7,  7.0,   3.0,  2.0,  20.0, 0.0,  0.07, 21.99, 8.28, 3.0}},
         {40, {-7.92, 0.92, -1.99, 1.06, 0.32, 2.45, 0.72, 1.92, -2.24, 2.01, 0.0,
               0.0,   1.7,  7.0,   3.0,  2.0,  20.0, 0.0,  0.1,  22.86, 8.76, 3.0}},
         {50, {-7.92, 0.87, -1.9, 1.05, 0.53, 1.63, 0.95, 1.45, -2.24, 2.0,  0.0,
               0.0,   1.7,  7.0,  3.0,  2.0,  20.0, 0.0,  0.15, 25.93, 9.68, 3.0}},
         {60, {-7.96, 0.87, -1.85, 1.06, 0.33, 2.08, 0.97, 1.62, -2.22, 1.82, 0.0,
               0.0,   1.7,  7.0,   3.0,  2.0,  20.0, 0.0,  0.22, 27.79, 9.94, 3.0}},
         {70, {-7.91, 0.82, -1.69, 1.14, 0.55, 1.58, 1.1, 1.43, -2.19, 1.66, 0.0,
               0.0,   1.7,  7.0,   3.0,  2.0,  20.0, 0.0, 0.5,  28.5,  8.9,  3.0}},
         {80, {-7.79, 0.86, -1.46, 1.16, 0.45, 2.01, 0.97, 1.88, -2.41, 2.58,  0.0,
               0.0,   1.7,  7.0,   3.0,  2.0,  20.0, 0.0,  1.04, 37.53, 13.74, 3.0}},
         {90, {-7.74, 0.81, -1.32, 1.3, 0.4, 2.19, 1.35, 0.62, -2.45, 2.52,  0.0,
               0.0,   1.7,  7.0,   3.0, 2.0, 20.0, 0.0,  2.11, 29.23, 12.16, 3.0}},
     }},
    {"Ka",
     {
         {10, {-9.13, 1.91, -2.9, 1.32, -3.4, 6.28, -1.19, 3.81, -5.47, 4.39, 0.0,
               0.0,   1.7,  7.0,  3.0,  3.0,  20.0, 0.0,   0.03, 18.21, 2.13, 3.0}},
         {20, {-8.39, 0.94, -2.53, 1.18, -0.51, 3.75, -0.11, 3.33, -4.06, 4.04, 0.0,
               0.0,   1.7,  7.0,   3.0,  3.0,   20.0, 0.0,   0.05, 24.08, 6.52, 3.0}},
         {30, {-8.1, 0.92, -2.16, 1.08, 0.06, 2.95, 0.72, 1.93, -2.32, 1.54, 0.0,
               0.0,  1.7,  7.0,   3.0,  2.0,  20.0, 0.0,  0.07, 22.06, 7.72, 3.0}},
         {40, {-7.96, 0.94, -2.04, 1.09, 0.2, 2.65, 0.69, 1.91, -2.19, 1.73, 0.0,
               0.0,   1.7,  7.0,   3.0,  2.0, 20.0, 0.0,  0.09, 21.4,  8.45, 3.0}},
         {50, {-7.99, 0.89, -1.99, 1.08, 0.4, 1.85, 0.84, 1.7,  -2.16, 1.5,  0.0,
               0.0,   1.7,  7.0,   3.0,  2.0, 20.0, 0.0,  0.16, 24.26, 8.92, 3.0}},
         {60, {-8.05, 0.87, -1.95, 1.06, 0.32, 1.83, 0.99, 1.27, -2.24, 1.64, 0.0,
               0.0,   1.7,  7.0,   3.0,  2.0,  20.0, 0.0,  0.22, 24.15, 8.76, 3.0}},
         {70, {-8.01, 0.82, -1.81, 1.17, 0.46, 1.57, 0.95, 1.86, -2.29, 1.66, 0.0,
               0.0,   1.7,  7.0,   3.0,  2.0,  20.0, 0.0,  0.51, 25.99, 9.0,  3.0}},
         {80, {-8.05, 1.65, -1.56, 1.2, 0.33, 1.99, 0.92, 1.84, -2.65, 2.86, 0.0,
               0.0,   1.7,  7.0,   3.0, 2.0,  20.0, 0.0,  0.89, 36.07, 13.6, 3.0}},
         {90, {-7.91, 0.76, -1.53, 1.27, 0.24, 2.18, 1.29, 0.59, -2.23, 1.12,  0.0,
               0.0,   1.7,  7.0,   3.0,  2.0,  20.0, 0.0,  1.68, 24.51, 10.56, 3.0}},
     }},
};

ThreeGppChannelModel::ThreeGppChannelModel()
{
    NS_LOG_FUNCTION(this);
    m_uniformRv = CreateObject<UniformRandomVariable>();
    m_uniformRvShuffle = CreateObject<UniformRandomVariable>();
    m_uniformRvDoppler = CreateObject<UniformRandomVariable>();

    m_normalRv = CreateObject<NormalRandomVariable>();
    m_normalRv->SetAttribute("Mean", DoubleValue(0.0));
    m_normalRv->SetAttribute("Variance", DoubleValue(1.0));
}

ThreeGppChannelModel::~ThreeGppChannelModel()
{
    NS_LOG_FUNCTION(this);
}

void
ThreeGppChannelModel::DoDispose()
{
    NS_LOG_FUNCTION(this);
    if (m_channelConditionModel)
    {
        m_channelConditionModel->Dispose();
    }
    m_channelMatrixMap.clear();
    m_channelParamsMap.clear();
    m_channelConditionModel = nullptr;
}

TypeId
ThreeGppChannelModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ThreeGppChannelModel")
            .SetGroupName("Spectrum")
            .SetParent<MatrixBasedChannelModel>()
            .AddConstructor<ThreeGppChannelModel>()
            .AddAttribute("Frequency",
                          "The operating Frequency in Hz",
                          DoubleValue(500.0e6),
                          MakeDoubleAccessor(&ThreeGppChannelModel::SetFrequency,
                                             &ThreeGppChannelModel::GetFrequency),
                          MakeDoubleChecker<double>())
            .AddAttribute(
                "Scenario",
                "The 3GPP scenario (RMa, UMa, UMi-StreetCanyon, InH-OfficeOpen, InH-OfficeMixed, "
                "NTN-DenseUrban, NTN-Urban, NTN-Suburban, NTN-Rural)",
                StringValue("UMa"),
                MakeStringAccessor(&ThreeGppChannelModel::SetScenario,
                                   &ThreeGppChannelModel::GetScenario),
                MakeStringChecker())
            .AddAttribute("ChannelConditionModel",
                          "Pointer to the channel condition model",
                          PointerValue(),
                          MakePointerAccessor(&ThreeGppChannelModel::SetChannelConditionModel,
                                              &ThreeGppChannelModel::GetChannelConditionModel),
                          MakePointerChecker<ChannelConditionModel>())
            .AddAttribute("UpdatePeriod",
                          "Specify the channel coherence time",
                          TimeValue(MilliSeconds(0)),
                          MakeTimeAccessor(&ThreeGppChannelModel::m_updatePeriod),
                          MakeTimeChecker())
            // attributes for the blockage model
            .AddAttribute("Blockage",
                          "Enable blockage model A (sec 7.6.4.1)",
                          BooleanValue(false),
                          MakeBooleanAccessor(&ThreeGppChannelModel::m_blockage),
                          MakeBooleanChecker())
            .AddAttribute("NumNonselfBlocking",
                          "number of non-self-blocking regions",
                          IntegerValue(4),
                          MakeIntegerAccessor(&ThreeGppChannelModel::m_numNonSelfBlocking),
                          MakeIntegerChecker<uint16_t>())
            .AddAttribute("PortraitMode",
                          "true for portrait mode, false for landscape mode",
                          BooleanValue(true),
                          MakeBooleanAccessor(&ThreeGppChannelModel::m_portraitMode),
                          MakeBooleanChecker())
            .AddAttribute("BlockerSpeed",
                          "The speed of moving blockers, the unit is m/s",
                          DoubleValue(1),
                          MakeDoubleAccessor(&ThreeGppChannelModel::m_blockerSpeed),
                          MakeDoubleChecker<double>())
            .AddAttribute("vScatt",
                          "Maximum speed of the vehicle in the layout (see 3GPP TR 37.885 v15.3.0, "
                          "Sec. 6.2.3)."
                          "Used to compute the additional contribution for the Doppler of"
                          "delayed (reflected) paths",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&ThreeGppChannelModel::m_vScatt),
                          MakeDoubleChecker<double>(0.0))

        ;
    return tid;
}

void
ThreeGppChannelModel::SetChannelConditionModel(Ptr<ChannelConditionModel> model)
{
    NS_LOG_FUNCTION(this);
    m_channelConditionModel = model;
}

Ptr<ChannelConditionModel>
ThreeGppChannelModel::GetChannelConditionModel() const
{
    NS_LOG_FUNCTION(this);
    return m_channelConditionModel;
}

void
ThreeGppChannelModel::SetFrequency(double f)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(f >= 500.0e6 && f <= 100.0e9,
                  "Frequency should be between 0.5 and 100 GHz but is " << f);
    m_frequency = f;
}

double
ThreeGppChannelModel::GetFrequency() const
{
    NS_LOG_FUNCTION(this);
    return m_frequency;
}

void
ThreeGppChannelModel::SetScenario(const std::string& scenario)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(scenario == "RMa" || scenario == "UMa" || scenario == "UMi-StreetCanyon" ||
                      scenario == "InH-OfficeOpen" || scenario == "InH-OfficeMixed" ||
                      scenario == "V2V-Urban" || scenario == "V2V-Highway" ||
                      scenario == "NTN-DenseUrban" || scenario == "NTN-Urban" ||
                      scenario == "NTN-Suburban" || scenario == "NTN-Rural",
                  "Unknown scenario, choose between: RMa, UMa, UMi-StreetCanyon, "
                  "InH-OfficeOpen, InH-OfficeMixed, V2V-Urban, V2V-Highway, "
                  "NTN-DenseUrban, NTN-Urban, NTN-Suburban or NTN-Rural");
    m_scenario = scenario;
}

std::string
ThreeGppChannelModel::GetScenario() const
{
    NS_LOG_FUNCTION(this);
    return m_scenario;
}

Ptr<const ThreeGppChannelModel::ParamsTable>
ThreeGppChannelModel::GetThreeGppTable(const Ptr<const MobilityModel> aMob,
                                       const Ptr<const MobilityModel> bMob,
                                       Ptr<const ChannelCondition> channelCondition) const
{
    NS_LOG_FUNCTION(this);

    // NOTE we assume hUT = min (height(a), height(b)) and
    // hBS = max (height (a), height (b))
    double hUT = std::min(aMob->GetPosition().z, bMob->GetPosition().z);
    double hBS = std::max(aMob->GetPosition().z, bMob->GetPosition().z);

    double distance2D = sqrt(pow(aMob->GetPosition().x - bMob->GetPosition().x, 2) +
                             pow(aMob->GetPosition().y - bMob->GetPosition().y, 2));

    double fcGHz = m_frequency / 1.0e9;
    Ptr<ParamsTable> table3gpp = Create<ParamsTable>();
    // table3gpp includes the following parameters:
    // numOfCluster, raysPerCluster, uLgDS, sigLgDS, uLgASD, sigLgASD,
    // uLgASA, sigLgASA, uLgZSA, sigLgZSA, uLgZSD, sigLgZSD, offsetZOD,
    // cDS, cASD, cASA, cZSA, uK, sigK, rTau, uXpr, sigXpr, shadowingStd

    bool los = channelCondition->IsLos();
    bool o2i = channelCondition->IsO2i();

    // In NLOS case, parameter uK and sigK are not used and they are set to 0
    if (m_scenario == "RMa")
    {
        if (los && !o2i)
        {
            // 3GPP mentioned that 3.91 ns should be used when the Cluster DS (cDS)
            // entry is N/A.
            table3gpp->m_numOfCluster = 11;
            table3gpp->m_raysPerCluster = 20;
            table3gpp->m_uLgDS = -7.49;
            table3gpp->m_sigLgDS = 0.55;
            table3gpp->m_uLgASD = 0.90;
            table3gpp->m_sigLgASD = 0.38;
            table3gpp->m_uLgASA = 1.52;
            table3gpp->m_sigLgASA = 0.24;
            table3gpp->m_uLgZSA = 0.47;
            table3gpp->m_sigLgZSA = 0.40;
            table3gpp->m_uLgZSD = 0.34;
            table3gpp->m_sigLgZSD =
                std::max(-1.0, -0.17 * (distance2D / 1000.0) - 0.01 * (hUT - 1.5) + 0.22);
            table3gpp->m_offsetZOD = 0;
            table3gpp->m_cDS = 3.91e-9;
            table3gpp->m_cASD = 2;
            table3gpp->m_cASA = 3;
            table3gpp->m_cZSA = 3;
            table3gpp->m_uK = 7;
            table3gpp->m_sigK = 4;
            table3gpp->m_rTau = 3.8;
            table3gpp->m_uXpr = 12;
            table3gpp->m_sigXpr = 4;
            table3gpp->m_perClusterShadowingStd = 3;

            for (uint8_t row = 0; row < 7; row++)
            {
                for (uint8_t column = 0; column < 7; column++)
                {
                    table3gpp->m_sqrtC[row][column] = sqrtC_RMa_LOS[row][column];
                }
            }
        }
        else if (!los && !o2i)
        {
            table3gpp->m_numOfCluster = 10;
            table3gpp->m_raysPerCluster = 20;
            table3gpp->m_uLgDS = -7.43;
            table3gpp->m_sigLgDS = 0.48;
            table3gpp->m_uLgASD = 0.95;
            table3gpp->m_sigLgASD = 0.45;
            table3gpp->m_uLgASA = 1.52;
            table3gpp->m_sigLgASA = 0.13;
            table3gpp->m_uLgZSA = 0.58;
            table3gpp->m_sigLgZSA = 0.37;
            table3gpp->m_uLgZSD =
                std::max(-1.0, -0.19 * (distance2D / 1000.0) - 0.01 * (hUT - 1.5) + 0.28);
            table3gpp->m_sigLgZSD = 0.30;
            table3gpp->m_offsetZOD = atan((35 - 3.5) / distance2D) - atan((35 - 1.5) / distance2D);
            table3gpp->m_cDS = 3.91e-9;
            table3gpp->m_cASD = 2;
            table3gpp->m_cASA = 3;
            table3gpp->m_cZSA = 3;
            table3gpp->m_uK = 0;
            table3gpp->m_sigK = 0;
            table3gpp->m_rTau = 1.7;
            table3gpp->m_uXpr = 7;
            table3gpp->m_sigXpr = 3;
            table3gpp->m_perClusterShadowingStd = 3;

            for (uint8_t row = 0; row < 6; row++)
            {
                for (uint8_t column = 0; column < 6; column++)
                {
                    table3gpp->m_sqrtC[row][column] = sqrtC_RMa_NLOS[row][column];
                }
            }
        }
        else // o2i
        {
            table3gpp->m_numOfCluster = 10;
            table3gpp->m_raysPerCluster = 20;
            table3gpp->m_uLgDS = -7.47;
            table3gpp->m_sigLgDS = 0.24;
            table3gpp->m_uLgASD = 0.67;
            table3gpp->m_sigLgASD = 0.18;
            table3gpp->m_uLgASA = 1.66;
            table3gpp->m_sigLgASA = 0.21;
            table3gpp->m_uLgZSA = 0.93;
            table3gpp->m_sigLgZSA = 0.22;
            table3gpp->m_uLgZSD =
                std::max(-1.0, -0.19 * (distance2D / 1000.0) - 0.01 * (hUT - 1.5) + 0.28);
            table3gpp->m_sigLgZSD = 0.30;
            table3gpp->m_offsetZOD = atan((35 - 3.5) / distance2D) - atan((35 - 1.5) / distance2D);
            table3gpp->m_cDS = 3.91e-9;
            table3gpp->m_cASD = 2;
            table3gpp->m_cASA = 3;
            table3gpp->m_cZSA = 3;
            table3gpp->m_uK = 0;
            table3gpp->m_sigK = 0;
            table3gpp->m_rTau = 1.7;
            table3gpp->m_uXpr = 7;
            table3gpp->m_sigXpr = 3;
            table3gpp->m_perClusterShadowingStd = 3;

            for (uint8_t row = 0; row < 6; row++)
            {
                for (uint8_t column = 0; column < 6; column++)
                {
                    table3gpp->m_sqrtC[row][column] = sqrtC_RMa_O2I[row][column];
                }
            }
        }
    }
    else if (m_scenario == "UMa")
    {
        if (los && !o2i)
        {
            table3gpp->m_numOfCluster = 12;
            table3gpp->m_raysPerCluster = 20;
            table3gpp->m_uLgDS = -6.955 - 0.0963 * log10(fcGHz);
            table3gpp->m_sigLgDS = 0.66;
            table3gpp->m_uLgASD = 1.06 + 0.1114 * log10(fcGHz);
            table3gpp->m_sigLgASD = 0.28;
            table3gpp->m_uLgASA = 1.81;
            table3gpp->m_sigLgASA = 0.20;
            table3gpp->m_uLgZSA = 0.95;
            table3gpp->m_sigLgZSA = 0.16;
            table3gpp->m_uLgZSD =
                std::max(-0.5, -2.1 * distance2D / 1000.0 - 0.01 * (hUT - 1.5) + 0.75);
            table3gpp->m_sigLgZSD = 0.40;
            table3gpp->m_offsetZOD = 0;
            table3gpp->m_cDS = std::max(0.25, -3.4084 * log10(fcGHz) + 6.5622) * 1e-9;
            table3gpp->m_cASD = 5;
            table3gpp->m_cASA = 11;
            table3gpp->m_cZSA = 7;
            table3gpp->m_uK = 9;
            table3gpp->m_sigK = 3.5;
            table3gpp->m_rTau = 2.5;
            table3gpp->m_uXpr = 8;
            table3gpp->m_sigXpr = 4;
            table3gpp->m_perClusterShadowingStd = 3;

            for (uint8_t row = 0; row < 7; row++)
            {
                for (uint8_t column = 0; column < 7; column++)
                {
                    table3gpp->m_sqrtC[row][column] = sqrtC_UMa_LOS[row][column];
                }
            }
        }
        else
        {
            double uLgZSD = std::max(-0.5, -2.1 * distance2D / 1000.0 - 0.01 * (hUT - 1.5) + 0.9);

            double afc = 0.208 * log10(fcGHz) - 0.782;
            double bfc = 25;
            double cfc = -0.13 * log10(fcGHz) + 2.03;
            double efc = 7.66 * log10(fcGHz) - 5.96;

            double offsetZOD = efc - std::pow(10, afc * log10(std::max(bfc, distance2D)) + cfc);

            if (!los && !o2i)
            {
                table3gpp->m_numOfCluster = 20;
                table3gpp->m_raysPerCluster = 20;
                table3gpp->m_uLgDS = -6.28 - 0.204 * log10(fcGHz);
                table3gpp->m_sigLgDS = 0.39;
                table3gpp->m_uLgASD = 1.5 - 0.1144 * log10(fcGHz);
                table3gpp->m_sigLgASD = 0.28;
                table3gpp->m_uLgASA = 2.08 - 0.27 * log10(fcGHz);
                table3gpp->m_sigLgASA = 0.11;
                table3gpp->m_uLgZSA = -0.3236 * log10(fcGHz) + 1.512;
                table3gpp->m_sigLgZSA = 0.16;
                table3gpp->m_uLgZSD = uLgZSD;
                table3gpp->m_sigLgZSD = 0.49;
                table3gpp->m_offsetZOD = offsetZOD;
                table3gpp->m_cDS = std::max(0.25, -3.4084 * log10(fcGHz) + 6.5622) * 1e-9;
                table3gpp->m_cASD = 2;
                table3gpp->m_cASA = 15;
                table3gpp->m_cZSA = 7;
                table3gpp->m_uK = 0;
                table3gpp->m_sigK = 0;
                table3gpp->m_rTau = 2.3;
                table3gpp->m_uXpr = 7;
                table3gpp->m_sigXpr = 3;
                table3gpp->m_perClusterShadowingStd = 3;

                for (uint8_t row = 0; row < 6; row++)
                {
                    for (uint8_t column = 0; column < 6; column++)
                    {
                        table3gpp->m_sqrtC[row][column] = sqrtC_UMa_NLOS[row][column];
                    }
                }
            }
            else //(o2i)
            {
                table3gpp->m_numOfCluster = 12;
                table3gpp->m_raysPerCluster = 20;
                table3gpp->m_uLgDS = -6.62;
                table3gpp->m_sigLgDS = 0.32;
                table3gpp->m_uLgASD = 1.25;
                table3gpp->m_sigLgASD = 0.42;
                table3gpp->m_uLgASA = 1.76;
                table3gpp->m_sigLgASA = 0.16;
                table3gpp->m_uLgZSA = 1.01;
                table3gpp->m_sigLgZSA = 0.43;
                table3gpp->m_uLgZSD = uLgZSD;
                table3gpp->m_sigLgZSD = 0.49;
                table3gpp->m_offsetZOD = offsetZOD;
                table3gpp->m_cDS = 11e-9;
                table3gpp->m_cASD = 5;
                table3gpp->m_cASA = 8;
                table3gpp->m_cZSA = 3;
                table3gpp->m_uK = 0;
                table3gpp->m_sigK = 0;
                table3gpp->m_rTau = 2.2;
                table3gpp->m_uXpr = 9;
                table3gpp->m_sigXpr = 5;
                table3gpp->m_perClusterShadowingStd = 4;

                for (uint8_t row = 0; row < 6; row++)
                {
                    for (uint8_t column = 0; column < 6; column++)
                    {
                        table3gpp->m_sqrtC[row][column] = sqrtC_UMa_O2I[row][column];
                    }
                }
            }
        }
    }
    else if (m_scenario == "UMi-StreetCanyon")
    {
        if (los && !o2i)
        {
            table3gpp->m_numOfCluster = 12;
            table3gpp->m_raysPerCluster = 20;
            table3gpp->m_uLgDS = -0.24 * log10(1 + fcGHz) - 7.14;
            table3gpp->m_sigLgDS = 0.38;
            table3gpp->m_uLgASD = -0.05 * log10(1 + fcGHz) + 1.21;
            table3gpp->m_sigLgASD = 0.41;
            table3gpp->m_uLgASA = -0.08 * log10(1 + fcGHz) + 1.73;
            table3gpp->m_sigLgASA = 0.014 * log10(1 + fcGHz) + 0.28;
            table3gpp->m_uLgZSA = -0.1 * log10(1 + fcGHz) + 0.73;
            table3gpp->m_sigLgZSA = -0.04 * log10(1 + fcGHz) + 0.34;
            table3gpp->m_uLgZSD =
                std::max(-0.21, -14.8 * distance2D / 1000.0 + 0.01 * std::abs(hUT - hBS) + 0.83);
            table3gpp->m_sigLgZSD = 0.35;
            table3gpp->m_offsetZOD = 0;
            table3gpp->m_cDS = 5e-9;
            table3gpp->m_cASD = 3;
            table3gpp->m_cASA = 17;
            table3gpp->m_cZSA = 7;
            table3gpp->m_uK = 9;
            table3gpp->m_sigK = 5;
            table3gpp->m_rTau = 3;
            table3gpp->m_uXpr = 9;
            table3gpp->m_sigXpr = 3;
            table3gpp->m_perClusterShadowingStd = 3;

            for (uint8_t row = 0; row < 7; row++)
            {
                for (uint8_t column = 0; column < 7; column++)
                {
                    table3gpp->m_sqrtC[row][column] = sqrtC_UMi_LOS[row][column];
                }
            }
        }
        else
        {
            double uLgZSD =
                std::max(-0.5, -3.1 * distance2D / 1000.0 + 0.01 * std::max(hUT - hBS, 0.0) + 0.2);
            double offsetZOD = -1 * std::pow(10, -1.5 * log10(std::max(10.0, distance2D)) + 3.3);
            if (!los && !o2i)
            {
                table3gpp->m_numOfCluster = 19;
                table3gpp->m_raysPerCluster = 20;
                table3gpp->m_uLgDS = -0.24 * log10(1 + fcGHz) - 6.83;
                table3gpp->m_sigLgDS = 0.16 * log10(1 + fcGHz) + 0.28;
                table3gpp->m_uLgASD = -0.23 * log10(1 + fcGHz) + 1.53;
                table3gpp->m_sigLgASD = 0.11 * log10(1 + fcGHz) + 0.33;
                table3gpp->m_uLgASA = -0.08 * log10(1 + fcGHz) + 1.81;
                table3gpp->m_sigLgASA = 0.05 * log10(1 + fcGHz) + 0.3;
                table3gpp->m_uLgZSA = -0.04 * log10(1 + fcGHz) + 0.92;
                table3gpp->m_sigLgZSA = -0.07 * log10(1 + fcGHz) + 0.41;
                table3gpp->m_uLgZSD = uLgZSD;
                table3gpp->m_sigLgZSD = 0.35;
                table3gpp->m_offsetZOD = offsetZOD;
                table3gpp->m_cDS = 11e-9;
                table3gpp->m_cASD = 10;
                table3gpp->m_cASA = 22;
                table3gpp->m_cZSA = 7;
                table3gpp->m_uK = 0;
                table3gpp->m_sigK = 0;
                table3gpp->m_rTau = 2.1;
                table3gpp->m_uXpr = 8;
                table3gpp->m_sigXpr = 3;
                table3gpp->m_perClusterShadowingStd = 3;

                for (uint8_t row = 0; row < 6; row++)
                {
                    for (uint8_t column = 0; column < 6; column++)
                    {
                        table3gpp->m_sqrtC[row][column] = sqrtC_UMi_NLOS[row][column];
                    }
                }
            }
            else //(o2i)
            {
                table3gpp->m_numOfCluster = 12;
                table3gpp->m_raysPerCluster = 20;
                table3gpp->m_uLgDS = -6.62;
                table3gpp->m_sigLgDS = 0.32;
                table3gpp->m_uLgASD = 1.25;
                table3gpp->m_sigLgASD = 0.42;
                table3gpp->m_uLgASA = 1.76;
                table3gpp->m_sigLgASA = 0.16;
                table3gpp->m_uLgZSA = 1.01;
                table3gpp->m_sigLgZSA = 0.43;
                table3gpp->m_uLgZSD = uLgZSD;
                table3gpp->m_sigLgZSD = 0.35;
                table3gpp->m_offsetZOD = offsetZOD;
                table3gpp->m_cDS = 11e-9;
                table3gpp->m_cASD = 5;
                table3gpp->m_cASA = 8;
                table3gpp->m_cZSA = 3;
                table3gpp->m_uK = 0;
                table3gpp->m_sigK = 0;
                table3gpp->m_rTau = 2.2;
                table3gpp->m_uXpr = 9;
                table3gpp->m_sigXpr = 5;
                table3gpp->m_perClusterShadowingStd = 4;

                for (uint8_t row = 0; row < 6; row++)
                {
                    for (uint8_t column = 0; column < 6; column++)
                    {
                        table3gpp->m_sqrtC[row][column] = sqrtC_UMi_O2I[row][column];
                    }
                }
            }
        }
    }
    else if (m_scenario == "InH-OfficeMixed" || m_scenario == "InH-OfficeOpen")
    {
        NS_ASSERT_MSG(!o2i, "The indoor scenario does out support outdoor to indoor");
        if (los)
        {
            table3gpp->m_numOfCluster = 15;
            table3gpp->m_raysPerCluster = 20;
            table3gpp->m_uLgDS = -0.01 * log10(1 + fcGHz) - 7.692;
            table3gpp->m_sigLgDS = 0.18;
            table3gpp->m_uLgASD = 1.60;
            table3gpp->m_sigLgASD = 0.18;
            table3gpp->m_uLgASA = -0.19 * log10(1 + fcGHz) + 1.781;
            table3gpp->m_sigLgASA = 0.12 * log10(1 + fcGHz) + 0.119;
            table3gpp->m_uLgZSA = -0.26 * log10(1 + fcGHz) + 1.44;
            table3gpp->m_sigLgZSA = -0.04 * log10(1 + fcGHz) + 0.264;
            table3gpp->m_uLgZSD = -1.43 * log10(1 + fcGHz) + 2.228;
            table3gpp->m_sigLgZSD = 0.13 * log10(1 + fcGHz) + 0.30;
            table3gpp->m_offsetZOD = 0;
            table3gpp->m_cDS = 3.91e-9;
            table3gpp->m_cASD = 5;
            table3gpp->m_cASA = 8;
            table3gpp->m_cZSA = 9;
            table3gpp->m_uK = 7;
            table3gpp->m_sigK = 4;
            table3gpp->m_rTau = 3.6;
            table3gpp->m_uXpr = 11;
            table3gpp->m_sigXpr = 4;
            table3gpp->m_perClusterShadowingStd = 6;

            for (uint8_t row = 0; row < 7; row++)
            {
                for (uint8_t column = 0; column < 7; column++)
                {
                    table3gpp->m_sqrtC[row][column] = sqrtC_office_LOS[row][column];
                }
            }
        }
        else
        {
            table3gpp->m_numOfCluster = 19;
            table3gpp->m_raysPerCluster = 20;
            table3gpp->m_uLgDS = -0.28 * log10(1 + fcGHz) - 7.173;
            table3gpp->m_sigLgDS = 0.1 * log10(1 + fcGHz) + 0.055;
            table3gpp->m_uLgASD = 1.62;
            table3gpp->m_sigLgASD = 0.25;
            table3gpp->m_uLgASA = -0.11 * log10(1 + fcGHz) + 1.863;
            table3gpp->m_sigLgASA = 0.12 * log10(1 + fcGHz) + 0.059;
            table3gpp->m_uLgZSA = -0.15 * log10(1 + fcGHz) + 1.387;
            table3gpp->m_sigLgZSA = -0.09 * log10(1 + fcGHz) + 0.746;
            table3gpp->m_uLgZSD = 1.08;
            table3gpp->m_sigLgZSD = 0.36;
            table3gpp->m_offsetZOD = 0;
            table3gpp->m_cDS = 3.91e-9;
            table3gpp->m_cASD = 5;
            table3gpp->m_cASA = 11;
            table3gpp->m_cZSA = 9;
            table3gpp->m_uK = 0;
            table3gpp->m_sigK = 0;
            table3gpp->m_rTau = 3;
            table3gpp->m_uXpr = 10;
            table3gpp->m_sigXpr = 4;
            table3gpp->m_perClusterShadowingStd = 3;

            for (uint8_t row = 0; row < 6; row++)
            {
                for (uint8_t column = 0; column < 6; column++)
                {
                    table3gpp->m_sqrtC[row][column] = sqrtC_office_NLOS[row][column];
                }
            }
        }
    }
    else if (m_scenario == "V2V-Urban")
    {
        if (channelCondition->IsLos())
        {
            // 3GPP mentioned that 3.91 ns should be used when the Cluster DS (cDS)
            // entry is N/A.
            table3gpp->m_numOfCluster = 12;
            table3gpp->m_raysPerCluster = 20;
            table3gpp->m_uLgDS = -0.2 * log10(1 + fcGHz) - 7.5;
            table3gpp->m_sigLgDS = 0.1;
            table3gpp->m_uLgASD = -0.1 * log10(1 + fcGHz) + 1.6;
            table3gpp->m_sigLgASD = 0.1;
            table3gpp->m_uLgASA = -0.1 * log10(1 + fcGHz) + 1.6;
            table3gpp->m_sigLgASA = 0.1;
            table3gpp->m_uLgZSA = -0.1 * log10(1 + fcGHz) + 0.73;
            table3gpp->m_sigLgZSA = -0.04 * log10(1 + fcGHz) + 0.34;
            table3gpp->m_uLgZSD = -0.1 * log10(1 + fcGHz) + 0.73;
            table3gpp->m_sigLgZSD = -0.04 * log10(1 + fcGHz) + 0.34;
            table3gpp->m_offsetZOD = 0;
            table3gpp->m_cDS = 5 * 1e-9;
            table3gpp->m_cASD = 17;
            table3gpp->m_cASA = 17;
            table3gpp->m_cZSA = 7;
            table3gpp->m_uK = 3.48;
            table3gpp->m_sigK = 2;
            table3gpp->m_rTau = 3;
            table3gpp->m_uXpr = 9;
            table3gpp->m_sigXpr = 3;
            table3gpp->m_perClusterShadowingStd = 4;

            for (uint8_t row = 0; row < 7; row++)
            {
                for (uint8_t column = 0; column < 7; column++)
                {
                    table3gpp->m_sqrtC[row][column] = sqrtC_UMi_LOS[row][column];
                }
            }
        }
        else if (channelCondition->IsNlos())
        {
            table3gpp->m_numOfCluster = 19;
            table3gpp->m_raysPerCluster = 20;
            table3gpp->m_uLgDS = -0.3 * log10(1 + fcGHz) - 7;
            table3gpp->m_sigLgDS = 0.28;
            table3gpp->m_uLgASD = -0.08 * log10(1 + fcGHz) + 1.81;
            table3gpp->m_sigLgASD = 0.05 * log10(1 + fcGHz) + 0.3;
            table3gpp->m_uLgASA = -0.08 * log10(1 + fcGHz) + 1.81;
            table3gpp->m_sigLgASA = 0.05 * log10(1 + fcGHz) + 0.3;
            table3gpp->m_uLgZSA = -0.04 * log10(1 + fcGHz) + 0.92;
            table3gpp->m_sigLgZSA = -0.07 * log10(1 + fcGHz) + 0.41;
            table3gpp->m_uLgZSD = -0.04 * log10(1 + fcGHz) + 0.92;
            table3gpp->m_sigLgZSD = -0.07 * log10(1 + fcGHz) + 0.41;
            table3gpp->m_offsetZOD = 0;
            table3gpp->m_cDS = 11 * 1e-9;
            table3gpp->m_cASD = 22;
            table3gpp->m_cASA = 22;
            table3gpp->m_cZSA = 7;
            table3gpp->m_uK = 0;   // N/A
            table3gpp->m_sigK = 0; // N/A
            table3gpp->m_rTau = 2.1;
            table3gpp->m_uXpr = 8;
            table3gpp->m_sigXpr = 3;
            table3gpp->m_perClusterShadowingStd = 4;

            for (uint8_t row = 0; row < 6; row++)
            {
                for (uint8_t column = 0; column < 6; column++)
                {
                    table3gpp->m_sqrtC[row][column] = sqrtC_UMi_NLOS[row][column];
                }
            }
        }
        else if (channelCondition->IsNlosv())
        {
            table3gpp->m_numOfCluster = 19;
            table3gpp->m_raysPerCluster = 20;
            table3gpp->m_uLgDS = -0.4 * log10(1 + fcGHz) - 7;
            table3gpp->m_sigLgDS = 0.1;
            table3gpp->m_uLgASD = -0.1 * log10(1 + fcGHz) + 1.7;
            table3gpp->m_sigLgASD = 0.1;
            table3gpp->m_uLgASA = -0.1 * log10(1 + fcGHz) + 1.7;
            table3gpp->m_sigLgASA = 0.1;
            table3gpp->m_uLgZSA = -0.04 * log10(1 + fcGHz) + 0.92;
            table3gpp->m_sigLgZSA = -0.07 * log10(1 + fcGHz) + 0.41;
            table3gpp->m_uLgZSD = -0.04 * log10(1 + fcGHz) + 0.92;
            table3gpp->m_sigLgZSD = -0.07 * log10(1 + fcGHz) + 0.41;
            table3gpp->m_offsetZOD = 0;
            table3gpp->m_cDS = 11 * 1e-9;
            table3gpp->m_cASD = 22;
            table3gpp->m_cASA = 22;
            table3gpp->m_cZSA = 7;
            table3gpp->m_uK = 0;
            table3gpp->m_sigK = 4.5;
            table3gpp->m_rTau = 2.1;
            table3gpp->m_uXpr = 8;
            table3gpp->m_sigXpr = 3;
            table3gpp->m_perClusterShadowingStd = 4;

            for (uint8_t row = 0; row < 6; row++)
            {
                for (uint8_t column = 0; column < 6; column++)
                {
                    table3gpp->m_sqrtC[row][column] = sqrtC_UMi_LOS[row][column];
                }
            }
        }
        else
        {
            NS_FATAL_ERROR("Unknown channel condition");
        }
    }
    else if (m_scenario == "V2V-Highway")
    {
        if (channelCondition->IsLos())
        {
            table3gpp->m_numOfCluster = 12;
            table3gpp->m_raysPerCluster = 20;
            table3gpp->m_uLgDS = -8.3;
            table3gpp->m_sigLgDS = 0.2;
            table3gpp->m_uLgASD = 1.4;
            table3gpp->m_sigLgASD = 0.1;
            table3gpp->m_uLgASA = 1.4;
            table3gpp->m_sigLgASA = 0.1;
            table3gpp->m_uLgZSA = -0.1 * log10(1 + fcGHz) + 0.73;
            table3gpp->m_sigLgZSA = -0.04 * log10(1 + fcGHz) + 0.34;
            table3gpp->m_uLgZSD = -0.1 * log10(1 + fcGHz) + 0.73;
            table3gpp->m_sigLgZSD = -0.04 * log10(1 + fcGHz) + 0.34;
            table3gpp->m_offsetZOD = 0;
            table3gpp->m_cDS = 5 * 1e-9;
            table3gpp->m_cASD = 17;
            table3gpp->m_cASA = 17;
            table3gpp->m_cZSA = 7;
            table3gpp->m_uK = 9;
            table3gpp->m_sigK = 3.5;
            table3gpp->m_rTau = 3;
            table3gpp->m_uXpr = 9;
            table3gpp->m_sigXpr = 3;
            table3gpp->m_perClusterShadowingStd = 4;

            for (uint8_t row = 0; row < 7; row++)
            {
                for (uint8_t column = 0; column < 7; column++)
                {
                    table3gpp->m_sqrtC[row][column] = sqrtC_UMi_LOS[row][column];
                }
            }
        }
        else if (channelCondition->IsNlosv())
        {
            table3gpp->m_numOfCluster = 19;
            table3gpp->m_raysPerCluster = 20;
            table3gpp->m_uLgDS = -8.3;
            table3gpp->m_sigLgDS = 0.3;
            table3gpp->m_uLgASD = 1.5;
            table3gpp->m_sigLgASD = 0.1;
            table3gpp->m_uLgASA = 1.5;
            table3gpp->m_sigLgASA = 0.1;
            table3gpp->m_uLgZSA = -0.04 * log10(1 + fcGHz) + 0.92;
            table3gpp->m_sigLgZSA = -0.07 * log10(1 + fcGHz) + 0.41;
            table3gpp->m_uLgZSD = -0.04 * log10(1 + fcGHz) + 0.92;
            table3gpp->m_sigLgZSD = -0.07 * log10(1 + fcGHz) + 0.41;
            table3gpp->m_offsetZOD = 0;
            table3gpp->m_cDS = 11 * 1e-9;
            table3gpp->m_cASD = 22;
            table3gpp->m_cASA = 22;
            table3gpp->m_cZSA = 7;
            table3gpp->m_uK = 0;
            table3gpp->m_sigK = 4.5;
            table3gpp->m_rTau = 2.1;
            table3gpp->m_uXpr = 8.0;
            table3gpp->m_sigXpr = 3;
            table3gpp->m_perClusterShadowingStd = 4;

            for (uint8_t row = 0; row < 6; row++)
            {
                for (uint8_t column = 0; column < 6; column++)
                {
                    table3gpp->m_sqrtC[row][column] = sqrtC_UMi_LOS[row][column];
                }
            }
        }
        else if (channelCondition->IsNlos())
        {
            NS_LOG_WARN(
                "The fast fading parameters for the NLOS condition in the Highway scenario are not "
                "defined in TR 37.885, use the ones defined in TDoc R1-1803671 instead");

            table3gpp->m_numOfCluster = 19;
            table3gpp->m_raysPerCluster = 20;
            table3gpp->m_uLgDS = -0.3 * log10(1 + fcGHz) - 7;
            table3gpp->m_sigLgDS = 0.28;
            table3gpp->m_uLgASD = -0.08 * log10(1 + fcGHz) + 1.81;
            table3gpp->m_sigLgASD = 0.05 * log10(1 + fcGHz) + 0.3;
            table3gpp->m_uLgASA = -0.08 * log10(1 + fcGHz) + 1.81;
            table3gpp->m_sigLgASA = 0.05 * log10(1 + fcGHz) + 0.3;
            table3gpp->m_uLgZSA = -0.04 * log10(1 + fcGHz) + 0.92;
            table3gpp->m_sigLgZSA = -0.07 * log10(1 + fcGHz) + 0.41;
            table3gpp->m_uLgZSD = -0.04 * log10(1 + fcGHz) + 0.92;
            table3gpp->m_sigLgZSD = -0.07 * log10(1 + fcGHz) + 0.41;
            table3gpp->m_offsetZOD = 0;
            table3gpp->m_cDS = 11 * 1e-9;
            table3gpp->m_cASD = 22;
            table3gpp->m_cASA = 22;
            table3gpp->m_cZSA = 7;
            table3gpp->m_uK = 0;   // N/A
            table3gpp->m_sigK = 0; // N/A
            table3gpp->m_rTau = 2.1;
            table3gpp->m_uXpr = 8;
            table3gpp->m_sigXpr = 3;
            table3gpp->m_perClusterShadowingStd = 4;

            for (uint8_t row = 0; row < 6; row++)
            {
                for (uint8_t column = 0; column < 6; column++)
                {
                    table3gpp->m_sqrtC[row][column] = sqrtC_UMi_NLOS[row][column];
                }
            }
        }
        else
        {
            NS_FATAL_ERROR("Unknown channel condition");
        }
    }
    else if (m_scenario.substr(0, 3) == "NTN")
    {
        std::string freqBand = (fcGHz < 13) ? "S" : "Ka";

        double elevAngle = 0;
        bool isSatellite = false; // flag to indicate if one of the two nodes is a satellite
                                  // if so, parameters will be set accordingly to NOTE 8 of
                                  // Table 6.7.2 from 3GPP 38.811 V15.4.0 (2020-09)

        Ptr<MobilityModel> aMobNonConst = ConstCast<MobilityModel>(aMob);
        Ptr<MobilityModel> bMobNonConst = ConstCast<MobilityModel>(bMob);

        if (DynamicCast<GeocentricConstantPositionMobilityModel>(
                ConstCast<MobilityModel>(aMob)) && // Transform to NS_ASSERT
            DynamicCast<GeocentricConstantPositionMobilityModel>(
                ConstCast<MobilityModel>(bMob))) // check if aMob and bMob are of type
                                                 // GeocentricConstantPositionMobilityModel
        {
            Ptr<GeocentricConstantPositionMobilityModel> aNTNMob =
                DynamicCast<GeocentricConstantPositionMobilityModel>(aMobNonConst);
            Ptr<GeocentricConstantPositionMobilityModel> bNTNMob =
                DynamicCast<GeocentricConstantPositionMobilityModel>(bMobNonConst);

            if (aNTNMob->GetGeographicPosition().z <
                bNTNMob->GetGeographicPosition().z) // b is the HAPS/Satellite
            {
                elevAngle = aNTNMob->GetElevationAngle(bNTNMob);
                if (bNTNMob->GetGeographicPosition().z > 50000)
                {
                    isSatellite = true;
                }
            }
            else // a is the HAPS/Satellite
            {
                elevAngle = bNTNMob->GetElevationAngle(aNTNMob);
                if (aNTNMob->GetGeographicPosition().z > 50000)
                {
                    isSatellite = true;
                }
            }
        }
        else
        {
            NS_FATAL_ERROR("Mobility Models needs to be of type Geocentric for NTN scenarios");
        }

        // Round the elevation angle into a two-digits integer between 10 and 90.
        int elevAngleQuantized = (elevAngle < 10) ? 10 : round(elevAngle / 10) * 10;

        if (m_scenario == "NTN-DenseUrban")
        {
            if (channelCondition->IsLos())
            {
                table3gpp->m_uLgDS =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgDS];
                table3gpp->m_sigLgDS =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgDS];

                // table3gpp->m_uLgASD=-1.79769e+308;  //FOR SATELLITES
                table3gpp->m_uLgASD =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASD];
                // table3gpp->m_sigLgASD=0; //FOR SATELLITES
                table3gpp->m_sigLgASD =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASD];

                table3gpp->m_uLgASA =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASA];
                table3gpp->m_sigLgASA =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASA];
                table3gpp->m_uLgZSA =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSA];
                table3gpp->m_sigLgZSA =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSA];

                // table3gpp->m_uLgZSD=-1.79769e+308;  //FOR SATELLITES
                table3gpp->m_uLgZSD =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSD];
                // table3gpp->m_sigLgZSD= 0; //FOR SATELLITES
                table3gpp->m_sigLgZSD =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSD];

                table3gpp->m_uK =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uK];
                table3gpp->m_sigK =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigK];
                table3gpp->m_rTau =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::rTau];
                table3gpp->m_uXpr =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uXpr];
                table3gpp->m_sigXpr =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigXpr];
                table3gpp->m_numOfCluster = NTNDenseUrbanLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::numOfCluster];
                table3gpp->m_raysPerCluster = NTNDenseUrbanLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::raysPerCluster];
                table3gpp->m_cDS =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cDS] *
                    1e-9;
                table3gpp->m_cASD =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASD];
                table3gpp->m_cASA =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASA];
                table3gpp->m_cZSA =
                    NTNDenseUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cZSA];
                table3gpp->m_perClusterShadowingStd = NTNDenseUrbanLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::perClusterShadowingStd];

                for (uint8_t row = 0; row < 7; row++)
                {
                    for (uint8_t column = 0; column < 7; column++)
                    {
                        table3gpp->m_sqrtC[row][column] = sqrtC_NTN_DenseUrban_LOS[row][column];
                    }
                }
            }
            else if (channelCondition->IsNlos())
            {
                NS_LOG_UNCOND("Dense Urban NLOS");
                table3gpp->m_uLgDS =
                    NTNDenseUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgDS];
                table3gpp->m_sigLgDS =
                    NTNDenseUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgDS];
                table3gpp->m_uLgASD =
                    NTNDenseUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASD];
                table3gpp->m_sigLgASD = NTNDenseUrbanNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::sigLgASD];
                table3gpp->m_uLgASA =
                    NTNDenseUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASA];
                table3gpp->m_sigLgASA = NTNDenseUrbanNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::sigLgASA];
                table3gpp->m_uLgZSA =
                    NTNDenseUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSA];
                table3gpp->m_sigLgZSA = NTNDenseUrbanNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::sigLgZSA];
                table3gpp->m_uLgZSD =
                    NTNDenseUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSD];
                table3gpp->m_sigLgZSD = NTNDenseUrbanNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::sigLgZSD];
                table3gpp->m_rTau =
                    NTNDenseUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::rTau];
                table3gpp->m_uXpr =
                    NTNDenseUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uXpr];
                table3gpp->m_sigXpr =
                    NTNDenseUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigXpr];
                table3gpp->m_numOfCluster = NTNDenseUrbanNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::numOfCluster];
                table3gpp->m_raysPerCluster = NTNDenseUrbanNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::raysPerCluster];
                table3gpp->m_cDS =
                    NTNDenseUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cDS] *
                    1e-9;
                table3gpp->m_cASD =
                    NTNDenseUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASD];
                table3gpp->m_cASA =
                    NTNDenseUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASA];
                table3gpp->m_cZSA =
                    NTNDenseUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cZSA];
                table3gpp->m_perClusterShadowingStd = NTNDenseUrbanNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::perClusterShadowingStd];

                for (uint8_t row = 0; row < 6; row++)
                {
                    for (uint8_t column = 0; column < 6; column++)
                    {
                        table3gpp->m_sqrtC[row][column] = sqrtC_NTN_DenseUrban_NLOS[row][column];
                    }
                }
            }
        }
        else if (m_scenario == "NTN-Urban")
        {
            if (channelCondition->IsLos())
            {
                table3gpp->m_uLgDS =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgDS];
                table3gpp->m_sigLgDS =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgDS];
                table3gpp->m_uLgASD =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASD];
                table3gpp->m_sigLgASD =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASD];
                table3gpp->m_uLgASA =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASA];
                table3gpp->m_sigLgASA =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASA];
                table3gpp->m_uLgZSA =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSA];
                table3gpp->m_sigLgZSA =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSA];
                table3gpp->m_uLgZSD =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSD];
                table3gpp->m_sigLgZSD =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSD];
                table3gpp->m_uK =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uK];
                table3gpp->m_sigK =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigK];
                table3gpp->m_rTau =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::rTau];
                table3gpp->m_uXpr =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uXpr];
                table3gpp->m_sigXpr =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigXpr];
                table3gpp->m_numOfCluster =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::numOfCluster];
                table3gpp->m_raysPerCluster = NTNUrbanLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::raysPerCluster];
                table3gpp->m_cDS =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cDS] * 1e-9;
                table3gpp->m_cASD =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASD];
                table3gpp->m_cASA =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASA];
                table3gpp->m_cZSA =
                    NTNUrbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cZSA];
                table3gpp->m_perClusterShadowingStd = NTNUrbanLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::perClusterShadowingStd];

                for (uint8_t row = 0; row < 7; row++)
                {
                    for (uint8_t column = 0; column < 7; column++)
                    {
                        table3gpp->m_sqrtC[row][column] = sqrtC_NTN_Urban_LOS[row][column];
                    }
                }
            }
            else if (channelCondition->IsNlos())
            {
                table3gpp->m_uLgDS =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgDS];
                table3gpp->m_sigLgDS =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgDS];
                table3gpp->m_uLgASD =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASD];
                table3gpp->m_sigLgASD =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASD];
                table3gpp->m_uLgASA =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASA];
                table3gpp->m_sigLgASA =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASA];
                table3gpp->m_uLgZSA =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSA];
                table3gpp->m_sigLgZSA =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSA];
                table3gpp->m_uLgZSD =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSD];
                table3gpp->m_sigLgZSD =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSD];
                table3gpp->m_uK =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uK];
                table3gpp->m_sigK =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigK];
                table3gpp->m_rTau =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::rTau];
                table3gpp->m_uXpr =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uXpr];
                table3gpp->m_sigXpr =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigXpr];
                table3gpp->m_numOfCluster =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::numOfCluster];
                table3gpp->m_raysPerCluster = NTNUrbanNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::raysPerCluster];
                table3gpp->m_cDS =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cDS] * 1e-9;
                table3gpp->m_cASD =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASD];
                table3gpp->m_cASA =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASA];
                table3gpp->m_cZSA =
                    NTNUrbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cZSA];
                table3gpp->m_perClusterShadowingStd = NTNUrbanNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::perClusterShadowingStd];

                for (uint8_t row = 0; row < 6; row++)
                {
                    for (uint8_t column = 0; column < 6; column++)
                    {
                        table3gpp->m_sqrtC[row][column] =
                            sqrtC_NTN_Urban_NLOS.at(elevAngleQuantized)[row][column];
                    }
                }
            }
        }
        else if (m_scenario == "NTN-Suburban")
        {
            if (channelCondition->IsLos())
            {
                table3gpp->m_uLgDS =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgDS];
                table3gpp->m_sigLgDS =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgDS];
                table3gpp->m_uLgASD =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASD];
                table3gpp->m_sigLgASD =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASD];
                table3gpp->m_uLgASA =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASA];
                table3gpp->m_sigLgASA =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASA];
                table3gpp->m_uLgZSA =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSA];
                table3gpp->m_sigLgZSA =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSA];
                table3gpp->m_uLgZSD =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSD];
                table3gpp->m_sigLgZSD =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSD];
                table3gpp->m_uK =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uK];
                table3gpp->m_sigK =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigK];
                table3gpp->m_rTau =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::rTau];
                table3gpp->m_uXpr =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uXpr];
                table3gpp->m_sigXpr =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigXpr];
                table3gpp->m_numOfCluster = NTNSuburbanLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::numOfCluster];
                table3gpp->m_raysPerCluster = NTNSuburbanLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::raysPerCluster];
                table3gpp->m_cDS =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cDS] * 1e-9;
                table3gpp->m_cASD =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASD];
                table3gpp->m_cASA =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASA];
                table3gpp->m_cZSA =
                    NTNSuburbanLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cZSA];
                table3gpp->m_perClusterShadowingStd = NTNSuburbanLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::perClusterShadowingStd];

                for (uint8_t row = 0; row < 7; row++)
                {
                    for (uint8_t column = 0; column < 7; column++)
                    {
                        table3gpp->m_sqrtC[row][column] = sqrtC_NTN_Suburban_LOS[row][column];
                    }
                }
            }
            else if (channelCondition->IsNlos())
            {
                table3gpp->m_uLgDS =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgDS];
                table3gpp->m_sigLgDS =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgDS];
                table3gpp->m_uLgASD =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASD];
                table3gpp->m_sigLgASD =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASD];
                table3gpp->m_uLgASA =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASA];
                table3gpp->m_sigLgASA =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASA];
                table3gpp->m_uLgZSA =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSA];
                table3gpp->m_sigLgZSA =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSA];
                table3gpp->m_uLgZSD =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSD];
                table3gpp->m_sigLgZSD =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSD];
                table3gpp->m_uK =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uK];
                table3gpp->m_sigK =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigK];
                table3gpp->m_rTau =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::rTau];
                table3gpp->m_uXpr =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uXpr];
                table3gpp->m_sigXpr =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigXpr];
                table3gpp->m_numOfCluster = NTNSuburbanNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::numOfCluster];
                table3gpp->m_raysPerCluster = NTNSuburbanNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::raysPerCluster];
                table3gpp->m_cDS =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cDS] *
                    1e-9;
                table3gpp->m_cASD =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASD];
                table3gpp->m_cASA =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASA];
                table3gpp->m_cZSA =
                    NTNSuburbanNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cZSA];
                table3gpp->m_perClusterShadowingStd = NTNSuburbanNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::perClusterShadowingStd];

                for (uint8_t row = 0; row < 6; row++)
                {
                    for (uint8_t column = 0; column < 6; column++)
                    {
                        table3gpp->m_sqrtC[row][column] = sqrtC_NTN_Suburban_NLOS[row][column];
                    }
                }
            }
        }
        else if (m_scenario == "NTN-Rural")
        {
            if (channelCondition->IsLos())
            {
                table3gpp->m_uLgDS =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgDS];
                table3gpp->m_sigLgDS =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgDS];
                table3gpp->m_uLgASD =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASD];
                table3gpp->m_sigLgASD =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASD];
                table3gpp->m_uLgASA =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASA];
                table3gpp->m_sigLgASA =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASA];
                table3gpp->m_uLgZSA =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSA];
                table3gpp->m_sigLgZSA =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSA];
                table3gpp->m_uLgZSD =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSD];
                table3gpp->m_sigLgZSD =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSD];
                table3gpp->m_uK =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uK];
                table3gpp->m_sigK =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigK];
                table3gpp->m_rTau =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::rTau];
                table3gpp->m_uXpr =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uXpr];
                table3gpp->m_sigXpr =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigXpr];
                table3gpp->m_numOfCluster =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::numOfCluster];
                table3gpp->m_raysPerCluster = NTNRuralLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::raysPerCluster];
                table3gpp->m_cDS =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cDS] * 1e-9;
                table3gpp->m_cASD =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASD];
                table3gpp->m_cASA =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASA];
                table3gpp->m_cZSA =
                    NTNRuralLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cZSA];
                table3gpp->m_perClusterShadowingStd = NTNRuralLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::perClusterShadowingStd];

                for (uint8_t row = 0; row < 7; row++)
                {
                    for (uint8_t column = 0; column < 7; column++)
                    {
                        table3gpp->m_sqrtC[row][column] = sqrtC_NTN_Rural_LOS[row][column];
                    }
                }
            }
            else if (channelCondition->IsNlos())
            {
                table3gpp->m_uLgDS =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgDS];
                table3gpp->m_sigLgDS =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgDS];
                table3gpp->m_uLgASD =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASD];
                table3gpp->m_sigLgASD =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASD];
                table3gpp->m_uLgASA =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgASA];
                table3gpp->m_sigLgASA =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgASA];
                table3gpp->m_uLgZSA =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSA];
                table3gpp->m_sigLgZSA =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSA];
                table3gpp->m_uLgZSD =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uLgZSD];
                table3gpp->m_sigLgZSD =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigLgZSD];
                table3gpp->m_uK =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uK];
                table3gpp->m_sigK =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigK];
                table3gpp->m_rTau =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::rTau];
                table3gpp->m_uXpr =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::uXpr];
                table3gpp->m_sigXpr =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::sigXpr];
                table3gpp->m_numOfCluster =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::numOfCluster];
                table3gpp->m_raysPerCluster = NTNRuralNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::raysPerCluster];
                table3gpp->m_cDS =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cDS] * 1e-9;
                table3gpp->m_cASD =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASD];
                table3gpp->m_cASA =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cASA];
                table3gpp->m_cZSA =
                    NTNRuralNLOS.at(freqBand).at(elevAngleQuantized)[Table3gppParams::cZSA];
                table3gpp->m_perClusterShadowingStd = NTNRuralNLOS.at(freqBand).at(
                    elevAngleQuantized)[Table3gppParams::perClusterShadowingStd];

                if (freqBand == "S")
                {
                    for (uint8_t row = 0; row < 6; row++)
                    {
                        for (uint8_t column = 0; column < 6; column++)
                        {
                            table3gpp->m_sqrtC[row][column] =
                                sqrtC_NTN_Rural_NLOS_S.at(elevAngleQuantized)[row][column];
                        }
                    }
                }
                else if (freqBand == "Ka")
                {
                    for (uint8_t row = 0; row < 6; row++)
                    {
                        for (uint8_t column = 0; column < 6; column++)
                        {
                            table3gpp->m_sqrtC[row][column] =
                                sqrtC_NTN_Rural_NLOS_Ka.at(elevAngleQuantized)[row][column];
                        }
                    }
                }
            }
        }
        // Parameters that should be set to -inf are instead set to the minimum
        // value of double
        if (isSatellite)
        {
            table3gpp->m_uLgASD = std::numeric_limits<double>::min();
            table3gpp->m_sigLgASD = 0;
            table3gpp->m_uLgZSD = std::numeric_limits<double>::min();
            table3gpp->m_sigLgZSD = 0;
        }
    }
    else
    {
        NS_FATAL_ERROR("unknown scenarios");
    }

    return table3gpp;
}

bool
ThreeGppChannelModel::ChannelParamsNeedsUpdate(Ptr<const ThreeGppChannelParams> channelParams,
                                               Ptr<const ChannelCondition> channelCondition) const
{
    NS_LOG_FUNCTION(this);

    bool update = false;

    // if the channel condition is different the channel has to be updated
    if (!channelCondition->IsEqual(channelParams->m_losCondition, channelParams->m_o2iCondition))
    {
        NS_LOG_DEBUG("Update the channel condition");
        update = true;
    }

    // if the coherence time is over the channel has to be updated
    if (!m_updatePeriod.IsZero() &&
        Simulator::Now() - channelParams->m_generatedTime > m_updatePeriod)
    {
        NS_LOG_DEBUG("Generation time " << channelParams->m_generatedTime.As(Time::NS) << " now "
                                        << Now().As(Time::NS));
        update = true;
    }

    return update;
}

bool
ThreeGppChannelModel::ChannelMatrixNeedsUpdate(Ptr<const ThreeGppChannelParams> channelParams,
                                               Ptr<const ChannelMatrix> channelMatrix)
{
    return channelParams->m_generatedTime > channelMatrix->m_generatedTime;
}

bool
ThreeGppChannelModel::AntennaSetupChanged(Ptr<const PhasedArrayModel> aAntenna,
                                          Ptr<const PhasedArrayModel> bAntenna,
                                          Ptr<const ChannelMatrix> channelMatrix)
{
    // This allows changing the number of antenna ports during execution,
    // which is used by nr's initial association.
    size_t sAntNumElems = aAntenna->GetNumElems();
    size_t uAntNumElems = bAntenna->GetNumElems();
    size_t chanNumRows = channelMatrix->m_channel.GetNumRows();
    size_t chanNumCols = channelMatrix->m_channel.GetNumCols();
    return (((uAntNumElems != chanNumRows) || (sAntNumElems != chanNumCols)) &&
            ((uAntNumElems != chanNumCols) || (sAntNumElems != chanNumRows))) ||
           aAntenna->IsChannelOutOfDate(bAntenna);
}

Ptr<const MatrixBasedChannelModel::ChannelMatrix>
ThreeGppChannelModel::GetChannel(Ptr<const MobilityModel> aMob,
                                 Ptr<const MobilityModel> bMob,
                                 Ptr<const PhasedArrayModel> aAntenna,
                                 Ptr<const PhasedArrayModel> bAntenna)
{
    NS_LOG_FUNCTION(this);

    // Compute the channel params key. The key is reciprocal, i.e., key (a, b) = key (b, a)
    uint64_t channelParamsKey =
        GetKey(aMob->GetObject<Node>()->GetId(), bMob->GetObject<Node>()->GetId());
    // Compute the channel matrix key. The key is reciprocal, i.e., key (a, b) = key (b, a)
    uint64_t channelMatrixKey = GetKey(aAntenna->GetId(), bAntenna->GetId());

    // retrieve the channel condition
    Ptr<const ChannelCondition> condition =
        m_channelConditionModel->GetChannelCondition(aMob, bMob);

    // Check if the channel is present in the map and return it, otherwise
    // generate a new channel
    bool updateParams = false;
    bool updateMatrix = false;
    bool notFoundParams = false;
    bool notFoundMatrix = false;
    Ptr<ChannelMatrix> channelMatrix;
    Ptr<ThreeGppChannelParams> channelParams;

    if (m_channelParamsMap.find(channelParamsKey) != m_channelParamsMap.end())
    {
        channelParams = m_channelParamsMap[channelParamsKey];
        // check if it has to be updated
        updateParams = ChannelParamsNeedsUpdate(channelParams, condition);
    }
    else
    {
        NS_LOG_DEBUG("channel params not found");
        notFoundParams = true;
    }

    // get the 3GPP parameters
    Ptr<const ParamsTable> table3gpp = GetThreeGppTable(aMob, bMob, condition);

    if (notFoundParams || updateParams)
    {
        // Step 4: Generate large scale parameters. All LSPS are uncorrelated.
        // Step 5: Generate Delays.
        // Step 6: Generate cluster powers.
        // Step 7: Generate arrival and departure angles for both azimuth and elevation.
        // Step 8: Coupling of rays within a cluster for both azimuth and elevation
        // shuffle all the arrays to perform random coupling
        // Step 9: Generate the cross polarization power ratios
        // Step 10: Draw initial phases
        channelParams = GenerateChannelParameters(condition, table3gpp, aMob, bMob);
        // store or replace the channel parameters
        m_channelParamsMap[channelParamsKey] = channelParams;
    }

    if (m_channelMatrixMap.find(channelMatrixKey) != m_channelMatrixMap.end())
    {
        // channel matrix present in the map
        NS_LOG_DEBUG("channel matrix present in the map");
        channelMatrix = m_channelMatrixMap[channelMatrixKey];
        updateMatrix = ChannelMatrixNeedsUpdate(channelParams, channelMatrix);
        updateMatrix |= AntennaSetupChanged(aAntenna, bAntenna, channelMatrix);
    }
    else
    {
        NS_LOG_DEBUG("channel matrix not found");
        notFoundMatrix = true;
    }

    // If the channel is not present in the map or if it has to be updated
    // generate a new realization
    if (notFoundMatrix || updateMatrix)
    {
        // channel matrix not found or has to be updated, generate a new one
        channelMatrix = GetNewChannel(channelParams, table3gpp, aMob, bMob, aAntenna, bAntenna);
        channelMatrix->m_antennaPair =
            std::make_pair(aAntenna->GetId(),
                           bAntenna->GetId()); // save antenna pair, with the exact order of s and u
                                               // antennas at the moment of the channel generation

        // store or replace the channel matrix in the channel map
        m_channelMatrixMap[channelMatrixKey] = channelMatrix;
    }

    return channelMatrix;
}

Ptr<const MatrixBasedChannelModel::ChannelParams>
ThreeGppChannelModel::GetParams(Ptr<const MobilityModel> aMob, Ptr<const MobilityModel> bMob) const
{
    NS_LOG_FUNCTION(this);

    // Compute the channel key. The key is reciprocal, i.e., key (a, b) = key (b, a)
    uint64_t channelParamsKey =
        GetKey(aMob->GetObject<Node>()->GetId(), bMob->GetObject<Node>()->GetId());

    if (m_channelParamsMap.find(channelParamsKey) != m_channelParamsMap.end())
    {
        return m_channelParamsMap.find(channelParamsKey)->second;
    }
    else
    {
        NS_LOG_WARN("Channel params map not found. Returning a nullptr.");
        return nullptr;
    }
}

Ptr<ThreeGppChannelModel::ThreeGppChannelParams>
ThreeGppChannelModel::GenerateChannelParameters(const Ptr<const ChannelCondition> channelCondition,
                                                const Ptr<const ParamsTable> table3gpp,
                                                const Ptr<const MobilityModel> aMob,
                                                const Ptr<const MobilityModel> bMob) const
{
    NS_LOG_FUNCTION(this);
    // create a channel matrix instance
    Ptr<ThreeGppChannelParams> channelParams = Create<ThreeGppChannelParams>();
    channelParams->m_generatedTime = Simulator::Now();
    channelParams->m_nodeIds =
        std::make_pair(aMob->GetObject<Node>()->GetId(), bMob->GetObject<Node>()->GetId());
    channelParams->m_losCondition = channelCondition->GetLosCondition();
    channelParams->m_o2iCondition = channelCondition->GetO2iCondition();

    // Step 4: Generate large scale parameters. All LSPS are uncorrelated.
    DoubleVector LSPsIndep;
    DoubleVector LSPs;
    uint8_t paramNum = 6;
    if (channelParams->m_losCondition == ChannelCondition::LOS)
    {
        paramNum = 7;
    }

    // Generate paramNum independent LSPs.
    for (uint8_t iter = 0; iter < paramNum; iter++)
    {
        LSPsIndep.push_back(m_normalRv->GetValue());
    }
    for (uint8_t row = 0; row < paramNum; row++)
    {
        double temp = 0;
        for (uint8_t column = 0; column < paramNum; column++)
        {
            temp += table3gpp->m_sqrtC[row][column] * LSPsIndep[column];
        }
        LSPs.push_back(temp);
    }

    // NOTE the shadowing is generated in the propagation loss model
    double DS;
    double ASD;
    double ASA;
    double ZSA;
    double ZSD;
    double kFactor = 0;
    if (channelParams->m_losCondition == ChannelCondition::LOS)
    {
        kFactor = LSPs[1] * table3gpp->m_sigK + table3gpp->m_uK;
        DS = pow(10, LSPs[2] * table3gpp->m_sigLgDS + table3gpp->m_uLgDS);
        ASD = pow(10, LSPs[3] * table3gpp->m_sigLgASD + table3gpp->m_uLgASD);
        ASA = pow(10, LSPs[4] * table3gpp->m_sigLgASA + table3gpp->m_uLgASA);
        ZSD = pow(10, LSPs[5] * table3gpp->m_sigLgZSD + table3gpp->m_uLgZSD);
        ZSA = pow(10, LSPs[6] * table3gpp->m_sigLgZSA + table3gpp->m_uLgZSA);
    }
    else
    {
        DS = pow(10, LSPs[1] * table3gpp->m_sigLgDS + table3gpp->m_uLgDS);
        ASD = pow(10, LSPs[2] * table3gpp->m_sigLgASD + table3gpp->m_uLgASD);
        ASA = pow(10, LSPs[3] * table3gpp->m_sigLgASA + table3gpp->m_uLgASA);
        ZSD = pow(10, LSPs[4] * table3gpp->m_sigLgZSD + table3gpp->m_uLgZSD);
        ZSA = pow(10, LSPs[5] * table3gpp->m_sigLgZSA + table3gpp->m_uLgZSA);
    }
    ASD = std::min(ASD, 104.0);
    ASA = std::min(ASA, 104.0);
    ZSD = std::min(ZSD, 52.0);
    ZSA = std::min(ZSA, 52.0);

    // save DS and K_factor parameters in the structure
    channelParams->m_DS = DS;
    channelParams->m_K_factor = kFactor;

    NS_LOG_INFO("K-factor=" << kFactor << ", DS=" << DS << ", ASD=" << ASD << ", ASA=" << ASA
                            << ", ZSD=" << ZSD << ", ZSA=" << ZSA);

    // Step 5: Generate Delays.
    DoubleVector clusterDelay;
    double minTau = 100.0;
    for (uint8_t cIndex = 0; cIndex < table3gpp->m_numOfCluster; cIndex++)
    {
        double tau = -1 * table3gpp->m_rTau * DS * log(m_uniformRv->GetValue(0, 1)); //(7.5-1)
        if (minTau > tau)
        {
            minTau = tau;
        }
        clusterDelay.push_back(tau);
    }

    for (uint8_t cIndex = 0; cIndex < table3gpp->m_numOfCluster; cIndex++)
    {
        clusterDelay[cIndex] -= minTau;
    }
    std::sort(clusterDelay.begin(), clusterDelay.end()); //(7.5-2)

    /* since the scaled Los delays are not to be used in cluster power generation,
     * we will generate cluster power first and resume to compute Los cluster delay later.*/

    // Step 6: Generate cluster powers.
    DoubleVector clusterPower;
    double powerSum = 0;
    for (uint8_t cIndex = 0; cIndex < table3gpp->m_numOfCluster; cIndex++)
    {
        double power =
            exp(-1 * clusterDelay[cIndex] * (table3gpp->m_rTau - 1) / table3gpp->m_rTau / DS) *
            pow(10,
                -1 * m_normalRv->GetValue() * table3gpp->m_perClusterShadowingStd / 10.0); //(7.5-5)
        powerSum += power;
        clusterPower.push_back(power);
    }
    channelParams->m_clusterPower = clusterPower;

    double powerMax = 0;

    for (uint8_t cIndex = 0; cIndex < table3gpp->m_numOfCluster; cIndex++)
    {
        channelParams->m_clusterPower[cIndex] =
            channelParams->m_clusterPower[cIndex] / powerSum; //(7.5-6)
    }

    DoubleVector clusterPowerForAngles; // this power is only for equation (7.5-9) and (7.5-14), not
                                        // for (7.5-22)
    if (channelParams->m_losCondition == ChannelCondition::LOS)
    {
        double kLinear = pow(10, kFactor / 10.0);

        for (uint8_t cIndex = 0; cIndex < table3gpp->m_numOfCluster; cIndex++)
        {
            if (cIndex == 0)
            {
                clusterPowerForAngles.push_back(channelParams->m_clusterPower[cIndex] /
                                                    (1 + kLinear) +
                                                kLinear / (1 + kLinear)); //(7.5-8)
            }
            else
            {
                clusterPowerForAngles.push_back(channelParams->m_clusterPower[cIndex] /
                                                (1 + kLinear)); //(7.5-8)
            }
            if (powerMax < clusterPowerForAngles[cIndex])
            {
                powerMax = clusterPowerForAngles[cIndex];
            }
        }
    }
    else
    {
        for (uint8_t cIndex = 0; cIndex < table3gpp->m_numOfCluster; cIndex++)
        {
            clusterPowerForAngles.push_back(channelParams->m_clusterPower[cIndex]); //(7.5-6)
            if (powerMax < clusterPowerForAngles[cIndex])
            {
                powerMax = clusterPowerForAngles[cIndex];
            }
        }
    }

    // remove clusters with less than -25 dB power compared to the maxim cluster power;
    // double thresh = pow(10, -2.5);
    double thresh = 0.0032;
    for (uint8_t cIndex = table3gpp->m_numOfCluster; cIndex > 0; cIndex--)
    {
        if (clusterPowerForAngles[cIndex - 1] < thresh * powerMax)
        {
            clusterPowerForAngles.erase(clusterPowerForAngles.begin() + cIndex - 1);
            channelParams->m_clusterPower.erase(channelParams->m_clusterPower.begin() + cIndex - 1);
            clusterDelay.erase(clusterDelay.begin() + cIndex - 1);
        }
    }

    NS_ASSERT(channelParams->m_clusterPower.size() < UINT8_MAX);
    channelParams->m_reducedClusterNumber = channelParams->m_clusterPower.size();
    // Resume step 5 to compute the delay for LoS condition.
    if (channelParams->m_losCondition == ChannelCondition::LOS)
    {
        double cTau =
            0.7705 - 0.0433 * kFactor + 2e-4 * pow(kFactor, 2) + 17e-6 * pow(kFactor, 3); //(7.5-3)
        for (uint8_t cIndex = 0; cIndex < channelParams->m_reducedClusterNumber; cIndex++)
        {
            clusterDelay[cIndex] = clusterDelay[cIndex] / cTau; //(7.5-4)
        }
    }

    // Step 7: Generate arrival and departure angles for both azimuth and elevation.

    double cNlos;
    // According to table 7.5-6, only cluster number equals to 8, 10, 11, 12, 19 and 20 is valid.
    // Not sure why the other cases are in Table 7.5-2.
    // Added case 2 and 3 for the NTN according to table 6.7.2-1aa (28.811)
    switch (table3gpp->m_numOfCluster) // Table 7.5-2
    {
    case 2:
        cNlos = 0.501;
        break;
    case 3:
        cNlos = 0.680;
        break;
    case 4:
        cNlos = 0.779;
        break;
    case 5:
        cNlos = 0.860;
        break;
    case 8:
        cNlos = 1.018;
        break;
    case 10:
        cNlos = 1.090;
        break;
    case 11:
        cNlos = 1.123;
        break;
    case 12:
        cNlos = 1.146;
        break;
    case 14:
        cNlos = 1.190;
        break;
    case 15:
        cNlos = 1.221;
        break;
    case 16:
        cNlos = 1.226;
        break;
    case 19:
        cNlos = 1.273;
        break;
    case 20:
        cNlos = 1.289;
        break;
    default:
        NS_FATAL_ERROR("Invalid cluster number");
    }

    double cPhi = cNlos;

    if (channelParams->m_losCondition == ChannelCondition::LOS)
    {
        cPhi *= (1.1035 - 0.028 * kFactor - 2e-3 * pow(kFactor, 2) +
                 1e-4 * pow(kFactor, 3)); //(7.5-10))
    }

    // Added case 2, 3 and 4 for the NTN according to table 6.7.2-1ab (28.811)
    switch (table3gpp->m_numOfCluster) // Table 7.5-4
    {
    case 2:
        cNlos = 0.430;
        break;
    case 3:
        cNlos = 0.594;
        break;
    case 4:
        cNlos = 0.697;
        break;
    case 8:
        cNlos = 0.889;
        break;
    case 10:
        cNlos = 0.957;
        break;
    case 11:
        cNlos = 1.031;
        break;
    case 12:
        cNlos = 1.104;
        break;
    case 15:
        cNlos = 1.1088;
        break;
    case 19:
        cNlos = 1.184;
        break;
    case 20:
        cNlos = 1.178;
        break;
    default:
        NS_FATAL_ERROR("Invalid cluster number");
    }

    double cTheta = cNlos;
    if (channelCondition->IsLos())
    {
        cTheta *= (1.3086 + 0.0339 * kFactor - 0.0077 * pow(kFactor, 2) +
                   2e-4 * pow(kFactor, 3)); //(7.5-15)
    }

    DoubleVector clusterAoa;
    DoubleVector clusterAod;
    DoubleVector clusterZoa;
    DoubleVector clusterZod;
    for (uint8_t cIndex = 0; cIndex < channelParams->m_reducedClusterNumber; cIndex++)
    {
        double logCalc = -1 * log(clusterPowerForAngles[cIndex] / powerMax);
        double angle = 2 * sqrt(logCalc) / 1.4 / cPhi; //(7.5-9)
        clusterAoa.push_back(ASA * angle);
        clusterAod.push_back(ASD * angle);
        angle = logCalc / cTheta; //(7.5-14)
        clusterZoa.push_back(ZSA * angle);
        clusterZod.push_back(ZSD * angle);
    }

    Angles sAngle(bMob->GetPosition(), aMob->GetPosition());
    Angles uAngle(aMob->GetPosition(), bMob->GetPosition());

    for (uint8_t cIndex = 0; cIndex < channelParams->m_reducedClusterNumber; cIndex++)
    {
        int Xn = 1;
        if (m_uniformRv->GetValue(0, 1) < 0.5)
        {
            Xn = -1;
        }
        clusterAoa[cIndex] = clusterAoa[cIndex] * Xn + (m_normalRv->GetValue() * ASA / 7.0) +
                             RadiansToDegrees(uAngle.GetAzimuth()); //(7.5-11)
        clusterAod[cIndex] = clusterAod[cIndex] * Xn + (m_normalRv->GetValue() * ASD / 7.0) +
                             RadiansToDegrees(sAngle.GetAzimuth());
        if (channelCondition->IsO2i())
        {
            clusterZoa[cIndex] =
                clusterZoa[cIndex] * Xn + (m_normalRv->GetValue() * ZSA / 7.0) + 90; //(7.5-16)
        }
        else
        {
            clusterZoa[cIndex] = clusterZoa[cIndex] * Xn + (m_normalRv->GetValue() * ZSA / 7.0) +
                                 RadiansToDegrees(uAngle.GetInclination()); //(7.5-16)
        }
        clusterZod[cIndex] = clusterZod[cIndex] * Xn + (m_normalRv->GetValue() * ZSD / 7.0) +
                             RadiansToDegrees(sAngle.GetInclination()) +
                             table3gpp->m_offsetZOD; //(7.5-19)
    }

    if (channelParams->m_losCondition == ChannelCondition::LOS)
    {
        // The 7.5-12 can be rewrite as Theta_n,ZOA = Theta_n,ZOA - (Theta_1,ZOA - Theta_LOS,ZOA) =
        // Theta_n,ZOA - diffZOA, Similar as AOD, ZSA and ZSD.
        double diffAoa = clusterAoa[0] - RadiansToDegrees(uAngle.GetAzimuth());
        double diffAod = clusterAod[0] - RadiansToDegrees(sAngle.GetAzimuth());
        double diffZsa = clusterZoa[0] - RadiansToDegrees(uAngle.GetInclination());
        double diffZsd = clusterZod[0] - RadiansToDegrees(sAngle.GetInclination());

        for (uint8_t cIndex = 0; cIndex < channelParams->m_reducedClusterNumber; cIndex++)
        {
            clusterAoa[cIndex] -= diffAoa; //(7.5-12)
            clusterAod[cIndex] -= diffAod;
            clusterZoa[cIndex] -= diffZsa; //(7.5-17)
            clusterZod[cIndex] -= diffZsd;
        }
    }

    double sizeTemp = clusterZoa.size();
    for (uint8_t ind = 0; ind < 4; ind++)
    {
        DoubleVector angleDegree;
        switch (ind)
        {
        case 0:
            angleDegree = clusterAoa;
            break;
        case 1:
            angleDegree = clusterZoa;
            break;
        case 2:
            angleDegree = clusterAod;
            break;
        case 3:
            angleDegree = clusterZod;
            break;
        default:
            NS_FATAL_ERROR("Programming Error");
        }
        for (uint8_t nIndex = 0; nIndex < sizeTemp; nIndex++)
        {
            while (angleDegree[nIndex] > 360)
            {
                angleDegree[nIndex] -= 360;
            }

            while (angleDegree[nIndex] < 0)
            {
                angleDegree[nIndex] += 360;
            }

            if (ind == 1 || ind == 3)
            {
                if (angleDegree[nIndex] > 180)
                {
                    angleDegree[nIndex] = 360 - angleDegree[nIndex];
                }
            }
        }
        switch (ind)
        {
        case 0:
            clusterAoa = angleDegree;
            break;
        case 1:
            clusterZoa = angleDegree;
            break;
        case 2:
            clusterAod = angleDegree;
            break;
        case 3:
            clusterZod = angleDegree;
            break;
        default:
            NS_FATAL_ERROR("Programming Error");
        }
    }

    DoubleVector attenuationDb;
    if (m_blockage)
    {
        attenuationDb = CalcAttenuationOfBlockage(channelParams, clusterAoa, clusterZoa);
        for (uint8_t cInd = 0; cInd < channelParams->m_reducedClusterNumber; cInd++)
        {
            channelParams->m_clusterPower[cInd] =
                channelParams->m_clusterPower[cInd] / pow(10, attenuationDb[cInd] / 10.0);
        }
    }
    else
    {
        attenuationDb.push_back(0);
    }

    // store attenuation
    channelParams->m_attenuation_dB = attenuationDb;

    // Step 8: Coupling of rays within a cluster for both azimuth and elevation
    // shuffle all the arrays to perform random coupling
    MatrixBasedChannelModel::Double2DVector rayAoaRadian(
        channelParams->m_reducedClusterNumber,
        DoubleVector(table3gpp->m_raysPerCluster,
                     0)); // rayAoaRadian[n][m], where n is cluster index, m is ray index
    MatrixBasedChannelModel::Double2DVector rayAodRadian(
        channelParams->m_reducedClusterNumber,
        DoubleVector(table3gpp->m_raysPerCluster,
                     0)); // rayAodRadian[n][m], where n is cluster index, m is ray index
    MatrixBasedChannelModel::Double2DVector rayZoaRadian(
        channelParams->m_reducedClusterNumber,
        DoubleVector(table3gpp->m_raysPerCluster,
                     0)); // rayZoaRadian[n][m], where n is cluster index, m is ray index
    MatrixBasedChannelModel::Double2DVector rayZodRadian(
        channelParams->m_reducedClusterNumber,
        DoubleVector(table3gpp->m_raysPerCluster,
                     0)); // rayZodRadian[n][m], where n is cluster index, m is ray index

    const double pow10_uLgZSD = pow(10, table3gpp->m_uLgZSD);
    for (uint8_t nInd = 0; nInd < channelParams->m_reducedClusterNumber; nInd++)
    {
        for (uint8_t mInd = 0; mInd < table3gpp->m_raysPerCluster; mInd++)
        {
            double tempAoa = clusterAoa[nInd] + table3gpp->m_cASA * offSetAlpha[mInd]; //(7.5-13)
            double tempZoa = clusterZoa[nInd] + table3gpp->m_cZSA * offSetAlpha[mInd]; //(7.5-18)
            std::tie(rayAoaRadian[nInd][mInd], rayZoaRadian[nInd][mInd]) =
                WrapAngles(DegreesToRadians(tempAoa), DegreesToRadians(tempZoa));

            double tempAod = clusterAod[nInd] + table3gpp->m_cASD * offSetAlpha[mInd];    //(7.5-13)
            double tempZod = clusterZod[nInd] + 0.375 * pow10_uLgZSD * offSetAlpha[mInd]; //(7.5-20)
            std::tie(rayAodRadian[nInd][mInd], rayZodRadian[nInd][mInd]) =
                WrapAngles(DegreesToRadians(tempAod), DegreesToRadians(tempZod));
        }
    }

    for (uint8_t cIndex = 0; cIndex < channelParams->m_reducedClusterNumber; cIndex++)
    {
        Shuffle(rayAodRadian[cIndex].begin(), rayAodRadian[cIndex].end(), m_uniformRvShuffle);
        Shuffle(rayAoaRadian[cIndex].begin(), rayAoaRadian[cIndex].end(), m_uniformRvShuffle);
        Shuffle(rayZodRadian[cIndex].begin(), rayZodRadian[cIndex].end(), m_uniformRvShuffle);
        Shuffle(rayZoaRadian[cIndex].begin(), rayZoaRadian[cIndex].end(), m_uniformRvShuffle);
    }

    // store values
    channelParams->m_rayAodRadian = rayAodRadian;
    channelParams->m_rayAoaRadian = rayAoaRadian;
    channelParams->m_rayZodRadian = rayZodRadian;
    channelParams->m_rayZoaRadian = rayZoaRadian;

    // Step 9: Generate the cross polarization power ratios
    // Step 10: Draw initial phases

    // vector containing the cross polarization power ratios, as defined by 7.5-21
    auto& crossPolarizationPowerRatios = channelParams->m_crossPolarizationPowerRatios;
    // rayAoaRadian[n][m], where n is cluster index, m is ray index
    auto& clusterPhase = channelParams->m_clusterPhase;

    const double uXprLinear = pow(10, table3gpp->m_uXpr / 10.0);     // convert to linear
    const double sigXprLinear = pow(10, table3gpp->m_sigXpr / 10.0); // convert to linear

    // store the PHI values for all the possible combination of polarization
    clusterPhase.resize(channelParams->m_reducedClusterNumber);
    crossPolarizationPowerRatios.resize(channelParams->m_reducedClusterNumber);
    for (uint8_t nInd = 0; nInd < channelParams->m_reducedClusterNumber; nInd++)
    {
        clusterPhase[nInd].resize(table3gpp->m_raysPerCluster);
        crossPolarizationPowerRatios[nInd].resize(table3gpp->m_raysPerCluster);
        for (uint8_t mInd = 0; mInd < table3gpp->m_raysPerCluster; mInd++)
        {
            clusterPhase[nInd][mInd].resize(4);
            // used to store the XPR values
            crossPolarizationPowerRatios[nInd][mInd] =
                std::pow(10, (m_normalRv->GetValue() * sigXprLinear + uXprLinear) / 10.0);
            for (uint8_t pInd = 0; pInd < 4; pInd++)
            {
                // used to store the PHI values
                clusterPhase[nInd][mInd][pInd] = m_uniformRv->GetValue(-1 * M_PI, M_PI);
            }
        }
    }

    uint8_t cluster1st = 0;
    uint8_t cluster2nd = 0; // first and second strongest cluster;
    double maxPower = 0;
    for (uint8_t cIndex = 0; cIndex < channelParams->m_reducedClusterNumber; cIndex++)
    {
        if (maxPower < channelParams->m_clusterPower[cIndex])
        {
            maxPower = channelParams->m_clusterPower[cIndex];
            cluster1st = cIndex;
        }
    }
    channelParams->m_cluster1st = cluster1st;
    maxPower = 0;
    for (uint8_t cIndex = 0; cIndex < channelParams->m_reducedClusterNumber; cIndex++)
    {
        if (maxPower < channelParams->m_clusterPower[cIndex] && cluster1st != cIndex)
        {
            maxPower = channelParams->m_clusterPower[cIndex];
            cluster2nd = cIndex;
        }
    }
    channelParams->m_cluster2nd = cluster2nd;

    NS_LOG_INFO("1st strongest cluster:" << +cluster1st
                                         << ", 2nd strongest cluster:" << +cluster2nd);

    // store the delays and the angles for the subclusters
    if (cluster1st == cluster2nd)
    {
        clusterDelay.push_back(clusterDelay[cluster1st] + 1.28 * table3gpp->m_cDS);
        clusterDelay.push_back(clusterDelay[cluster1st] + 2.56 * table3gpp->m_cDS);

        clusterAoa.push_back(clusterAoa[cluster1st]);
        clusterAoa.push_back(clusterAoa[cluster1st]);

        clusterZoa.push_back(clusterZoa[cluster1st]);
        clusterZoa.push_back(clusterZoa[cluster1st]);

        clusterAod.push_back(clusterAod[cluster1st]);
        clusterAod.push_back(clusterAod[cluster1st]);

        clusterZod.push_back(clusterZod[cluster1st]);
        clusterZod.push_back(clusterZod[cluster1st]);
    }
    else
    {
        double min;
        double max;
        if (cluster1st < cluster2nd)
        {
            min = cluster1st;
            max = cluster2nd;
        }
        else
        {
            min = cluster2nd;
            max = cluster1st;
        }
        clusterDelay.push_back(clusterDelay[min] + 1.28 * table3gpp->m_cDS);
        clusterDelay.push_back(clusterDelay[min] + 2.56 * table3gpp->m_cDS);
        clusterDelay.push_back(clusterDelay[max] + 1.28 * table3gpp->m_cDS);
        clusterDelay.push_back(clusterDelay[max] + 2.56 * table3gpp->m_cDS);

        clusterAoa.push_back(clusterAoa[min]);
        clusterAoa.push_back(clusterAoa[min]);
        clusterAoa.push_back(clusterAoa[max]);
        clusterAoa.push_back(clusterAoa[max]);

        clusterZoa.push_back(clusterZoa[min]);
        clusterZoa.push_back(clusterZoa[min]);
        clusterZoa.push_back(clusterZoa[max]);
        clusterZoa.push_back(clusterZoa[max]);

        clusterAod.push_back(clusterAod[min]);
        clusterAod.push_back(clusterAod[min]);
        clusterAod.push_back(clusterAod[max]);
        clusterAod.push_back(clusterAod[max]);

        clusterZod.push_back(clusterZod[min]);
        clusterZod.push_back(clusterZod[min]);
        clusterZod.push_back(clusterZod[max]);
        clusterZod.push_back(clusterZod[max]);
    }

    channelParams->m_delay = clusterDelay;
    channelParams->m_angle.clear();
    channelParams->m_angle.push_back(clusterAoa);
    channelParams->m_angle.push_back(clusterZoa);
    channelParams->m_angle.push_back(clusterAod);
    channelParams->m_angle.push_back(clusterZod);

    // Precompute angles sincos
    channelParams->m_cachedAngleSincos.resize(channelParams->m_angle.size());
    for (size_t direction = 0; direction < channelParams->m_angle.size(); direction++)
    {
        channelParams->m_cachedAngleSincos[direction].resize(
            channelParams->m_angle[direction].size());
        for (size_t cluster = 0; cluster < channelParams->m_angle[direction].size(); cluster++)
        {
            channelParams->m_cachedAngleSincos[direction][cluster] = {
                sin(channelParams->m_angle[direction][cluster] * DEG2RAD),
                cos(channelParams->m_angle[direction][cluster] * DEG2RAD)};
        }
    }

    // Compute alpha and D as described in 3GPP TR 37.885 v15.3.0, Sec. 6.2.3
    // These terms account for an additional Doppler contribution due to the
    // presence of moving objects in the surrounding environment, such as in
    // vehicular scenarios.
    // This contribution is applied only to the delayed (reflected) paths and
    // must be properly configured by setting the value of
    // m_vScatt, which is defined as "maximum speed of the vehicle in the
    // layout".
    // By default, m_vScatt is set to 0, so there is no additional Doppler
    // contribution.

    DoubleVector dopplerTermAlpha;
    DoubleVector dopplerTermD;

    // 2 or 4 is added to account for additional subrays for the 1st and 2nd clusters, if there is
    // only one cluster then would be added 2 more subrays (see creation of Husn channel matrix)
    uint8_t updatedClusterNumber = (channelParams->m_reducedClusterNumber == 1)
                                       ? channelParams->m_reducedClusterNumber + 2
                                       : channelParams->m_reducedClusterNumber + 4;

    for (uint8_t cIndex = 0; cIndex < updatedClusterNumber; cIndex++)
    {
        double alpha = 0;
        double D = 0;
        if (cIndex != 0)
        {
            alpha = m_uniformRvDoppler->GetValue(-1, 1);
            D = m_uniformRvDoppler->GetValue(-m_vScatt, m_vScatt);
        }
        dopplerTermAlpha.push_back(alpha);
        dopplerTermD.push_back(D);
    }
    channelParams->m_alpha = dopplerTermAlpha;
    channelParams->m_D = dopplerTermD;

    return channelParams;
}

Ptr<MatrixBasedChannelModel::ChannelMatrix>
ThreeGppChannelModel::GetNewChannel(Ptr<const ThreeGppChannelParams> channelParams,
                                    Ptr<const ParamsTable> table3gpp,
                                    const Ptr<const MobilityModel> sMob,
                                    const Ptr<const MobilityModel> uMob,
                                    Ptr<const PhasedArrayModel> sAntenna,
                                    Ptr<const PhasedArrayModel> uAntenna) const
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT_MSG(m_frequency > 0.0, "Set the operating frequency first!");

    // create a channel matrix instance
    Ptr<ChannelMatrix> channelMatrix = Create<ChannelMatrix>();
    channelMatrix->m_generatedTime = Simulator::Now();
    // save in which order is generated this matrix
    channelMatrix->m_nodeIds =
        std::make_pair(sMob->GetObject<Node>()->GetId(), uMob->GetObject<Node>()->GetId());
    // check if channelParams structure is generated in direction s-to-u or u-to-s
    bool isSameDirection = (channelParams->m_nodeIds == channelMatrix->m_nodeIds);

    MatrixBasedChannelModel::Double2DVector rayAodRadian;
    MatrixBasedChannelModel::Double2DVector rayAoaRadian;
    MatrixBasedChannelModel::Double2DVector rayZodRadian;
    MatrixBasedChannelModel::Double2DVector rayZoaRadian;

    // if channel params is generated in the same direction in which we
    // generate the channel matrix, angles and zenith od departure and arrival are ok,
    // just set them to corresponding variable that will be used for the generation
    // of channel matrix, otherwise we need to flip angles and zeniths of departure and arrival
    if (isSameDirection)
    {
        rayAodRadian = channelParams->m_rayAodRadian;
        rayAoaRadian = channelParams->m_rayAoaRadian;
        rayZodRadian = channelParams->m_rayZodRadian;
        rayZoaRadian = channelParams->m_rayZoaRadian;
    }
    else
    {
        rayAodRadian = channelParams->m_rayAoaRadian;
        rayAoaRadian = channelParams->m_rayAodRadian;
        rayZodRadian = channelParams->m_rayZoaRadian;
        rayZoaRadian = channelParams->m_rayZodRadian;
    }

    // Step 11: Generate channel coefficients for each cluster n and each receiver
    //  and transmitter element pair u,s.
    // where n is cluster index, u and s are receive and transmit antenna element.
    size_t uSize = uAntenna->GetNumElems();
    size_t sSize = sAntenna->GetNumElems();

    // NOTE: Since each of the strongest 2 clusters are divided into 3 sub-clusters,
    // the total cluster will generally be numReducedCLuster + 4.
    // However, it might be that m_cluster1st = m_cluster2nd. In this case the
    // total number of clusters will be numReducedCLuster + 2.
    uint16_t numOverallCluster = (channelParams->m_cluster1st != channelParams->m_cluster2nd)
                                     ? channelParams->m_reducedClusterNumber + 4
                                     : channelParams->m_reducedClusterNumber + 2;
    Complex3DVector hUsn(uSize, sSize, numOverallCluster); // channel coefficient hUsn (u, s, n);
    NS_ASSERT(channelParams->m_reducedClusterNumber <= channelParams->m_clusterPhase.size());
    NS_ASSERT(channelParams->m_reducedClusterNumber <= channelParams->m_clusterPower.size());
    NS_ASSERT(channelParams->m_reducedClusterNumber <=
              channelParams->m_crossPolarizationPowerRatios.size());
    NS_ASSERT(channelParams->m_reducedClusterNumber <= rayZoaRadian.size());
    NS_ASSERT(channelParams->m_reducedClusterNumber <= rayZodRadian.size());
    NS_ASSERT(channelParams->m_reducedClusterNumber <= rayAoaRadian.size());
    NS_ASSERT(channelParams->m_reducedClusterNumber <= rayAodRadian.size());
    NS_ASSERT(table3gpp->m_raysPerCluster <= channelParams->m_clusterPhase[0].size());
    NS_ASSERT(table3gpp->m_raysPerCluster <=
              channelParams->m_crossPolarizationPowerRatios[0].size());
    NS_ASSERT(table3gpp->m_raysPerCluster <= rayZoaRadian[0].size());
    NS_ASSERT(table3gpp->m_raysPerCluster <= rayZodRadian[0].size());
    NS_ASSERT(table3gpp->m_raysPerCluster <= rayAoaRadian[0].size());
    NS_ASSERT(table3gpp->m_raysPerCluster <= rayAodRadian[0].size());

    double x = sMob->GetPosition().x - uMob->GetPosition().x;
    double y = sMob->GetPosition().y - uMob->GetPosition().y;
    double distance2D = sqrt(x * x + y * y);
    // NOTE we assume hUT = min (height(a), height(b)) and
    // hBS = max (height (a), height (b))
    double hUt = std::min(sMob->GetPosition().z, uMob->GetPosition().z);
    double hBs = std::max(sMob->GetPosition().z, uMob->GetPosition().z);
    // compute the 3D distance using eq. 7.4-1
    double distance3D = std::sqrt(distance2D * distance2D + (hBs - hUt) * (hBs - hUt));

    Angles sAngle(uMob->GetPosition(), sMob->GetPosition());
    Angles uAngle(sMob->GetPosition(), uMob->GetPosition());

    Double2DVector sinCosA; // cached multiplications of sin and cos of the ZoA and AoA angles
    Double2DVector sinSinA; // cached multiplications of sines of the ZoA and AoA angles
    Double2DVector cosZoA;  // cached cos of the ZoA angle
    Double2DVector sinCosD; // cached multiplications of sin and cos of the ZoD and AoD angles
    Double2DVector sinSinD; // cached multiplications of the cosines of the ZoA and AoA angles
    Double2DVector cosZoD;  // cached cos of the ZoD angle

    // contains part of the ray expression, cached as independent from the u- and s-indexes,
    // but calculate it for different polarization angles of s and u
    std::map<std::pair<uint8_t, uint8_t>, Complex2DVector> raysPreComp;
    for (size_t polSa = 0; polSa < sAntenna->GetNumPols(); ++polSa)
    {
        for (size_t polUa = 0; polUa < uAntenna->GetNumPols(); ++polUa)
        {
            raysPreComp[std::make_pair(polSa, polUa)] =
                Complex2DVector(channelParams->m_reducedClusterNumber, table3gpp->m_raysPerCluster);
        }
    }

    // resize to appropriate dimensions
    sinCosA.resize(channelParams->m_reducedClusterNumber);
    sinSinA.resize(channelParams->m_reducedClusterNumber);
    cosZoA.resize(channelParams->m_reducedClusterNumber);
    sinCosD.resize(channelParams->m_reducedClusterNumber);
    sinSinD.resize(channelParams->m_reducedClusterNumber);
    cosZoD.resize(channelParams->m_reducedClusterNumber);
    for (uint8_t nIndex = 0; nIndex < channelParams->m_reducedClusterNumber; nIndex++)
    {
        sinCosA[nIndex].resize(table3gpp->m_raysPerCluster);
        sinSinA[nIndex].resize(table3gpp->m_raysPerCluster);
        cosZoA[nIndex].resize(table3gpp->m_raysPerCluster);
        sinCosD[nIndex].resize(table3gpp->m_raysPerCluster);
        sinSinD[nIndex].resize(table3gpp->m_raysPerCluster);
        cosZoD[nIndex].resize(table3gpp->m_raysPerCluster);
    }
    // pre-compute the terms which are independent from uIndex and sIndex
    for (uint8_t nIndex = 0; nIndex < channelParams->m_reducedClusterNumber; nIndex++)
    {
        for (uint8_t mIndex = 0; mIndex < table3gpp->m_raysPerCluster; mIndex++)
        {
            DoubleVector initialPhase = channelParams->m_clusterPhase[nIndex][mIndex];
            NS_ASSERT(4 <= initialPhase.size());
            double k = channelParams->m_crossPolarizationPowerRatios[nIndex][mIndex];

            // cache the component of the "rays" terms which depend on the random angle of arrivals
            // and departures and initial phases only
            for (uint8_t polUa = 0; polUa < uAntenna->GetNumPols(); ++polUa)
            {
                auto [rxFieldPatternPhi, rxFieldPatternTheta] = uAntenna->GetElementFieldPattern(
                    Angles(channelParams->m_rayAoaRadian[nIndex][mIndex],
                           channelParams->m_rayZoaRadian[nIndex][mIndex]),
                    polUa);
                for (uint8_t polSa = 0; polSa < sAntenna->GetNumPols(); ++polSa)
                {
                    auto [txFieldPatternPhi, txFieldPatternTheta] =
                        sAntenna->GetElementFieldPattern(
                            Angles(channelParams->m_rayAodRadian[nIndex][mIndex],
                                   channelParams->m_rayZodRadian[nIndex][mIndex]),
                            polSa);
                    raysPreComp[std::make_pair(polSa, polUa)](nIndex, mIndex) =
                        std::complex<double>(cos(initialPhase[0]), sin(initialPhase[0])) *
                            rxFieldPatternTheta * txFieldPatternTheta +
                        std::complex<double>(cos(initialPhase[1]), sin(initialPhase[1])) *
                            std::sqrt(1.0 / k) * rxFieldPatternTheta * txFieldPatternPhi +
                        std::complex<double>(cos(initialPhase[2]), sin(initialPhase[2])) *
                            std::sqrt(1.0 / k) * rxFieldPatternPhi * txFieldPatternTheta +
                        std::complex<double>(cos(initialPhase[3]), sin(initialPhase[3])) *
                            rxFieldPatternPhi * txFieldPatternPhi;
                }
            }

            // cache the component of the "rxPhaseDiff" terms which depend on the random angle of
            // arrivals only
            double sinRayZoa = sin(rayZoaRadian[nIndex][mIndex]);
            double sinRayAoa = sin(rayAoaRadian[nIndex][mIndex]);
            double cosRayAoa = cos(rayAoaRadian[nIndex][mIndex]);
            sinCosA[nIndex][mIndex] = sinRayZoa * cosRayAoa;
            sinSinA[nIndex][mIndex] = sinRayZoa * sinRayAoa;
            cosZoA[nIndex][mIndex] = cos(rayZoaRadian[nIndex][mIndex]);

            // cache the component of the "txPhaseDiff" terms which depend on the random angle of
            // departure only
            double sinRayZod = sin(rayZodRadian[nIndex][mIndex]);
            double sinRayAod = sin(rayAodRadian[nIndex][mIndex]);
            double cosRayAod = cos(rayAodRadian[nIndex][mIndex]);
            sinCosD[nIndex][mIndex] = sinRayZod * cosRayAod;
            sinSinD[nIndex][mIndex] = sinRayZod * sinRayAod;
            cosZoD[nIndex][mIndex] = cos(rayZodRadian[nIndex][mIndex]);
        }
    }

    // The following for loops computes the channel coefficients
    // Keeps track of how many sub-clusters have been added up to now
    uint8_t numSubClustersAdded = 0;
    for (uint8_t nIndex = 0; nIndex < channelParams->m_reducedClusterNumber; nIndex++)
    {
        for (size_t uIndex = 0; uIndex < uSize; uIndex++)
        {
            Vector uLoc = uAntenna->GetElementLocation(uIndex);

            for (size_t sIndex = 0; sIndex < sSize; sIndex++)
            {
                Vector sLoc = sAntenna->GetElementLocation(sIndex);
                // Compute the N-2 weakest cluster, assuming 0 slant angle and a
                // polarization slant angle configured in the array (7.5-22)
                if (nIndex != channelParams->m_cluster1st && nIndex != channelParams->m_cluster2nd)
                {
                    std::complex<double> rays(0, 0);
                    for (uint8_t mIndex = 0; mIndex < table3gpp->m_raysPerCluster; mIndex++)
                    {
                        // lambda_0 is accounted in the antenna spacing uLoc and sLoc.
                        double rxPhaseDiff =
                            2 * M_PI *
                            (sinCosA[nIndex][mIndex] * uLoc.x + sinSinA[nIndex][mIndex] * uLoc.y +
                             cosZoA[nIndex][mIndex] * uLoc.z);

                        double txPhaseDiff =
                            2 * M_PI *
                            (sinCosD[nIndex][mIndex] * sLoc.x + sinSinD[nIndex][mIndex] * sLoc.y +
                             cosZoD[nIndex][mIndex] * sLoc.z);
                        // NOTE Doppler is computed in the CalcBeamformingGain function and is
                        // simplified to only account for the center angle of each cluster.
                        rays += raysPreComp[std::make_pair(sAntenna->GetElemPol(sIndex),
                                                           uAntenna->GetElemPol(uIndex))](nIndex,
                                                                                          mIndex) *
                                std::complex<double>(cos(rxPhaseDiff), sin(rxPhaseDiff)) *
                                std::complex<double>(cos(txPhaseDiff), sin(txPhaseDiff));
                    }
                    rays *=
                        sqrt(channelParams->m_clusterPower[nIndex] / table3gpp->m_raysPerCluster);
                    hUsn(uIndex, sIndex, nIndex) = rays;
                }
                else //(7.5-28)
                {
                    std::complex<double> raysSub1(0, 0);
                    std::complex<double> raysSub2(0, 0);
                    std::complex<double> raysSub3(0, 0);

                    for (uint8_t mIndex = 0; mIndex < table3gpp->m_raysPerCluster; mIndex++)
                    {
                        // ZML:Just remind me that the angle offsets for the 3 subclusters were not
                        // generated correctly.
                        double rxPhaseDiff =
                            2 * M_PI *
                            (sinCosA[nIndex][mIndex] * uLoc.x + sinSinA[nIndex][mIndex] * uLoc.y +
                             cosZoA[nIndex][mIndex] * uLoc.z);

                        double txPhaseDiff =
                            2 * M_PI *
                            (sinCosD[nIndex][mIndex] * sLoc.x + sinSinD[nIndex][mIndex] * sLoc.y +
                             cosZoD[nIndex][mIndex] * sLoc.z);

                        std::complex<double> raySub =
                            raysPreComp[std::make_pair(sAntenna->GetElemPol(sIndex),
                                                       uAntenna->GetElemPol(uIndex))](nIndex,
                                                                                      mIndex) *
                            std::complex<double>(cos(rxPhaseDiff), sin(rxPhaseDiff)) *
                            std::complex<double>(cos(txPhaseDiff), sin(txPhaseDiff));

                        switch (mIndex)
                        {
                        case 9:
                        case 10:
                        case 11:
                        case 12:
                        case 17:
                        case 18:
                            raysSub2 += raySub;
                            break;
                        case 13:
                        case 14:
                        case 15:
                        case 16:
                            raysSub3 += raySub;
                            break;
                        default: // case 1,2,3,4,5,6,7,8,19,20
                            raysSub1 += raySub;
                            break;
                        }
                    }
                    raysSub1 *=
                        sqrt(channelParams->m_clusterPower[nIndex] / table3gpp->m_raysPerCluster);
                    raysSub2 *=
                        sqrt(channelParams->m_clusterPower[nIndex] / table3gpp->m_raysPerCluster);
                    raysSub3 *=
                        sqrt(channelParams->m_clusterPower[nIndex] / table3gpp->m_raysPerCluster);
                    hUsn(uIndex, sIndex, nIndex) = raysSub1;
                    hUsn(uIndex,
                         sIndex,
                         channelParams->m_reducedClusterNumber + numSubClustersAdded) = raysSub2;
                    hUsn(uIndex,
                         sIndex,
                         channelParams->m_reducedClusterNumber + numSubClustersAdded + 1) =
                        raysSub3;
                }
            }
        }
        if (nIndex == channelParams->m_cluster1st || nIndex == channelParams->m_cluster2nd)
        {
            numSubClustersAdded += 2;
        }
    }

    if (channelParams->m_losCondition == ChannelCondition::LOS) //(7.5-29) && (7.5-30)
    {
        double lambda = 3.0e8 / m_frequency; // the wavelength of the carrier frequency
        std::complex<double> phaseDiffDueToDistance(cos(-2 * M_PI * distance3D / lambda),
                                                    sin(-2 * M_PI * distance3D / lambda));

        const double sinUAngleIncl = sin(uAngle.GetInclination());
        const double cosUAngleIncl = cos(uAngle.GetInclination());
        const double sinUAngleAz = sin(uAngle.GetAzimuth());
        const double cosUAngleAz = cos(uAngle.GetAzimuth());
        const double sinSAngleIncl = sin(sAngle.GetInclination());
        const double cosSAngleIncl = cos(sAngle.GetInclination());
        const double sinSAngleAz = sin(sAngle.GetAzimuth());
        const double cosSAngleAz = cos(sAngle.GetAzimuth());

        for (size_t uIndex = 0; uIndex < uSize; uIndex++)
        {
            Vector uLoc = uAntenna->GetElementLocation(uIndex);
            double rxPhaseDiff = 2 * M_PI *
                                 (sinUAngleIncl * cosUAngleAz * uLoc.x +
                                  sinUAngleIncl * sinUAngleAz * uLoc.y + cosUAngleIncl * uLoc.z);

            for (size_t sIndex = 0; sIndex < sSize; sIndex++)
            {
                Vector sLoc = sAntenna->GetElementLocation(sIndex);
                std::complex<double> ray(0, 0);
                double txPhaseDiff =
                    2 * M_PI *
                    (sinSAngleIncl * cosSAngleAz * sLoc.x + sinSAngleIncl * sinSAngleAz * sLoc.y +
                     cosSAngleIncl * sLoc.z);

                auto [rxFieldPatternPhi, rxFieldPatternTheta] = uAntenna->GetElementFieldPattern(
                    Angles(uAngle.GetAzimuth(), uAngle.GetInclination()),
                    uAntenna->GetElemPol(uIndex));
                auto [txFieldPatternPhi, txFieldPatternTheta] = sAntenna->GetElementFieldPattern(
                    Angles(sAngle.GetAzimuth(), sAngle.GetInclination()),
                    sAntenna->GetElemPol(sIndex));

                ray = (rxFieldPatternTheta * txFieldPatternTheta -
                       rxFieldPatternPhi * txFieldPatternPhi) *
                      phaseDiffDueToDistance *
                      std::complex<double>(cos(rxPhaseDiff), sin(rxPhaseDiff)) *
                      std::complex<double>(cos(txPhaseDiff), sin(txPhaseDiff));

                double kLinear = pow(10, channelParams->m_K_factor / 10.0);
                // the LOS path should be attenuated if blockage is enabled.
                hUsn(uIndex, sIndex, 0) =
                    sqrt(1.0 / (kLinear + 1)) * hUsn(uIndex, sIndex, 0) +
                    sqrt(kLinear / (1 + kLinear)) * ray /
                        pow(10,
                            channelParams->m_attenuation_dB[0] / 10.0); //(7.5-30) for tau = tau1
                for (size_t nIndex = 1; nIndex < hUsn.GetNumPages(); nIndex++)
                {
                    hUsn(uIndex, sIndex, nIndex) *=
                        sqrt(1.0 / (kLinear + 1)); //(7.5-30) for tau = tau2...tauN
                }
            }
        }
    }

    NS_LOG_DEBUG("Husn (sAntenna, uAntenna):" << sAntenna->GetId() << ", " << uAntenna->GetId());
    for (size_t cIndex = 0; cIndex < hUsn.GetNumPages(); cIndex++)
    {
        for (size_t rowIdx = 0; rowIdx < hUsn.GetNumRows(); rowIdx++)
        {
            for (size_t colIdx = 0; colIdx < hUsn.GetNumCols(); colIdx++)
            {
                NS_LOG_DEBUG(" " << hUsn(rowIdx, colIdx, cIndex) << ",");
            }
        }
    }

    NS_LOG_INFO("size of coefficient matrix (rows, columns, clusters) = ("
                << hUsn.GetNumRows() << ", " << hUsn.GetNumCols() << ", " << hUsn.GetNumPages()
                << ")");
    channelMatrix->m_channel = hUsn;
    return channelMatrix;
}

std::pair<double, double>
ThreeGppChannelModel::WrapAngles(double azimuthRad, double inclinationRad)
{
    inclinationRad = WrapTo2Pi(inclinationRad);
    if (inclinationRad > M_PI)
    {
        // inclination must be in [0, M_PI]
        inclinationRad -= M_PI;
        azimuthRad += M_PI;
    }

    azimuthRad = WrapTo2Pi(azimuthRad);

    NS_ASSERT_MSG(0 <= inclinationRad && inclinationRad <= M_PI,
                  "inclinationRad=" << inclinationRad << " not valid, should be in [0, pi]");
    NS_ASSERT_MSG(0 <= azimuthRad && azimuthRad <= 2 * M_PI,
                  "azimuthRad=" << azimuthRad << " not valid, should be in [0, 2*pi]");

    return std::make_pair(azimuthRad, inclinationRad);
}

MatrixBasedChannelModel::DoubleVector
ThreeGppChannelModel::CalcAttenuationOfBlockage(
    const Ptr<ThreeGppChannelModel::ThreeGppChannelParams> channelParams,
    const DoubleVector& clusterAOA,
    const DoubleVector& clusterZOA) const
{
    NS_LOG_FUNCTION(this);

    auto clusterNum = clusterAOA.size();

    // Initial power attenuation for all clusters to be 0 dB
    DoubleVector powerAttenuation(clusterNum, 0);

    // step a: the number of non-self blocking blockers is stored in m_numNonSelfBlocking.

    // step b:Generate the size and location of each blocker
    // generate self blocking (i.e., for blockage from the human body)
    // table 7.6.4.1-1 Self-blocking region parameters.
    //  Defaults: landscape mode
    double phiSb = 40;
    double xSb = 160;
    double thetaSb = 110;
    double ySb = 75;
    if (m_portraitMode)
    {
        phiSb = 260;
        xSb = 120;
        thetaSb = 100;
        ySb = 80;
    }

    // generate or update non-self blocking
    if (channelParams->m_nonSelfBlocking.empty()) // generate new blocking regions
    {
        for (uint16_t blockInd = 0; blockInd < m_numNonSelfBlocking; blockInd++)
        {
            // draw value from table 7.6.4.1-2 Blocking region parameters
            DoubleVector table;
            table.push_back(m_normalRv->GetValue()); // phi_k: store the normal RV that will be
                                                     // mapped to uniform (0,360) later.
            if (m_scenario == "InH-OfficeMixed" || m_scenario == "InH-OfficeOpen")
            {
                table.push_back(m_uniformRv->GetValue(15, 45)); // x_k
                table.push_back(90);                            // Theta_k
                table.push_back(m_uniformRv->GetValue(5, 15));  // y_k
                table.push_back(2);                             // r
            }
            else
            {
                table.push_back(m_uniformRv->GetValue(5, 15)); // x_k
                table.push_back(90);                           // Theta_k
                table.push_back(5);                            // y_k
                table.push_back(10);                           // r
            }
            channelParams->m_nonSelfBlocking.push_back(table);
        }
    }
    else
    {
        double deltaX = sqrt(pow(channelParams->m_preLocUT.x - channelParams->m_locUT.x, 2) +
                             pow(channelParams->m_preLocUT.y - channelParams->m_locUT.y, 2));
        // if deltaX and speed are both 0, the autocorrelation is 1, skip updating
        if (deltaX > 1e-6 || m_blockerSpeed > 1e-6)
        {
            double corrDis;
            // draw value from table 7.6.4.1-4: Spatial correlation distance for different
            // m_scenarios.
            if (m_scenario == "InH-OfficeMixed" || m_scenario == "InH-OfficeOpen")
            {
                // InH, correlation distance = 5;
                corrDis = 5;
            }
            else
            {
                if (channelParams->m_o2iCondition == ChannelCondition::O2I) // outdoor to indoor
                {
                    corrDis = 5;
                }
                else // LOS or NLOS
                {
                    corrDis = 10;
                }
            }
            double R;
            if (m_blockerSpeed > 1e-6) // speed not equal to 0
            {
                double corrT = corrDis / m_blockerSpeed;
                R = exp(-1 * (deltaX / corrDis +
                              (Now().GetSeconds() - channelParams->m_generatedTime.GetSeconds()) /
                                  corrT));
            }
            else
            {
                R = exp(-1 * (deltaX / corrDis));
            }

            NS_LOG_INFO("Distance change:"
                        << deltaX << " Speed:" << m_blockerSpeed << " Time difference:"
                        << Now().GetSeconds() - channelParams->m_generatedTime.GetSeconds()
                        << " correlation:" << R);

            // In order to generate correlated uniform random variables, we first generate
            // correlated normal random variables and map the normal RV to uniform RV. Notice the
            // correlation will change if the RV is transformed from normal to uniform. To
            // compensate the distortion, the correlation of the normal RV is computed such that the
            // uniform RV would have the desired correlation when transformed from normal RV.

            // The following formula was obtained from MATLAB numerical simulation.

            if (R * R * (-0.069) + R * 1.074 - 0.002 <
                1) // transform only when the correlation of normal RV is smaller than 1
            {
                R = R * R * (-0.069) + R * 1.074 - 0.002;
            }
            for (uint16_t blockInd = 0; blockInd < m_numNonSelfBlocking; blockInd++)
            {
                // Generate a new correlated normal RV with the following formula
                channelParams->m_nonSelfBlocking[blockInd][PHI_INDEX] =
                    R * channelParams->m_nonSelfBlocking[blockInd][PHI_INDEX] +
                    sqrt(1 - R * R) * m_normalRv->GetValue();
            }
        }
    }

    // step c: Determine the attenuation of each blocker due to blockers
    for (std::size_t cInd = 0; cInd < clusterNum; cInd++)
    {
        NS_ASSERT_MSG(clusterAOA[cInd] >= 0 && clusterAOA[cInd] <= 360,
                      "the AOA should be the range of [0,360]");
        NS_ASSERT_MSG(clusterZOA[cInd] >= 0 && clusterZOA[cInd] <= 180,
                      "the ZOA should be the range of [0,180]");

        // check self blocking
        NS_LOG_INFO("AOA=" << clusterAOA[cInd] << " Block Region[" << phiSb - xSb / 2.0 << ","
                           << phiSb + xSb / 2.0 << "]");
        NS_LOG_INFO("ZOA=" << clusterZOA[cInd] << " Block Region[" << thetaSb - ySb / 2.0 << ","
                           << thetaSb + ySb / 2.0 << "]");
        if (std::abs(clusterAOA[cInd] - phiSb) < (xSb / 2.0) &&
            std::abs(clusterZOA[cInd] - thetaSb) < (ySb / 2.0))
        {
            powerAttenuation[cInd] += 30; // attenuate by 30 dB.
            NS_LOG_INFO("Cluster[" << +cInd
                                   << "] is blocked by self blocking region and reduce 30 dB power,"
                                      "the attenuation is ["
                                   << powerAttenuation[cInd] << " dB]");
        }

        // check non-self blocking
        for (uint16_t blockInd = 0; blockInd < m_numNonSelfBlocking; blockInd++)
        {
            // The normal RV is transformed to uniform RV with the desired correlation.
            double phiK =
                (0.5 * erfc(-1 * channelParams->m_nonSelfBlocking[blockInd][PHI_INDEX] / sqrt(2))) *
                360;
            while (phiK > 360)
            {
                phiK -= 360;
            }

            while (phiK < 0)
            {
                phiK += 360;
            }

            double xK = channelParams->m_nonSelfBlocking[blockInd][X_INDEX];
            double thetaK = channelParams->m_nonSelfBlocking[blockInd][THETA_INDEX];
            double yK = channelParams->m_nonSelfBlocking[blockInd][Y_INDEX];

            NS_LOG_INFO("AOA=" << clusterAOA[cInd] << " Block Region[" << phiK - xK << ","
                               << phiK + xK << "]");
            NS_LOG_INFO("ZOA=" << clusterZOA[cInd] << " Block Region[" << thetaK - yK << ","
                               << thetaK + yK << "]");

            if (std::abs(clusterAOA[cInd] - phiK) < (xK) &&
                std::abs(clusterZOA[cInd] - thetaK) < (yK))
            {
                double A1 = clusterAOA[cInd] - (phiK + xK / 2.0);   //(7.6-24)
                double A2 = clusterAOA[cInd] - (phiK - xK / 2.0);   //(7.6-25)
                double Z1 = clusterZOA[cInd] - (thetaK + yK / 2.0); //(7.6-26)
                double Z2 = clusterZOA[cInd] - (thetaK - yK / 2.0); //(7.6-27)
                int signA1;
                int signA2;
                int signZ1;
                int signZ2;
                // draw sign for the above parameters according to table 7.6.4.1-3 Description of
                // signs
                if (xK / 2.0 < clusterAOA[cInd] - phiK && clusterAOA[cInd] - phiK <= xK)
                {
                    signA1 = -1;
                }
                else
                {
                    signA1 = 1;
                }
                if (-1 * xK < clusterAOA[cInd] - phiK && clusterAOA[cInd] - phiK <= -1 * xK / 2.0)
                {
                    signA2 = -1;
                }
                else
                {
                    signA2 = 1;
                }

                if (yK / 2.0 < clusterZOA[cInd] - thetaK && clusterZOA[cInd] - thetaK <= yK)
                {
                    signZ1 = -1;
                }
                else
                {
                    signZ1 = 1;
                }
                if (-1 * yK < clusterZOA[cInd] - thetaK &&
                    clusterZOA[cInd] - thetaK <= -1 * yK / 2.0)
                {
                    signZ2 = -1;
                }
                else
                {
                    signZ2 = 1;
                }
                double lambda = 3e8 / m_frequency;
                double fA1 =
                    atan(signA1 * M_PI / 2.0 *
                         sqrt(M_PI / lambda * channelParams->m_nonSelfBlocking[blockInd][R_INDEX] *
                              (1.0 / cos(DegreesToRadians(A1)) - 1))) /
                    M_PI; //(7.6-23)
                double fA2 =
                    atan(signA2 * M_PI / 2.0 *
                         sqrt(M_PI / lambda * channelParams->m_nonSelfBlocking[blockInd][R_INDEX] *
                              (1.0 / cos(DegreesToRadians(A2)) - 1))) /
                    M_PI;
                double fZ1 =
                    atan(signZ1 * M_PI / 2.0 *
                         sqrt(M_PI / lambda * channelParams->m_nonSelfBlocking[blockInd][R_INDEX] *
                              (1.0 / cos(DegreesToRadians(Z1)) - 1))) /
                    M_PI;
                double fZ2 =
                    atan(signZ2 * M_PI / 2.0 *
                         sqrt(M_PI / lambda * channelParams->m_nonSelfBlocking[blockInd][R_INDEX] *
                              (1.0 / cos(DegreesToRadians(Z2)) - 1))) /
                    M_PI;
                double lDb = -20 * log10(1 - (fA1 + fA2) * (fZ1 + fZ2)); //(7.6-22)
                powerAttenuation[cInd] += lDb;
                NS_LOG_INFO("Cluster[" << +cInd << "] is blocked by no-self blocking, the loss is ["
                                       << lDb << "] dB");
            }
        }
    }
    return powerAttenuation;
}

int64_t
ThreeGppChannelModel::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_normalRv->SetStream(stream);
    m_uniformRv->SetStream(stream + 1);
    m_uniformRvShuffle->SetStream(stream + 2);
    m_uniformRvDoppler->SetStream(stream + 3);
    return 4;
}

} // namespace ns3
