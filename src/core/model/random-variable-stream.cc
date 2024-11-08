/*
 * Copyright (c) 2006 Georgia Tech Research Corporation
 * Copyright (c) 2011 Mathieu Lacage
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Rajib Bhattacharjea<raj.b@gatech.edu>
 *          Hadi Arbabi<marbabi@cs.odu.edu>
 *          Mathieu Lacage <mathieu.lacage@gmail.com>
 *
 * Modified by Mitch Watrous <watrous@u.washington.edu>
 *
 */
#include "random-variable-stream.h"

#include "assert.h"
#include "boolean.h"
#include "double.h"
#include "integer.h"
#include "log.h"
#include "pointer.h"
#include "rng-seed-manager.h"
#include "rng-stream.h"
#include "string.h"
#include "uinteger.h"

#include <algorithm> // upper_bound
#include <cmath>
#include <iostream>
#include <numbers>

/**
 * @file
 * @ingroup randomvariable
 * ns3::RandomVariableStream and related implementations
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RandomVariableStream");

NS_OBJECT_ENSURE_REGISTERED(RandomVariableStream);

TypeId
RandomVariableStream::GetTypeId()
{
    static TypeId tid = TypeId("ns3::RandomVariableStream")
                            .SetParent<Object>()
                            .SetGroupName("Core")
                            .AddAttribute("Stream",
                                          "The stream number for this RNG stream. -1 means "
                                          "\"allocate a stream automatically\". "
                                          "Note that if -1 is set, Get will return -1 so that it "
                                          "is not possible to know which "
                                          "value was automatically allocated.",
                                          IntegerValue(-1),
                                          MakeIntegerAccessor(&RandomVariableStream::SetStream,
                                                              &RandomVariableStream::GetStream),
                                          MakeIntegerChecker<int64_t>())
                            .AddAttribute("Antithetic",
                                          "Set this RNG stream to generate antithetic values",
                                          BooleanValue(false),
                                          MakeBooleanAccessor(&RandomVariableStream::SetAntithetic,
                                                              &RandomVariableStream::IsAntithetic),
                                          MakeBooleanChecker());
    return tid;
}

RandomVariableStream::RandomVariableStream()
    : m_rng(nullptr)
{
    NS_LOG_FUNCTION(this);
}

RandomVariableStream::~RandomVariableStream()
{
    delete m_rng;
}

void
RandomVariableStream::SetAntithetic(bool isAntithetic)
{
    NS_LOG_FUNCTION(this << isAntithetic);
    m_isAntithetic = isAntithetic;
}

bool
RandomVariableStream::IsAntithetic() const
{
    return m_isAntithetic;
}

uint32_t
RandomVariableStream::GetInteger()
{
    auto value = static_cast<uint32_t>(GetValue());
    NS_LOG_DEBUG(GetInstanceTypeId().GetName()
                 << " integer value: " << value << " stream: " << GetStream());
    return value;
}

void
RandomVariableStream::SetStream(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    // negative values are not legal.
    NS_ASSERT(stream >= -1);
    delete m_rng;
    if (stream == -1)
    {
        // The first 2^63 streams are reserved for automatic stream
        // number assignment.
        uint64_t nextStream = RngSeedManager::GetNextStreamIndex();
        NS_ASSERT(nextStream <= ((1ULL) << 63));
        NS_LOG_INFO(GetInstanceTypeId().GetName() << " automatic stream: " << nextStream);
        m_rng = new RngStream(RngSeedManager::GetSeed(), nextStream, RngSeedManager::GetRun());
    }
    else
    {
        // The last 2^63 streams are reserved for deterministic stream
        // number assignment.
        uint64_t base = ((1ULL) << 63);
        uint64_t target = base + stream;
        NS_LOG_INFO(GetInstanceTypeId().GetName() << " configured stream: " << stream);
        m_rng = new RngStream(RngSeedManager::GetSeed(), target, RngSeedManager::GetRun());
    }
    m_stream = stream;
}

int64_t
RandomVariableStream::GetStream() const
{
    return m_stream;
}

RngStream*
RandomVariableStream::Peek() const
{
    return m_rng;
}

NS_OBJECT_ENSURE_REGISTERED(UniformRandomVariable);

TypeId
UniformRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UniformRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<UniformRandomVariable>()
            .AddAttribute("Min",
                          "The lower bound on the values returned by this RNG stream.",
                          DoubleValue(0),
                          MakeDoubleAccessor(&UniformRandomVariable::m_min),
                          MakeDoubleChecker<double>())
            .AddAttribute("Max",
                          "The upper bound on the values returned by this RNG stream.",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&UniformRandomVariable::m_max),
                          MakeDoubleChecker<double>());
    return tid;
}

UniformRandomVariable::UniformRandomVariable()
{
    // m_min and m_max are initialized after constructor by attributes
    NS_LOG_FUNCTION(this);
}

double
UniformRandomVariable::GetMin() const
{
    return m_min;
}

double
UniformRandomVariable::GetMax() const
{
    return m_max;
}

double
UniformRandomVariable::GetValue(double min, double max)
{
    double v = min + Peek()->RandU01() * (max - min);
    if (IsAntithetic())
    {
        v = min + (max - v);
    }
    NS_LOG_DEBUG("value: " << v << " stream: " << GetStream() << " min: " << min
                           << " max: " << max);
    return v;
}

uint32_t
UniformRandomVariable::GetInteger(uint32_t min, uint32_t max)
{
    NS_ASSERT(min <= max);
    auto v = static_cast<uint32_t>(GetValue((double)(min), (double)(max) + 1.0));
    NS_LOG_DEBUG("integer value: " << v << " stream: " << GetStream() << " min: " << min << " max "
                                   << max);
    return v;
}

double
UniformRandomVariable::GetValue()
{
    return GetValue(m_min, m_max);
}

uint32_t
UniformRandomVariable::GetInteger()
{
    auto v = static_cast<uint32_t>(GetValue(m_min, m_max + 1));
    NS_LOG_DEBUG("integer value: " << v << " stream: " << GetStream());
    return v;
}

NS_OBJECT_ENSURE_REGISTERED(ConstantRandomVariable);

TypeId
ConstantRandomVariable::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ConstantRandomVariable")
                            .SetParent<RandomVariableStream>()
                            .SetGroupName("Core")
                            .AddConstructor<ConstantRandomVariable>()
                            .AddAttribute("Constant",
                                          "The constant value returned by this RNG stream.",
                                          DoubleValue(0),
                                          MakeDoubleAccessor(&ConstantRandomVariable::m_constant),
                                          MakeDoubleChecker<double>());
    return tid;
}

ConstantRandomVariable::ConstantRandomVariable()
{
    // m_constant is initialized after constructor by attributes
    NS_LOG_FUNCTION(this);
}

double
ConstantRandomVariable::GetConstant() const
{
    NS_LOG_FUNCTION(this);
    return m_constant;
}

double
ConstantRandomVariable::GetValue(double constant)
{
    NS_LOG_DEBUG("value: " << constant << " stream: " << GetStream());
    return constant;
}

uint32_t
ConstantRandomVariable::GetInteger(uint32_t constant)
{
    NS_LOG_DEBUG("integer value: " << constant << " stream: " << GetStream());
    return constant;
}

double
ConstantRandomVariable::GetValue()
{
    return GetValue(m_constant);
}

NS_OBJECT_ENSURE_REGISTERED(SequentialRandomVariable);

TypeId
SequentialRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SequentialRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<SequentialRandomVariable>()
            .AddAttribute("Min",
                          "The first value of the sequence.",
                          DoubleValue(0),
                          MakeDoubleAccessor(&SequentialRandomVariable::m_min),
                          MakeDoubleChecker<double>())
            .AddAttribute("Max",
                          "One more than the last value of the sequence.",
                          DoubleValue(0),
                          MakeDoubleAccessor(&SequentialRandomVariable::m_max),
                          MakeDoubleChecker<double>())
            .AddAttribute("Increment",
                          "The sequence random variable increment.",
                          StringValue("ns3::ConstantRandomVariable[Constant=1]"),
                          MakePointerAccessor(&SequentialRandomVariable::m_increment),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute("Consecutive",
                          "The number of times each member of the sequence is repeated.",
                          IntegerValue(1),
                          MakeIntegerAccessor(&SequentialRandomVariable::m_consecutive),
                          MakeIntegerChecker<uint32_t>());
    return tid;
}

SequentialRandomVariable::SequentialRandomVariable()
    : m_current(0),
      m_currentConsecutive(0),
      m_isCurrentSet(false)
{
    // m_min, m_max, m_increment, and m_consecutive are initialized
    // after constructor by attributes.
    NS_LOG_FUNCTION(this);
}

double
SequentialRandomVariable::GetMin() const
{
    return m_min;
}

double
SequentialRandomVariable::GetMax() const
{
    return m_max;
}

Ptr<RandomVariableStream>
SequentialRandomVariable::GetIncrement() const
{
    return m_increment;
}

uint32_t
SequentialRandomVariable::GetConsecutive() const
{
    return m_consecutive;
}

double
SequentialRandomVariable::GetValue()
{
    // Set the current sequence value if it hasn't been set.
    if (!m_isCurrentSet)
    {
        // Start the sequence at its minimum value.
        m_current = m_min;
        m_isCurrentSet = true;
    }

    // Return a sequential series of values
    double r = m_current;
    if (++m_currentConsecutive == m_consecutive)
    { // Time to advance to next
        m_currentConsecutive = 0;
        m_current += m_increment->GetValue();
        if (m_current >= m_max)
        {
            m_current = m_min + (m_current - m_max);
        }
    }
    NS_LOG_DEBUG("value: " << r << " stream: " << GetStream());
    return r;
}

NS_OBJECT_ENSURE_REGISTERED(ExponentialRandomVariable);

TypeId
ExponentialRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ExponentialRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<ExponentialRandomVariable>()
            .AddAttribute("Mean",
                          "The mean of the values returned by this RNG stream.",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&ExponentialRandomVariable::m_mean),
                          MakeDoubleChecker<double>())
            .AddAttribute("Bound",
                          "The upper bound on the values returned by this RNG stream.",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&ExponentialRandomVariable::m_bound),
                          MakeDoubleChecker<double>());
    return tid;
}

ExponentialRandomVariable::ExponentialRandomVariable()
{
    // m_mean and m_bound are initialized after constructor by attributes
    NS_LOG_FUNCTION(this);
}

double
ExponentialRandomVariable::GetMean() const
{
    return m_mean;
}

double
ExponentialRandomVariable::GetBound() const
{
    return m_bound;
}

double
ExponentialRandomVariable::GetValue(double mean, double bound)
{
    while (true)
    {
        // Get a uniform random variable in [0,1].
        double v = Peek()->RandU01();
        if (IsAntithetic())
        {
            v = (1 - v);
        }

        // Calculate the exponential random variable.
        double r = -mean * std::log(v);

        // Use this value if it's acceptable.
        if (bound == 0 || r <= bound)
        {
            NS_LOG_DEBUG("value: " << r << " stream: " << GetStream() << " mean: " << mean
                                   << " bound: " << bound);
            return r;
        }
    }
}

uint32_t
ExponentialRandomVariable::GetInteger(uint32_t mean, uint32_t bound)
{
    NS_LOG_FUNCTION(this << mean << bound);
    auto v = static_cast<uint32_t>(GetValue(mean, bound));
    NS_LOG_DEBUG("integer value: " << v << " stream: " << GetStream() << " mean: " << mean
                                   << " bound: " << bound);
    return v;
}

double
ExponentialRandomVariable::GetValue()
{
    return GetValue(m_mean, m_bound);
}

NS_OBJECT_ENSURE_REGISTERED(ParetoRandomVariable);

TypeId
ParetoRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ParetoRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<ParetoRandomVariable>()
            .AddAttribute(
                "Scale",
                "The scale parameter for the Pareto distribution returned by this RNG stream.",
                DoubleValue(1.0),
                MakeDoubleAccessor(&ParetoRandomVariable::m_scale),
                MakeDoubleChecker<double>())
            .AddAttribute(
                "Shape",
                "The shape parameter for the Pareto distribution returned by this RNG stream.",
                DoubleValue(2.0),
                MakeDoubleAccessor(&ParetoRandomVariable::m_shape),
                MakeDoubleChecker<double>())
            .AddAttribute(
                "Bound",
                "The upper bound on the values returned by this RNG stream (if non-zero).",
                DoubleValue(0.0),
                MakeDoubleAccessor(&ParetoRandomVariable::m_bound),
                MakeDoubleChecker<double>());
    return tid;
}

ParetoRandomVariable::ParetoRandomVariable()
{
    // m_shape, m_shape, and m_bound are initialized after constructor
    // by attributes
    NS_LOG_FUNCTION(this);
}

double
ParetoRandomVariable::GetScale() const
{
    return m_scale;
}

double
ParetoRandomVariable::GetShape() const
{
    return m_shape;
}

double
ParetoRandomVariable::GetBound() const
{
    return m_bound;
}

double
ParetoRandomVariable::GetValue(double scale, double shape, double bound)
{
    while (true)
    {
        // Get a uniform random variable in [0,1].
        double v = Peek()->RandU01();
        if (IsAntithetic())
        {
            v = (1 - v);
        }

        // Calculate the Pareto random variable.
        double r = (scale * (1.0 / std::pow(v, 1.0 / shape)));

        // Use this value if it's acceptable.
        if (bound == 0 || r <= bound)
        {
            NS_LOG_DEBUG("value: " << r << " stream: " << GetStream() << " scale: " << scale
                                   << " shape: " << shape << " bound: " << bound);
            return r;
        }
    }
}

uint32_t
ParetoRandomVariable::GetInteger(uint32_t scale, uint32_t shape, uint32_t bound)
{
    auto v = static_cast<uint32_t>(GetValue(scale, shape, bound));
    NS_LOG_DEBUG("integer value: " << v << " stream: " << GetStream() << " scale: " << scale
                                   << " shape: " << shape << " bound: " << bound);
    return v;
}

double
ParetoRandomVariable::GetValue()
{
    return GetValue(m_scale, m_shape, m_bound);
}

NS_OBJECT_ENSURE_REGISTERED(WeibullRandomVariable);

TypeId
WeibullRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::WeibullRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<WeibullRandomVariable>()
            .AddAttribute(
                "Scale",
                "The scale parameter for the Weibull distribution returned by this RNG stream.",
                DoubleValue(1.0),
                MakeDoubleAccessor(&WeibullRandomVariable::m_scale),
                MakeDoubleChecker<double>())
            .AddAttribute(
                "Shape",
                "The shape parameter for the Weibull distribution returned by this RNG stream.",
                DoubleValue(1),
                MakeDoubleAccessor(&WeibullRandomVariable::m_shape),
                MakeDoubleChecker<double>())
            .AddAttribute("Bound",
                          "The upper bound on the values returned by this RNG stream.",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&WeibullRandomVariable::m_bound),
                          MakeDoubleChecker<double>());
    return tid;
}

WeibullRandomVariable::WeibullRandomVariable()
{
    // m_scale, m_shape, and m_bound are initialized after constructor
    // by attributes
    NS_LOG_FUNCTION(this);
}

double
WeibullRandomVariable::GetScale() const
{
    return m_scale;
}

double
WeibullRandomVariable::GetShape() const
{
    return m_shape;
}

double
WeibullRandomVariable::GetBound() const
{
    return m_bound;
}

double
WeibullRandomVariable::GetMean(double scale, double shape)
{
    NS_LOG_FUNCTION(scale << shape);
    return scale * std::tgamma(1 + (1 / shape));
}

double
WeibullRandomVariable::GetMean() const
{
    NS_LOG_FUNCTION(this);
    return GetMean(m_scale, m_shape);
}

double
WeibullRandomVariable::GetValue(double scale, double shape, double bound)
{
    double exponent = 1.0 / shape;
    while (true)
    {
        // Get a uniform random variable in [0,1].
        double v = Peek()->RandU01();
        if (IsAntithetic())
        {
            v = (1 - v);
        }

        // Calculate the Weibull random variable.
        double r = scale * std::pow(-std::log(v), exponent);

        // Use this value if it's acceptable.
        if (bound == 0 || r <= bound)
        {
            NS_LOG_DEBUG("value: " << r << " stream: " << GetStream() << " scale: " << scale
                                   << " shape: " << shape << " bound: " << bound);
            return r;
        }
    }
}

uint32_t
WeibullRandomVariable::GetInteger(uint32_t scale, uint32_t shape, uint32_t bound)
{
    auto v = static_cast<uint32_t>(GetValue(scale, shape, bound));
    NS_LOG_DEBUG("integer value: " << v << " stream: " << GetStream() << " scale: " << scale
                                   << " shape: " << shape << " bound: " << bound);
    return v;
}

double
WeibullRandomVariable::GetValue()
{
    NS_LOG_FUNCTION(this);
    return GetValue(m_scale, m_shape, m_bound);
}

NS_OBJECT_ENSURE_REGISTERED(NormalRandomVariable);

const double NormalRandomVariable::INFINITE_VALUE = 1e307;

TypeId
NormalRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NormalRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<NormalRandomVariable>()
            .AddAttribute("Mean",
                          "The mean value for the normal distribution returned by this RNG stream.",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&NormalRandomVariable::m_mean),
                          MakeDoubleChecker<double>())
            .AddAttribute(
                "Variance",
                "The variance value for the normal distribution returned by this RNG stream.",
                DoubleValue(1.0),
                MakeDoubleAccessor(&NormalRandomVariable::m_variance),
                MakeDoubleChecker<double>())
            .AddAttribute("Bound",
                          "The bound on the values returned by this RNG stream.",
                          DoubleValue(INFINITE_VALUE),
                          MakeDoubleAccessor(&NormalRandomVariable::m_bound),
                          MakeDoubleChecker<double>());
    return tid;
}

NormalRandomVariable::NormalRandomVariable()
    : m_nextValid(false)
{
    // m_mean, m_variance, and m_bound are initialized after constructor
    // by attributes
    NS_LOG_FUNCTION(this);
}

double
NormalRandomVariable::GetMean() const
{
    return m_mean;
}

double
NormalRandomVariable::GetVariance() const
{
    return m_variance;
}

double
NormalRandomVariable::GetBound() const
{
    return m_bound;
}

double
NormalRandomVariable::GetValue(double mean, double variance, double bound)
{
    if (m_nextValid)
    { // use previously generated
        m_nextValid = false;
        double x2 = mean + m_v2 * m_y * std::sqrt(variance);
        if (std::fabs(x2 - mean) <= bound)
        {
            NS_LOG_DEBUG("value: " << x2 << " stream: " << GetStream() << " mean: " << mean
                                   << " variance: " << variance << " bound: " << bound);
            return x2;
        }
    }
    while (true)
    { // See Simulation Modeling and Analysis p. 466 (Averill Law)
        // for algorithm; basically a Box-Muller transform:
        // http://en.wikipedia.org/wiki/Box-Muller_transform
        double u1 = Peek()->RandU01();
        double u2 = Peek()->RandU01();
        if (IsAntithetic())
        {
            u1 = (1 - u1);
            u2 = (1 - u2);
        }
        double v1 = 2 * u1 - 1;
        double v2 = 2 * u2 - 1;
        double w = v1 * v1 + v2 * v2;
        if (w <= 1.0)
        { // Got good pair
            double y = std::sqrt((-2 * std::log(w)) / w);
            double x1 = mean + v1 * y * std::sqrt(variance);
            // if x1 is in bounds, return it, cache v2 and y
            if (std::fabs(x1 - mean) <= bound)
            {
                m_nextValid = true;
                m_y = y;
                m_v2 = v2;
                NS_LOG_DEBUG("value: " << x1 << " stream: " << GetStream() << " mean: " << mean
                                       << " variance: " << variance << " bound: " << bound);
                return x1;
            }
            // otherwise try and return the other if it is valid
            double x2 = mean + v2 * y * std::sqrt(variance);
            if (std::fabs(x2 - mean) <= bound)
            {
                m_nextValid = false;
                NS_LOG_DEBUG("value: " << x2 << " stream: " << GetStream() << " mean: " << mean
                                       << " variance: " << variance << " bound: " << bound);
                return x2;
            }
            // otherwise, just run this loop again
        }
    }
}

uint32_t
NormalRandomVariable::GetInteger(uint32_t mean, uint32_t variance, uint32_t bound)
{
    auto v = static_cast<uint32_t>(GetValue(mean, variance, bound));
    NS_LOG_DEBUG("integer value: " << v << " stream: " << GetStream() << " mean: " << mean
                                   << " variance: " << variance << " bound: " << bound);
    return v;
}

double
NormalRandomVariable::GetValue()
{
    return GetValue(m_mean, m_variance, m_bound);
}

NS_OBJECT_ENSURE_REGISTERED(LogNormalRandomVariable);

TypeId
LogNormalRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LogNormalRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<LogNormalRandomVariable>()
            .AddAttribute(
                "Mu",
                "The mu value for the log-normal distribution returned by this RNG stream.",
                DoubleValue(0.0),
                MakeDoubleAccessor(&LogNormalRandomVariable::m_mu),
                MakeDoubleChecker<double>())
            .AddAttribute(
                "Sigma",
                "The sigma value for the log-normal distribution returned by this RNG stream.",
                DoubleValue(1.0),
                MakeDoubleAccessor(&LogNormalRandomVariable::m_sigma),
                MakeDoubleChecker<double>());
    return tid;
}

LogNormalRandomVariable::LogNormalRandomVariable()
    : m_nextValid(false)
{
    // m_mu and m_sigma are initialized after constructor by
    // attributes
    NS_LOG_FUNCTION(this);
}

double
LogNormalRandomVariable::GetMu() const
{
    return m_mu;
}

double
LogNormalRandomVariable::GetSigma() const
{
    return m_sigma;
}

// The code from this function was adapted from the GNU Scientific
// Library 1.8:
/* randist/lognormal.c
 *
 * Copyright (C) 1996, 1997, 1998, 1999, 2000 James Theiler, Brian Gough
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
/* The lognormal distribution has the form

   p(x) dx = 1/(x * sqrt(2 pi sigma^2)) exp(-(ln(x) - zeta)^2/2 sigma^2) dx

   for x > 0. Lognormal random numbers are the exponentials of
   gaussian random numbers */
