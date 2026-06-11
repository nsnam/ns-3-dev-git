/*
 * Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering,
 * New York University
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "three-gpp-spectrum-propagation-loss-model.h"

#include "spectrum-signal-parameters.h"
#include "three-gpp-channel-model.h"

#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ThreeGppSpectrumPropagationLossModel");

NS_OBJECT_ENSURE_REGISTERED(ThreeGppSpectrumPropagationLossModel);

ThreeGppSpectrumPropagationLossModel::ThreeGppSpectrumPropagationLossModel()
{
    NS_LOG_FUNCTION(this);
}

ThreeGppSpectrumPropagationLossModel::~ThreeGppSpectrumPropagationLossModel()
{
    NS_LOG_FUNCTION(this);
}

void
ThreeGppSpectrumPropagationLossModel::DoDispose()
{
    m_longTermMap.clear();
    m_channelModel = nullptr;
}

TypeId
ThreeGppSpectrumPropagationLossModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ThreeGppSpectrumPropagationLossModel")
            .SetParent<PhasedArraySpectrumPropagationLossModel>()
            .SetGroupName("Spectrum")
            .AddConstructor<ThreeGppSpectrumPropagationLossModel>()
            .AddAttribute(
                "ChannelModel",
                "The channel model. It needs to implement the MatrixBasedChannelModel interface",
                StringValue("ns3::ThreeGppChannelModel"),
                MakePointerAccessor(&ThreeGppSpectrumPropagationLossModel::SetChannelModel,
                                    &ThreeGppSpectrumPropagationLossModel::GetChannelModel),
                MakePointerChecker<MatrixBasedChannelModel>());
    return tid;
}

void
ThreeGppSpectrumPropagationLossModel::SetChannelModel(Ptr<MatrixBasedChannelModel> channel)
{
    m_channelModel = channel;
}

Ptr<MatrixBasedChannelModel>
ThreeGppSpectrumPropagationLossModel::GetChannelModel() const
{
    return m_channelModel;
}

double
ThreeGppSpectrumPropagationLossModel::GetFrequency() const
{
    NS_ASSERT_MSG(m_channelModel != nullptr, "Channel model is not set");
    DoubleValue freq;
    m_channelModel->GetAttribute("Frequency", freq);
    return freq.Get();
}

void
ThreeGppSpectrumPropagationLossModel::SetChannelModelAttribute(const std::string& name,
                                                               const AttributeValue& value)
{
    NS_ASSERT_MSG(m_channelModel != nullptr, "Channel model is not set");
    m_channelModel->SetAttribute(name, value);
}

void
ThreeGppSpectrumPropagationLossModel::GetChannelModelAttribute(const std::string& name,
                                                               AttributeValue& value) const
{
    NS_ASSERT_MSG(m_channelModel != nullptr, "Channel model is not set");
    m_channelModel->GetAttribute(name, value);
}

namespace
{

/**
 * Compute the array-index offsets, relative to a port's first element, of all
 * elements belonging to one port of a phased array.
 *
 * The sub-array partition model is adopted for TXRU virtualization, as
 * described in Section 5.2.2 of 3GPP TR 36.897: each port drives a rectangular
 * sub-grid of elements (with equal beam weights for all ports), so consecutive
 * elements of a port walk a sub-grid row and then jump to the next array row.
 * The resulting offsets are the same for every port of the array, which lets
 * the caller compute them once and reuse them across ports.
 * Support of the full-connection model for TXRU virtualization would need
 * extensions.
 *
 * @param antenna the phased array
 * @return offsets of a port's elements, relative to the port's first element
 */
std::vector<size_t>
PortElementOffsets(Ptr<const PhasedArrayModel> antenna)
{
    const size_t portElems = antenna->GetNumElemsPerPort();
    const size_t hElemsPerPort = antenna->GetHElemsPerPort();
    const size_t rowStride = antenna->GetNumColumns();
    std::vector<size_t> offsets(portElems);
    for (size_t i = 0; i < portElems; ++i)
    {
        // Element i of a port sits at sub-grid row (i / hElemsPerPort) and
        // column (i % hElemsPerPort); rows are rowStride elements apart.
        offsets[i] =
            (hElemsPerPort > 0) ? (i / hElemsPerPort) * rowStride + (i % hElemsPerPort) : i;
    }
    return offsets;
}

} // namespace

