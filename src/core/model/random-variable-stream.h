/*
 * Copyright (c) 2006 Georgia Tech Research Corporation
 * Copyright (c) 2011 Mathieu Lacage
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Rajib Bhattacharjea<raj.b@gatech.edu>
 *          Hadi Arbabi<marbabi@cs.odu.edu>
 *          Mathieu Lacage <mathieu.lacage@gmail.com>
 *          Alessio Bugetti <alessiobugetti98@gmail.com>
 *
 * Modified by Mitch Watrous <watrous@u.washington.edu>
 *
 */
#ifndef RANDOM_VARIABLE_STREAM_H
#define RANDOM_VARIABLE_STREAM_H

#include "attribute-helper.h"
#include "object.h"
#include "type-id.h"

#include <map>
#include <stdint.h>

/**
 * @file
 * @ingroup randomvariable
 * ns3::RandomVariableStream declaration, and related classes.
 */

namespace ns3
{

/**
 * @ingroup core
 * @defgroup randomvariable Random Variables
 *
 * @brief ns-3 random numbers are provided via instances of
 * ns3::RandomVariableStream.
 *
 * - By default, ns-3 simulations use a fixed seed; if there is any
 *   randomness in the simulation, each run of the program will yield
 *   identical results unless the seed and/or run number is changed.
 * - In ns-3.3 and earlier, ns-3 simulations used a random seed by default;
 *   this marks a change in policy starting with ns-3.4.
 * - In ns-3.14 and earlier, ns-3 simulations used a different wrapper
 *   class called ns3::RandomVariable.  This implementation is documented
 *   above under Legacy Random Variables. As of ns-3.15, this class has
 *   been replaced by ns3::RandomVariableStream; the underlying
 *   pseudo-random number generator has not changed.
 * - To obtain randomness across multiple simulation runs, you must
 *   either set the seed differently or set the run number differently.
 *   To set a seed, call ns3::RngSeedManager::SetSeed() at the beginning
 *   of the program; to set a run number with the same seed, call
 *   ns3::RngSeedManager::SetRun() at the beginning of the program.
 * - Each RandomVariableStream used in ns-3 has a virtual random number
 *   generator associated with it; all random variables use either
 *   a fixed or random seed based on the use of the global seed.
 * - If you intend to perform multiple runs of the same scenario,
 *   with different random numbers, please be sure to read the manual
 *   section on how to perform independent replications.
 */

class RngStream;

/**
 * @ingroup randomvariable
 * @brief The basic uniform Random Number Generator (RNG).
 *
 * @note The underlying random number generation method used
 * by ns-3 is the RngStream code by Pierre L'Ecuyer at
 * the University of Montreal.
 *
 * ns-3 has a rich set of random number generators that allow stream
 * numbers to be set deterministically if desired.  Class
 * RandomVariableStream defines the base class functionality required
 * for all such random number generators.
 *
 * By default, the underlying generator is seeded all the time with
 * the same seed value and run number coming from the ns3::GlobalValue
 * @ref GlobalValueRngSeed "RngSeed" and @ref GlobalValueRngRun
 * "RngRun".  Also by default, the stream number value for the
 * underlying RngStream is automatically allocated.
 *
 * Instances can be configured to return "antithetic" values.
 * See the documentation for the specific distributions to see
 * how this modifies the returned values.
 */
class RandomVariableStream : public Object
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();
    /**
     * @brief Default constructor.
     */
    RandomVariableStream();
    /**
     * @brief Destructor.
     */
    ~RandomVariableStream() override;

    // Delete copy constructor and assignment operator to avoid misuse
    RandomVariableStream(const RandomVariableStream&) = delete;
    RandomVariableStream& operator=(const RandomVariableStream&) = delete;

    /**
     * @brief Specifies the stream number for the RngStream.
     * @param [in] stream The stream number for the RngStream.
     * -1 means "allocate a stream number automatically".
     */
    void SetStream(int64_t stream);

    /**
     * @brief Returns the stream number for the RngStream.
     * @return The stream number for the RngStream.
     * -1 means this stream was allocated automatically.
     */
    int64_t GetStream() const;

    /**
     * @brief Specify whether antithetic values should be generated.
     * @param [in] isAntithetic If \c true antithetic value will be generated.
     */
    void SetAntithetic(bool isAntithetic);

    /**
     * @brief Check if antithetic values will be generated.
     * @return \c true if antithetic values will be generated.
     */
    bool IsAntithetic() const;

    /**
     * @brief Get the next random value drawn from the distribution.
     * @return A random value.
     */
    virtual double GetValue() = 0;

    /** @copydoc GetValue() */
    // The base implementation returns `(uint32_t)GetValue()`
    virtual uint32_t GetInteger();

  protected:
    /**
     * @brief Get the pointer to the underlying RngStream.
     * @return The underlying RngStream
     */
    RngStream* Peek() const;

  private:
    /** Pointer to the underlying RngStream. */
    RngStream* m_rng;

    /** Indicates if antithetic values should be generated by this RNG stream. */
    bool m_isAntithetic;

    /** The stream number for the RngStream. */
    int64_t m_stream;

    // end of class RandomVariableStream
};

/**
 * @ingroup randomvariable
 * @brief The uniform distribution Random Number Generator (RNG).
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed uniform distribution.  It also supports the generation of
 * single random numbers from various uniform distributions.
 *
 * The output range is \f$ x \in \f$ [\c Min, \c Max) for floating point values,
 * (\c Max _excluded_), and \f$ x \in \f$ [\c Min, \c Max] (\c Max _included_)
 * for integral values.
 *
 * This distribution has mean
 *
 *    \f[
 *       \mu = \frac{\text{Max} + \text{Min}}{2}
 *    \f]
 *
 * and variance
 *
 *   \f[
 *       \sigma^2 = \frac{\left(\text{Max} - \text{Min}\right)^2}{12}
 *   \f]
 *
 * The uniform RNG value \f$x\f$ is generated by
 *
 *   \f[
 *      x = \text{Min} + u (\text{Max} - \text{Min})
 *   \f]
 *
 * where \f$u\f$ is a uniform random variable on [0,1).
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   double min = 0.0;
 *   double max = 10.0;
 *
 *   Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
 *   x->SetAttribute ("Min", DoubleValue (min));
 *   x->SetAttribute ("Max", DoubleValue (max));
 *
 *   double value = x->GetValue ();
 * @endcode
 *
 * @par Antithetic Values.
 *
 * Normally this RNG returns values \f$x\f$ for floating point values
 * (or \f$k\f$ for integer values) uniformly in the interval [\c Min, \c Max)
 * (or [\c Min, \c Max] for integer values).
 * If an instance of this RNG is configured to return antithetic values,
 * the actual value returned is calculated as follows:
 *
 *   - Compute the initial random value \f$x\f$ as normal.
 *   - Compute the distance from the maximum, \f$y = \text{Max} - x\f$
 *   - Return \f$x' = \text{Min} + y = \text{Min} + (\text{Max} - x)\f$:
 */
class UniformRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a uniform distribution RNG with the default range.
     */
    UniformRandomVariable();

    /**
     * @brief Get the lower bound on randoms returned by GetValue().
     * @return The lower bound on values from GetValue().
     */
    double GetMin() const;

    /**
     * @brief Get the upper bound on values returned by GetValue().
     * @return The upper bound on values from GetValue().
     */
    double GetMax() const;

    /**
     * @copydoc GetValue()
     *
     * @param [in] min Low end of the range (included).
     * @param [in] max High end of the range (excluded).
     */
    double GetValue(double min, double max);

    /**
     * @copydoc GetValue(double,double)
     * @note The upper limit is included in the output range, unlike GetValue(double,double).
     */
    uint32_t GetInteger(uint32_t min, uint32_t max);

    // Inherited
    /**
     * @copydoc RandomVariableStream::GetValue()
     * @note The upper limit is excluded from the output range, unlike GetInteger().
     */
    double GetValue() override;

    /**
     * @copydoc RandomVariableStream::GetInteger()
     * @note The upper limit is included in the output range, unlike GetValue().
     */
    uint32_t GetInteger() override;

  private:
    /** The lower bound on values that can be returned by this RNG stream. */
    double m_min;

    /** The upper bound on values that can be returned by this RNG stream. */
    double m_max;

    // end of class UniformRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The Random Number Generator (RNG) that returns a constant.
 *
 * This RNG returns the same \c Constant value for every sample.
 *
 * This distribution has mean equal to the \c Constant and zero variance.
 *
 * @par Antithetic Values.
 *
 * This RNG ignores the antithetic setting.
 */
class ConstantRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a constant RNG with the default constant value.
     */
    ConstantRandomVariable();

    /**
     * @brief Get the constant value returned by this RNG stream.
     * @return The constant value.
     */
    double GetConstant() const;

    /**
     * @copydoc GetValue()
     * @param [in] constant The value to return.
     */
    double GetValue(double constant);
    /** @copydoc GetValue(double) */
    uint32_t GetInteger(uint32_t constant);

    // Inherited
    /*
     * @copydoc RandomVariableStream::GetValue()
     * @note This RNG always returns the same value.
     */
    double GetValue() override;
    /* \note This RNG always returns the same value. */
    using RandomVariableStream::GetInteger;

  private:
    /** The constant value returned by this RNG stream. */
    double m_constant;

    // end of class ConstantRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The Random Number Generator (RNG) that returns a pattern of
 * sequential values.
 *
 * This RNG has four configuration attributes:
 *
 *  - An increment, \c Increment.
 *  - A consecutive repeat number, \c Consecutive.
 *  - The minimum value, \c Min.
 *  - The maximum value, \c Max.
 *
 * The RNG starts at the \c Min value.  Each return value is
 * repeated \c Consecutive times, before advancing by the \c Increment,
 * modulo the \c Max.  In other words when the \c Increment would cause
 * the value to equal or exceed \c Max it is reset to \c Min plus the
 * remainder:
 *
 * \code{.cc}
 *     m_current += m_increment->GetValue();
 *     if (m_current >= m_max)
 *     {
 *         m_current = m_min + (m_current - m_max);
 *     }
 * @endcode
 *
 * This RNG returns values in the range \f$ x \in [\text{Min}, \text{Max}) \f$.
 * See the Example, below, for how this executes in practice.
 *
 * Note the \c Increment attribute is itself a RandomVariableStream,
 * which enables more varied patterns than in the example given here.
 *
 * @par Example
 *
 * For example, if an instance is configured with:
 *
 *   Attribute   | Value
 *   :---------- | -----:
 *   Min         |    2
 *   Max         |   13
 *   Increment   |    4
 *   Consecutive |    3
 *
 * The sequence will return this pattern:
 *
 *    \f[
 *       x \in \\
 *       \underbrace{  2, 2,  2, }_{\times 3} \\
 *       \underbrace{ 6,  6,  6, }_{\times 3} \\
 *       \underbrace{10, 10, 10, }_{\times 3} \\
 *       \underbrace{ 3,  3,  3, }_{\times 3} \\
 *       \dots
 *    \f]
 *
 * The last value (3) is the result of the update rule in the code snippet
 * above, `2 + (14 - 13)`.
 *
 * @par Antithetic Values.
 *
 * This RNG ignores the antithetic setting.
 */
class SequentialRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a sequential RNG with the default values
     * for the sequence parameters.
     */
    SequentialRandomVariable();

    /**
     * @brief Get the first value of the sequence.
     * @return The first value of the sequence.
     */
    double GetMin() const;

    /**
     * @brief Get the limit of the sequence, which is (at least)
     * one more than the last value of the sequence.
     * @return The limit of the sequence.
     */
    double GetMax() const;

    /**
     * @brief Get the increment for the sequence.
     * @return The increment between distinct values for the sequence.
     */
    Ptr<RandomVariableStream> GetIncrement() const;

    /**
     * @brief Get the number of times each distinct value of the sequence
     * is repeated before incrementing to the next value.
     * @return The number of times each value is repeated.
     */
    uint32_t GetConsecutive() const;

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /** The first value of the sequence. */
    double m_min;

    /** Strict upper bound on the sequence. */
    double m_max;

    /** Increment between distinct values. */
    Ptr<RandomVariableStream> m_increment;

    /** The number of times each distinct value is repeated. */
    uint32_t m_consecutive;

    /** The current sequence value. */
    double m_current;

    /** The number of times the current distinct value has been repeated. */
    uint32_t m_currentConsecutive;

    /** Indicates if the current sequence value has been properly initialized. */
    bool m_isCurrentSet;

    // end of class SequentialRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The exponential distribution Random Number Generator (RNG).
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed exponential distribution.  It also supports the generation of
 * single random numbers from various exponential distributions.
 *
 * The probability density function of an exponential variable
 * is defined as:
 *
 *   \f[
 *      P(x; \alpha) dx = \alpha  e^{-\alpha x} dx, \\
 *          \quad x \in [0, +\infty)
 *   \f]
 *
 * where \f$ \alpha = \frac{1}{\mu} \f$
 * and \f$\mu\f$ is the \c Mean configurable attribute.  This distribution
 * has variance \f$ \sigma^2 = \alpha^2 \f$.
 *
 * The exponential RNG value \f$x\f$ is generated by
 *
 *   \f[
 *      x = - \frac{\log(u)}{\alpha} = - \text{Mean} \log(u)
 *   \f]
 *
 * where \f$u\f$ is a uniform random variable on [0,1).
 *
 * @par Bounded Distribution
 *
 * Since the exponential distributions can theoretically return unbounded
 * values, it is sometimes useful to specify a fixed upper \c Bound.  The
 * bounded version is defined over the interval \f$[0,b]\f$ as:
 *
 *   \f[
 *      P(x; \alpha, b) dx = \alpha  e^{-\alpha x} dx, \\
 *          \quad x \in [0, b]
 *   \f]
 *
 * Note that in this case the true mean of the distribution is smaller
 * than the nominal mean value:
 *
 *   \f[
 *      \langle x | P(x; \alpha, b) \rangle = \frac{1}{\alpha} -
 *          \left(\frac{1}{\alpha} + b\right)\exp^{-\alpha b}
 *   \f]
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   double mean = 3.14;
 *   double bound = 0.0;
 *
 *   Ptr<ExponentialRandomVariable> x = CreateObject<ExponentialRandomVariable> ();
 *   x->SetAttribute ("Mean", DoubleValue (mean));
 *   x->SetAttribute ("Bound", DoubleValue (bound));
 *
 *   double value = x->GetValue ();
 * @endcode
 *
 * @par Antithetic Values.
 *
 * If an instance of this RNG is configured to return antithetic values,
 * the actual value returned, \f$x'\f$, is generated as follows:
 *
 *   \f[
 *      x' = - \frac{\log(1 - u)}{\alpha} = - Mean \log(1 - u),
 *   \f]
 *
 * where \f$u\f$ is a uniform random variable on [0,1).
 */
class ExponentialRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates an exponential distribution RNG with the default
     * values for the mean and upper bound.
     */
    ExponentialRandomVariable();

    /**
     * @brief Get the configured mean value of this RNG.
     *
     * @note This will not be the actual mean if the distribution is
     * truncated by a bound.
     * @return The configured mean value.
     */
    double GetMean() const;

    /**
     * @brief Get the configured upper bound of this RNG.
     * @return The upper bound.
     */
    double GetBound() const;

    /**
     * @copydoc GetValue()
     * @param [in] mean Mean value of the unbounded exponential distribution.
     * @param [in] bound Upper bound on values returned.
     */
    double GetValue(double mean, double bound);

    /** @copydoc GetValue(double,double) */
    uint32_t GetInteger(uint32_t mean, uint32_t bound);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /** The mean value of the unbounded exponential distribution. */
    double m_mean;

    /** The upper bound on values that can be returned by this RNG stream. */
    double m_bound;

    // end of class ExponentialRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The Pareto distribution Random Number Generator (RNG).
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed Pareto distribution.  It also supports the generation of
 * single random numbers from various Pareto distributions.
 *
 * The probability density function of a Pareto variable is:
 *
 *   \f[
 *      P(x; x_m, \alpha) dx = \alpha \frac{x_m^\alpha}{x^{\alpha + 1}} dx, \\
 *          \quad x \in [x_m, +\infty)
 *   \f]
 *
 * where the minimum value \f$x_m > 0\f$ is called the \c Scale parameter
 * and \f$ \alpha > 0\f$ is called the Pareto index or \c Shape parameter.
 *
 * The resulting distribution will have mean
 *
 *   \f[
 *      \mu = x_m \frac{\alpha}{\alpha - 1} \mbox{ for $\alpha > 1$}
 *   \f]
 *
 * and variance
 *
 *   \f[
 *      \sigma^2 = x_m \frac{1}{(\alpha - 1)(\alpha - 2)} \mbox { for $\alpha > 2$}
 *   \f]
 *
 * The minimum value \f$x_m\f$ can be inferred from the desired mean \f$\mu\f$
 * and the parameter \f$\alpha\f$ with the equation
 *
 *   \f[
 *      x_m = \mu \frac{\alpha - 1}{\alpha} \mbox{ for $\alpha > 1$}
 *   \f]
 *
 * The Pareto RNG value \f$x\f$ is generated by
 *
 *   \f[
 *      x = \frac{x_m}{u^{\frac{1}{\alpha}}}
 *   \f]
 *
 * where \f$u\f$ is a uniform random variable on [0,1).
 *
 * @par Bounded Distribution
 *
 * Since Pareto distributions can theoretically return unbounded
 * values, it is sometimes useful to specify a fixed upper \c Bound.  The
 * bounded version is defined over the interval \f$ x \in [x_m, b] \f$.
 * Note however when the upper limit is specified, the mean
 * of the resulting distribution is slightly smaller than the mean value
 * in the unbounded case.
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   double scale = 5.0;
 *   double shape = 2.0;
 *
 *   Ptr<ParetoRandomVariable> x = CreateObject<ParetoRandomVariable> ();
 *   x->SetAttribute ("Scale", DoubleValue (scale));
 *   x->SetAttribute ("Shape", DoubleValue (shape));
 *
 *   double value = x->GetValue ();
 * @endcode
 *
 * @par Antithetic Values.
 *
 * If an instance of this RNG is configured to return antithetic values,
 * the actual value returned, \f$x'\f$, is generated as follows:
 *
 *   \f[
 *      x' = \frac{x_m}{{(1 - u)}^{\frac{1}{\alpha}}} ,
 *   \f]
 *
 * which now involves the distance \f$u\f$ is from 1 in the denominator.
 */
class ParetoRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a Pareto distribution RNG with the default
     * values for the mean, the shape, and upper bound.
     */
    ParetoRandomVariable();

    /**
     * @brief Returns the scale parameter for the Pareto distribution returned by this RNG stream.
     * @return The scale parameter for the Pareto distribution returned by this RNG stream.
     */
    double GetScale() const;

    /**
     * @brief Returns the shape parameter for the Pareto distribution returned by this RNG stream.
     * @return The shape parameter for the Pareto distribution returned by this RNG stream.
     */
    double GetShape() const;

    /**
     * @brief Returns the upper bound on values that can be returned by this RNG stream.
     * @return The upper bound on values that can be returned by this RNG stream.
     */
    double GetBound() const;

    /**
     * @copydoc GetValue()
     * @param [in] scale Mean parameter for the Pareto distribution.
     * @param [in] shape Shape parameter for the Pareto distribution.
     * @param [in] bound Upper bound on values returned.
     */
    double GetValue(double scale, double shape, double bound);

    /** @copydoc GetValue(double,double,double) */
    uint32_t GetInteger(uint32_t scale, uint32_t shape, uint32_t bound);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /** The scale parameter for the Pareto distribution returned by this RNG stream. */
    double m_scale;

    /** The shape parameter for the Pareto distribution returned by this RNG stream. */
    double m_shape;

    /** The upper bound on values that can be returned by this RNG stream. */
    double m_bound;

    // end of class ParetoRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The Weibull distribution Random Number Generator (RNG)
 * which allows stream numbers to be set deterministically.
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed Weibull distribution.  It also supports the generation of
 * single random numbers from various Weibull distributions.
 *
 * The probability density function is:
 *
 *   \f[
 *      P(x; \lambda, k) dx = \frac{k}{\lambda} \\
 *          \left(\frac{x}{\lambda}\right)^{k-1} \\
 *          e^{-\left(\frac{x}{\lambda}\right)^k} dx, \\
 *          \quad x \in [0, +\infty)
 *   \f]
 *
 * where \f$ k > 0\f$ is the \c Shape parameter and \f$ \lambda > 0\f$
 * is the \c Scale parameter.
 *
 * The mean \f$\mu\f$ is related to the \c Scale and \c Shape parameters
 * by the following relation:
 *
 *   \f[
 *      \mu = \lambda\Gamma\left(1+\frac{1}{k}\right)
 *   \f]
 *
 * where \f$ \Gamma \f$ is the Gamma function.
 *
 * The variance of the distribution is
 *
 *   \f[
 *      \sigma^2 = \lambda^2 \left[ \Gamma\left(1 + \frac{2}{k}\right) - \\
 *                                  \left( \Gamma\left(1 + \frac{1}{k}\right)\right)^2
 *                           \right]
 *   \f]
 *
 * For \f$ k > 1 \f$ the mean rapidly approaches just \f$\lambda\f$,
 * with variance
 *
 *   \f[
 *      \sigma^2 \approx \frac{\pi^2}{6 k^2} + \mathcal{O}(k^{-3})
 *   \f]
 *
 * The Weibull RNG value \f$x\f$ is generated by
 *
 *   \f[
 *      x = \lambda {(-\log(u))}^{\frac{1}{k}}
 *   \f]
 *
 * where \f$u\f$ is a uniform random variable on [0,1).
 *
 * @par Bounded Distribution
 *
 * Since Weibull distributions can theoretically return unbounded values,
 * it is sometimes useful to specify a fixed upper limit.    The
 * bounded version is defined over the interval \f$ x \in [0, b] \f$.
 * Note however when the upper limit is specified, the mean of the
 * resulting distribution is slightly smaller than the mean value
 * in the unbounded case.
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   double scale = 5.0;
 *   double shape = 1.0;
 *
 *   Ptr<WeibullRandomVariable> x = CreateObject<WeibullRandomVariable> ();
 *   x->SetAttribute ("Scale", DoubleValue (scale));
 *   x->SetAttribute ("Shape", DoubleValue (shape));
 *
 *   double value = x->GetValue ();
 * @endcode
 *
 * @par Antithetic Values.
 *
 * If an instance of this RNG is configured to return antithetic values,
 * the actual value returned, \f$x'\f$, is generated as follows:
 *
 *   \f[
 *      x' = \lambda {(-\log(1 - u))}^{\frac{1}{k}} ,
 *   \f]
 *
 * which now involves the log of the distance \f$u\f$ is from 1.
 */
class WeibullRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a Weibull distribution RNG with the default
     * values for the scale, shape, and upper bound.
     */
    WeibullRandomVariable();

    /**
     * @brief Returns the scale parameter for the Weibull distribution returned by this RNG stream.
     * @return The scale parameter for the Weibull distribution returned by this RNG stream.
     */
    double GetScale() const;

    /**
     * @brief Returns the shape parameter for the Weibull distribution returned by this RNG stream.
     * @return The shape parameter for the Weibull distribution returned by this RNG stream.
     */
    double GetShape() const;

    /**
     * @brief Returns the upper bound on values that can be returned by this RNG stream.
     * @return The upper bound on values that can be returned by this RNG stream.
     */
    double GetBound() const;

    /**
     * @brief Returns the mean value for the Weibull distribution returned by this RNG stream.
     * @return The mean value for the Weibull distribution returned by this RNG stream.
     */
    double GetMean() const;

    /**
     * @copydoc GetMean()
     * @param [in] scale Scale parameter for the Weibull distribution.
     * @param [in] shape Shape parameter for the Weibull distribution.
     */
    static double GetMean(double scale, double shape);

    /**
     * @copydoc GetValue()
     * @param [in] scale Scale parameter for the Weibull distribution.
     * @param [in] shape Shape parameter for the Weibull distribution.
     * @param [in] bound Upper bound on values returned.
     */
    double GetValue(double scale, double shape, double bound);

    /** @copydoc GetValue(double,double,double) */
    uint32_t GetInteger(uint32_t scale, uint32_t shape, uint32_t bound);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /** The scale parameter for the Weibull distribution returned by this RNG stream. */
    double m_scale;

    /** The shape parameter for the Weibull distribution returned by this RNG stream. */
    double m_shape;

    /** The upper bound on values that can be returned by this RNG stream. */
    double m_bound;

    // end of class WeibullRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The normal (Gaussian) distribution Random Number Generator
 * (RNG) that allows stream numbers to be set deterministically.
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed normal distribution.  It also supports the generation of
 * single random numbers from various normal distributions.
 *
 * The probability density function is:
 *
 *   \f[
 *      P(x; \mu, \sigma) dx = \frac{1}{\sqrt{2\pi\sigma^2}}
 *          e^{-\frac{(x-\mu)^2}{2\sigma^2}} dx, \\
 *          \quad x \in (-\infty, +\infty)
 *   \f]
 *
 * where the \c Mean is given by \f$\mu\f$ and the \c Variance is \f$\sigma^2\f$
 *
 * If \f$u_1\f$, \f$u_2\f$ are uniform variables over [0,1]
 * then the Gaussian RNG values, \f$x_1\f$ and \f$x_2\f$, are
 * calculated as follows:
 *
 *   \f{eqnarray*}{
 *      v_1 & = & 2 u_1 - 1     \\
 *      v_2 & = & 2 u_2 - 1     \\
 *      r^2 & = & v_1^2 + v_2^2     \\
 *      y & = & \sqrt{\frac{-2 \log(r^2)}{r^2}}     \\
 *      x_1 & = & \mu + v_1 y \sqrt{\sigma^2}     \\
 *      x_2 & = & \mu + v_2 y \sqrt{\sigma^2}  .
 *   \f}
 *
 * Note this algorithm consumes two uniform random values and produces
 * two normally distributed values.  The implementation used here
 * caches \f$y\f$ and \f$v_2\f$ to generate \f$x_2\f$ on the next call.
 *
 * @par Bounded Distribution
 *
 * Since normal distributions can theoretically return unbounded
 * values, it is sometimes useful to specify a fixed bound.  The
 * NormalRandomVariable is bounded symmetrically about the mean by
 * the \c Bound parameter, \f$b\f$, _i.e._ its values are confined to the interval
 * \f$[\mu - b, \mu + b]\f$.  This preserves the mean but decreases the variance.
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   double mean = 5.0;
 *   double variance = 2.0;
 *
 *   Ptr<NormalRandomVariable> x = CreateObject<NormalRandomVariable> ();
 *   x->SetAttribute ("Mean", DoubleValue (mean));
 *   x->SetAttribute ("Variance", DoubleValue (variance));
 *
 *   // The expected value for the mean of the values returned by a
 *   // normally distributed random variable is equal to mean.
 *   double value = x->GetValue ();
 * @endcode
 *
 * @par Antithetic Values.
 *
 * If an instance of this RNG is configured to return antithetic values,
 * the actual values returned, \f$x_1^{\prime}\f$ and \f$x_2^{\prime}\f$,
 * are calculated as follows:
 *
 *   \f{eqnarray*}{
 *      v_1^{\prime} & = & 2 (1 - u_1) - 1     \\
 *      v_2^{\prime} & = & 2 (1 - u_2) - 1     \\
 *      r^{\prime 2} & = & v_1^{\prime 2}  + v_2^{\prime 2}     \\
 *      y^{\prime} & = & \sqrt{\frac{-2 \log(r^{\prime 2})}{r^{\prime 2}}}     \\
 *      x_1^{\prime} & = & \mu + v_1^{\prime} y^{\prime} \sqrt{\sigma^2}     \\
 *      x_2^{\prime} & = & \mu + v_2^{\prime} y^{\prime} \sqrt{\sigma^2}  ,
 *   \f}
 *
 * which now involves the distances \f$u_1\f$ and \f$u_2\f$ are from 1.
 */
