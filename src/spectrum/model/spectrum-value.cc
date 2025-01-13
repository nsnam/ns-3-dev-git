
/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "spectrum-value.h"

#include "ns3/log.h"
#include "ns3/math.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SpectrumValue");

SpectrumValue::SpectrumValue()
{
}

SpectrumValue::SpectrumValue(Ptr<const SpectrumModel> sof)
    : m_spectrumModel(sof),
      m_values(sof->GetNumBands())
{
}

double&
SpectrumValue::operator[](size_t index)
{
    return m_values.at(index);
}

const double&
SpectrumValue::operator[](size_t index) const
{
    return m_values.at(index);
}

SpectrumModelUid_t
SpectrumValue::GetSpectrumModelUid() const
{
    return m_spectrumModel->GetUid();
}

Ptr<const SpectrumModel>
SpectrumValue::GetSpectrumModel() const
{
    return m_spectrumModel;
}

Values::const_iterator
SpectrumValue::ConstValuesBegin() const
{
    return m_values.begin();
}

Values::const_iterator
SpectrumValue::ConstValuesEnd() const
{
    return m_values.end();
}

Values::iterator
SpectrumValue::ValuesBegin()
{
    return m_values.begin();
}

Values::iterator
SpectrumValue::ValuesEnd()
{
    return m_values.end();
}

Bands::const_iterator
SpectrumValue::ConstBandsBegin() const
{
    return m_spectrumModel->Begin();
}

Bands::const_iterator
SpectrumValue::ConstBandsEnd() const
{
    return m_spectrumModel->End();
}

void
SpectrumValue::Add(const SpectrumValue& x)
{
    auto it1 = m_values.begin();
    auto it2 = x.m_values.begin();

    NS_ASSERT(m_spectrumModel == x.m_spectrumModel);
    NS_ASSERT(m_values.size() == x.m_values.size());

    while (it1 != m_values.end())
    {
        *it1 += *it2;
        ++it1;
        ++it2;
    }
}

void
SpectrumValue::Add(double s)
{
    auto it1 = m_values.begin();

    while (it1 != m_values.end())
    {
        *it1 += s;
        ++it1;
    }
}

void
SpectrumValue::Subtract(const SpectrumValue& x)
{
    auto it1 = m_values.begin();
    auto it2 = x.m_values.begin();

    NS_ASSERT(m_spectrumModel == x.m_spectrumModel);
    NS_ASSERT(m_values.size() == x.m_values.size());

    while (it1 != m_values.end())
    {
        *it1 -= *it2;
        ++it1;
        ++it2;
    }
}

void
SpectrumValue::Subtract(double s)
{
    Add(-s);
}

void
SpectrumValue::Multiply(const SpectrumValue& x)
{
    auto it1 = m_values.begin();
    auto it2 = x.m_values.begin();

    NS_ASSERT(m_spectrumModel == x.m_spectrumModel);
    NS_ASSERT(m_values.size() == x.m_values.size());

    while (it1 != m_values.end())
    {
        *it1 *= *it2;
        ++it1;
        ++it2;
    }
}

void
SpectrumValue::Multiply(double s)
{
    auto it1 = m_values.begin();

    while (it1 != m_values.end())
    {
        *it1 *= s;
        ++it1;
    }
}

void
SpectrumValue::Divide(const SpectrumValue& x)
{
    auto it1 = m_values.begin();
    auto it2 = x.m_values.begin();

    NS_ASSERT(m_spectrumModel == x.m_spectrumModel);
    NS_ASSERT(m_values.size() == x.m_values.size());

    while (it1 != m_values.end())
    {
        *it1 /= *it2;
        ++it1;
        ++it2;
    }
}

void
SpectrumValue::Divide(double s)
{
    NS_LOG_FUNCTION(this << s);
    auto it1 = m_values.begin();

    while (it1 != m_values.end())
    {
        *it1 /= s;
        ++it1;
    }
}

void
SpectrumValue::ChangeSign()
{
    auto it1 = m_values.begin();

    while (it1 != m_values.end())
    {
        *it1 = -(*it1);
        ++it1;
    }
}

void
SpectrumValue::ShiftLeft(int n)
{
    int i = 0;
    while (i < (int)m_values.size() - n)
    {
        m_values.at(i) = m_values.at(i + n);
        i++;
    }
    while (i < (int)m_values.size())
    {
        m_values.at(i) = 0;
        i++;
    }
}

