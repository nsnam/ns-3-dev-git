/*
 * Copyright (c) 2022 SIGNET Lab, Department of Information Engineering,
 * University of Padova
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
 */

#include "two-ray-spectrum-propagation-loss-model.h"

#include <ns3/double.h>
#include <ns3/log.h>
#include <ns3/node.h>
#include <ns3/object-factory.h>
#include <ns3/pointer.h>
#include <ns3/random-variable-stream.h>
#include <ns3/string.h>

#include <algorithm>

namespace ns3
{

/**
 * Lookup table associating the simulation parameters to the corresponding fitted FTR parameters.
 * The table is implemented as a nested map.
 */
static const TwoRaySpectrumPropagationLossModel::FtrParamsLookupTable
    SIM_PARAMS_TO_FTR_PARAMS_TABLE = {
        {"InH-OfficeOpen",
         {{ChannelCondition::LosConditionValue::NLOS,
           {{500000000.0,   5500000000.0,  10500000000.0, 15500000000.0, 20500000000.0,
             25500000000.0, 30500000000.0, 35500000000.0, 40500000000.0, 45500000000.0,
             50500000000.0, 55500000000.0, 60500000000.0, 65500000000.0, 70500000000.0,
             75500000000.0, 80500000000.0, 85500000000.0, 90500000000.0, 95500000000.0},
            {TwoRaySpectrumPropagationLossModel::FtrParams(1.2695458989293662e+01,
                                                           3.081006083786e-03,
                                                           1.6128465196199136e+02,
                                                           2.7586206896551727e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.253444859420023e+00,
                                                           6.334753732969e-04,
                                                           7.882966657847448e+02,
                                                           3.4482758620689657e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.2440475852275e+00,
                                                           8.688106642961e-04,
                                                           5.744993815655636e+02,
                                                           1.3793103448275863e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.249919762622172e+00,
                                                           4.2221674779498e-03,
                                                           1.1742258806909902e+02,
                                                           3.4482758620689657e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.25697129961412e+00,
                                                           4.2211041740365e-03,
                                                           1.174524189370726e+02,
                                                           6.896551724137931e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.2440475852275e+00,
                                                           5.7752786216822e-03,
                                                           8.557590962327636e+01,
                                                           1.724137931034e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.251094645653469e+00,
                                                           5.7927052949143e-03,
                                                           8.53154561719156e+01,
                                                           1.36482758620689e-02),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.724107824633563e+00,
                                                           1.295909468305e-04,
                                                           3.8572942113521017e+03,
                                                           6.9862068965517e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.737785902809381e+00,
                                                           2.44090689375e-04,
                                                           2.047418976079014e+03,
                                                           6.206896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.733508517158048e+00,
                                                           5.006171052321396e-05,
                                                           9.98667310933466e+03,
                                                           1.310344827586e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.742066005621902e+00,
                                                           9.428291321439076e-05,
                                                           5.302187851896829e+03,
                                                           1.03448275862e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.748062717676833e+00,
                                                           2.446180232004e-04,
                                                           2.0430031092487088e+03,
                                                           6.8896551724137e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.737785902809381e+00,
                                                           1.6399062365392e-03,
                                                           3.0389548052160666e+02,
                                                           8.965517241379312e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.735219145469659e+00,
                                                           1.296074026206e-04,
                                                           3.856804337484282e+03,
                                                           1.172413793103e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.736930208361434e+00,
                                                           1.294265037695e-04,
                                                           3.862196373520788e+03,
                                                           6.6965517241379e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.727524738792882e+00,
                                                           3.354626304689e-04,
                                                           1.4894789821180611e+03,
                                                           1.51724137931e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.26520488726192e+00,
                                                           1.07946501149867e-02,
                                                           4.531924098269983e+01,
                                                           6.206896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.736074622586126e+00,
                                                           2.2451744578083e-03,
                                                           2.2169984332891184e+02,
                                                           7.0965517241379e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.733508517158048e+00,
                                                           4.2274879823188e-03,
                                                           1.172735473385635e+02,
                                                           1.241379310344e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.729233846942953e+00,
                                                           3.360165965941e-04,
                                                           1.487021737818881e+03,
                                                           6.896551724137932e-06)}}},
          {ChannelCondition::LosConditionValue::LOS,
           {{500000000.0,   5500000000.0,  10500000000.0, 15500000000.0, 20500000000.0,
             25500000000.0, 30500000000.0, 35500000000.0, 40500000000.0, 45500000000.0,
             50500000000.0, 55500000000.0, 60500000000.0, 65500000000.0, 70500000000.0,
             75500000000.0, 80500000000.0, 85500000000.0, 90500000000.0, 95500000000.0},
            {TwoRaySpectrumPropagationLossModel::FtrParams(6.221618627242562e+01,
                                                           5.7818075439484e-03,
                                                           8.547814653106583e+01,
                                                           1.310344827586e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.222408869140862e+01,
                                                           5.7868906372814e-03,
                                                           8.540218579193503e+01,
                                                           1.931034482758e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.1712469578065404e+02,
                                                           7.9044249538951e-03,
                                                           6.225570840591133e+01,
                                                           2.e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.174524189370726e+02,
                                                           7.9192593798279e-03,
                                                           6.213721725968569e+01,
                                                           1.586206896551e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.212932587331828e+01,
                                                           4.2317491714902e-03,
                                                           1.1715445096995552e+02,
                                                           1.03448275862e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(5.74353468732457e+02,
                                                           7.9252008170168e-03,
                                                           6.208988397195024e+01,
                                                           5.517241379310345e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.295678257898671e+01,
                                                           1.6409445497067e-03,
                                                           3.0370255688368803e+02,
                                                           1.448275862068e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.1721398402813756e+02,
                                                           7.9222295504019e-03,
                                                           6.211354610705902e+01,
                                                           1.3793103448275863e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.395445815898452e+01,
                                                           6.863706760298528e-05,
                                                           7.283693496699635e+03,
                                                           2.06896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.220038444513843e+01,
                                                           5.7854378744067e-03,
                                                           8.542388196956914e+01,
                                                           1.3793103448275863e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.293586044359225e+01,
                                                           2.246026238725e-03,
                                                           2.2161538684598275e+02,
                                                           1.724137931034e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.534227035109218e+01,
                                                           4.2328151340042e-03,
                                                           1.1712469578065404e+02,
                                                           2.7586206896551727e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.296096860066171e+01,
                                                           1.1934793291051e-03,
                                                           4.179431587180458e+02,
                                                           9.655172413793105e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.395750074607037e+01,
                                                           4.990302323746383e-05,
                                                           1.0018433043580286e+04,
                                                           1.51724137931e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.1831489711041263e+02,
                                                           7.926191482854e-03,
                                                           6.20819985968797e+01,
                                                           1.3793103448275863e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.290659176610291e+01,
                                                           1.1937818051584e-03,
                                                           4.1783700843777945e+02,
                                                           1.448275862068e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.176778335658431e+02,
                                                           7.926191482854e-03,
                                                           6.20819985968797e+01,
                                                           2.e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.292331353831016e+01,
                                                           8.692513904904e-04,
                                                           5.742075929586678e+02,
                                                           1.3793103448275863e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.21474697532092e+02,
                                                           7.9212393717809e-03,
                                                           6.212143548915179e+01,
                                                           6.206896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.29902856434769e+01,
                                                           2.2511436839814e-03,
                                                           2.211093231666549e+02,
                                                           1.586206896551e-04)}}}}},
        {"UMi-StreetCanyon",
         {{ChannelCondition::LosConditionValue::NLOS,
           {{500000000.0,   5500000000.0,  10500000000.0, 15500000000.0, 20500000000.0,
             25500000000.0, 30500000000.0, 35500000000.0, 40500000000.0, 45500000000.0,
             50500000000.0, 55500000000.0, 60500000000.0, 65500000000.0, 70500000000.0,
             75500000000.0, 80500000000.0, 85500000000.0, 90500000000.0, 95500000000.0},
            {TwoRaySpectrumPropagationLossModel::FtrParams(1.2689010957474556e+01,
                                                           3.36059247219e-04,
                                                           1.486832887020803e+03,
                                                           1.724137931034e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.2734216047612705e+01,
                                                           8.70685292542e-04,
                                                           5.732603031000779e+02,
                                                           7.0551724137931e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.2690622658336885e+01,
                                                           3.086065900037e-03,
                                                           1.6101857516847045e+02,
                                                           0.e+00),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.2711593407712735e+01,
                                                           3.0864554585433e-03,
                                                           1.6099812591365952e+02,
                                                           1.931034482758e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.29902856434769e+01,
                                                           1.47463951390083e-02,
                                                           3.290659176610291e+01,
                                                           1.310344827586e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.2718052915001302e+01,
                                                           3.0884039842316e-03,
                                                           1.6089591858864094e+02,
                                                           1.51724137931e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.748207473187881e+01,
                                                           1.07973332444199e-02,
                                                           4.530773068511169e+01,
                                                           6.8068965517241e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.255795670251574e+00,
                                                           5.00680684988557e-05,
                                                           9.985404808314654e+03,
                                                           6.9586206896551e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.254620190193195e+00,
                                                           4.617679219256e-04,
                                                           1.0817950064503143e+03,
                                                           2.07655172413793e-02),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.7488737060551784e+01,
                                                           1.08201661923657e-02,
                                                           4.521001111357971e+01,
                                                           6.206896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.264028212239072e+00,
                                                           2.445869720667e-04,
                                                           2.0432626022764905e+03,
                                                           1.655172413793e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.251094645653469e+00,
                                                           1.295909468305e-04,
                                                           3.8572942113521017e+03,
                                                           1.655172413793e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.261675310485298e+00,
                                                           6.868066254013259e-05,
                                                           7.27906954953051e+03,
                                                           1.103448275862e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.242873597168312e+00,
                                                           2.443697244117e-04,
                                                           2.0450799765751449e+03,
                                                           1.241379310344e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.264028212239072e+00,
                                                           6.329934316604e-04,
                                                           7.888976118731497e+02,
                                                           6.7241379310344e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.2440475852275e+00,
                                                           5.0074427281897054e-05,
                                                           9.984136668368055e+03,
                                                           1.03448275862e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.242873597168312e+00,
                                                           1.7827515329e-04,
                                                           2.80365331692424e+03,
                                                           6.206896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.242873597168312e+00,
                                                           2.448665741566e-04,
                                                           2.040928351071028e+03,
                                                           1.37862068965517e-02),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.25697129961412e+00,
                                                           3.357181936422e-04,
                                                           1.4883443652113867e+03,
                                                           2.06896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.262851686653192e+00,
                                                           2.2474465885176e-03,
                                                           2.21474697532092e+02,
                                                           1.379310344827e-04)}}},
          {ChannelCondition::LosConditionValue::LOS,
           {{500000000.0,   5500000000.0,  10500000000.0, 15500000000.0, 20500000000.0,
             25500000000.0, 30500000000.0, 35500000000.0, 40500000000.0, 45500000000.0,
             50500000000.0, 55500000000.0, 60500000000.0, 65500000000.0, 70500000000.0,
             75500000000.0, 80500000000.0, 85500000000.0, 90500000000.0, 95500000000.0},
            {TwoRaySpectrumPropagationLossModel::FtrParams(6.212932587331828e+01,
                                                           3.0821730014407e-03,
                                                           1.6122321062648754e+02,
                                                           0.e+00),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.290659176610291e+01,
                                                           1.297720754633e-04,
                                                           3.8519090192542544e+03,
                                                           7.586206896551724e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.297771800493267e+01,
                                                           2.445559248725e-04,
                                                           2.0435221282639047e+03,
                                                           6.896551724137931e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.295259708893377e+01,
                                                           1.77777895565e-04,
                                                           2.81149813657053e+03,
                                                           6.896551724137932e-06),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.5181310193778565e+01,
                                                           1.6374169558852e-03,
                                                           3.043589974153497e+02,
                                                           1.51724137931e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.2965155154026256e+01,
                                                           6.879413898912483e-05,
                                                           7.267061020126167e+03,
                                                           6.206896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.29902856434769e+01,
                                                           6.85760811693159e-05,
                                                           7.290171957835979e+03,
                                                           6.896551724137932e-06),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.295259708893377e+01,
                                                           1.77935963353e-04,
                                                           2.8089996795357683e+03,
                                                           8.965517241379312e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.531348546843247e+01,
                                                           3.077119528339e-03,
                                                           1.614896255719667e+02,
                                                           6.896551724137932e-06),
             TwoRaySpectrumPropagationLossModel::FtrParams(7.281843565459175e+03,
                                                           7.9123332420076e-03,
                                                           6.219248503657932e+01,
                                                           2.06896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.8582741457603433e+03,
                                                           7.9143115301227e-03,
                                                           6.217668922899309e+01,
                                                           1.586206896551e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.2948412130435365e+01,
                                                           1.294922559454e-04,
                                                           3.860234761488555e+03,
                                                           1.51724137931e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(5.748643258446342e+02,
                                                           7.9143115301227e-03,
                                                           6.217668922899309e+01,
                                                           1.241379310344e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.2965155154026256e+01,
                                                           1.779585559315e-04,
                                                           2.8086429383948757e+03,
                                                           1.586206896551e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.215300303953259e+01,
                                                           3.0845081545813e-03,
                                                           1.6110039816473343e+02,
                                                           1.862068965517e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(7.27814511240983e+03,
                                                           7.9073896501251e-03,
                                                           6.223199211412119e+01,
                                                           7.586206896551724e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.299866672987281e+01,
                                                           1.779585559315e-04,
                                                           2.8086429383948757e+03,
                                                           4.137931034482759e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.8553350891312034e+03,
                                                           7.9054130644305e-03,
                                                           6.224780197124505e+01,
                                                           1.241379310344e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.292749530889876e+01,
                                                           3.362299038257e-04,
                                                           1.4860777236373208e+03,
                                                           6.206896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.5284718859438e+01,
                                                           2.2451744578083e-03,
                                                           2.2169984332891184e+02,
                                                           1.448275862068e-04)}}}}},
        {"InH-OfficeMixed",
         {{ChannelCondition::LosConditionValue::NLOS,
           {{500000000.0,   5500000000.0,  10500000000.0, 15500000000.0, 20500000000.0,
             25500000000.0, 30500000000.0, 35500000000.0, 40500000000.0, 45500000000.0,
             50500000000.0, 55500000000.0, 60500000000.0, 65500000000.0, 70500000000.0,
             75500000000.0, 80500000000.0, 85500000000.0, 90500000000.0, 95500000000.0},
            {TwoRaySpectrumPropagationLossModel::FtrParams(1.2726131915259918e+01,
                                                           4.2274879823188e-03,
                                                           1.172735473385635e+02,
                                                           1.793103448275e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.251094645653469e+00,
                                                           1.6390760575064e-03,
                                                           3.040499076660591e+02,
                                                           1.931034482758e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.261675310485298e+00,
                                                           8.70354182635e-04,
                                                           5.734787696501294e+02,
                                                           1.793103448275e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.26520488726192e+00,
                                                           4.2211041740365e-03,
                                                           1.174524189370726e+02,
                                                           1.793103448275e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.254620190193195e+00,
                                                           4.2179158550445e-03,
                                                           1.1754195701936764e+02,
                                                           1.586206896551e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.254620190193195e+00,
                                                           4.2226992294898e-03,
                                                           1.1740767547642784e+02,
                                                           6.7379310344827e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.731798323316221e+00,
                                                           9.44506754620425e-05,
                                                           5.292768388146024e+03,
                                                           1.03448275862e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.245221722401345e+00,
                                                           5.77890490064e-03,
                                                           8.55215829982986e+01,
                                                           1.51724137931e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.7258160647265015e+00,
                                                           6.329131436638e-04,
                                                           7.889978140847802e+02,
                                                           6.8344827586206e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.724107824633563e+00,
                                                           8.691411880231e-04,
                                                           5.74280526213736e+02,
                                                           6.896551724137931e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.736930208361434e+00,
                                                           2.448976607693e-04,
                                                           2.0406691544916384e+03,
                                                           1.37310344827586e-02),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.731798323316221e+00,
                                                           8.682600702285e-04,
                                                           5.748643258446342e+02,
                                                           1.793103448275e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.737785902809381e+00,
                                                           1.6407368346666e-03,
                                                           3.03741131810801e+02,
                                                           1.724137931034e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.736930208361434e+00,
                                                           1.77935963353e-04,
                                                           2.8089996795357683e+03,
                                                           1.03448275862e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.23935252748936e+00,
                                                           7.910355440443e-03,
                                                           6.22082848570447e+01,
                                                           6.7379310344827e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.723253867292691e+00,
                                                           4.610068300136e-04,
                                                           1.0835826296873358e+03,
                                                           1.172413793103e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.723253867292691e+00,
                                                           6.344403562566e-04,
                                                           7.870961465788879e+02,
                                                           2.e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.744635372264776e+00,
                                                           6.33636101933e-04,
                                                           7.880964521665514e+02,
                                                           1.379310344827e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.728379238600564e+00,
                                                           4.603053960641e-04,
                                                           1.0852353652060228e+03,
                                                           4.827586206896552e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.740353638327108e+00,
                                                           2.2468783413341e-03,
                                                           2.2153096253672345e+02,
                                                           2.e-04)}}},
          {ChannelCondition::LosConditionValue::LOS,
           {{500000000.0,   5500000000.0,  10500000000.0, 15500000000.0, 20500000000.0,
             25500000000.0, 30500000000.0, 35500000000.0, 40500000000.0, 45500000000.0,
             50500000000.0, 55500000000.0, 60500000000.0, 65500000000.0, 70500000000.0,
             75500000000.0, 80500000000.0, 85500000000.0, 90500000000.0, 95500000000.0},
            {TwoRaySpectrumPropagationLossModel::FtrParams(3.857539171618605e+03,
                                                           1.08074007896676e-02,
                                                           4.526459309976018e+01,
                                                           0.e+00),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.296096860066171e+01,
                                                           1.194841073721e-03,
                                                           4.174656947244478e+02,
                                                           1.310344827586e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.2150282824788948e+02,
                                                           7.9271822705314e-03,
                                                           6.207411422324672e+01,
                                                           1.103448275862e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.520426947143501e+01,
                                                           4.2317491714902e-03,
                                                           1.1715445096995552e+02,
                                                           1.931034482758e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(5.749373425149475e+02,
                                                           7.9281731800636e-03,
                                                           6.206623085092408e+01,
                                                           1.793103448275e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.221618627242562e+01,
                                                           5.7796304264225e-03,
                                                           8.551072181262026e+01,
                                                           7.586206896551724e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.1719909792804452e+02,
                                                           7.9014613513626e-03,
                                                           6.227943373585816e+01,
                                                           1.448275862068e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.289405601071394e+01,
                                                           3.355052108261e-04,
                                                           1.4892898192515456e+03,
                                                           8.965517241379312e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.294004380783209e+01,
                                                           1.6378315743006e-03,
                                                           3.042816955330164e+02,
                                                           2.7586206896551727e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.1724376190085948e+02,
                                                           7.9162903049039e-03,
                                                           6.2160897433266726e+01,
                                                           0.e+00),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.2965155154026256e+01,
                                                           1.1936305575734e-03,
                                                           4.178900802074444e+02,
                                                           6.896551724137932e-06),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.291495159031198e+01,
                                                           2.247730765756e-03,
                                                           2.214465703888772e+02,
                                                           5.517241379310345e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.045523380497429e+02,
                                                           7.9133223252391e-03,
                                                           6.218458663124002e+01,
                                                           1.862068965517e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.2150282824788948e+02,
                                                           7.9133223252391e-03,
                                                           6.218458663124002e+01,
                                                           6.896551724137931e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.2948412130435365e+01,
                                                           1.6409445497067e-03,
                                                           3.0370255688368803e+02,
                                                           2.e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.040112935246124e+02,
                                                           7.9153008566728e-03,
                                                           6.216879282971112e+01,
                                                           1.793103448275e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.526171872148543e+01,
                                                           4.2232310474297e-03,
                                                           1.1739276477764624e+02,
                                                           7.586206896551724e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.6130513761004684e+02,
                                                           7.9143115301227e-03,
                                                           6.217668922899309e+01,
                                                           1.862068965517e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.04436318936002e+02,
                                                           7.9242102730054e-03,
                                                           6.2097770348585485e+01,
                                                           2.06896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.0417061383577973e+03,
                                                           1.08121021183973e-02,
                                                           4.524447628451687e+01,
                                                           2.e-04)}}}}},
        {"RMa",
         {{ChannelCondition::LosConditionValue::NLOS,
           {{500000000.0,   5500000000.0,  10500000000.0, 15500000000.0, 20500000000.0,
             25500000000.0, 30500000000.0, 35500000000.0, 40500000000.0, 45500000000.0,
             50500000000.0, 55500000000.0, 60500000000.0, 65500000000.0, 70500000000.0,
             75500000000.0, 80500000000.0, 85500000000.0, 90500000000.0, 95500000000.0},
            {TwoRaySpectrumPropagationLossModel::FtrParams(6.736074622586126e+00,
                                                           3.353349217972e-04,
                                                           1.4900466148894875e+03,
                                                           7.0275862068965e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.745492045387843e+00,
                                                           6.33073729829e-04,
                                                           7.887974223871352e+02,
                                                           6.9862068965517e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.26520488726192e+00,
                                                           1.08228555237886e-02,
                                                           4.5198528558475154e+01,
                                                           8.275862068965519e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.724107824633563e+00,
                                                           6.333146853796e-04,
                                                           7.884969302666273e+02,
                                                           4.827586206896552e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.727524738792882e+00,
                                                           1.295580415165e-04,
                                                           3.8582741457603433e+03,
                                                           1.793103448275e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.736930208361434e+00,
                                                           1.297555987707e-04,
                                                           3.8523982713408623e+03,
                                                           8.275862068965519e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.731798323316221e+00,
                                                           2.448665741566e-04,
                                                           2.040928351071028e+03,
                                                           1.37724137931034e-02),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.2700297163351271e+01,
                                                           1.47354930616003e-02,
                                                           3.293167761063697e+01,
                                                           3.4482758620689657e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.7258160647265015e+00,
                                                           1.780941716339e-04,
                                                           2.80650344277193e+03,
                                                           1.37310344827586e-02),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.235832799160756e+00,
                                                           7.910355440443e-03,
                                                           6.22082848570447e+01,
                                                           8.275862068965519e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.741209767603669e+00,
                                                           9.430686099733985e-05,
                                                           5.300841188565312e+03,
                                                           2.7586206896551727e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.736930208361434e+00,
                                                           4.613579473452e-04,
                                                           1.0827572060417654e+03,
                                                           2.7586206896551727e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.735219145469659e+00,
                                                           3.363579531122e-04,
                                                           1.4855116028137206e+03,
                                                           6.8206896551724e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.741209767603669e+00,
                                                           8.698026120933e-04,
                                                           5.738430655969447e+02,
                                                           2.06896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.742066005621902e+00,
                                                           4.990302323746383e-05,
                                                           1.0018433043580286e+04,
                                                           6.896551724137931e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.241699758204852e+00,
                                                           5.7934325329886e-03,
                                                           8.530462116421079e+01,
                                                           9.655172413793105e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.741209767603669e+00,
                                                           3.355052108261e-04,
                                                           1.4892898192515456e+03,
                                                           1.379310344827e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.2700297163351271e+01,
                                                           1.47282293521982e-02,
                                                           3.2948412130435365e+01,
                                                           1.3793103448275863e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.26520488726192e+00,
                                                           5.7956147886341e-03,
                                                           8.527212439663107e+01,
                                                           6.9448275862068e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.728379238600564e+00,
                                                           4.996643770687291e-05,
                                                           1.0005716967361968e+04,
                                                           6.896551724137931e-05)}}},
          {ChannelCondition::LosConditionValue::LOS,
           {{500000000.0,   5500000000.0,  10500000000.0, 15500000000.0, 20500000000.0,
             25500000000.0, 30500000000.0, 35500000000.0, 40500000000.0, 45500000000.0,
             50500000000.0, 55500000000.0, 60500000000.0, 65500000000.0, 70500000000.0,
             75500000000.0, 80500000000.0, 85500000000.0, 90500000000.0, 95500000000.0},
            {TwoRaySpectrumPropagationLossModel::FtrParams(2.804365576918811e+03,
                                                           1.47100852650808e-02,
                                                           3.29902856434769e+01,
                                                           2.06896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.2153096253672345e+02,
                                                           1.08134457224711e-02,
                                                           4.523873026531778e+01,
                                                           1.103448275862e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.171693313991936e+02,
                                                           1.08174775144271e-02,
                                                           4.522149658579409e+01,
                                                           8.275862068965519e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.400013756546019e+01,
                                                           1.193328119751e-03,
                                                           4.1799624397043993e+02,
                                                           6.8620689655172e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.6144861002079188e+02,
                                                           1.07973332444199e-02,
                                                           4.530773068511169e+01,
                                                           1.793103448275e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.8125695840352355e+03,
                                                           1.4702833700715e-02,
                                                           3.300704994545942e+01,
                                                           1.448275862068e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.520426947143501e+01,
                                                           7.9162903049039e-03,
                                                           6.2160897433266726e+01,
                                                           6.8068965517241e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.0830322773699647e+03,
                                                           1.47445775831495e-02,
                                                           3.2910771412767524e+01,
                                                           9.655172413793105e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.5296223310938565e+01,
                                                           7.9014613513626e-03,
                                                           6.227943373585816e+01,
                                                           2.7586206896551727e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.859254329118079e+03,
                                                           1.47245987995756e-02,
                                                           3.295678257898671e+01,
                                                           1.793103448275e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.1719909792804452e+02,
                                                           1.0795991598188e-02,
                                                           4.531348546843247e+01,
                                                           1.241379310344e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.2160897433266726e+01,
                                                           7.9113442804138e-03,
                                                           6.220038444513843e+01,
                                                           4.827586206896552e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.400013756546019e+01,
                                                           3.0778964494433e-03,
                                                           1.6144861002079188e+02,
                                                           1.931034482758e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.7439938881994646e+01,
                                                           6.334753732969e-04,
                                                           7.882966657847448e+02,
                                                           1.3793103448275863e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.53307542044457e+01,
                                                           7.9153008566728e-03,
                                                           6.216879282971112e+01,
                                                           7.586206896551724e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.2965155154026256e+01,
                                                           5.7781794648689e-03,
                                                           8.553244556351568e+01,
                                                           1.3793103448275863e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(8.540218579193503e+01,
                                                           1.07973332444199e-02,
                                                           4.530773068511169e+01,
                                                           1.172413793103e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.7439938881994646e+01,
                                                           9.44506754620425e-05,
                                                           5.292768388146024e+03,
                                                           3.4482758620689657e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.74443694478918e+01,
                                                           1.297391241696e-04,
                                                           3.8528875855700608e+03,
                                                           5.517241379310345e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(8.535880996662047e+01,
                                                           1.07946501149867e-02,
                                                           4.531924098269983e+01,
                                                           1.379310344827e-04)}}}}},
        {"UMa",
         {{ChannelCondition::LosConditionValue::NLOS,
           {{500000000.0,   5500000000.0,  10500000000.0, 15500000000.0, 20500000000.0,
             25500000000.0, 30500000000.0, 35500000000.0, 40500000000.0, 45500000000.0,
             50500000000.0, 55500000000.0, 60500000000.0, 65500000000.0, 70500000000.0,
             75500000000.0, 80500000000.0, 85500000000.0, 90500000000.0, 95500000000.0},
            {TwoRaySpectrumPropagationLossModel::FtrParams(1.7439938881994646e+01,
                                                           2.2485835121112e-03,
                                                           2.2136221039019213e+02,
                                                           7.e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.399708956352588e+01,
                                                           7.909366722081e-03,
                                                           6.221618627242562e+01,
                                                           3.4482758620689657e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.7490958400945924e+01,
                                                           3.0778964494433e-03,
                                                           1.6144861002079188e+02,
                                                           2.e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.2700297163351271e+01,
                                                           8.689208249277e-04,
                                                           5.744264205160073e+02,
                                                           4.137931034482759e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.2700297163351271e+01,
                                                           4.993472040654454e-05,
                                                           1.00120729866762e+04,
                                                           1.241379310344e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.272451570474495e+01,
                                                           2.447112002444e-04,
                                                           2.0422248278813e+03,
                                                           1.51724137931e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.2716437730514135e+01,
                                                           2.2511436839814e-03,
                                                           2.211093231666549e+02,
                                                           6.903448275862e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.2701910297736497e+01,
                                                           4.2253589833499e-03,
                                                           1.1733314091660613e+02,
                                                           1.103448275862e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.2687399461297195e+01,
                                                           4.2205726216469e-03,
                                                           1.1746733721285608e+02,
                                                           7.0965517241379e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.253444859420023e+00,
                                                           9.42948863457656e-05,
                                                           5.301514477472012e+03,
                                                           1.586206896551e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.2693846674220437e+01,
                                                           5.7767288633179e-03,
                                                           8.555417483326706e+01,
                                                           6.896551724137932e-06),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.2722899699487671e+01,
                                                           5.7883437606822e-03,
                                                           8.538049512475196e+01,
                                                           8.965517241379312e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.2440475852275e+00,
                                                           1.781620182218e-04,
                                                           2.805434306201756e+03,
                                                           1.931034482758e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.237005892929576e+00,
                                                           4.994740491021047e-05,
                                                           1.0009530094583308e+04,
                                                           1.586206896551e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.252269677913102e+00,
                                                           1.29541591992e-04,
                                                           3.858764206316572e+03,
                                                           1.931034482758e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.269913082102622e+00,
                                                           1.295909468305e-04,
                                                           3.8572942113521017e+03,
                                                           6.9310344827586e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.24874502880026e+00,
                                                           2.443387047809e-04,
                                                           2.0453397333972343e+03,
                                                           6.206896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.262851686653192e+00,
                                                           9.421110633952792e-05,
                                                           5.306229894934544e+03,
                                                           1.379310344827e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.237005892929576e+00,
                                                           1.298050351238e-04,
                                                           3.8509307014772426e+03,
                                                           1.38137931034482e-02),
             TwoRaySpectrumPropagationLossModel::FtrParams(9.24757044416878e+00,
                                                           1.297391241696e-04,
                                                           3.8528875855700608e+03,
                                                           1.103448275862e-04)}}},
          {ChannelCondition::LosConditionValue::LOS,
           {{500000000.0,   5500000000.0,  10500000000.0, 15500000000.0, 20500000000.0,
             25500000000.0, 30500000000.0, 35500000000.0, 40500000000.0, 45500000000.0,
             50500000000.0, 55500000000.0, 60500000000.0, 65500000000.0, 70500000000.0,
             75500000000.0, 80500000000.0, 85500000000.0, 90500000000.0, 95500000000.0},
            {TwoRaySpectrumPropagationLossModel::FtrParams(8.553244556351568e+01,
                                                           3.0845081545813e-03,
                                                           1.6110039816473343e+02,
                                                           2.e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(8.556504153815182e+01,
                                                           4.2290854294851e-03,
                                                           1.1722887201899576e+02,
                                                           6.896551724137932e-06),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.531348546843247e+01,
                                                           1.1945383299169e-03,
                                                           4.175717506735425e+02,
                                                           8.965517241379312e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.534227035109218e+01,
                                                           1.6382462973584e-03,
                                                           3.042044132840143e+02,
                                                           1.03448275862e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.6142810615183308e+02,
                                                           5.7847116281393e-03,
                                                           8.543473212524282e+01,
                                                           1.448275862068e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.0824822045768908e+03,
                                                           7.9271822705314e-03,
                                                           6.207411422324672e+01,
                                                           1.03448275862e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(5.307577927251286e+03,
                                                           7.9064012965024e-03,
                                                           6.223989654069086e+01,
                                                           6.7655172413793e-03),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.289823406514352e+01,
                                                           1.1928746063501e-03,
                                                           4.1815554018694206e+02,
                                                           1.793103448275e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(5.742075929586678e+02,
                                                           7.9143115301227e-03,
                                                           6.217668922899309e+01,
                                                           6.206896551724138e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.215300303953259e+01,
                                                           5.7847116281393e-03,
                                                           8.543473212524282e+01,
                                                           6.896551724137931e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.290659176610291e+01,
                                                           8.689208249277e-04,
                                                           5.744264205160073e+02,
                                                           1.724137931034e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.2175616553118e+02,
                                                           7.909366722081e-03,
                                                           6.221618627242562e+01,
                                                           4.827586206896552e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.220038444513843e+01,
                                                           5.7752786216822e-03,
                                                           8.557590962327636e+01,
                                                           4.137931034482759e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(4.5181310193778565e+01,
                                                           4.2216357928014e-03,
                                                           1.1743750255590038e+02,
                                                           1.862068965517e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.299447592055982e+01,
                                                           2.2485835121112e-03,
                                                           2.2136221039019213e+02,
                                                           1.448275862068e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(6.206623085092408e+01,
                                                           5.7803560422275e-03,
                                                           8.549986200630548e+01,
                                                           1.931034482758e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(1.1713957243052236e+02,
                                                           7.9153008566728e-03,
                                                           6.216879282971112e+01,
                                                           4.827586206896552e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.4024535522229847e+01,
                                                           6.325118562159e-04,
                                                           7.894990160837534e+02,
                                                           1.448275862068e-04),
             TwoRaySpectrumPropagationLossModel::FtrParams(2.394229167421632e+01,
                                                           1.778907867973e-04,
                                                           2.809713297758256e+03,
                                                           8.275862068965519e-05),
             TwoRaySpectrumPropagationLossModel::FtrParams(3.2910771412767524e+01,
                                                           2.2474465885176e-03,
                                                           2.21474697532092e+02,
                                                           7.586206896551724e-05)}}}}},
};

NS_LOG_COMPONENT_DEFINE("TwoRaySpectrumPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED(TwoRaySpectrumPropagationLossModel);

TwoRaySpectrumPropagationLossModel::TwoRaySpectrumPropagationLossModel()
{
    NS_LOG_FUNCTION(this);

    // Create the Random Number Generator (RNG) variables only once to speed-up the tests
    m_uniformRv = CreateObject<UniformRandomVariable>();
    m_uniformRv->SetAttribute("Min", DoubleValue(0));
    m_uniformRv->SetAttribute("Max", DoubleValue(2 * M_PI));

    m_normalRv = CreateObject<NormalRandomVariable>();
    m_normalRv->SetAttribute("Mean", DoubleValue(0));

    m_gammaRv = CreateObject<GammaRandomVariable>();
}

TwoRaySpectrumPropagationLossModel::~TwoRaySpectrumPropagationLossModel()
{
    NS_LOG_FUNCTION(this);

    m_uniformRv = nullptr;
    m_normalRv = nullptr;
    m_gammaRv = nullptr;
}

void
TwoRaySpectrumPropagationLossModel::DoDispose()
{
}

TypeId
TwoRaySpectrumPropagationLossModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TwoRaySpectrumPropagationLossModel")
            .SetParent<PhasedArraySpectrumPropagationLossModel>()
            .SetGroupName("Spectrum")
            .AddConstructor<TwoRaySpectrumPropagationLossModel>()
            .AddAttribute(
                "ChannelConditionModel",
                "Pointer to the channel condition model.",
                PointerValue(),
                MakePointerAccessor(&TwoRaySpectrumPropagationLossModel::m_channelConditionModel),
                MakePointerChecker<ChannelConditionModel>())
            .AddAttribute(
                "Scenario",
                "The 3GPP scenario (RMa, UMa, UMi-StreetCanyon, InH-OfficeOpen, InH-OfficeMixed).",
                StringValue("RMa"),
                MakeStringAccessor(&TwoRaySpectrumPropagationLossModel::SetScenario),
                MakeStringChecker())
            .AddAttribute("Frequency",
                          "The operating Frequency in Hz",
                          DoubleValue(500.0e6),
                          MakeDoubleAccessor(&TwoRaySpectrumPropagationLossModel::SetFrequency),
                          MakeDoubleChecker<double>());
    return tid;
}

void
TwoRaySpectrumPropagationLossModel::SetScenario(const std::string& scenario)
{
    NS_LOG_FUNCTION(this);
    if (scenario != "RMa" && scenario != "UMa" && scenario != "UMi-StreetCanyon" &&
        scenario != "InH-OfficeOpen" && scenario != "InH-OfficeMixed" && scenario != "V2V-Urban" &&
        scenario != "V2V-Highway")
    {
        NS_ABORT_MSG("Unknown scenario (" + scenario +
                     "), choose between: RMa, UMa, UMi-StreetCanyon, "
                     "InH-OfficeOpen, InH-OfficeMixed, V2V-Urban or V2V-Highway");
    }

    if (SIM_PARAMS_TO_FTR_PARAMS_TABLE.find(scenario) == SIM_PARAMS_TO_FTR_PARAMS_TABLE.end())
    {
        NS_ABORT_MSG("The specified scenario has not been calibrated yet.");
    }

    m_scenario = scenario;
}

void
TwoRaySpectrumPropagationLossModel::SetFrequency(double f)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(f >= 500.0e6 && f <= 100.0e9,
                  "Frequency should be between 0.5 and 100 GHz but is " << f);
    m_frequency = f;
}

ChannelCondition::LosConditionValue
TwoRaySpectrumPropagationLossModel::GetLosCondition(Ptr<const MobilityModel> a,
                                                    Ptr<const MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT(m_channelConditionModel);
    auto cond = m_channelConditionModel->GetChannelCondition(a, b);

    return cond->GetLosCondition();
}

TwoRaySpectrumPropagationLossModel::FtrParams
TwoRaySpectrumPropagationLossModel::GetFtrParameters(Ptr<const MobilityModel> a,
                                                     Ptr<const MobilityModel> b) const
{
    NS_LOG_FUNCTION(this);

    // Retrieve LOS condition
    ChannelCondition::LosConditionValue cond = GetLosCondition(a, b);

    // Retrieve the corresponding tuple and vectors
    NS_ASSERT_MSG(SIM_PARAMS_TO_FTR_PARAMS_TABLE.find(m_scenario)->second.find(cond) !=
                      SIM_PARAMS_TO_FTR_PARAMS_TABLE.find(m_scenario)->second.end(),
                  "The specified scenario and channel condition are not supported");
    auto scenAndCondTuple = SIM_PARAMS_TO_FTR_PARAMS_TABLE.find(m_scenario)->second.find(cond);

    // Get references to the corresponding vectors
    auto& fcVec = std::get<0>(scenAndCondTuple->second);
    auto& ftrParamsVec = std::get<1>(scenAndCondTuple->second);

    // Find closest carrier frequency which has been calibrated
    auto idxOfClosestFc = SearchClosestFc(fcVec, m_frequency);

    // Retrieve the corresponding FTR parameters
    NS_ASSERT(ftrParamsVec.size() >= idxOfClosestFc && idxOfClosestFc >= 0);
    FtrParams params = ftrParamsVec[idxOfClosestFc];

    return params;
}

double
TwoRaySpectrumPropagationLossModel::CalcBeamformingGain(
    Ptr<const MobilityModel> a,
    Ptr<const MobilityModel> b,
    Ptr<const PhasedArrayModel> aPhasedArrayModel,
    Ptr<const PhasedArrayModel> bPhasedArrayModel) const
{
    NS_LOG_FUNCTION(this);

    // Get the relative angles between tx and rx phased arrays
    Angles aAngle(b->GetPosition(), a->GetPosition());
    Angles bAngle(a->GetPosition(), b->GetPosition());

    // Compute the beamforming vectors and and array responses
    auto aArrayResponse = aPhasedArrayModel->GetSteeringVector(aAngle);
    auto aAntennaFields = aPhasedArrayModel->GetElementFieldPattern(aAngle);
    auto aBfVector = aPhasedArrayModel->GetBeamformingVector();
    auto bArrayResponse = bPhasedArrayModel->GetSteeringVector(bAngle);
    auto bAntennaFields = bPhasedArrayModel->GetElementFieldPattern(bAngle);
    auto bBfVector = bPhasedArrayModel->GetBeamformingVector();

    std::complex<double> aArrayOverallResponse = 0;
    std::complex<double> bArrayOverallResponse = 0;

    // Compute the dot products between the array responses and the beamforming vectors
    for (size_t i = 0; i < aPhasedArrayModel->GetNumElems(); i++)
    {
        aArrayOverallResponse += aArrayResponse[i] * aBfVector[i];
    }
    for (size_t i = 0; i < bPhasedArrayModel->GetNumElems(); i++)
    {
        bArrayOverallResponse += bArrayResponse[i] * bBfVector[i];
    }

    double gain = norm(aArrayOverallResponse) *
                  (std::pow(aAntennaFields.first, 2) + std::pow(aAntennaFields.second, 2)) *
                  norm(bArrayOverallResponse) *
                  (std::pow(bAntennaFields.first, 2) + std::pow(bAntennaFields.second, 2));

    // Retrieve LOS condition to check if a correction factor needs to be introduced
    ChannelCondition::LosConditionValue cond = GetLosCondition(a, b);
    if (cond == ChannelCondition::NLOS)
    {
        // The linear penalty factor to be multiplied to the beamforming gain whenever the link is
        // in NLOS
        constexpr double NLOS_BEAMFORMING_FACTOR = 1.0 / 19;
        gain *= NLOS_BEAMFORMING_FACTOR;
    }

    return gain;
}

double
TwoRaySpectrumPropagationLossModel::GetFtrFastFading(const FtrParams& params) const
{
    NS_LOG_FUNCTION(this);

    // Set the RNG parameters
    m_normalRv->SetAttribute("Variance", DoubleValue(params.m_sigma));
    m_gammaRv->SetAttribute("Alpha", DoubleValue(params.m_m));
    m_gammaRv->SetAttribute("Beta", DoubleValue(1.0 / params.m_m));

    // Compute the specular components amplitudes from the FTR parameters
    double cmnSqrtTerm = sqrt(1 - std::pow(params.m_delta, 2));
    double v1 = sqrt(params.m_sigma) * sqrt(params.m_k * (1 - cmnSqrtTerm));
    double v2 = sqrt(params.m_sigma) * sqrt(params.m_k * (1 + cmnSqrtTerm));
    double sqrtGamma = sqrt(m_gammaRv->GetValue());

    // Sample the random phases of the specular components, which are uniformly distributed in [0,
    // 2*PI]
    double phi1 = m_uniformRv->GetValue();
    double phi2 = m_uniformRv->GetValue();

    // Sample the normal-distributed real and imaginary parts of the diffuse components
    double x = m_normalRv->GetValue();
    double y = m_normalRv->GetValue();

    // Compute the channel response by combining the above terms
    std::complex<double> h = sqrtGamma * v1 * std::complex<double>(cos(phi1), sin(phi1)) +
                             sqrtGamma * v2 * std::complex<double>(cos(phi2), sin(phi2)) +
                             std::complex<double>(x, y);

    return norm(h);
}

Ptr<SpectrumSignalParameters>
TwoRaySpectrumPropagationLossModel::DoCalcRxPowerSpectralDensity(
    Ptr<const SpectrumSignalParameters> params,
    Ptr<const MobilityModel> a,
    Ptr<const MobilityModel> b,
    Ptr<const PhasedArrayModel> aPhasedArrayModel,
    Ptr<const PhasedArrayModel> bPhasedArrayModel) const
{
    NS_LOG_FUNCTION(this);
    uint32_t aId = a->GetObject<Node>()->GetId(); // Id of the node a
    uint32_t bId = b->GetObject<Node>()->GetId(); // Id of the node b

    NS_ASSERT_MSG(aId != bId, "The two nodes must be different from one another");
    NS_ASSERT_MSG(a->GetDistanceFrom(b) > 0.0,
                  "The position of a and b devices cannot be the same");

    // Retrieve the antenna of device a
    NS_ASSERT_MSG(aPhasedArrayModel, "Antenna not found for node " << aId);
    NS_LOG_DEBUG("a node " << a->GetObject<Node>() << " antenna " << aPhasedArrayModel);

    // Retrieve the antenna of the device b
    NS_ASSERT_MSG(bPhasedArrayModel, "Antenna not found for device " << bId);
    NS_LOG_DEBUG("b node " << bId << " antenna " << bPhasedArrayModel);

    // Retrieve FTR params from table
    FtrParams ftrParams = GetFtrParameters(a, b);

    // Compute the FTR fading
    double fading = GetFtrFastFading(ftrParams);

    // Compute the beamforming gain
    double bfGain = CalcBeamformingGain(a, b, aPhasedArrayModel, bPhasedArrayModel);

    Ptr<SpectrumSignalParameters> rxParams = params->Copy();
    // Apply the above terms to the TX PSD to calculate RX PSD
    (*(rxParams->psd)) *= (fading * bfGain);

    return rxParams;
}

std::size_t
TwoRaySpectrumPropagationLossModel::SearchClosestFc(const std::vector<double>& frequencies,
                                                    double targetFc) const
{
    auto it = std::min_element(std::begin(frequencies),
                               std::end(frequencies),
                               [targetFc](double lhs, double rhs) {
                                   return std::abs(lhs - targetFc) < std::abs(rhs - targetFc);
                               });
    return std::distance(std::begin(frequencies), it);
}

int64_t
TwoRaySpectrumPropagationLossModel::DoAssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_normalRv->SetStream(stream);
    m_uniformRv->SetStream(stream + 1);
    m_gammaRv->SetStream(stream + 2);
    return 3;
}

} // namespace ns3