double
LogNormalRandomVariable::GetValue(double mu, double sigma)
{
    if (m_nextValid)
    { // use previously generated
        m_nextValid = false;
        double v = std::exp(sigma * m_v2 * m_normal + mu);
        NS_LOG_DEBUG("value: " << v << " stream: " << GetStream() << " mu: " << mu
                               << " sigma: " << sigma);
        return v;
    }

    double v1;
    double v2;
    double r2;
    double normal;
    double x;

    do
    {
        /* choose x,y in uniform square (-1,-1) to (+1,+1) */

        double u1 = Peek()->RandU01();
        double u2 = Peek()->RandU01();
        if (IsAntithetic())
        {
            u1 = (1 - u1);
            u2 = (1 - u2);
        }

        v1 = -1 + 2 * u1;
        v2 = -1 + 2 * u2;

        /* see if it is in the unit circle */
        r2 = v1 * v1 + v2 * v2;
    } while (r2 > 1.0 || r2 == 0);

    m_normal = std::sqrt(-2.0 * std::log(r2) / r2);
    normal = v1 * m_normal;
    m_nextValid = true;
    m_v2 = v2;

    x = std::exp(sigma * normal + mu);
    NS_LOG_DEBUG("value: " << x << " stream: " << GetStream() << " mu: " << mu
                           << " sigma: " << sigma);

    return x;
}