Ptr<const MatrixBasedChannelModel::Complex3DVector>
ThreeGppSpectrumPropagationLossModel::CalcLongTerm(
    Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix,
    Ptr<const PhasedArrayModel> sAnt,
    Ptr<const PhasedArrayModel> uAnt) const
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT_MSG(sAnt != nullptr && uAnt != nullptr, "Improper call to the method");
    const PhasedArrayModel::ComplexVector& sW = sAnt->GetBeamformingVectorRef();
    const PhasedArrayModel::ComplexVector& uW = uAnt->GetBeamformingVectorRef();
    NS_ASSERT(uW.GetSize() == channelMatrix->m_channel.GetNumRows());
    NS_ASSERT(sW.GetSize() == channelMatrix->m_channel.GetNumCols());
    const size_t numClusters = channelMatrix->m_channel.GetNumPages();
    const auto sPorts = static_cast<uint16_t>(sAnt->GetNumPorts());
    const auto uPorts = static_cast<uint16_t>(uAnt->GetNumPorts());
    Ptr<MatrixBasedChannelModel::Complex3DVector> longTerm =
        Create<MatrixBasedChannelModel::Complex3DVector>(uPorts, sPorts, numClusters);

    // Calculate the long term uW^H * Husn * sW for each port pair and cluster;
    // the result is a matrix with dimensions #uPorts, #sPorts, #clusters.
    //
    // The element walk within a port is the same for every port (sub-array
    // partition model, see PortElementOffsets), so precompute it once per
    // side. This also keeps the inner loops free of the row-hop bookkeeping.
    const std::vector<size_t> sOffsets = PortElementOffsets(sAnt);
    const std::vector<size_t> uOffsets = PortElementOffsets(uAnt);

    // Copy the beamforming weights into plain vectors, with the rx side
    // already conjugated, so the inner loops use indexed loads instead of
    // a ValArray accessor and a std::conj per element.
    // Note that the weight of a port's i-th element is indexed by the same
    // array offset as the element itself, per the sub-array partition model.
    std::vector<std::complex<double>> uWeightsConj(uW.GetSize());
    for (size_t k = 0; k < uW.GetSize(); ++k)
    {
        uWeightsConj[k] = std::conj(uW[k]);
    }
    std::vector<std::complex<double>> sWeights(sW.GetSize());
    for (size_t k = 0; k < sW.GetSize(); ++k)
    {
        sWeights[k] = sW[k];
    }

    // Each cluster page of the channel matrix is column-major: element
    // (uIndex, sIndex) lives at pagePtr[uIndex + numRows * sIndex].
    const size_t numRows = channelMatrix->m_channel.GetNumRows();
    const size_t sPortElems = sOffsets.size();
    const size_t uPortElems = uOffsets.size();
    for (uint16_t sPortIdx = 0; sPortIdx < sPorts; ++sPortIdx)
    {
        const size_t startS = sAnt->ArrayIndexFromPortIndex(sPortIdx, 0);
        for (uint16_t uPortIdx = 0; uPortIdx < uPorts; ++uPortIdx)
        {
            const size_t startU = uAnt->ArrayIndexFromPortIndex(uPortIdx, 0);
            for (size_t cIndex = 0; cIndex < numClusters; ++cIndex)
            {
                const auto* pagePtr = channelMatrix->m_channel.GetPagePtr(cIndex);
                std::complex<double> txSum(0, 0);
                if (uPortElems == 1)
                {
                    // One rx element per port (the typical NR UE layout):
                    // the rx-side reduction is a single multiply.
                    const std::complex<double> uWeightConj0 = uWeightsConj[0];
                    for (size_t tIndex = 0; tIndex < sPortElems; ++tIndex)
                    {
                        const size_t sIndex = startS + sOffsets[tIndex];
                        txSum += sWeights[sOffsets[tIndex]] *
                                 (uWeightConj0 * pagePtr[startU + numRows * sIndex]);
                    }
                }
                else
                {
                    for (size_t tIndex = 0; tIndex < sPortElems; ++tIndex)
                    {
                        const size_t sIndex = startS + sOffsets[tIndex];
                        const auto* column = &pagePtr[numRows * sIndex];
                        std::complex<double> rxSum(0, 0);
                        for (size_t rIndex = 0; rIndex < uPortElems; ++rIndex)
                        {
                            rxSum +=
                                uWeightsConj[uOffsets[rIndex]] * column[startU + uOffsets[rIndex]];
                        }
                        txSum += sWeights[sOffsets[tIndex]] * rxSum;
                    }
                }
                longTerm->Elem(uPortIdx, sPortIdx, cIndex) = txSum;
            }
        }
    }
    return longTerm;
}