class NormalRandomVariable : public RandomVariableStream
{
  public:
    /** Large constant to bound the range. */
    static const double INFINITE_VALUE;

    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a normal distribution RNG with the default
     * values for the mean, variance, and bound.
     */
    NormalRandomVariable();

    /**
     * @brief Returns the mean value for the normal distribution returned by this RNG stream.
     * @return The mean value for the normal distribution returned by this RNG stream.
     */
    double GetMean() const;

    /**
     * @brief Returns the variance value for the normal distribution returned by this RNG stream.
     * @return The variance value for the normal distribution returned by this RNG stream.
     */
    double GetVariance() const;

    /**
     * @brief Returns the bound on values that can be returned by this RNG stream.
     * @return The bound on values that can be returned by this RNG stream.
     */
    double GetBound() const;

    /**
     * @copydoc GetValue()
     * @param [in] mean Mean value for the normal distribution.
     * @param [in] variance Variance value for the normal distribution.
     * @param [in] bound Bound on values returned.
     */
    double GetValue(double mean,
                    double variance,
                    double bound = NormalRandomVariable::INFINITE_VALUE);

    /** @copydoc GetValue(double,double,double) */
    uint32_t GetInteger(uint32_t mean, uint32_t variance, uint32_t bound);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /** The mean value for the normal distribution returned by this RNG stream. */
    double m_mean;

    /** The variance value for the normal distribution returned by this RNG stream. */
    double m_variance;

    /** The bound on values that can be returned by this RNG stream. */
    double m_bound;

    /** True if the next value is valid. */
    bool m_nextValid;

    /** The algorithm produces two values at a time. Cache parameters for possible reuse.*/
    double m_v2;
    /** The algorithm produces two values at a time. Cache parameters for possible reuse.*/
    double m_y;

    // end of class NormalRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The log-normal distribution Random Number Generator
 * (RNG) that allows stream numbers to be set deterministically.
 *
 * This class supports the creation of objects that return random
 * numbers from a fixed log-normal distribution.  It also supports the
 * generation of single random numbers from various log-normal
 * distributions.  If one takes the natural logarithm of a random
 * variable following the log-normal distribution, the obtained values
 * follow a normal distribution.
 *
 * The probability density function is defined using two parameters
 * \c Mu = \f$\mu\f$ and \c Sigma = \f$\sigma\f$ as:
 *
 *   \f[
 *      P(x; \mu, \sigma) dx = \frac{1}{x\sqrt{2\pi\sigma^2}}
 *          e^{-\frac{(ln(x) - \mu)^2}{2\sigma^2}} dx, \\
 *          \quad x \in [0, +\infty)
 *   \f]
 *
 * The distribution has mean value
 *
 *   \f[
 *      \langle x | P(x; \mu, \sigma) \rangle = e^{\mu+\frac{\sigma^2}{2}}
 *   \f]
 *
 * and variance
 *
 *   \f[
 *      var(x) = (e^{\sigma^2}-1)e^{2\mu+\sigma^2}
 *   \f]
 *
 * Note these are the mean and variance of the log-normal distribution,
 * not the \c Mu or \c Sigma configuration variables.
 *
 * If the desired mean and variance are known the \f$\mu\f$ and \f$\sigma\f$
 * parameters can be calculated instead with the following equations:
 *
 *   \f[
 *      \mu = ln(\langle x \rangle) - \\
 *            \frac{1}{2}ln\left(1+\frac{var(x)}{{\langle x \rangle}^2}\right)
 *   \f]
 *
 * and
 *
 *   \f[
 *      \sigma^2 = ln\left(1+\frac{var(x)}{{\langle x \rangle}^2}\right)
 *   \f]
 *
 * If \f$u_1\f$, \f$u_2\f$ are uniform variables over [0,1]
 * then the log-normal RNG value, \f$x\f$ is generated as follows:
 *
 *   \f{eqnarray*}{
 *      v_1 & = & 2 u_1 - 1     \\
 *      v_2 & = & 2 u_2 - 1     \\
 *      r^2 & = & v_1^2 + v_2^2     \\
 *      y   & = & \sqrt{\frac{-2 \log{r^2}}{r^2}}       \\
 *      x   & = &  \exp\left(\mu + v_1 y \sigma\right)  .
 *   \f}
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   double mu = 5.0;
 *   double sigma = 2.0;
 *
 *   Ptr<LogNormalRandomVariable> x = CreateObject<LogNormalRandomVariable> ();
 *   x->SetAttribute ("Mu", DoubleValue (mu));
 *   x->SetAttribute ("Sigma", DoubleValue (sigma));
 *
 *   double value = x->GetValue ();
 * @endcode
 *
 * @par Antithetic Values.
 *
 * If an instance of this RNG is configured to return antithetic values,
 * the actual value returned, \f$x'\f$, is generated as follows:
 *
 *   \f{eqnarray*}{
 *      v_1^{\prime} & = & 2 (1 - u_1) - 1     \\
 *      v_2^{\prime} & = & 2 (1 - u_2) - 1     \\
 *      r^{\prime 2} & = & v_1^{\prime 2}  + v_2^{\prime 2}     \\
 *      y^{\prime} & = & v_1^{\prime}\sqrt{\frac{-2 \log(r^{\prime 2})}{r^{\prime 2}}}     \\
 *      x^{\prime} & = & \exp\left(\mu + y^{\prime} \sigma\right)  .
 *   \f}
 *
 * which now involves the distances \f$u_1\f$ and \f$u_2\f$ are from 1.
 */
class LogNormalRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a log-normal distribution RNG with the default
     * values for mu and sigma.
     */
    LogNormalRandomVariable();

    /**
     * @brief Returns the mu value for the log-normal distribution returned by this RNG stream.
     * @return The mu value for the log-normal distribution returned by this RNG stream.
     */
    double GetMu() const;

    /**
     * @brief Returns the sigma value for the log-normal distribution returned by this RNG stream.
     * @return The sigma value for the log-normal distribution returned by this RNG stream.
     */
    double GetSigma() const;

    /**
     * @copydoc GetValue()
     * @param [in] mu Mu value for the log-normal distribution.
     * @param [in] sigma Sigma value for the log-normal distribution.
     */
    double GetValue(double mu, double sigma);

    /** @copydoc GetValue(double,double) */
    uint32_t GetInteger(uint32_t mu, uint32_t sigma);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /** The mu value for the log-normal distribution returned by this RNG stream. */
    double m_mu;

    /** The sigma value for the log-normal distribution returned by this RNG stream. */
    double m_sigma;

    /** True if m_normal is valid. */
    bool m_nextValid;

    /** The algorithm produces two values at a time. Cache parameters for possible reuse.*/
    double m_v2;

    /** The algorithm produces two values at a time. Cache parameters for possible reuse.*/
    double m_normal;