uint32_t
LogNormalRandomVariable::GetInteger(uint32_t mu, uint32_t sigma)
{
    auto v = static_cast<uint32_t>(GetValue(mu, sigma));
    NS_LOG_DEBUG("integer value: " << v << " stream: " << GetStream() << " mu: " << mu
                                   << " sigma: " << sigma);
    return v;
}

double
LogNormalRandomVariable::GetValue()
{
    return GetValue(m_mu, m_sigma);
}

NS_OBJECT_ENSURE_REGISTERED(GammaRandomVariable);

TypeId
GammaRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::GammaRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<GammaRandomVariable>()
            .AddAttribute("Alpha",
                          "The alpha value for the gamma distribution returned by this RNG stream.",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&GammaRandomVariable::m_alpha),
                          MakeDoubleChecker<double>())
            .AddAttribute("Beta",
                          "The beta value for the gamma distribution returned by this RNG stream.",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&GammaRandomVariable::m_beta),
                          MakeDoubleChecker<double>());
    return tid;
}

GammaRandomVariable::GammaRandomVariable()
    : m_nextValid(false)
{
    // m_alpha and m_beta are initialized after constructor by
    // attributes
    NS_LOG_FUNCTION(this);
}

double
GammaRandomVariable::GetAlpha() const
{
    return m_alpha;
}

