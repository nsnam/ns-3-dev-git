/*
 * Copyright (c) 2026, Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "spatial-gaussian-field.h"

#include "ns3/rng-seed-manager.h"

#include <cmath>
#include <cstring>
#include <utility>

namespace ns3
{

SpatialGaussianField::SpatialGaussianField(Salt salt, CellGenerator generator)
    : m_salt(static_cast<uint64_t>(salt)),
      m_generator(generator)
{
}

uint64_t
SpatialGaussianField::SplitMix64(uint64_t x)
{
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

uint64_t
SpatialGaussianField::Prefix(uint64_t fieldKey) const
{
    uint64_t h = SplitMix64(static_cast<uint64_t>(RngSeedManager::GetSeed()));
    h = SplitMix64(h ^ (static_cast<uint64_t>(RngSeedManager::GetRun()) + 0x1000ULL));
    h = SplitMix64(h ^ m_salt);
    return SplitMix64(h ^ fieldKey);
}

double
SpatialGaussianField::CellFromPrefix(uint64_t prefix, int64_t ix, int64_t iy) const
{
    uint64_t h = SplitMix64(prefix ^ static_cast<uint64_t>(static_cast<uint32_t>(ix)));
    h = SplitMix64(h ^ static_cast<uint64_t>(static_cast<uint32_t>(iy)));

    if (m_generator == CellGenerator::BoxMuller)
    {
        // Two independent uniforms, u1 in (0,1] and u2 in [0,1), from the
        // 53-bit mantissa of the mixed state.
        const double inv = 1.0 / 9007199254740992.0; // 2^-53
        const double u1 = (static_cast<double>(SplitMix64(h) >> 11) + 1.0) * inv;
        const double u2 = static_cast<double>(SplitMix64(h + 1) >> 11) * inv;
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
    }

    const auto sum =
        static_cast<double>((h & 0xFFFF) + ((h >> 16) & 0xFFFF) + ((h >> 32) & 0xFFFF) + (h >> 48));
    // Zero-mean: subtract 4 * 65535 / 2; unit variance: divide by
    // sqrt(4 * (2^32 - 1) / 12).
    return (sum - 131070.0) * (1.0 / 37837.2267);
}

SpatialGaussianField::Window
SpatialGaussianField::ComputeWindow(const Vector& position, double corrDist)
{
    Window win;
    // Filtering white noise with an exponential kernel of decay length lambda
    // yields a (1 + tau/lambda) * exp(-tau/lambda) autocorrelation in the
    // continuum; the half-lambda grid and the 14-cell truncation below decay
    // faster, so lambda is scaled by the numerically fitted constant that puts
    // the 1/e point of the discrete filter at tau = corrDist, matching the
    // exp(-d/dcor) autocorrelation of TR 38.901.
    const double lambda = corrDist / 1.996;
    // Grid spacing of half the kernel length resolves the exponential kernel
    // shape; the 14-cell window covers the +-3*lambda filter support (the
    // truncated tail carries weight exp(-3), absorbed by the L2
    // normalization).
    const double spacing = 0.5 * lambda;
    win.ix = static_cast<int64_t>(std::floor(position.x / spacing)) - 6;
    win.iy = static_cast<int64_t>(std::floor(position.y / spacing)) - 6;
    double wx2Sum = 0.0;
    double wy2Sum = 0.0;
    // win.ix and win.iy are negative near the origin: keep the cell index
    // arithmetic signed, or the sum wraps to a huge unsigned value.
    for (int64_t k = 0; std::cmp_less(k, WINDOW_CELLS); k++)
    {
        win.wx[k] = std::exp(-std::abs((win.ix + k) * spacing - position.x) / lambda);
        win.wy[k] = std::exp(-std::abs((win.iy + k) * spacing - position.y) / lambda);
        wx2Sum += win.wx[k] * win.wx[k];
        wy2Sum += win.wy[k] * win.wy[k];
    }
    // The squared L2 norm of the separable 2D weights factorizes into the
    // product of the squared 1D norms.
    win.invL2Norm = 1.0 / std::sqrt(wx2Sum * wy2Sum);
    return win;
}

double
SpatialGaussianField::SampleWindow(uint64_t fieldKey, const Window& window) const
{
    return SampleWindowFromPrefix(Prefix(fieldKey), window);
}

double
SpatialGaussianField::SampleWindowFromPrefix(uint64_t prefix, const Window& win) const
{
    double acc = 0.0;
    for (int64_t j = 0; std::cmp_less(j, WINDOW_CELLS); j++)
    {
        double rowAcc = 0.0;
        for (int64_t i = 0; std::cmp_less(i, WINDOW_CELLS); i++)
        {
            rowAcc += win.wx[i] * CellFromPrefix(prefix, win.ix + i, win.iy + j);
        }
        acc += win.wy[j] * rowAcc;
    }
    // i.i.d. N(0,1) cell values combined with L2-normalized weights yield an
    // exactly N(0,1) marginal at every position.
    return acc * win.invL2Norm;
}

double
SpatialGaussianField::Sample(uint64_t fieldKey, const Vector& position, double corrDist) const
{
    const uint64_t prefix = Prefix(fieldKey);
    if (corrDist <= 0.0)
    {
        // Single deterministic draw keyed on the bit pattern of the position.
        int64_t ix;
        int64_t iy;
        std::memcpy(&ix, &position.x, sizeof(ix));
        std::memcpy(&iy, &position.y, sizeof(iy));
        // CellFromPrefix keeps the low 32 bits of each coordinate: fold the
        // exponent and high mantissa bits in so they still tell positions apart.
        return CellFromPrefix(prefix, ix ^ (ix >> 32), iy ^ (iy >> 32));
    }
    return SampleWindowFromPrefix(prefix, ComputeWindow(position, corrDist));
}

double
SpatialGaussianField::SampleUniform(uint64_t fieldKey,
                                    const Vector& position,
                                    double corrDist) const
{
    return 0.5 * std::erfc(-Sample(fieldKey, position, corrDist) * M_SQRT1_2);
}

} // namespace ns3