Ptr<SpectrumSignalParameters>
ThreeGppSpectrumPropagationLossModel::CalcBeamformingGain(
    Ptr<const SpectrumSignalParameters> params,
    Ptr<const MatrixBasedChannelModel::Complex3DVector> longTerm,
    Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix,
    Ptr<const MatrixBasedChannelModel::ChannelParams> channelParams,
    const Vector& sSpeed,
    const Vector& uSpeed,
    const uint8_t numTxPorts,
    const uint8_t numRxPorts,
    const bool isReverse) const

{
    NS_LOG_FUNCTION(this);
    Ptr<SpectrumSignalParameters> rxParams = params->Copy();
    const size_t numCluster = channelMatrix->m_channel.GetNumPages();
    // compute the doppler term
    // NOTE the update of Doppler is simplified by only taking the center angle of
    // each cluster in to consideration.
    const double slotTime = Simulator::Now().GetSeconds();
    const double factor = 2 * M_PI * slotTime * GetFrequency() / 3e8;
    PhasedArrayModel::ComplexVector doppler(numCluster);

    // Make sure that all the structures that are passed to this function
    // are of the correct dimensions before using the operator [].
    NS_ASSERT(numCluster <= channelParams->m_alpha.size());
    NS_ASSERT(numCluster <= channelParams->m_D.size());
    NS_ASSERT(numCluster <= channelParams->m_angle[MatrixBasedChannelModel::ZOA_INDEX].size());
    NS_ASSERT(numCluster <= channelParams->m_angle[MatrixBasedChannelModel::ZOD_INDEX].size());
    NS_ASSERT(numCluster <= channelParams->m_angle[MatrixBasedChannelModel::AOA_INDEX].size());
    NS_ASSERT(numCluster <= channelParams->m_angle[MatrixBasedChannelModel::AOD_INDEX].size());
    NS_ASSERT(numCluster <= longTerm->GetNumPages());

    // check if channelParams structure is generated in direction s-to-u or u-to-s
    const bool isSameDir = channelParams->m_nodeIds == channelMatrix->m_nodeIds;

    // if channel params is generated in the same direction in which we
    // generate the channel matrix, angles and zenith of departure and arrival are ok,
    // just set them to corresponding variable that will be used for the generation
    // of channel matrix, otherwise we need to flip angles and zeniths of departure and arrival
    using DPV = std::vector<std::pair<double, double>>;
    const auto& cachedAngleSincos = channelParams->m_cachedAngleSincos;
    NS_ASSERT_MSG(cachedAngleSincos.size() > MatrixBasedChannelModel::ZOD_INDEX,
                  "Cached angle sin/cos not initialized");
    const DPV& zoa = cachedAngleSincos[isSameDir ? MatrixBasedChannelModel::ZOA_INDEX
                                                 : MatrixBasedChannelModel::ZOD_INDEX];
    const DPV& zod = cachedAngleSincos[isSameDir ? MatrixBasedChannelModel::ZOD_INDEX
                                                 : MatrixBasedChannelModel::ZOA_INDEX];
    const DPV& aoa = cachedAngleSincos[isSameDir ? MatrixBasedChannelModel::AOA_INDEX
                                                 : MatrixBasedChannelModel::AOD_INDEX];
    const DPV& aod = cachedAngleSincos[isSameDir ? MatrixBasedChannelModel::AOD_INDEX
                                                 : MatrixBasedChannelModel::AOA_INDEX];
    NS_ASSERT(numCluster <= zoa.size());
    NS_ASSERT(numCluster <= zod.size());
    NS_ASSERT(numCluster <= aoa.size());
    NS_ASSERT(numCluster <= aod.size());

    for (size_t cIndex = 0; cIndex < numCluster; cIndex++)
    {
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

        const double alpha = channelParams->m_alpha[cIndex];
        const double D = channelParams->m_D[cIndex];

        // cluster angle angle[direction][n], where direction = 0(aoa), 1(zoa).
        const double tempDoppler =
            factor *
            (zoa[cIndex].first * aoa[cIndex].second * uSpeed.x +
             zoa[cIndex].first * aoa[cIndex].first * uSpeed.y + zoa[cIndex].second * uSpeed.z +
             (zod[cIndex].first * aod[cIndex].second * sSpeed.x +
              zod[cIndex].first * aod[cIndex].first * sSpeed.y + zod[cIndex].second * sSpeed.z) +
             2 * alpha * D);
        // std::polar folds cos+sin into one sincos on modern libstdc++.
        doppler[cIndex] = std::polar(1.0, tempDoppler);
    }

    NS_ASSERT(numCluster <= doppler.GetSize());

    // set the channel matrix
    rxParams->spectrumChannelMatrix = GenSpectrumChannelMatrix(rxParams->psd,
                                                               longTerm,
                                                               channelMatrix,
                                                               channelParams,
                                                               doppler,
                                                               numTxPorts,
                                                               numRxPorts,
                                                               isReverse);

    NS_ASSERT_MSG(rxParams->psd->GetValuesN() == rxParams->spectrumChannelMatrix->GetNumPages(),
                  "RX PSD and the spectrum channel matrix should have the same number of RBs ");

    NS_ASSERT_MSG(!params->precodingMatrix || (params->precodingMatrix &&
                                               params->precodingMatrix->GetNumPages() ==
                                                   rxParams->spectrumChannelMatrix->GetNumPages()),
                  "Unexpected mismatch in the number of RBs and channel matrix and precoding "
                  "matrix. MultiModelSpectrumChannel conversion is not yet supported.");

    // Calculate the RX PSD from the spectrum channel matrix H and the
    // precoding matrix P as PSD = Trace((H*P)^h * (H*P)), i.e., per RB:
    //   PSD[rb] = sum_rx sum_txStream |(H*P)[rx, txStream, rb]|^2
    auto specMat = rxParams->spectrumChannelMatrix;
    const uint32_t numRb = rxParams->psd->GetValuesN();
    if (!rxParams->precodingMatrix)
    {
        // When no precoding matrix is set, the default is a single isotropic
        // column P[tx, 0, rb] = 1/sqrt(numTx). In that case H*P reduces to
        // the row sums of H scaled by 1/sqrt(numTx), so
        //   PSD[rb] = (1/numTx) * sum_rx |sum_tx H[rx, tx, rb]|^2
        // and the H*P product never needs to be formed.
        const size_t numRxPorts = specMat->GetNumRows();
        const size_t numTxPorts = specMat->GetNumCols();
        const double invNumTxPorts = 1.0 / static_cast<double>(numTxPorts);
        for (uint32_t rb = 0; rb < numRb; ++rb)
        {
            // Each per-RB page is column-major: (rx, tx) is page[rx + numRxPorts * tx].
            const std::complex<double>* page = specMat->GetPagePtr(rb);
            double psd = 0.0;
            for (size_t rx = 0; rx < numRxPorts; ++rx)
            {
                std::complex<double> rowSum(0.0, 0.0);
                for (size_t tx = 0; tx < numTxPorts; ++tx)
                {
                    rowSum += page[rx + numRxPorts * tx];
                }
                psd += std::norm(rowSum);
            }
            (*rxParams->psd)[rb] = psd * invNumTxPorts;
        }
    }
    else
    {
        // Explicit precoding matrix: form H*P, then sum the squared
        // magnitudes. std::norm(z) is |z|^2 without the square root.
        MatrixBasedChannelModel::Complex3DVector hP = *specMat * *rxParams->precodingMatrix;
        for (uint32_t rb = 0; rb < numRb; ++rb)
        {
            double psd = 0.0;
            for (size_t rx = 0; rx < hP.GetNumRows(); ++rx)
            {
                for (size_t txStream = 0; txStream < hP.GetNumCols(); ++txStream)
                {
                    psd += std::norm(hP(rx, txStream, rb));
                }
            }
            (*rxParams->psd)[rb] = psd;
        }
    }
    return rxParams;
}