double
GammaRandomVariable::GetBeta() const
{
    return m_beta;
}

/*
  The code for the following generator functions was adapted from ns-2
  tools/ranvar.cc

  Originally the algorithm was devised by Marsaglia in 2000:
  G. Marsaglia, W. W. Tsang: A simple method for generating Gamma variables
  ACM Transactions on mathematical software, Vol. 26, No. 3, Sept. 2000

  The Gamma distribution density function has the form

                             x^(alpha-1) * exp(-x/beta)
        p(x; alpha, beta) = ----------------------------
                             beta^alpha * Gamma(alpha)

  for x > 0.
*/
double
GammaRandomVariable::GetValue(double alpha, double beta)
{
    if (alpha < 1)
    {
        double u = Peek()->RandU01();
        if (IsAntithetic())
        {
            u = (1 - u);
        }
        double v = GetValue(1.0 + alpha, beta) * std::pow(u, 1.0 / alpha);
        NS_LOG_DEBUG("value: " << v << " stream: " << GetStream() << " alpha: " << alpha
                               << " beta: " << beta);
        return GetValue(1.0 + alpha, beta) * std::pow(u, 1.0 / alpha);
    }

    double x;
    double v;
    double u;
    double d = alpha - 1.0 / 3.0;
    double c = (1.0 / 3.0) / std::sqrt(d);

    while (true)
    {
        do
        {
            // Get a value from a normal distribution that has mean
            // zero, variance 1, and no bound.
            double mean = 0.0;
            double variance = 1.0;
            double bound = NormalRandomVariable::INFINITE_VALUE;
            x = GetNormalValue(mean, variance, bound);

            v = 1.0 + c * x;
        } while (v <= 0);

        v = v * v * v;
        u = Peek()->RandU01();
        if (IsAntithetic())
        {
            u = (1 - u);
        }
        if (u < 1 - 0.0331 * x * x * x * x)
        {
            break;
        }
        if (std::log(u) < 0.5 * x * x + d * (1 - v + std::log(v)))
        {
            break;
        }
    }

    double value = beta * d * v;
    NS_LOG_DEBUG("value: " << value << " stream: " << GetStream() << " alpha: " << alpha
                           << " beta: " << beta);
    return value;
}