void
SpectrumValue::ShiftRight(int n)
{
    int i = m_values.size() - 1;
    while (i - n >= 0)
    {
        m_values.at(i) = m_values.at(i - n);
        i = i - 1;
    }
    while (i >= 0)
    {
        m_values.at(i) = 0;
        --i;
    }
}

void
SpectrumValue::Pow(double exp)
{
    NS_LOG_FUNCTION(this << exp);
    auto it1 = m_values.begin();

    while (it1 != m_values.end())
    {
        *it1 = std::pow(*it1, exp);
        ++it1;
    }
}

void
SpectrumValue::Exp(double base)
{
    NS_LOG_FUNCTION(this << base);
    auto it1 = m_values.begin();

    while (it1 != m_values.end())
    {
        *it1 = std::pow(base, *it1);
        ++it1;
    }
}

void
SpectrumValue::Log10()
{
    NS_LOG_FUNCTION(this);
    auto it1 = m_values.begin();

    while (it1 != m_values.end())
    {
        *it1 = std::log10(*it1);
        ++it1;
    }
}

void
SpectrumValue::Log2()
{
    NS_LOG_FUNCTION(this);
    auto it1 = m_values.begin();

    while (it1 != m_values.end())
    {
        *it1 = log2(*it1);
        ++it1;
    }
}

void
SpectrumValue::Log()
{
    NS_LOG_FUNCTION(this);
    auto it1 = m_values.begin();

    while (it1 != m_values.end())
    {
        *it1 = std::log(*it1);
        ++it1;
    }
}

double
Norm(const SpectrumValue& x)
{
    double s = 0;
    auto it1 = x.ConstValuesBegin();
    while (it1 != x.ConstValuesEnd())
    {
        s += (*it1) * (*it1);
        ++it1;
    }
    return std::sqrt(s);
}

double
Sum(const SpectrumValue& x)
{
    double s = 0;
    auto it1 = x.ConstValuesBegin();
    while (it1 != x.ConstValuesEnd())
    {
        s += (*it1);
        ++it1;
    }
    return s;
}

double
Prod(const SpectrumValue& x)
{
    double s = 0;
    auto it1 = x.ConstValuesBegin();
    while (it1 != x.ConstValuesEnd())
    {
        s *= (*it1);
        ++it1;
    }
    return s;
}

double
Integral(const SpectrumValue& arg)
{
    double i = 0;
    auto vit = arg.ConstValuesBegin();
    auto bit = arg.ConstBandsBegin();
    while (vit != arg.ConstValuesEnd())
    {
        NS_ASSERT(bit != arg.ConstBandsEnd());
        i += (*vit) * (bit->fh - bit->fl);
        ++vit;
        ++bit;
    }
    NS_ASSERT(bit == arg.ConstBandsEnd());
    return i;
}

Ptr<SpectrumValue>
SpectrumValue::Copy() const
{
    Ptr<SpectrumValue> p = Create<SpectrumValue>(m_spectrumModel);
    *p = *this;
    return p;

    //  return Copy<SpectrumValue> (*this)
}

/**
 * @brief Output stream operator
 * @param os output stream
 * @param pvf the SpectrumValue to print
 * @return an output stream
 */
std::ostream&
operator<<(std::ostream& os, const SpectrumValue& pvf)
{
    auto it1 = pvf.ConstValuesBegin();
    bool first = true;
    while (it1 != pvf.ConstValuesEnd())
    {
        if (!first)
        {
            os << " ";
        }
        else
        {
            first = false;
        }
        os << *it1;
        ++it1;
    }
    return os;
}

SpectrumValue
operator+(const SpectrumValue& lhs, const SpectrumValue& rhs)
{
    SpectrumValue res = lhs;
    res.Add(rhs);
    return res;
}

bool
operator==(const SpectrumValue& lhs, const SpectrumValue& rhs)
{
    return (lhs.m_values == rhs.m_values);
}

bool
operator!=(const SpectrumValue& lhs, const SpectrumValue& rhs)
{
    return (lhs.m_values != rhs.m_values);
}

SpectrumValue
operator+(const SpectrumValue& lhs, double rhs)
{
    SpectrumValue res = lhs;
    res.Add(rhs);
    return res;
}

SpectrumValue
operator+(double lhs, const SpectrumValue& rhs)
{
    SpectrumValue res = rhs;
    res.Add(lhs);
    return res;
}

