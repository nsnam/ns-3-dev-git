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

SpectrumConverter::SpectrumConverter(Ptr<const SpectrumModel> fromSpectrumModel,
                                     Ptr<const SpectrumModel> toSpectrumModel)
{
    NS_LOG_FUNCTION(this);
    m_fromSpectrumModel = fromSpectrumModel;
    m_toSpectrumModel = toSpectrumModel;

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
    NS_ASSERT(*(fvvf->GetSpectrumModel()) == *m_fromSpectrumModel);

    auto tvvf = Create<SpectrumValue>(m_toSpectrumModel);

    auto tvit = tvvf->ValuesBegin();
    size_t i = 0; // Index of conversion coefficient

    for (auto convIt = m_conversionRowPtr.begin(); convIt != m_conversionRowPtr.end(); ++convIt)
    {
        double sum = 0;
        while (i < *convIt)
        {
            sum += (*fvvf)[m_conversionColInd.at(i)] * m_conversionMatrix.at(i);
            ++i;
        }
        *tvit = sum;
        ++tvit;
    }

    return tvvf;
}

} // namespace ns3