double
GammaRandomVariable::GetValue()
{
    return GetValue(m_alpha, m_beta);
}

double
GammaRandomVariable::GetNormalValue(double mean, double variance, double bound)
{
    if (m_nextValid)
    { // use previously generated
        m_nextValid = false;
        double x2 = mean + m_v2 * m_y * std::sqrt(variance);
        if (std::fabs(x2 - mean) <= bound)
        {
            NS_LOG_DEBUG("value: " << x2 << " stream: " << GetStream() << " mean: " << mean
                                   << " variance: " << variance << " bound: " << bound);
            return x2;
        }
    }
    while (true)
    { // See Simulation Modeling and Analysis p. 466 (Averill Law)
        // for algorithm; basically a Box-Muller transform:
        // http://en.wikipedia.org/wiki/Box-Muller_transform
        double u1 = Peek()->RandU01();
        double u2 = Peek()->RandU01();
        if (IsAntithetic())
        {
            u1 = (1 - u1);
            u2 = (1 - u2);
        }
        double v1 = 2 * u1 - 1;
        double v2 = 2 * u2 - 1;
        double w = v1 * v1 + v2 * v2;
        if (w <= 1.0)
        { // Got good pair
            double y = std::sqrt((-2 * std::log(w)) / w);
            double x1 = mean + v1 * y * std::sqrt(variance);
            // if x1 is in bounds, return it, cache v2 an y
            if (std::fabs(x1 - mean) <= bound)
            {
                m_nextValid = true;
                m_y = y;
                m_v2 = v2;
                NS_LOG_DEBUG("value: " << x1 << " stream: " << GetStream() << " mean: " << mean
                                       << " variance: " << variance << " bound: " << bound);
                return x1;
            }
            // otherwise try and return the other if it is valid
            double x2 = mean + v2 * y * std::sqrt(variance);
            if (std::fabs(x2 - mean) <= bound)
            {
                m_nextValid = false;
                NS_LOG_DEBUG("value: " << x2 << " stream: " << GetStream() << " mean: " << mean
                                       << " variance: " << variance << " bound: " << bound);
                return x2;
            }
            // otherwise, just run this loop again
        }
    }
}

NS_OBJECT_ENSURE_REGISTERED(ErlangRandomVariable);

TypeId
ErlangRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ErlangRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<ErlangRandomVariable>()
            .AddAttribute("K",
                          "The k value for the Erlang distribution returned by this RNG stream.",
                          IntegerValue(1),
                          MakeIntegerAccessor(&ErlangRandomVariable::m_k),
                          MakeIntegerChecker<uint32_t>())
            .AddAttribute(
                "Lambda",
                "The lambda value for the Erlang distribution returned by this RNG stream.",
                DoubleValue(1.0),
                MakeDoubleAccessor(&ErlangRandomVariable::m_lambda),
                MakeDoubleChecker<double>());
    return tid;
}

ErlangRandomVariable::ErlangRandomVariable()
{
    // m_k and m_lambda are initialized after constructor by attributes
    NS_LOG_FUNCTION(this);
}

uint32_t
ErlangRandomVariable::GetK() const
{
    return m_k;
}

double
ErlangRandomVariable::GetLambda() const
{
    return m_lambda;
}

/*
  The code for the following generator functions was adapted from ns-2
  tools/ranvar.cc

  The Erlang distribution density function has the form

                           x^(k-1) * exp(-x/lambda)
        p(x; k, lambda) = ---------------------------
                             lambda^k * (k-1)!

  for x > 0.
*/
double
ErlangRandomVariable::GetValue(uint32_t k, double lambda)
{
    double mean = lambda;
    double bound = 0.0;

    double result = 0;
    for (unsigned int i = 0; i < k; ++i)
    {
        result += GetExponentialValue(mean, bound);
    }
    NS_LOG_DEBUG("value: " << result << " stream: " << GetStream() << " k: " << k
                           << " lambda: " << lambda);
    return result;
}

uint32_t
ErlangRandomVariable::GetInteger(uint32_t k, uint32_t lambda)
{
    auto v = static_cast<uint32_t>(GetValue(k, lambda));
    NS_LOG_DEBUG("integer value: " << v << " stream: " << GetStream() << " k: " << k
                                   << " lambda: " << lambda);
    return v;
}

double
ErlangRandomVariable::GetValue()
{
    return GetValue(m_k, m_lambda);
}

double
ErlangRandomVariable::GetExponentialValue(double mean, double bound)
{
    while (true)
    {
        // Get a uniform random variable in [0,1].
        double v = Peek()->RandU01();
        if (IsAntithetic())
        {
            v = (1 - v);
        }

        // Calculate the exponential random variable.
        double r = -mean * std::log(v);

        // Use this value if it's acceptable.
        if (bound == 0 || r <= bound)
        {
            NS_LOG_DEBUG("value: " << r << " stream: " << GetStream() << " mean:: " << mean
                                   << " bound: " << bound);
            return r;
        }
    }
}

NS_OBJECT_ENSURE_REGISTERED(TriangularRandomVariable);

TypeId
TriangularRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TriangularRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<TriangularRandomVariable>()
            .AddAttribute(
                "Mean",
                "The mean value for the triangular distribution returned by this RNG stream.",
                DoubleValue(0.5),
                MakeDoubleAccessor(&TriangularRandomVariable::m_mean),
                MakeDoubleChecker<double>())
            .AddAttribute("Min",
                          "The lower bound on the values returned by this RNG stream.",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&TriangularRandomVariable::m_min),
                          MakeDoubleChecker<double>())
            .AddAttribute("Max",
                          "The upper bound on the values returned by this RNG stream.",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&TriangularRandomVariable::m_max),
                          MakeDoubleChecker<double>());
    return tid;
}

TriangularRandomVariable::TriangularRandomVariable()
{
    // m_mean, m_min, and m_max are initialized after constructor by
    // attributes
    NS_LOG_FUNCTION(this);
}

double
TriangularRandomVariable::GetMean() const
{
    return m_mean;
}

double
TriangularRandomVariable::GetMin() const
{
    return m_min;
}

double
TriangularRandomVariable::GetMax() const
{
    return m_max;
}