Ptr<MatrixBasedChannelModel::Complex3DVector>
ThreeGppSpectrumPropagationLossModel::GenSpectrumChannelMatrix(
    Ptr<SpectrumValue> inPsd,
    Ptr<const MatrixBasedChannelModel::Complex3DVector> longTerm,
    Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix,
    Ptr<const MatrixBasedChannelModel::ChannelParams> channelParams,
    const PhasedArrayModel::ComplexVector& doppler,
    uint8_t numTxPorts,
    uint8_t numRxPorts,
    const bool isReverse)
{
    const size_t numCluster = channelMatrix->m_channel.GetNumPages();
    const auto numRb = inPsd->GetValuesN();
    NS_ASSERT_MSG(numCluster <= channelParams->m_delay.size(),
                  "Channel params delays size is smaller than number of clusters");

    // Avoid the full-copy of `*longTerm` on the (common) non-reverse
    // path. `directionalLongTerm` is read-only below; in the reverse
    // case we still need the materialised Transpose, but otherwise
    // we keep a pointer-aliased view of the cached matrix.
    MatrixBasedChannelModel::Complex3DVector reversedLongTerm;
    if (isReverse)
    {
        reversedLongTerm = longTerm->Transpose();
    }
    const MatrixBasedChannelModel::Complex3DVector& directionalLongTerm =
        isReverse ? reversedLongTerm : *longTerm;

    Ptr<MatrixBasedChannelModel::Complex3DVector> chanSpct =
        Create<MatrixBasedChannelModel::Complex3DVector>(numRxPorts,
                                                         numTxPorts,
                                                         static_cast<uint16_t>(numRb));

    // Precompute the delay until numRb, numCluster or RB width changes
    // Whenever the channelParams is updated, the number of numRbs, numClusters
    // and RB width (12*SCS) are reset, ensuring these values are updated too

    if (const double rbWidth = inPsd->ConstBandsBegin()->fh - inPsd->ConstBandsBegin()->fl;
        channelParams->m_cachedDelaySincos.GetNumRows() != numRb ||
        channelParams->m_cachedDelaySincos.GetNumCols() != numCluster ||
        channelParams->m_cachedRbWidth != rbWidth)
    {
        channelParams->m_cachedRbWidth = rbWidth;
        channelParams->m_cachedDelaySincos = ComplexMatrixArray(numRb, numCluster);
        auto sbit = inPsd->ConstBandsBegin(); // band iterator
        for (unsigned i = 0; i < numRb; i++)
        {
            const double fsb = sbit->fc; // center frequency of the sub-band
            for (std::size_t cIndex = 0; cIndex < numCluster; cIndex++)
            {
                const double delay = -2 * M_PI * fsb * channelParams->m_delay[cIndex];
                channelParams->m_cachedDelaySincos(i, cIndex) =
                    std::complex(cos(delay), sin(delay));
            }
            ++sbit;
        }
    }

    // The remaining work is the contraction
    //
    //   chanSpct[rx, tx, rb] =
    //       sqrt(psd[rb]) * sum_c longTerm[rx, tx, c] * delaySincos[rb, c] * doppler[c],
    //
    // the dominant cost of CalcRxPowerSpectralDensity at high port counts and
    // wide bandwidths. To let the compiler autovectorize it, the inputs are
    // first packed into cluster-contiguous buffers with the real and imaginary
    // parts split into separate arrays (structure-of-arrays), so the hot loop
    // reads plain, contiguous double lanes:
    //   longTermRe/Im[c * rxtx + i] = longTerm[rx, tx, c]  (i = rx + numRxPorts*tx)
    //   phasorRe/Im[c * numRb + rb] = delaySincos[rb, c] * doppler[c]
    //
    // The scratch buffers are thread_local so that this hot path does not pay
    // a heap allocation per call: resize() only reallocates when the sizes
    // grow, and every element is overwritten before being read.
    const size_t rxtx = static_cast<size_t>(numRxPorts) * numTxPorts;
    thread_local std::vector<double> longTermRe;
    thread_local std::vector<double> longTermIm;
    thread_local std::vector<double> phasorRe;
    thread_local std::vector<double> phasorIm;
    thread_local std::vector<double> sqrtPsd;
    thread_local std::vector<double> rbSumRe;
    thread_local std::vector<double> rbSumIm;

    longTermRe.resize(numCluster * rxtx);
    longTermIm.resize(numCluster * rxtx);
    const auto& longTermValues = directionalLongTerm.GetValues();
    for (size_t k = 0; k < numCluster * rxtx; ++k)
    {
        longTermRe[k] = longTermValues[k].real();
        longTermIm[k] = longTermValues[k].imag();
    }

    phasorRe.resize(numCluster * numRb);
    phasorIm.resize(numCluster * numRb);
    const auto& delaySincos = channelParams->m_cachedDelaySincos.GetValues();
    for (size_t c = 0; c < numCluster; ++c)
    {
        const std::complex<double> dopplerC = doppler[c];
        for (size_t rb = 0; rb < numRb; ++rb)
        {
            const std::complex<double> phasor = delaySincos[c * numRb + rb] * dopplerC;
            phasorRe[c * numRb + rb] = phasor.real();
            phasorIm[c * numRb + rb] = phasor.imag();
        }
    }

    // Precompute sqrt(psd[rb]) for all RBs; subbands with zero TX power keep
    // a zero channel coefficient.
    sqrtPsd.resize(numRb);
    {
        auto vit = inPsd->ValuesBegin();
        for (size_t rb = 0; rb < numRb; ++rb, ++vit)
        {
            sqrtPsd[rb] = (*vit > 0.0) ? std::sqrt(*vit) : 0.0;
        }
    }

    // Per RB, accumulate over all clusters into a small (rxtx-sized,
    // L1-resident) split re/im buffer, then write that RB page once. The
    // split layout plus __restrict turn the inner loop into two contiguous
    // FMA reductions that vectorize over SIMD lanes, and the small
    // accumulator avoids re-streaming the whole multi-MB output once per
    // cluster. The summation order (c ascending) matches a straightforward
    // per-element cluster loop bit for bit, up to FMA contraction.
    rbSumRe.resize(rxtx);
    rbSumIm.resize(rxtx);
    for (size_t rb = 0; rb < numRb; ++rb)
    {
        double* __restrict sumRe = rbSumRe.data();
        double* __restrict sumIm = rbSumIm.data();
        std::fill_n(sumRe, rxtx, 0.0);
        std::fill_n(sumIm, rxtx, 0.0);
        for (size_t c = 0; c < numCluster; ++c)
        {
            const double phRe = phasorRe[c * numRb + rb];
            const double phIm = phasorIm[c * numRb + rb];
            const double* __restrict ltRe = &longTermRe[c * rxtx];
            const double* __restrict ltIm = &longTermIm[c * rxtx];
            for (size_t i = 0; i < rxtx; ++i)
            {
                // sum[i] += longTerm[i] * phasor, on the split re/im arrays.
                sumRe[i] += ltRe[i] * phRe - ltIm[i] * phIm;
                sumIm[i] += ltRe[i] * phIm + ltIm[i] * phRe;
            }
        }
        // Multiply with the square root of the input PSD so that the norm
        // (absolute value squared) of chanSpct will be the output PSD.
        const double scale = sqrtPsd[rb];
        std::complex<double>* page = chanSpct->GetPagePtr(rb);
        if (scale == 0.0)
        {
            std::fill_n(page, rxtx, std::complex<double>(0.0, 0.0));
        }
        else
        {
            for (size_t i = 0; i < rxtx; ++i)
            {
                page[i] = std::complex<double>(sumRe[i] * scale, sumIm[i] * scale);
            }
        }
    }
    return chanSpct;
}

