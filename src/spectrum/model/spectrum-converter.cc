/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "spectrum-converter.h"

#include "ns3/assert.h"
#include "ns3/log.h"

#include <algorithm>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SpectrumConverter");

namespace
{

/**
 * This method converts the transmitted PSD from the TX SpectrumModel to the RX SpectrumModel in the
 * case the two models are fully aligned, i.e., each band in the TX model has a corresponding band
 * in the RX model with the same boundaries and hence the conversion is just a matter of copying
 * values in the overlapping bands and setting to zero the values in the non-overlapping bands.
 *
 * @param txPsd the transmitted PSD to be converted
 * @param txSpectrumModel the SpectrumModel over which the transmitted PSD is defined
 * @param rxSpectrumModel the SpectrumModel over which the converted PSD should be defined
 * @return the converted PSD
 */
Ptr<SpectrumValue>
ConvertAlignedSpectrumModels(Ptr<const SpectrumValue> txPsd,
                             Ptr<const SpectrumModel> txSpectrumModel,
                             Ptr<const SpectrumModel> rxSpectrumModel)
{
    // Create a new SpectrumValue with zeroes
    auto convertedPsd = Create<SpectrumValue>(rxSpectrumModel);

    // Copy values in the overlapping bands
    auto fromIt = txPsd->ConstValuesBegin();
    auto toIt = convertedPsd->ValuesBegin();
    auto fromBandIt = txSpectrumModel->Begin();
    auto toBandIt = rxSpectrumModel->Begin();

    while (fromBandIt != txSpectrumModel->End() && toBandIt != rxSpectrumModel->End())
    {
        if (fromBandIt->fh <= toBandIt->fl)
        {
            ++fromBandIt;
            ++fromIt;
        }
        else if (toBandIt->fh <= fromBandIt->fl)
        {
            ++toBandIt;
            ++toIt;
        }
        else
        {
            // overlapping bands
            *toIt = *fromIt;
            ++fromBandIt;
            ++fromIt;
            ++toBandIt;
            ++toIt;
        }
    }

    return convertedPsd;
}

} // namespace

SpectrumConverter::SpectrumConverter(Ptr<const SpectrumModel> fromSpectrumModel,
                                     Ptr<const SpectrumModel> toSpectrumModel)
{
    NS_LOG_FUNCTION(this);
    m_fromSpectrumModel = fromSpectrumModel;
    m_toSpectrumModel = toSpectrumModel;

    if (fromSpectrumModel->IsOrthogonal(*toSpectrumModel) ||
        fromSpectrumModel->IsAligned(*toSpectrumModel))
    {
        // no conversion matrix needed
        return;
    }

    size_t rowPtr = 0;
    m_conversionMatrix.reserve(fromSpectrumModel->GetNumBands() * toSpectrumModel->GetNumBands());
    m_conversionColInd.reserve(fromSpectrumModel->GetNumBands() * toSpectrumModel->GetNumBands());
    m_conversionRowPtr.reserve(toSpectrumModel->GetNumBands());
    for (auto toit = toSpectrumModel->Begin(); toit != toSpectrumModel->End(); ++toit)
    {
        size_t colInd = 0;
        auto bandInv = 1.0 / (toit->fh - toit->fl);
        for (auto fromit = fromSpectrumModel->Begin(); fromit != fromSpectrumModel->End(); ++fromit)
        {
            if (fromit->fh <= toit->fl || toit->fh <= fromit->fl)
            {
                ++colInd;
                continue;
            }
            const auto maxLow = (fromit->fl > toit->fl) ? fromit->fl : toit->fl;
            const auto minHigh = (fromit->fh < toit->fh) ? fromit->fh : toit->fh;
            const auto coeff = (minHigh - maxLow) * bandInv;
            const auto c = (coeff > 1.0) ? 1.0 : coeff;
            NS_LOG_LOGIC("(" << fromit->fl << "," << fromit->fh << ")"
                             << " --> "
                             << "(" << toit->fl << "," << toit->fh << ")"
                             << " = " << c);
            if (c > 0)
            {
                m_conversionMatrix.push_back(c);
                m_conversionColInd.push_back(colInd);
                ++rowPtr;
            }
            ++colInd;
        }
        m_conversionRowPtr.push_back(rowPtr);
    }
}

Ptr<SpectrumValue>
SpectrumConverter::Convert(Ptr<const SpectrumValue> fvvf) const
{
    if (m_fromSpectrumModel->IsOrthogonal(*m_toSpectrumModel))
    {
        // orthogonal models, return zeroed SpectrumValue
        return Create<SpectrumValue>(m_toSpectrumModel);
    }

    if (m_fromSpectrumModel->IsAligned(*m_toSpectrumModel))
    {
        // aligned models, simply opy values in overlapping bands
        return ConvertAlignedSpectrumModels(fvvf, m_fromSpectrumModel, m_toSpectrumModel);
    }

    NS_ASSERT(*(fvvf->GetSpectrumModel()) == *m_fromSpectrumModel);

    auto tvvf = Create<SpectrumValue>(m_toSpectrumModel);

    auto tvit = tvvf->ValuesBegin();
    size_t i = 0; // Index of conversion coefficient

    for (auto convIt = m_conversionRowPtr.begin(); convIt != m_conversionRowPtr.end(); ++convIt)
    {
        double sum = 0;
        while (i < *convIt)
        {
            sum += (*fvvf)[m_conversionColInd[i]] * m_conversionMatrix[i];
            ++i;
        }
        *tvit = sum;
        ++tvit;
    }

    return tvvf;
}

} // namespace ns3