double
TriangularRandomVariable::GetValue(double mean, double min, double max)
{
    // Calculate the mode.
    double mode = 3.0 * mean - min - max;

    // Get a uniform random variable in [0,1].
    double u = Peek()->RandU01();
    if (IsAntithetic())
    {
        u = (1 - u);
    }

    // Calculate the triangular random variable.
    if (u <= (mode - min) / (max - min))
    {
        double v = min + std::sqrt(u * (max - min) * (mode - min));
        NS_LOG_DEBUG("value: " << v << " stream: " << GetStream() << " mean: " << mean
                               << " min: " << min << " max: " << max);
        return v;
    }
    else
    {
        double v = max - std::sqrt((1 - u) * (max - min) * (max - mode));
        NS_LOG_DEBUG("value: " << v << " stream: " << GetStream() << " mean: " << mean
                               << " min: " << min << " max: " << max);
        return v;
    }
}

uint32_t
TriangularRandomVariable::GetInteger(uint32_t mean, uint32_t min, uint32_t max)
{
    auto v = static_cast<uint32_t>(GetValue(mean, min, max));
    NS_LOG_DEBUG("integer value: " << v << " stream: " << GetStream() << " mean: " << mean
                                   << " min: " << min << " max: " << max);
    return v;
}

double
TriangularRandomVariable::GetValue()
{
    return GetValue(m_mean, m_min, m_max);
}

NS_OBJECT_ENSURE_REGISTERED(ZipfRandomVariable);

TypeId
ZipfRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ZipfRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<ZipfRandomVariable>()
            .AddAttribute("N",
                          "The n value for the Zipf distribution returned by this RNG stream.",
                          IntegerValue(1),
                          MakeIntegerAccessor(&ZipfRandomVariable::m_n),
                          MakeIntegerChecker<uint32_t>())
            .AddAttribute("Alpha",
                          "The alpha value for the Zipf distribution returned by this RNG stream.",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&ZipfRandomVariable::m_alpha),
                          MakeDoubleChecker<double>());
    return tid;
}

ZipfRandomVariable::ZipfRandomVariable()
{
    // m_n and m_alpha are initialized after constructor by attributes
    NS_LOG_FUNCTION(this);
}

uint32_t
ZipfRandomVariable::GetN() const
{
    return m_n;
}

double
ZipfRandomVariable::GetAlpha() const
{
    return m_alpha;
}

double
ZipfRandomVariable::GetValue(uint32_t n, double alpha)
{
    // Calculate the normalization constant c.
    m_c = 0.0;
    for (uint32_t i = 1; i <= n; i++)
    {
        m_c += (1.0 / std::pow((double)i, alpha));
    }
    m_c = 1.0 / m_c;

    // Get a uniform random variable in [0,1].
    double u = Peek()->RandU01();
    if (IsAntithetic())
    {
        u = (1 - u);
    }

    double sum_prob = 0;
    double zipf_value = 0;
    for (uint32_t i = 1; i <= n; i++)
    {
        sum_prob += m_c / std::pow((double)i, alpha);
        if (sum_prob > u)
        {
            zipf_value = i;
            break;
        }
    }
    NS_LOG_DEBUG("value: " << zipf_value << " stream: " << GetStream() << " n: " << n
                           << " alpha: " << alpha);
    return zipf_value;
}

uint32_t
ZipfRandomVariable::GetInteger(uint32_t n, uint32_t alpha)
{
    NS_LOG_FUNCTION(this << n << alpha);
    auto v = static_cast<uint32_t>(GetValue(n, alpha));
    NS_LOG_DEBUG("integer value: " << v << " stream: " << GetStream() << " n: " << n
                                   << " alpha: " << alpha);
    return v;
}

double
ZipfRandomVariable::GetValue()
{
    return GetValue(m_n, m_alpha);
}

NS_OBJECT_ENSURE_REGISTERED(ZetaRandomVariable);

TypeId
ZetaRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ZetaRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<ZetaRandomVariable>()
            .AddAttribute("Alpha",
                          "The alpha value for the zeta distribution returned by this RNG stream.",
                          DoubleValue(3.14),
                          MakeDoubleAccessor(&ZetaRandomVariable::m_alpha),
                          MakeDoubleChecker<double>());
    return tid;
}

ZetaRandomVariable::ZetaRandomVariable()
{
    // m_alpha is initialized after constructor by attributes
    NS_LOG_FUNCTION(this);
}

double
ZetaRandomVariable::GetAlpha() const
{
    return m_alpha;
}

double
ZetaRandomVariable::GetValue(double alpha)
{
    m_b = std::pow(2.0, alpha - 1.0);

    double u;
    double v;
    double X;
    double T;
    double test;

    do
    {
        // Get a uniform random variable in [0,1].
        u = Peek()->RandU01();
        if (IsAntithetic())
        {
            u = (1 - u);
        }

        // Get a uniform random variable in [0,1].
        v = Peek()->RandU01();
        if (IsAntithetic())
        {
            v = (1 - v);
        }

        X = std::floor(std::pow(u, -1.0 / (alpha - 1.0)));
        T = std::pow(1.0 + 1.0 / X, alpha - 1.0);
        test = v * X * (T - 1.0) / (m_b - 1.0);
    } while (test > (T / m_b));
    NS_LOG_DEBUG("value: " << X << " stream: " << GetStream() << " alpha: " << alpha);
    return X;
}

uint32_t
ZetaRandomVariable::GetInteger(uint32_t alpha)
{
    auto v = static_cast<uint32_t>(GetValue(alpha));
    NS_LOG_DEBUG("integer value: " << v << " stream: " << GetStream() << " alpha: " << alpha);
    return v;
}

double
ZetaRandomVariable::GetValue()
{
    return GetValue(m_alpha);
}

NS_OBJECT_ENSURE_REGISTERED(DeterministicRandomVariable);

TypeId
DeterministicRandomVariable::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DeterministicRandomVariable")
                            .SetParent<RandomVariableStream>()
                            .SetGroupName("Core")
                            .AddConstructor<DeterministicRandomVariable>();
    return tid;
}

DeterministicRandomVariable::DeterministicRandomVariable()
    : m_count(0),
      m_next(0),
      m_data(nullptr)
{
    NS_LOG_FUNCTION(this);
}

DeterministicRandomVariable::~DeterministicRandomVariable()
{
    // Delete any values currently set.
    NS_LOG_FUNCTION(this);
    if (m_data != nullptr)
    {
        delete[] m_data;
    }
}

void
DeterministicRandomVariable::SetValueArray(const std::vector<double>& values)
{
    SetValueArray(values.data(), values.size());
}

void
DeterministicRandomVariable::SetValueArray(const double* values, std::size_t length)
{
    NS_LOG_FUNCTION(this << values << length);
    // Delete any values currently set.
    if (m_data != nullptr)
    {
        delete[] m_data;
    }

    // Make room for the values being set.
    m_data = new double[length];
    m_count = length;
    m_next = length;

    // Copy the values.
    for (std::size_t i = 0; i < m_count; i++)
    {
        m_data[i] = values[i];
    }
}