    // end of class LogNormalRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The gamma distribution Random Number Generator (RNG) that
 * allows stream numbers to be set deterministically.
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed gamma distribution.  It also supports the generation of
 * single random numbers from various gamma distributions.
 *
 * The probability distribution is defined in terms two parameters,
 * \c Alpha = \f$\alpha > 0\f$ and \c Beta = \f$ \beta > 0\f$.
 * (Note the Wikipedia entry for the
 * [Gamma Distribution](https://en.wikipedia.org/wiki/Gamma_distribution)
 * uses either the parameters \f$k, \theta\f$ or \f$\alpha, \beta\f$.
 * The parameters used here \f$(\alpha, \beta)_{\mbox{ns-3}}\f$ correspond to
 * \f$(\alpha, \frac{1}{\beta})_{\mbox{Wikipedia}}\f$.)
 *
 * The probability density function is:
 *
 *   \f[
 *      P(x; \alpha, \beta) dx = x^{\alpha-1} \\
 *          \frac{e^{-\frac{x}{\beta}}}{\beta^\alpha \Gamma(\alpha)} dx, \\
 *          \quad x \in [0, +\infty)
 *   \f]
 *
 * where the mean is \f$ \mu = \alpha\beta \f$ and the variance is
 * \f$ \sigma^2 = \alpha \beta^2\f$.
 *
 * While gamma RNG values can be generated by an algorithm similar to
 * normal RNGs, the implementation used here is based on the paper
 * G. Marsaglia and W. W. Tsang,
 * [A simple method for generating Gamma variables](https://dl.acm.org/doi/10.1145/358407.358414),
 * ACM Transactions on Mathematical Software, Vol. 26, No. 3, Sept. 2000.
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   double alpha = 5.0;
 *   double beta = 2.0;
 *
 *   Ptr<GammaRandomVariable> x = CreateObject<GammaRandomVariable> ();
 *   x->SetAttribute ("Alpha", DoubleValue (alpha));
 *   x->SetAttribute ("Beta", DoubleValue (beta));
 *
 *   double value = x->GetValue ();
 * @endcode
 *
 * @par Antithetic Values.
 *
 * If an instance of this RNG is configured to return antithetic values,
 * the actual value returned, \f$x'\f$, is generated using the prescription
 * in the Marsaglia, _et al_. paper cited above.
 */
class GammaRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a gamma distribution RNG with the default values
     * for alpha and beta.
     */
    GammaRandomVariable();

    /**
     * @brief Returns the alpha value for the gamma distribution returned by this RNG stream.
     * @return The alpha value for the gamma distribution returned by this RNG stream.
     */
    double GetAlpha() const;

    /**
     * @brief Returns the beta value for the gamma distribution returned by this RNG stream.
     * @return The beta value for the gamma distribution returned by this RNG stream.
     */
    double GetBeta() const;

    /**
     * @copydoc GetValue()
     * @param [in] alpha Alpha value for the gamma distribution.
     * @param [in] beta Beta value for the gamma distribution.
     */
    double GetValue(double alpha, double beta);

    /** @copydoc GetValue(double,double) */
    uint32_t GetInteger(uint32_t alpha, uint32_t beta);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /**
     * @brief Returns a random double from a normal distribution with the specified mean, variance,
     * and bound.
     * @param [in] mean Mean value for the normal distribution.
     * @param [in] variance Variance value for the normal distribution.
     * @param [in] bound Bound on values returned.
     * @return A floating point random value.
     */
    double GetNormalValue(double mean, double variance, double bound);

    /** The alpha value for the gamma distribution returned by this RNG stream. */
    double m_alpha;

    /** The beta value for the gamma distribution returned by this RNG stream. */
    double m_beta;

    /** True if the next normal value is valid. */
    bool m_nextValid;

    /** The algorithm produces two values at a time. Cache parameters for possible reuse.*/
    double m_v2;
    /** The algorithm produces two values at a time. Cache parameters for possible reuse.*/
    double m_y;

    // end of class GammaRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The Erlang distribution Random Number Generator (RNG) that
 * allows stream numbers to be set deterministically.
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed Erlang distribution.  It also supports the generation of
 * single random numbers from various Erlang distributions.
 *
 * The Erlang distribution is a special case of the Gamma distribution
 * where \f$k = \alpha > 0 \f$ is a positive definite integer.
 * Erlang distributed variables can be generated using a much faster
 * algorithm than Gamma variables.
 *
 * The probability distribution is defined in terms two parameters,
 * the \c K or shape parameter \f$ \in {1,2 \dots} \f$, and
 * the \c Lambda or scale parameter \f$ \in (0,1] \f$.
 * (Note the Wikipedia entry for the
 * [Erlang Distribution](https://en.wikipedia.org/wiki/Erlang_distribution)
 * uses the parameters \f$(k, \lambda)\f$ or \f$(k, \beta)\f$.
 * The parameters used here \f$(k, \lambda)_{\mbox{ns-3}}\f$ correspond to
 * \f$(k, \frac{1}{\lambda} = \beta)_{\mbox{Wikipedia}}\f$.)
 *
 * The probability density function is:
 *
 *   \f[
 *      P(x; k, \lambda) dx = \lambda^k \\
 *                            \frac{x^{k-1} e^{-\frac{x}{\lambda}}}{(k-1)!} dx, \\
 *          \quad x \in [0, +\infty)
 *   \f]
 *
 * with mean \f$ \mu = k \lambda \f$ and variance \f$ \sigma^2 = k \lambda^2 \f$.
 *
 * The Erlang RNG value \f$x\f$ is generated by
 *
 *   \f[
 *      x = - \lambda \sum_{i = 1}^{k}{\ln u_i}
 *   \f]
 *
 * where the \f$u_i\f$ are \f$k\f$ uniform random variables on [0,1).
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   uint32_t k = 5;
 *   double lambda = 2.0;
 *
 *   Ptr<ErlangRandomVariable> x = CreateObject<ErlangRandomVariable> ();
 *   x->SetAttribute ("K", IntegerValue (k));
 *   x->SetAttribute ("Lambda", DoubleValue (lambda));
 *
 *   double value = x->GetValue ();
 * @endcode
 *
 * @par Antithetic Values.
 *
 * If an instance of this RNG is configured to return antithetic values,
 * the actual value returned, \f$x'\f$, is generated as follows:
 *
 *   \f[
 *      x' = - \lambda \sum_{i = 1}^{k}{\ln (1 - u_i)}
 *   \f]
 *
 * which now involves the log of the distance \f$u\f$ is from 1.
 */
class ErlangRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates an Erlang distribution RNG with the default values
     * for k and lambda.
     */
    ErlangRandomVariable();

    /**
     * @brief Returns the k value for the Erlang distribution returned by this RNG stream.
     * @return The k value for the Erlang distribution returned by this RNG stream.
     */
    uint32_t GetK() const;

    /**
     * @brief Returns the lambda value for the Erlang distribution returned by this RNG stream.
     * @return The lambda value for the Erlang distribution returned by this RNG stream.
     */
    double GetLambda() const;

    /**
     * @copydoc GetValue()
     * @param [in] k K value for the Erlang distribution.
     * @param [in] lambda Lambda value for the Erlang distribution.
     */
    double GetValue(uint32_t k, double lambda);

    /** @copydoc GetValue(uint32_t,double) */
    uint32_t GetInteger(uint32_t k, uint32_t lambda);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /**
     * @brief Returns a random double from an exponential distribution with the specified mean and
     * upper bound.
     * @param [in] mean Mean value of the random variables.
     * @param [in] bound Upper bound on values returned.
     * @return A floating point random value.
     */
    double GetExponentialValue(double mean, double bound);

    /** The k value for the Erlang distribution returned by this RNG stream. */
    uint32_t m_k;

    /** The lambda value for the Erlang distribution returned by this RNG stream. */
    double m_lambda;

    // end of class ErlangRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The triangular distribution Random Number Generator (RNG) that
 * allows stream numbers to be set deterministically.
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed triangular distribution.  It also supports the generation of
 * single random numbers from various triangular distributions.
 *
 * The probability density depends on three parameters, the end points
 * \f$a\f$ = \c Min and \f$b\f$ = \c Max, and the location of the peak or mode,
 * \f$c\f$. For historical reasons this formulation uses the \c Mean,
 * \f$\mu = \frac{(a + b + c)}{3}\f$ instead of the mode.
 * In terms of the \c Mean, the mode is \f$c = 3 \mu - a - b\f$.
 *
 * The probability is in the shape of a triangle defined on the interval
 * \f$ x \in [a, b] \f$:
 *
 *   \f[
 *      P(x; a, b, c) dx = \begin{array}{ll}
 *          0                                  &\mbox{ for $x \le a$} \\
 *          \frac{2(x - a)}{(b - a)(c - a)} dx &\mbox{ for $a \le x \le c$} \\
 *          \frac{2}{b - 1} dx                 &\mbox{ for $x = c$} \\
 *          \frac{2(b - x)}{(b - a)(b - c)} dx &\mbox{ for $c \le x \le b$} \\
 *          0                                  &\mbox{ for $b \le x$}
 *          \end{array}
 *   \f]
 *
 * The triangle RNG \f$x\f$ is generated by
 *
 *   \f[
 *      x = \left\{ \begin{array}{rl}
 *          a + \sqrt{u (b - a) (c - a)} &\mbox{ if $u \le (c - a)/(b - a)$} \\
 *          b - \sqrt{(1 - u) (b - a) (b - c) } &\mbox{ otherwise}
 *          \end{array}
 *          \right.
 *   \f]
 *
 * where \f$u\f$ is a uniform random variable on [0,1).
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   double mean = 5.0;
 *   double min = 2.0;
 *   double max = 10.0;
 *
 *   Ptr<TriangularRandomVariable> x = CreateObject<TriangularRandomVariable> ();
 *   x->SetAttribute ("Mean", DoubleValue (mean));
 *   x->SetAttribute ("Min", DoubleValue (min));
 *   x->SetAttribute ("Max", DoubleValue (max));
 *
 *   double value = x->GetValue ();
 * @endcode
 *
 * @par Antithetic Values.
 *
 * If an instance of this RNG is configured to return antithetic values,
 * the actual value returned, \f$x'\f$, is generated as follows:
 *
 *   \f[
 *      x = \left\{ \begin{array}{rl}
 *          a + \sqrt{(1 - u) (b - a) (c - a)} &\mbox{ if $(1 - u) \le (c - a)/(b - a)$} \\
 *          b - \sqrt{u (b - a) (b - c) } &\mbox{ otherwise}
 *          \end{array}
 *          \right.
 *   \f]
 *
 * which now involves the distance \f$u\f$ is from 1.
 */
class TriangularRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a triangular distribution RNG with the default
     * values for the mean, lower bound, and upper bound.
     */
    TriangularRandomVariable();

    /**
     * @brief Returns the mean value for the triangular distribution returned by this RNG stream.
     * @return The mean value for the triangular distribution returned by this RNG stream.
     */
    double GetMean() const;

    /**
     * @brief Returns the lower bound for the triangular distribution returned by this RNG stream.
     * @return The lower bound for the triangular distribution returned by this RNG stream.
     */
    double GetMin() const;

    /**
     * @brief Returns the upper bound on values that can be returned by this RNG stream.
     * @return The upper bound on values that can be returned by this RNG stream.
     */
    double GetMax() const;

    /**
     * @copydoc GetValue()
     * @param [in] mean Mean value for the triangular distribution.
     * @param [in] min Low end of the range.
     * @param [in] max High end of the range.
     */
    double GetValue(double mean, double min, double max);

    /** @copydoc GetValue(double,double,double) */
    uint32_t GetInteger(uint32_t mean, uint32_t min, uint32_t max);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /** The mean value for the triangular distribution returned by this RNG stream. */
    double m_mean;

    /** The lower bound on values that can be returned by this RNG stream. */
    double m_min;

    /** The upper bound on values that can be returned by this RNG stream. */
    double m_max;

    // end of class TriangularRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The Zipf distribution Random Number Generator (RNG) that
 * allows stream numbers to be set deterministically.
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed Zipf distribution.  It also supports the generation of
 * single random numbers from various Zipf distributions.
 *
 * Zipf's law states that given some corpus of natural language
 * utterances, the frequency of any word is inversely proportional
 * to its rank in the frequency table.
 *
 * Zipf's distribution has two parameters, \c Alpha and \c N, where:
 * \f$ \alpha \ge 0 \f$ (real) and \f$ N \in \{1,2,3 \dots\} \f$ (integer).
 * (Note the Wikipedia entry for the
 * [Zipf Distribution](https://en.wikipedia.org/wiki/Zipf%27s_law)
 * uses the symbol \f$s\f$ instead of \f$\alpha\f$.)
 *
 * The probability mass function is:
 *
 *   \f[
 *      P(k; \alpha, N) = \frac{1}{k^\alpha H_{N,\alpha}}
 *   \f]
 *
 * where the \c N-th generalized harmonic number is
 *
 *    \f[
 *        H_{N,\alpha} = \sum_{m=1}^N \frac{1}{m^\alpha}
 *    \f]
 *
 * Note the Zipf distribution is a discrete distribution, so the
 * returned values \f$k\f$ will always be integers in the range \f$k
 * \in {1,2 \dots N} \f$.
 *
 * The mean of the distribution is
 *
 *   \f[
 *      \mu = \frac{H_{N,\alpha - 1}}{H_{N,\alpha}}
 *   \f]
 *
 * The Zipf RNG value \f$k\f$ is the smallest value such that
 *
 *   \f[
 *      u < \frac{H_k,\alpha}{H_N,\alpha}
 *   \f]
 *
 * where \f$u\f$ is a uniform random variable on [0,1).
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   uint32_t n = 1;
 *   double alpha = 2.0;
 *
 *   Ptr<ZipfRandomVariable> x = CreateObject<ZipfRandomVariable> ();
 *   x->SetAttribute ("N", IntegerValue (n));
 *   x->SetAttribute ("Alpha", DoubleValue (alpha));
 *
 *   double value = x->GetValue ();
 * @endcode
 *
 * @par Antithetic Values.
 *
 * If an instance of this RNG is configured to return antithetic values,
 * the actual value returned, \f$k'\f$, is the value such that
 *
 *   \f[
 *      1 - u < \frac{H_{k'},\alpha}{H_N,\alpha}
 *   \f]
 *
 */
class ZipfRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a Zipf distribution RNG with the default values
     * for n and alpha.
     */
    ZipfRandomVariable();

    /**
     * @brief Returns the n value for the Zipf distribution returned by this RNG stream.
     * @return The n value for the Zipf distribution returned by this RNG stream.
     */
    uint32_t GetN() const;

    /**
     * @brief Returns the alpha value for the Zipf distribution returned by this RNG stream.
     * @return The alpha value for the Zipf distribution returned by this RNG stream.
     */
    double GetAlpha() const;

    /**
     * @copydoc GetValue()
     * @param [in] n N value for the Zipf distribution.
     * @param [in] alpha Alpha value for the Zipf distribution.
     * @return A floating point random value.
     */
    double GetValue(uint32_t n, double alpha);

    /** @copydoc GetValue(uint32_t,double) */
    uint32_t GetInteger(uint32_t n, uint32_t alpha);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /** The n value for the Zipf distribution returned by this RNG stream. */
    uint32_t m_n;

    /** The alpha value for the Zipf distribution returned by this RNG stream. */
    double m_alpha;

    /** The normalization constant. */
    double m_c;

    // end of class ZipfRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The zeta distribution Random Number Generator (RNG) that
 * allows stream numbers to be set deterministically.
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed zeta distribution.  It also supports the generation of
 * single random numbers from various zeta distributions.
 *
 * The Zeta distribution is related to Zipf distribution by letting
 * \f$N \rightarrow \infty\f$.
 *
 * Zeta distribution has one parameter, \c Alpha, \f$ \alpha > 1 \f$ (real).
 * (Note the Wikipedia entry for the
 * [Zeta Distribution](https://en.wikipedia.org/wiki/Zeta_distribution)
 * uses the symbol \f$s\f$ instead of \f$\alpha\f$.)
 * The probability mass Function is
 *
 *   \f[
 *      P(k; \alpha) = k^{-\alpha}/\zeta(\alpha)
 *   \f]
 *
 * where \f$ \zeta(\alpha) \f$ is the Riemann zeta function
 *
 *   \f[
 *     \zeta(\alpha) = \sum_{n=1}^\infty \frac{1}{n^\alpha}
 *   \f]
 *
 * Note the Zeta distribution is a discrete distribution, so the
 * returned values \f$k\f$ will always be integers in the range
 * \f$k \in {1,2 \dots} \f$.
 *
 * The mean value of the distribution is
 *
 *    \f[
 *       \mu = \frac{\zeta(\alpha - 1)}{\zeta(\alpha)}, \quad \alpha > 2
 *    \f]
 *
 * The Zeta RNG \f$x\f$ is generated by an accept-reject algorithm;
 * see the implementation of GetValue(double).
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   double alpha = 2.0;
 *
 *   Ptr<ZetaRandomVariable> x = CreateObject<ZetaRandomVariable> ();
 *   x->SetAttribute ("Alpha", DoubleValue (alpha));
 *
 *   double value = x->GetValue ();
 * @endcode
 *
 * @par Antithetic Values.
 *
 * If an instance of this RNG is configured to return antithetic values,
 * the actual value returned, \f$x'\f$, is generated by using
 * \f$ 1 - u \f$ instead. of \f$u\f$ on [0, 1].
 */
class ZetaRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a zeta distribution RNG with the default value for
     * alpha.
     */
    ZetaRandomVariable();

    /**
     * @brief Returns the alpha value for the zeta distribution returned by this RNG stream.
     * @return The alpha value for the zeta distribution returned by this RNG stream.
     */
    double GetAlpha() const;

    /**
     * @copydoc GetValue()
     * @param [in] alpha Alpha value for the zeta distribution.
     */
    double GetValue(double alpha);

    /** @copydoc GetValue(double) */
    uint32_t GetInteger(uint32_t alpha);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /** The alpha value for the zeta distribution returned by this RNG stream. */
    double m_alpha;

    /** Just for calculus simplifications. */
    double m_b;

    // end of class ZetaRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The Random Number Generator (RNG) that returns a predetermined sequence.
 *
 * Defines a random variable that has a specified, predetermined
 * sequence.  This would be useful when trying to force the RNG to
 * return a known sequence, perhaps to compare ns-3 to some other
 * simulator
 *
 * Creates a generator that returns successive elements from the value
 * array on successive calls to GetValue().  Note
 * that the values in the array are copied and stored by the generator
 * (deep-copy).  Also note that the sequence repeats if more values
 * are requested than are present in the array.
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   Ptr<DeterministicRandomVariable> s = CreateObject<DeterministicRandomVariable> ();
 *
 *   std::vector array{ 4, 4, 7, 7, 10, 10};
 *   s->SetValueArray (array);
 *
 *   double value = x->GetValue ();
 * @endcode
 *
 * This will return values in the repeating sequence
 *
 *   \f[
 *      x \in 4, 4, 7, 7, 10, 10, 4, \dots
 *   \f]
 *
 * @par Antithetic Values.
 *
 * This RNG ignores the antithetic setting.
 */
class DeterministicRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a deterministic RNG that will have a predetermined
     * sequence of values.
     */
    DeterministicRandomVariable();
    ~DeterministicRandomVariable() override;

    /**
     * @brief Sets the array of values that holds the predetermined sequence.
     *
     * Note that the values in the array are copied and stored
     * (deep-copy).
     * @param [in] values Array of random values to return in sequence.
     */
    void SetValueArray(const std::vector<double>& values);
    /**
     * @brief Sets the array of values that holds the predetermined sequence.
     *
     * Note that the values in the array are copied and stored
     * (deep-copy).
     * @param [in] values Array of random values to return in sequence.
     * @param [in] length Number of values in the array.
     */
    void SetValueArray(const double* values, std::size_t length);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /** Size of the array of values. */
    std::size_t m_count;

    /** Position of the next value in the array of values. */
    std::size_t m_next;

    /** Array of values to return in sequence. */
    double* m_data;

    // end of class DeterministicRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The Random Number Generator (RNG) that has a specified
 * empirical distribution.
 *
 * Defines a random variable that has a specified, empirical
 * distribution.  The cumulative probability distribution function
 * (CDF) is specified by a series of calls to the CDF() member
 * function, specifying a value \f$x\f$ and the probability \f$P(x)\f$
 * that the distribution is less than the specified value.  When
 * random values are requested, a uniform random variable
 * \f$ u \in [0, 1] \f$ is used to select a probability,
 * and the return value is chosen as the largest input value
 * with CDF less than the random value. This method is known as
 * [inverse transform sampling](http://en.wikipedia.org/wiki/Inverse_transform_sampling).
 *
 * This generator has two modes: *sampling* and *interpolating*.
 * In *sampling* mode this random variable generator
 * treats the CDF as an exact histogram and returns
 * one of the histogram inputs exactly.  This is appropriate
 * when the configured CDF represents the exact probability
 * distribution, for a categorical variable, for example.
 *
 * In *interpolating* mode this random variable generator linearly
 * interpolates between the CDF values defining the histogram bins.
 * This is appropriate when the configured CDF is an approximation
 * to a continuous underlying probability distribution.
 *
 * For historical reasons the default is sampling.  To switch modes
 * use the \c Interpolate Attribute, or call SetInterpolate().  You
 * can change modes at any time.
 *
 * If you find yourself switching frequently it could be simpler to
 * set the mode to sampling, then use the GetValue() function for
 * sampled values, and Interpolate() function for interpolated values.
 *
 * The CDF need not start with a probability of zero, nor end with a
 * probability of 1.0.  If the selected uniform random value
 * \f$ u \in [0,1] \f$ is less than the probability of the first CDF point,
 * that point is selected. If \f$u\f$ is greater than the probability of
 * the last CDF point the last point is selected.  In either case the
 * interpolating mode will *not* interpolate (since there is no value
 * beyond the first/last to work with), but simply return the extremal CDF
 * value, as in sampling.
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *    // Create the RNG with a non-uniform distribution between 0 and 10.
 *    // in sampling mode.
 *    Ptr<EmpiricalRandomVariable> x = CreateObject<EmpiricalRandomVariable> ();
 *    x->SetInterpolate (false);
 *    x->CDF ( 0.0,  0.0);
 *    x->CDF ( 5.0,  0.25);
 *    x->CDF (10.0,  1.0);
 *
 *    double value = x->GetValue ();
 * @endcode
 *
 * The expected values and probabilities returned by GetValue() are
 *
 * Value | Probability
 * ----: | ----------:
 *  0.0  |  0
 *  5.0  | 25%
 * 10.0  | 75%
 *
 * The only two values ever returned are 5 and 10, in the ratio 1:3.
 *
 * If instead you want linear interpolation between the points of the CDF
 * use the Interpolate() function:
 *
 *     double interp = x->Interpolate ();
 *
 * This will return continuous values on the range [0,1), 25% of the time
 * less than 5, and 75% of the time between 5 and 10.
 *
 * See empirical-random-variable-example.cc for an example.
 *
 * @par Antithetic Values.
 *
 * If an instance of this RNG is configured to return antithetic values,
 * the actual value returned, \f$x'\f$, is generated by using
 * \f$ 1 - u \f$ instead. of \f$u\f$ on [0, 1].
 */
class EmpiricalRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates an empirical RNG that has a specified, empirical
     * distribution, and configured for interpolating mode.
     */
    EmpiricalRandomVariable();

    /**
     * @brief Specifies a point in the empirical distribution
     *
     * @param [in] v The function value for this point
     * @param [in] c Probability that the function is less than or equal to \p v
     *               In other words this is cumulative distribution function
     *               at \p v.
     */
    void CDF(double v, double c); // Value, prob <= Value

    // Inherited
    /**
     * @copydoc RandomVariableStream::GetValue()
     * @note This does not interpolate the CDF, but treats it as a
     * stepwise continuous function.
     */
    double GetValue() override;
    using RandomVariableStream::GetInteger;

    /**
     * @brief Returns the next value in the empirical distribution using
     * linear interpolation.
     * @return The floating point next value in the empirical distribution
     * using linear interpolation.
     */
    virtual double Interpolate();

    /**
     * @brief Switch the mode between sampling the CDF and interpolating.
     * The default mode is sampling.
     * @param [in] interpolate If \c true set to interpolation, otherwise sampling.
     * @returns The previous interpolate flag value.
     */
    bool SetInterpolate(bool interpolate);

  private:
    /**
     * @brief Check that the CDF is valid.
     *
     * A valid CDF has
     *
     * - Strictly increasing arguments, and
     * - Strictly increasing CDF.
     *
     * It is a fatal error to fail validation.
     */
    void Validate();
    /**
     * @brief Do the initial rng draw and check against the extrema.
     *
     * If the extrema apply, \c value will have the extremal value
     * and the return will be \c true.
     *
     * If the extrema do not apply \c value will have the URNG value
     * and the return will be \c false.
     *
     * @param [out] value The extremal value, or the URNG.
     * @returns \c true if \p value is the extremal result,
     *          or \c false if \p value is the URNG value.
     */
    bool PreSample(double& value);
    /**
     * @brief Sample the CDF as a histogram (without interpolation).
     * @param [in] r The CDF value at which to sample the CDF.
     * @return The bin value corresponding to \c r.
     */
    double DoSampleCDF(double r);
    /**
     * @brief Linear interpolation between two points on the CDF to estimate
     * the value at \p r.
     *
     * @param [in] r  The argument value to interpolate to.
     * @returns The interpolated CDF at \pname{r}
     */
    double DoInterpolate(double r);

    /** \c true once the CDF has been validated. */
    bool m_validated;
    /**
     * The map of CDF points (x, F(x)).
     * The CDF points are stored in the std::map in reverse order, as follows:
     * Key: CDF F(x) [0, 1] | Value: domain value (x) [-inf, inf].
     */
    std::map<double, double> m_empCdf;
    /**
     * If \c true GetValue will interpolate,
     * otherwise treat CDF as normal histogram.
     */
    bool m_interpolate;

    // end of class EmpiricalRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The binomial distribution Random Number Generator (RNG).
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed binomial distribution. It also supports the generation of
 * single random numbers from various binomial distributions.
 *
 * The probability mass function of a binomial variable
 * is defined as:
 *
 *   \f[
 *      P(k; n, p) = \binom{n}{k} p^k (1-p)^{n-k}, \\
 *          \quad k \in [0, n]
 *   \f]
 *
 * where \f$ n \f$ is the number of trials and \f$ p \f$ is the probability
 * of success in each trial. The mean of this distribution
 * is \f$ \mu = np \f$ and the variance is \f$ \sigma^2 = np(1-p) \f$.
 *
 * The Binomial RNG value \f$n\f$ for a given number of trials \f$ n \f$
 * and success probability \f$ p \f$ is generated by
 *
 *   \f[
 *    k = \sum_{i=1}^{n} I(u_i \leq p)
 *   \f]
 *
 * where \f$u_i\f$ is a uniform random variable on [0,1) for each trial,
 * and \f$I\f$ is an indicator function that is 1 if \f$u_i \leq p\f$ and 0 otherwise.
 * The sum of these indicator functions over all trials gives the total
 * number of successes, which is the value of the binomial random variable.
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   uint32_t trials = 10;
 *   double probability = 0.5;
 *
 *   Ptr<BinomialRandomVariable> x = CreateObject<BinomialRandomVariable> ();
 *   x->SetAttribute ("Trials", UintegerValue (trials));
 *   x->SetAttribute ("Probability", DoubleValue (probability));
 *
 *   double successes = x->GetValue ();
 * @endcode
 *
 * @par Antithetic Values.
 *
 * If an instance of this RNG is configured to return antithetic values,
 * the actual value returned, \f$n'\f$, for the Binomial process is determined by:
 *
 *   \f[
 *      k' = \sum_{i=1}^{n} I((1 - u_i) \leq p)
 *   \f]
 *
 * where \f$u_i\f$ is a uniform random variable on [0,1) for each trial.
 * The antithetic approach uses \f$(1 - u_i)\f$ instead of \f$u_i\f$ in the indicator function.
 *
 * @par Efficiency and Alternative Methods
 *
 * There are alternative methods for generating binomial distributions that may offer greater
 * efficiency. However, this implementation opts for a simpler approach. Although not as efficient,
 * this method was chosen for its simplicity and sufficiency in most applications.
 */
class BinomialRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    BinomialRandomVariable();

    /**
     * @copydoc GetValue()
     * @param [in] trials Number of trials.
     * @param [in] probability Probability of success in each trial.
     * @return Returns a number within the range [0, trials] indicating the number of successful
     * trials.
     */
    double GetValue(uint32_t trials, double probability);

    /**
     * @copydoc GetValue(uint32_t,double)
     * This function is similar to GetValue(), but it returns a uint32_t instead of a double.
     */
    uint32_t GetInteger(uint32_t trials, uint32_t probability);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /** The number of trials. */
    uint32_t m_trials;

    /** The probability of success in each trial. */
    double m_probability;

    // end of class BinomialRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The Bernoulli distribution Random Number Generator (RNG).
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed Bernoulli distribution. It also supports the generation of
 * single random numbers from various Bernoulli distributions.
 *
 * The probability mass function of a Bernoulli variable
 * is defined as:
 *
 *   \f[
 *      P(n; p) = p^n (1-p)^{1-n}, \\
 *          \quad n \in \{0, 1\}
 *   \f]
 *
 * where \f$ p \f$ is the probability of success and n is the indicator of success, with n=1
 * representing success and n=0 representing failure. The mean of this distribution is \f$ \mu = p
 * \f$ and the variance is \f$ \sigma^2 = p(1-p) \f$.
 *
 * The Bernoulli RNG value \f$n\f$ is generated by
 *
 *   \f[
 *    n =
 *    \begin{cases}
 *    1 & \text{if } u \leq p \\
 *    0 & \text{otherwise}
 *    \end{cases}
 *   \f]
 *
 * where \f$u\f$ is a uniform random variable on [0,1) and \f$p\f$ is the probability of success.
 *
 * @par Example
 *
 * Here is an example of how to use this class:
 * \code{.cc}
 *   double probability = 0.5;
 *
 *   Ptr<BernoulliRandomVariable> x = CreateObject<BernoulliRandomVariable> ();
 *   x->SetAttribute ("Probability", DoubleValue (probability));
 *
 *   double success = x->GetValue ();
 * @endcode
 *
 * @par Antithetic Values.
 *
 * If an instance of this RNG is configured to return antithetic values,
 * the actual value returned, \f$x'\f$, is determined by:
 *
 *   \f[
 *      x' =
 *      \begin{cases}
 *      1 & \text{if } (1 - u) \leq p \\
 *      0 & \text{otherwise}
 *      \end{cases}
 *   \f]
 *
 * where \f$u\f$ is a uniform random variable on [0,1) and \f$p\f$ is the probability of success.
 */
class BernoulliRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    BernoulliRandomVariable();

    /**
     * @copydoc GetValue()
     * @param [in] probability Probability of success.
     * @return Returns 1 if the trial is successful, or 0 if it is not.
     */
    double GetValue(double probability);

    /**
     * @copydoc GetValue(double)
     * This function is similar to GetValue(), but it returns a uint32_t instead of a double.
     */
    uint32_t GetInteger(uint32_t probability);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

  private:
    /** The probability of success. */
    double m_probability;

    // end of class BernoulliRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The laplacian distribution Random Number Generator (RNG).
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed laplacian distribution.
 *
 * The probability density function of a laplacian variable
 * is defined as:
 *
 *   \f[
 *      P(x; \mu, \beta) dx = \frac{1}{2 \beta} e^{\frac{- \abs{x - \mu}}{\beta}} dx
 *   \f]
 *
 * where \f$\mu\f$ is the \c Location configurable attribute and \f$\beta\f$
 * is the \c Scale configurable attribute.  This distribution has mean \f$\mu\f$
 * and variance \f$ \sigma^2 = 2 \beta^2 \f$.
 *
 * The laplacian RNG value \f$x\f$ is generated by
 *
 *   \f[
 *      x = \mu - \beta sgn(u) \log(1 - 2 \abs{u})
 *   \f]
 *
 * where \f$u\f$ is a uniform random variable on [-0.5,0.5).
 *
 * @par Bounded Distribution
 *
 * The Laplacian distribution can be bounded symmetrically about the mean by
 * the \c Bound parameter, \f$b\f$, _i.e._ its values are confined to the interval
 * \f$[\mu - b, \mu + b]\f$.  This preserves the mean but decreases the variance.
 */
class LaplacianRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates an unbounded laplacian distribution RNG with the default
     * values for the location and the scale.
     */
    LaplacianRandomVariable();

    /**
     * @brief Get the configured location value of this RNG.
     *
     * @return The configured location value.
     */
    double GetLocation() const;

    /**
     * @brief Get the configured scale value of this RNG.
     *
     * @return The configured scale value.
     */
    double GetScale() const;

    /**
     * @brief Get the configured bound of this RNG.
     * @return The bound.
     */
    double GetBound() const;

    /**
     * @copydoc GetValue()
     * @param [in] location location value of the laplacian distribution.
     * @param [in] scale scale value of the laplacian distribution.
     * @param [in] bound bound on values returned.
     */
    double GetValue(double location, double scale, double bound);

    /** @copydoc GetValue(double,double,double) */
    uint32_t GetInteger(uint32_t location, uint32_t scale, uint32_t bound);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

    /**
     * @brief Returns the variance value for the laplacian distribution returned by this RNG stream.
     * @return The variance value for the laplacian distribution returned by this RNG stream.
     */
    double GetVariance() const;

    /**
     * @copydoc GetVariance()
     * @param [in] scale scale value of the laplacian distribution.
     */
    static double GetVariance(double scale);

  private:
    /** The location value of the laplacian distribution. */
    double m_location;

    /** The scale value of the laplacian distribution. */
    double m_scale;

    /** The bound on values that can be returned by this RNG stream. */
    double m_bound;

    // end of class LaplacianRandomVariable
};

/**
 * @ingroup randomvariable
 * @brief The Largest Extreme Value distribution Random Number Generator (RNG).
 *
 * This class supports the creation of objects that return random numbers from a fixed Largest
 * Extreme Value distribution. This corresponds to the type-I Generalized Extreme Value
 * distribution, also known as Gumbel distribution
 * (https://en.wikipedia.org/wiki/Gumbel_distribution).
 *
 * The probability density function of a Largest Extreme Value variable
 * is defined as:
 *
 *   \f[
 *      P(x; \mu, \beta) dx = \frac{1}{\beta} e^{-(z+ e^{-z})} dx, \quad z = \frac{x - \mu}{\beta}
 *   \f]
 *
 * where \f$\mu\f$ is the \c Location configurable attribute and \f$\beta\f$
 * is the \c Scale configurable attribute.
 *
 * The Largest Extreme Value RNG value \f$x\f$ is generated by
 *
 *   \f[
 *      x = \mu - \beta \log(-\log(u))
 *   \f]
 *
 * where \f$u\f$ is a uniform random variable on [0,1).
 *
 * The mean of the distribution is:
 *
 *   \f[
 *      E = \mu + y \beta
 *   \f]
 *
 * where \f$y\f$ is the Euler-Mascheroni constant.
 *
 * The variance of the distribution is
 *
 *   \f[
 *      \sigma^2 = 6 \pi^2 \beta^2
 *   \f]
 */
class LargestExtremeValueRandomVariable : public RandomVariableStream
{
  public:
    /**
     * @brief Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates a Largest Extreme Value distribution RNG with the default
     * values for the location and the scale.
     */
    LargestExtremeValueRandomVariable();

    /**
     * @brief Get the configured location value of this RNG.
     *
     * @return The configured location value.
     */
    double GetLocation() const;

    /**
     * @brief Get the configured scale value of this RNG.
     *
     * @return The configured scale value.
     */
    double GetScale() const;

    /**
     * @copydoc GetValue()
     * @param [in] location location value of the Largest Extreme Value distribution.
     * @param [in] scale scale value of the Largest Extreme Value distribution.
     */
    double GetValue(double location, double scale);

    /** @copydoc GetValue(double,double) */
    uint32_t GetInteger(uint32_t location, uint32_t scale);

    // Inherited
    double GetValue() override;
    using RandomVariableStream::GetInteger;

    /**
     * @brief Returns the mean value for the Largest Extreme Value distribution returned by this RNG
     * stream.
     * @return The mean value for the Largest Extreme Value distribution returned by this
     * RNG stream.
     */
    double GetMean() const;

    /**
     * @copydoc GetMean()
     * @param [in] location location value of the Largest Extreme Value distribution.
     * @param [in] scale scale value of the Largest Extreme Value distribution.
     */
    static double GetMean(double location, double scale);

    /**
     * @brief Returns the variance value for the Largest Extreme Value distribution returned by this
     * RNG stream.
     * @return The variance value for the Largest Extreme Value distribution returned by
     * this RNG stream.
     */
    double GetVariance() const;

    /**
     * @copydoc GetVariance()
     * @param [in] scale scale value of the Largest Extreme Value distribution.
     */
    static double GetVariance(double scale);

  private:
    /** The location value of the Largest Extreme Value distribution. */
    double m_location;

    /** The scale value of the Largest Extreme Value distribution. */
    double m_scale;

    // end of class LargestExtremeValueRandomVariable
};

} // namespace ns3

#endif /* RANDOM_VARIABLE_STREAM_H */