Ptr<const MatrixBasedChannelModel::Complex3DVector>
ThreeGppSpectrumPropagationLossModel::GetLongTerm(
    Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix,
    Ptr<const PhasedArrayModel> aPhasedArrayModel,
    Ptr<const PhasedArrayModel> bPhasedArrayModel) const
{
    Ptr<const MatrixBasedChannelModel::Complex3DVector>
        longTerm; // vector containing the long term component for each cluster

    // check if the channel matrix was generated considering a as the s-node and
    // b as the u-node or vice-versa
    const auto isReverse =
        channelMatrix->IsReverse(aPhasedArrayModel->GetId(), bPhasedArrayModel->GetId());
    const auto sAntenna = isReverse ? bPhasedArrayModel : aPhasedArrayModel;
    const auto uAntenna = isReverse ? aPhasedArrayModel : bPhasedArrayModel;

    PhasedArrayModel::ComplexVector sW;
    PhasedArrayModel::ComplexVector uW;
    if (!isReverse)
    {
        sW = aPhasedArrayModel->GetBeamformingVector();
        uW = bPhasedArrayModel->GetBeamformingVector();
    }
    else
    {
        sW = bPhasedArrayModel->GetBeamformingVector();
        uW = aPhasedArrayModel->GetBeamformingVector();
    }

    bool update = false;   // indicates whether the long term has to be updated
    bool notFound = false; // indicates if the long term has not been computed yet

    // compute the long term key, the key is unique for each tx-rx pair
    const uint64_t longTermId =
        MatrixBasedChannelModel::GetKey(aPhasedArrayModel->GetId(), bPhasedArrayModel->GetId());

    // look for the long term in the map and check if it is valid
    if (m_longTermMap.contains(longTermId))
    {
        NS_LOG_DEBUG("found the long term component in the map");
        longTerm = m_longTermMap[longTermId]->m_longTerm;

        // check if the channel matrix has been updated
        // or the s beam has been changed
        // or the u beam has been changed
        update = m_longTermMap[longTermId]->m_channel->m_generatedTime !=
                     channelMatrix->m_generatedTime ||
                 m_longTermMap[longTermId]->m_sW != sW || m_longTermMap[longTermId]->m_uW != uW;
    }
    else
    {
        NS_LOG_DEBUG("long term component NOT found");
        notFound = true;
    }

    if (update || notFound)
    {
        NS_LOG_DEBUG("compute the long term");
        // compute the long term component
        longTerm = CalcLongTerm(channelMatrix, sAntenna, uAntenna);
        Ptr<LongTerm> longTermItem = Create<LongTerm>();
        longTermItem->m_longTerm = longTerm;
        longTermItem->m_channel = channelMatrix;
        longTermItem->m_sW = std::move(sW);
        longTermItem->m_uW = std::move(uW);
        // store the long term to reduce computation load
        // only the small scale fading needs to be updated if the large scale parameters and antenna
        // weights remain unchanged.
        m_longTermMap[longTermId] = longTermItem;
    }

    return longTerm;
}