double
DeterministicRandomVariable::GetValue()
{
    // Make sure the array has been set.
    NS_ASSERT(m_count > 0);

    if (m_next == m_count)
    {
        m_next = 0;
    }
    double v = m_data[m_next++];
    NS_LOG_DEBUG("value: " << v << " stream: " << GetStream());
    return v;
}

NS_OBJECT_ENSURE_REGISTERED(EmpiricalRandomVariable);

TypeId
EmpiricalRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::EmpiricalRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<EmpiricalRandomVariable>()
            .AddAttribute("Interpolate",
                          "Treat the CDF as a smooth distribution and interpolate, "
                          "default is to treat the CDF as a histogram and sample.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&EmpiricalRandomVariable::m_interpolate),
                          MakeBooleanChecker());
    return tid;
}

EmpiricalRandomVariable::EmpiricalRandomVariable()
    : m_validated(false)
{
    NS_LOG_FUNCTION(this);
}

bool
EmpiricalRandomVariable::SetInterpolate(bool interpolate)
{
    NS_LOG_FUNCTION(this << interpolate);
    bool prev = m_interpolate;
    m_interpolate = interpolate;
    return prev;
}

bool
EmpiricalRandomVariable::PreSample(double& value)
{
    NS_LOG_FUNCTION(this << value);
    if (!m_validated)
    {
        Validate();
    }

    // Get a uniform random variable in [0, 1].
    double r = Peek()->RandU01();
    if (IsAntithetic())
    {
        r = (1 - r);
    }

    value = r;
    bool valid = false;
    // check extrema
    if (r <= m_empCdf.begin()->first)
    {
        value = m_empCdf.begin()->second; // Less than first
        valid = true;
    }
    else if (r >= m_empCdf.rbegin()->first)
    {
        value = m_empCdf.rbegin()->second; // Greater than last
        valid = true;
    }
    return valid;
}

double
EmpiricalRandomVariable::GetValue()
{
    double value;
    if (PreSample(value))
    {
        return value;
    }

    // value now has the (unused) URNG selector
    if (m_interpolate)
    {
        value = DoInterpolate(value);
    }
    else
    {
        value = DoSampleCDF(value);
    }
    NS_LOG_DEBUG("value: " << value << " stream: " << GetStream());
    return value;
}

double
EmpiricalRandomVariable::DoSampleCDF(double r)
{
    NS_LOG_FUNCTION(this << r);

    // Find first CDF that is greater than r
    auto bound = m_empCdf.upper_bound(r);

    return bound->second;
}

double
EmpiricalRandomVariable::Interpolate()
{
    NS_LOG_FUNCTION(this);

    double value;
    if (PreSample(value))
    {
        return value;
    }

    // value now has the (unused) URNG selector
    value = DoInterpolate(value);
    return value;
}

double
EmpiricalRandomVariable::DoInterpolate(double r)
{
    NS_LOG_FUNCTION(this << r);

    // Return a value from the empirical distribution
    // This code based (loosely) on code by Bruce Mah (Thanks Bruce!)

    // search
    auto upper = m_empCdf.upper_bound(r);
    auto lower = std::prev(upper, 1);

    if (upper == m_empCdf.begin())
    {
        lower = upper;
    }

    // Interpolate random value in range [v1..v2) based on [c1 .. r .. c2)
    double c1 = lower->first;
    double c2 = upper->first;
    double v1 = lower->second;
    double v2 = upper->second;

    double value = (v1 + ((v2 - v1) / (c2 - c1)) * (r - c1));
    return value;
}

void
EmpiricalRandomVariable::CDF(double v, double c)
{
    NS_LOG_FUNCTION(this << v << c);

    auto vPrevious = m_empCdf.find(c);

    if (vPrevious != m_empCdf.end())
    {
        NS_LOG_WARN("Empirical CDF already has a value " << vPrevious->second << " for CDF " << c
                                                         << ". Overwriting it with value " << v
                                                         << ".");
    }

    m_empCdf[c] = v;
}

void
EmpiricalRandomVariable::Validate()
{
    NS_LOG_FUNCTION(this);

    if (m_empCdf.empty())
    {
        NS_FATAL_ERROR("CDF is not initialized");
    }

    double vPrev = m_empCdf.begin()->second;

    // Check if values are non-decreasing
    for (const auto& cdfPair : m_empCdf)
    {
        const auto& vCurr = cdfPair.second;

        if (vCurr < vPrev)
        {
            NS_FATAL_ERROR("Empirical distribution has decreasing CDF values. Current CDF: "
                           << vCurr << ", prior CDF: " << vPrev);
        }

        vPrev = vCurr;
    }

    // Bounds check on CDF endpoints
    auto firstCdfPair = m_empCdf.begin();
    auto lastCdfPair = m_empCdf.rbegin();

    if (firstCdfPair->first < 0.0)
    {
        NS_FATAL_ERROR("Empirical distribution has invalid first CDF value. CDF: "
                       << firstCdfPair->first << ", Value: " << firstCdfPair->second);
    }

    if (lastCdfPair->first > 1.0)
    {
        NS_FATAL_ERROR("Empirical distribution has invalid last CDF value. CDF: "
                       << lastCdfPair->first << ", Value: " << lastCdfPair->second);
    }

    m_validated = true;
}

NS_OBJECT_ENSURE_REGISTERED(BinomialRandomVariable);

TypeId
BinomialRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::BinomialRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<BinomialRandomVariable>()
            .AddAttribute("Trials",
                          "The number of trials.",
                          IntegerValue(10),
                          MakeIntegerAccessor(&BinomialRandomVariable::m_trials),
                          MakeIntegerChecker<uint32_t>(0))
            .AddAttribute("Probability",
                          "The probability of success in each trial.",
                          DoubleValue(0.5),
                          MakeDoubleAccessor(&BinomialRandomVariable::m_probability),
                          MakeDoubleChecker<double>(0));
    return tid;
}

BinomialRandomVariable::BinomialRandomVariable()
{
    // m_trials and m_probability are initialized after constructor by attributes
    NS_LOG_FUNCTION(this);
}

double
BinomialRandomVariable::GetValue(uint32_t trials, double probability)
{
    double successes = 0;

    for (uint32_t i = 0; i < trials; ++i)
    {
        double v = Peek()->RandU01();
        if (IsAntithetic())
        {
            v = (1 - v);
        }

        if (v <= probability)
        {
            successes += 1;
        }
    }
    NS_LOG_DEBUG("value: " << successes << " stream: " << GetStream() << " trials: " << trials
                           << " probability: " << probability);
    return successes;
}

uint32_t
BinomialRandomVariable::GetInteger(uint32_t trials, uint32_t probability)
{
    auto v = static_cast<uint32_t>(GetValue(trials, probability));
    NS_LOG_DEBUG("integer value: " << v << " stream: " << GetStream() << " trials: " << trials
                                   << " probability: " << probability);
    return v;
}