SpectrumValue
operator-(const SpectrumValue& lhs, const SpectrumValue& rhs)
{
    SpectrumValue res = rhs;
    res.ChangeSign();
    res.Add(lhs);
    return res;
}

SpectrumValue
operator-(const SpectrumValue& lhs, double rhs)
{
    SpectrumValue res = lhs;
    res.Subtract(rhs);
    return res;
}

SpectrumValue
operator-(double lhs, const SpectrumValue& rhs)
{
    SpectrumValue res = rhs;
    res.Subtract(lhs);
    return res;
}

SpectrumValue
operator*(const SpectrumValue& lhs, const SpectrumValue& rhs)
{
    SpectrumValue res = lhs;
    res.Multiply(rhs);
    return res;
}

SpectrumValue
operator*(const SpectrumValue& lhs, double rhs)
{
    SpectrumValue res = lhs;
    res.Multiply(rhs);
    return res;
}

SpectrumValue
operator*(double lhs, const SpectrumValue& rhs)
{
    SpectrumValue res = rhs;
    res.Multiply(lhs);
    return res;
}

SpectrumValue
operator/(const SpectrumValue& lhs, const SpectrumValue& rhs)
{
    SpectrumValue res = lhs;
    res.Divide(rhs);
    return res;
}

SpectrumValue
operator/(const SpectrumValue& lhs, double rhs)
{
    SpectrumValue res = lhs;
    res.Divide(rhs);
    return res;
}

SpectrumValue
operator/(double lhs, const SpectrumValue& rhs)
{
    SpectrumValue res = rhs;
    res.Divide(lhs);
    return res;
}

SpectrumValue
operator+(const SpectrumValue& rhs)
{
    return rhs;
}

SpectrumValue
operator-(const SpectrumValue& rhs)
{
    SpectrumValue res = rhs;
    res.ChangeSign();
    return res;
}

SpectrumValue
Pow(double lhs, const SpectrumValue& rhs)
{
    SpectrumValue res = rhs;
    res.Exp(lhs);
    return res;
}

SpectrumValue
Pow(const SpectrumValue& lhs, double rhs)
{
    SpectrumValue res = lhs;
    res.Pow(rhs);
    return res;
}

SpectrumValue
Log10(const SpectrumValue& arg)
{
    SpectrumValue res = arg;
    res.Log10();
    return res;
}

SpectrumValue
Log2(const SpectrumValue& arg)
{
    SpectrumValue res = arg;
    res.Log2();
    return res;
}

SpectrumValue
Log(const SpectrumValue& arg)
{
    SpectrumValue res = arg;
    res.Log();
    return res;
}

SpectrumValue&
SpectrumValue::operator+=(const SpectrumValue& rhs)
{
    Add(rhs);
    return *this;
}

SpectrumValue&
SpectrumValue::operator-=(const SpectrumValue& rhs)
{
    Subtract(rhs);
    return *this;
}

SpectrumValue&
SpectrumValue::operator*=(const SpectrumValue& rhs)
{
    Multiply(rhs);
    return *this;
}

SpectrumValue&
SpectrumValue::operator/=(const SpectrumValue& rhs)
{
    Divide(rhs);
    return *this;
}

SpectrumValue&
SpectrumValue::operator+=(double rhs)
{
    Add(rhs);
    return *this;
}

SpectrumValue&
SpectrumValue::operator-=(double rhs)
{
    Subtract(rhs);
    return *this;
}

SpectrumValue&
SpectrumValue::operator*=(double rhs)
{
    Multiply(rhs);
    return *this;
}

SpectrumValue&
SpectrumValue::operator/=(double rhs)
{
    Divide(rhs);
    return *this;
}

SpectrumValue&
SpectrumValue::operator=(double rhs)
{
    auto it1 = m_values.begin();

    while (it1 != m_values.end())
    {
        *it1 = rhs;
        ++it1;
    }
    return *this;
}

SpectrumValue
SpectrumValue::operator<<(int n) const
{
    SpectrumValue res = *this;
    res.ShiftLeft(n);
    return res;
}

SpectrumValue
SpectrumValue::operator>>(int n) const
{
    SpectrumValue res = *this;
    res.ShiftRight(n);
    return res;
}

uint32_t
SpectrumValue::GetValuesN() const
{
    return m_values.size();
}

const double&
SpectrumValue::ValuesAt(uint32_t pos) const
{
    return m_values.at(pos);
}

} // namespace ns3