Ptr<SpectrumSignalParameters>
ThreeGppSpectrumPropagationLossModel::DoCalcRxPowerSpectralDensity(
    Ptr<const SpectrumSignalParameters> spectrumSignalParams,
    Ptr<const MobilityModel> a,
    Ptr<const MobilityModel> b,
    Ptr<const PhasedArrayModel> aPhasedArrayModel,
    Ptr<const PhasedArrayModel> bPhasedArrayModel) const
{
    NS_LOG_FUNCTION(this << spectrumSignalParams << a << b << aPhasedArrayModel
                         << bPhasedArrayModel);
    NS_ASSERT_MSG(m_channelModel != nullptr, "Channel model is not set");

    const uint32_t aId = a->GetObject<Node>()->GetId(); // id of the node a
    const uint32_t bId = b->GetObject<Node>()->GetId(); // id of the node b
    NS_ASSERT_MSG(aPhasedArrayModel, "Antenna not found for node " << aId);
    NS_LOG_DEBUG("a node " << aId << " antenna " << aPhasedArrayModel);
    NS_ASSERT_MSG(bPhasedArrayModel, "Antenna not found for node " << bId);
    NS_LOG_DEBUG("b node " << bId << " antenna " << bPhasedArrayModel);

    Ptr<const MatrixBasedChannelModel::ChannelMatrix> channelMatrix =
        m_channelModel->GetChannel(a, b, aPhasedArrayModel, bPhasedArrayModel);
    const Ptr<const MatrixBasedChannelModel::ChannelParams> channelParams =
        m_channelModel->GetParams(a, b);
    NS_ASSERT_MSG(channelMatrix != nullptr, "Channel matrix is null");
    NS_ASSERT_MSG(channelParams != nullptr, "Channel params are null");

    // retrieve the long term component
    const Ptr<const MatrixBasedChannelModel::Complex3DVector> longTerm =
        GetLongTerm(channelMatrix, aPhasedArrayModel, bPhasedArrayModel);

    const auto isReverse =
        channelMatrix->IsReverse(aPhasedArrayModel->GetId(), bPhasedArrayModel->GetId());

    // apply the beamforming gain
    return CalcBeamformingGain(spectrumSignalParams,
                               longTerm,
                               channelMatrix,
                               channelParams,
                               a->GetVelocity(),
                               b->GetVelocity(),
                               aPhasedArrayModel->GetNumPorts(),
                               bPhasedArrayModel->GetNumPorts(),
                               isReverse);
}

int64_t
ThreeGppSpectrumPropagationLossModel::DoAssignStreams(int64_t stream)
{
    return 0;
}

} // namespace ns3