double
BinomialRandomVariable::GetValue()
{
    return GetValue(m_trials, m_probability);
}

NS_OBJECT_ENSURE_REGISTERED(BernoulliRandomVariable);

TypeId
BernoulliRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::BernoulliRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<BernoulliRandomVariable>()
            .AddAttribute("Probability",
                          "The probability of the random variable returning a value of 1.",
                          DoubleValue(0.5),
                          MakeDoubleAccessor(&BernoulliRandomVariable::m_probability),
                          MakeDoubleChecker<double>(0));
    return tid;
}

BernoulliRandomVariable::BernoulliRandomVariable()
{
    // m_probability is initialized after constructor by attributes
    NS_LOG_FUNCTION(this);
}

double
BernoulliRandomVariable::GetValue(double probability)
{
    double v = Peek()->RandU01();
    if (IsAntithetic())
    {
        v = (1 - v);
    }

    double value = (v <= probability) ? 1.0 : 0.0;
    NS_LOG_DEBUG("value: " << value << " stream: " << GetStream()
                           << " probability: " << probability);
    return value;
}

uint32_t
BernoulliRandomVariable::GetInteger(uint32_t probability)
{
    auto v = static_cast<uint32_t>(GetValue(probability));
    NS_LOG_DEBUG("integer value: " << v << " stream: " << GetStream()
                                   << " probability: " << probability);
    return v;
}

double
BernoulliRandomVariable::GetValue()
{
    return GetValue(m_probability);
}

NS_OBJECT_ENSURE_REGISTERED(LaplacianRandomVariable);

TypeId
LaplacianRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LaplacianRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<LaplacianRandomVariable>()
            .AddAttribute("Location",
                          "The location parameter for the Laplacian distribution returned by this "
                          "RNG stream.",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&LaplacianRandomVariable::m_location),
                          MakeDoubleChecker<double>())
            .AddAttribute(
                "Scale",
                "The scale parameter for the Laplacian distribution returned by this RNG stream.",
                DoubleValue(1.0),
                MakeDoubleAccessor(&LaplacianRandomVariable::m_scale),
                MakeDoubleChecker<double>())
            .AddAttribute("Bound",
                          "The bound on the values returned by this RNG stream.",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&LaplacianRandomVariable::m_bound),
                          MakeDoubleChecker<double>());
    return tid;
}

LaplacianRandomVariable::LaplacianRandomVariable()
{
    NS_LOG_FUNCTION(this);
}

double
LaplacianRandomVariable::GetLocation() const
{
    return m_location;
}

double
LaplacianRandomVariable::GetScale() const
{
    return m_scale;
}

double
LaplacianRandomVariable::GetBound() const
{
    return m_bound;
}

double
LaplacianRandomVariable::GetValue(double location, double scale, double bound)
{
    NS_LOG_FUNCTION(this << location << scale << bound);
    NS_ABORT_MSG_IF(scale <= 0, "Scale parameter should be larger than 0");

    while (true)
    {
        // Get a uniform random variable in [-0.5,0.5].
        auto v = (Peek()->RandU01() - 0.5);
        if (IsAntithetic())
        {
            v = (1 - v);
        }

        // Calculate the laplacian random variable.
        const auto sgn = (v > 0) ? 1 : ((v < 0) ? -1 : 0);
        const auto r = location - (scale * sgn * std::log(1.0 - (2.0 * std::abs(v))));

        // Use this value if it's acceptable.
        if (bound == 0.0 || std::fabs(r - location) <= bound)
        {
            return r;
        }
    }
}

uint32_t
LaplacianRandomVariable::GetInteger(uint32_t location, uint32_t scale, uint32_t bound)
{
    NS_LOG_FUNCTION(this << location << scale << bound);
    return static_cast<uint32_t>(GetValue(location, scale, bound));
}

double
LaplacianRandomVariable::GetValue()
{
    NS_LOG_FUNCTION(this);
    return GetValue(m_location, m_scale, m_bound);
}

double
LaplacianRandomVariable::GetVariance(double scale)
{
    NS_LOG_FUNCTION(scale);
    return 2.0 * std::pow(scale, 2.0);
}

double
LaplacianRandomVariable::GetVariance() const
{
    return GetVariance(m_scale);
}

NS_OBJECT_ENSURE_REGISTERED(LargestExtremeValueRandomVariable);

TypeId
LargestExtremeValueRandomVariable::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LargestExtremeValueRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<LargestExtremeValueRandomVariable>()
            .AddAttribute("Location",
                          "The location parameter for the Largest Extreme Value distribution "
                          "returned by this RNG stream.",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&LargestExtremeValueRandomVariable::m_location),
                          MakeDoubleChecker<double>())
            .AddAttribute("Scale",
                          "The scale parameter for the Largest Extreme Value distribution "
                          "returned by this RNG stream.",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&LargestExtremeValueRandomVariable::m_scale),
                          MakeDoubleChecker<double>());
    return tid;
}

LargestExtremeValueRandomVariable::LargestExtremeValueRandomVariable()
{
    NS_LOG_FUNCTION(this);
}

double
LargestExtremeValueRandomVariable::GetLocation() const
{
    return m_location;
}

double
LargestExtremeValueRandomVariable::GetScale() const
{
    return m_scale;
}

double
LargestExtremeValueRandomVariable::GetValue(double location, double scale)
{
    NS_LOG_FUNCTION(this << location << scale);
    NS_ABORT_MSG_IF(scale <= 0, "Scale parameter should be larger than 0");

    // Get a uniform random variable in [0,1].
    auto v = Peek()->RandU01();
    if (IsAntithetic())
    {
        v = (1 - v);
    }

    // Calculate the largest extreme value random variable.
    const auto t = std::log(v) * (-1.0);
    const auto r = location - (scale * std::log(t));

    return r;
}

uint32_t
LargestExtremeValueRandomVariable::GetInteger(uint32_t location, uint32_t scale)
{
    NS_LOG_FUNCTION(this << location << scale);
    return static_cast<uint32_t>(GetValue(location, scale));
}

double
LargestExtremeValueRandomVariable::GetValue()
{
    NS_LOG_FUNCTION(this);
    return GetValue(m_location, m_scale);
}

double
LargestExtremeValueRandomVariable::GetMean(double location, double scale)
{
    NS_LOG_FUNCTION(location << scale);
    return (location + (scale * std::numbers::egamma));
}

double
LargestExtremeValueRandomVariable::GetMean() const
{
    return GetMean(m_location, m_scale);
}

double
LargestExtremeValueRandomVariable::GetVariance(double scale)
{
    NS_LOG_FUNCTION(scale);
    return std::pow((scale * std::numbers::pi), 2) / 6.0;
}

double
LargestExtremeValueRandomVariable::GetVariance() const
{
    return GetVariance(m_scale);
}

} // namespace ns3
