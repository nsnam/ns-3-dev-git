#include "units-energy.h"

#include "string-utils.h"
#include "units-aliases.h"

using namespace ns3;

// clang-format off
namespace ns3
{

dBm_t
dB_t::operator+(const dBm_t& rhs) const
{
    return dBm_t{val + rhs.val};
}

dBm_t
dB_t::operator-(const dBm_t& rhs) const
{
    return dBm_t{val - rhs.val};
}

bool
mWatt_t::operator==(const Watt_t& rhs) const
{
    return val == (rhs.val * ONE_KILO);
}

bool
mWatt_t::operator!=(const Watt_t& rhs) const
{
    return !(operator==(rhs));
}

bool
mWatt_t::operator<(const Watt_t& rhs) const
{
    return val < (rhs.val * ONE_KILO);
}

bool
mWatt_t::operator>(const Watt_t& rhs) const
{
    return val > (rhs.val * ONE_KILO);
}

bool
mWatt_t::operator<=(const Watt_t& rhs) const
{
    return val <= (rhs.val * ONE_KILO);
}

bool
mWatt_t::operator>=(const Watt_t& rhs) const
{
    return val >= (rhs.val * ONE_KILO);
}

mWatt_t
mWatt_t::operator+(const Watt_t& rhs) const
{
    return mWatt_t{val + (rhs.val * ONE_KILO)};
}

mWatt_t
mWatt_t::operator-(const Watt_t& rhs) const
{
    return mWatt_t{val - (rhs.val * ONE_KILO)};
}

mWatt_t&
mWatt_t::operator+=(const Watt_t& rhs)
{
    val += (rhs.val * ONE_KILO);
    return *this;
}

mWatt_t&
mWatt_t::operator-=(const Watt_t& rhs)
{
    val -= (rhs.val * ONE_KILO);
    return *this;
}

mWatt_t
operator*(const double& lfs, const mWatt_t& rhs)
{
    return mWatt_t{lfs * rhs.val};
}

bool
Watt_t::operator==(const mWatt_t& rhs) const
{
    return val == (rhs.val / ONE_KILO);
}

bool
Watt_t::operator!=(const mWatt_t& rhs) const
{
    return !(operator==(rhs));
}

bool
Watt_t::operator<(const mWatt_t& rhs) const
{
    return val < (rhs.val / ONE_KILO);
}

bool
Watt_t::operator>(const mWatt_t& rhs) const
{
    return val > (rhs.val / ONE_KILO);
}

bool
Watt_t::operator<=(const mWatt_t& rhs) const
{
    return val <= (rhs.val / ONE_KILO);
}

bool
Watt_t::operator>=(const mWatt_t& rhs) const
{
    return val >= (rhs.val / ONE_KILO);
}

Watt_t
Watt_t::operator+(const mWatt_t& rhs) const
{
    return Watt_t{((val * ONE_KILO) + rhs.val) / ONE_KILO};
}

Watt_t
Watt_t::operator-(const mWatt_t& rhs) const
{
    return Watt_t{((val * ONE_KILO) - rhs.val) / ONE_KILO};
}

Watt_t&
Watt_t::operator+=(const mWatt_t& rhs)
{
    val += (rhs.val / ONE_KILO);
    return *this;
}

Watt_t&
Watt_t::operator-=(const mWatt_t& rhs)
{
    val -= (rhs.val / ONE_KILO);
    return *this;
}

dBm_t
dBm_t::from_mWatt(const mWatt_t& input)
{
    return dBm_t{ToLogScale(input.val)};
}

mWatt_t
dBm_t::to_mWatt() const
{
    return mWatt_t{ToLinearScale(val)};
}

double
dBm_t::in_mWatt() const
{
    return to_mWatt().val;
}

dBm_t
dBm_t::from_Watt(const Watt_t& input)
{
    return dBm_t{ToLogScale(input.val) + 30.0}; // 30 == 10log10(1000)
}

Watt_t
dBm_t::to_Watt() const
{
    return Watt_t{ToLinearScale(val - 30.0)}; // -30 == 10log10(1/1000)
}

double
dBm_t::in_Watt() const
{
    return to_Watt().val;
}

double
dBm_t::in_dBm() const
{
    return val;
}

mWatt_t
mWatt_t::from_dBm(const dBm_t& from)
{
    return mWatt_t{ToLinearScale(from.val)};
}

dBm_t
mWatt_t::to_dBm() const
{
    return dBm_t{ToLogScale(val)};
}

double
mWatt_t::in_dBm() const
{
    return to_dBm().val;
}

mWatt_t
mWatt_t::from_Watt(const Watt_t& from)
{
    return mWatt_t{from.val * ONE_KILO};
}

Watt_t
mWatt_t::to_Watt() const
{
    return Watt_t{val / ONE_KILO};
}

double
mWatt_t::in_Watt() const
{
    return to_Watt().val;
}

double
mWatt_t::in_mWatt() const
{
    return val;
}

Watt_t
Watt_t::from_dBm(const dBm_t& from)
{
    return Watt_t{ToLinearScale(from.val - 30.0)}; // -30 == 10log10(1/1000)
}

dBm_t
Watt_t::to_dBm() const
{
    return dBm_t{ToLogScale(val) + 30.0}; // 30 == 10log10(1000)
}

double
Watt_t::in_dBm() const
{
    return to_dBm().val;
}

Watt_t
Watt_t::from_mWatt(const mWatt_t& from)
{
    return Watt_t{(from.val / ONE_KILO)};
}

mWatt_t
Watt_t::to_mWatt() const
{
    return mWatt_t{val * ONE_KILO};
}

double
Watt_t::in_mWatt() const
{
    return to_mWatt().val;
}

double
Watt_t::in_Watt() const
{
    return val;
}

double
dBm_per_Hz_t::in_dBm() const
{
    return val;
}

double
dBm_per_MHz_t::in_dBm() const
{
    return val;
}

/// User defined literals for dB
/// @param val Value in dB
/// @return dB_t
dB_t
operator""_dB(long double val)
{
    return dB_t{static_cast<double>(val)};
}

/// User defined literals for dB
/// @param val Value in dB
/// @return dB_t
dB_t
operator""_dB(unsigned long long val)
{
    return dB_t{static_cast<double>(val)};
}

/// User defined literals for dBr
/// @param val Value in dBr
/// @return dBr_t
dBr_t operator""_dBr(long double val)
{
    return dBr_t{static_cast<double>(val)};
}

/// User defined literals for dBr
/// @param val Value in dBr
/// @return dBr_t
dBr_t operator""_dBr(unsigned long long val)
{
    return dBr_t{static_cast<double>(val)};
}

/// User defined literals for dBm
/// @param val Value in dBm
/// @return dBm_t
dBm_t
operator""_dBm(long double val)
{
    return dBm_t{static_cast<double>(val)};
}

/// User defined literals for dBm
/// @param val Value in dBm
/// @return dBm_t
dBm_t
operator""_dBm(unsigned long long val)
{
    return dBm_t{static_cast<double>(val)};
}

/// User defined literals for mWatt
/// @param val Value in mWatt
/// @return mWatt_t
mWatt_t
operator""_mWatt(long double val)
{
    return mWatt_t{static_cast<double>(val)};
}

/// User defined literals for mWatt
/// @param val Value in mWatt
/// @return mWatt_t
mWatt_t
operator""_mWatt(unsigned long long val)
{
    return mWatt_t{static_cast<double>(val)};
}

/// User defined literals for pWatt
/// @param val Value in pWatt
/// @return mWatt_t
mWatt_t
operator""_pWatt(long double val)
{
    return mWatt_t{static_cast<double>(val * 1e-9)}; // NOLINT
}

/// User defined literals for pWatt
/// @param val Value in pWatt
/// @return mWatt_t
mWatt_t
operator""_pWatt(unsigned long long val)
{
    return mWatt_t{val * 1e-9}; // NOLINT
}

/// User defined literals for Watt
/// @param val Value in Watt
/// @return mWatt_t
Watt_t
operator""_Watt(long double val)
{
    return Watt_t{static_cast<double>(val)}; // NOLINT
}

/// User defined literals for Watt
/// @param val Value in Watt
/// @return mWatt_t
Watt_t
operator""_Watt(unsigned long long val)
{
    return Watt_t{static_cast<double>(val)}; // NOLINT
}

/// User defined literals for dBm_per_Hz
/// @param val Value in dBm_per_Hz
/// @return dBm_per_Hz_t
dBm_per_Hz_t
operator""_dBm_per_Hz(long double val)
{
    return dBm_per_Hz_t{static_cast<double>(val)};
}

/// User defined literals for dBm_per_Hz
/// @param val Value in dBm_per_Hz
/// @return dBm_per_Hz_t
dBm_per_Hz_t
operator""_dBm_per_Hz(unsigned long long val)
{
    return dBm_per_Hz_t{static_cast<double>(val)};
}

/// User defined literals for dBm_per_MHz
/// @param val Value in dBm_per_MHz
/// @return dBm_per_MHz_t
dBm_per_MHz_t operator""_dBm_per_MHz(long double val)
{
    return dBm_per_MHz_t{static_cast<double>(val)};
}

// Output-input operators overloading

/// Output stream for dB
/// @param os Output stream
/// @param rhs dB value
/// @return Output stream
std::ostream&
operator<<(std::ostream& os, const dB_t& rhs)
{
    return os << rhs.str();
}

/// Output stream for dBm
/// @param os Output stream
/// @param rhs dBm value
/// @return Output stream
std::ostream&
operator<<(std::ostream& os, const dBm_t& rhs)
{
    return os << rhs.str();
}

/// Output stream for mWatt
/// @param os Output stream
/// @param rhs mWatt value
/// @return Output stream
std::ostream&
operator<<(std::ostream& os, const mWatt_t& rhs)
{
    return os << rhs.str();
}

/// Output stream for Watt
/// @param os Output stream
/// @param rhs Watt value
/// @return Output stream
std::ostream&
operator<<(std::ostream& os, const Watt_t& rhs)
{
    return os << rhs.str();
}

/// Output stream for dBm_per_Hz
/// @param os Output stream
/// @param rhs dBm_per_Hz value
/// @return Output stream
std::ostream&
operator<<(std::ostream& os, const dBm_per_Hz_t& rhs)
{
    return os << rhs.str();
}

/// Output stream for dBm_per_MHz
/// @param os Output stream
/// @param rhs dBm_per_MHz value
/// @return Output stream
std::ostream&
operator<<(std::ostream& os, const dBm_per_MHz_t& rhs)
{
    return os << rhs.str();
}

/// Input stream for dB
/// @param is Input stream
/// @param rhs dB value
/// @return Input stream
std::istream&
operator>>(std::istream& is, dB_t& rhs)
{
    is >> rhs.val;
    return is;
}

/// Input stream for dBm
/// @param is Input stream
/// @param rhs dBm value
/// @return Input stream
std::istream&
operator>>(std::istream& is, dBm_t& rhs)
{
    is >> rhs.val;
    return is;
}

/// Input stream for mWatt
/// @param is Input stream
/// @param rhs mWatt value
/// @return Input stream
std::istream&
operator>>(std::istream& is, mWatt_t& rhs)
{
    is >> rhs.val;
    return is;
}

/// Input stream for Watt
/// @param is Input stream
/// @param rhs Watt value
/// @return Input stream
std::istream&
operator>>(std::istream& is, Watt_t& rhs)
{
    is >> rhs.val;
    return is;
}

/// Input stream for dBm_per_Hz
/// @param is Input stream
/// @param rhs dBm_per_Hz value
/// @return Input stream
std::istream&
operator>>(std::istream& is, dBm_per_Hz_t& rhs)
{
    is >> rhs.val;
    return is;
}

/// Input stream for dBm_per_MHz
/// @param is Input stream
/// @param rhs dBm_per_MHz value
/// @return Input stream
std::istream&
operator>>(std::istream& is, dBm_per_MHz_t& rhs)
{
    is >> rhs.val;
    return is;
}

/// @cond Doxygen bug: multiple @param
/// Convert string to dB
/// @param input String to convert
/// @return Optional dB value
std::optional<dB_t>
dB_t::from_str(const std::string& input)
{
    auto res = StringToDouble(input, "dB");
    return res.has_value() ? std::optional(dB_t{res.value()}) : std::nullopt;
}
/// @endcond

/// Convert string to dBm
/// @param input String to convert
/// @return Optional dBm value
std::optional<dBm_t>
dBm_t::from_str(const std::string& input)
{
    auto res = StringToDouble(input, "dBm");
    return res.has_value() ? std::optional(dBm_t{res.value()}) : std::nullopt;
}

/// Convert string to mWatt
/// @param input String to convert
/// @return Optional mWatt value
std::optional<mWatt_t>
mWatt_t::from_str(const std::string& input)
{
    auto res = StringToDouble(input, "mWatt");
    return res.has_value() ? std::optional(mWatt_t{res.value()}) : std::nullopt;
}

/// Convert string to Watt
/// @param input String to convert
/// @return Optional Watt value
std::optional<Watt_t>
Watt_t::from_str(const std::string& input)
{
    auto res = StringToDouble(input, "Watt");
    return res.has_value() ? std::optional(Watt_t{res.value()}) : std::nullopt;
}

/// @cond Doxygen bug: multiple @param
/// Convert string to dBm_per_Hz
/// @param input String to convert
/// @return Optional dBm_per_Hz value
std::optional<dBm_per_Hz_t>
dBm_per_Hz_t::from_str(const std::string& input)
{
    auto res = StringToDouble(input, "dBm_per_Hz");
    return res.has_value() ? std::optional(dBm_per_Hz_t{res.value()}) : std::nullopt;
}

/// Convert string to dBm_per_MHz
/// @param input String to convert
/// @return Optional dBm_per_MHz value
std::optional<dBm_per_MHz_t>
dBm_per_MHz_t::from_str(const std::string& input)
{
    auto res = StringToDouble(input, "dBm_per_MHz");
    return res.has_value() ? std::optional(dBm_per_MHz_t{res.value()}) : std::nullopt;
}
/// @endcond

/// @cond Doxygen warning about macro internals
ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(dB_t, dB);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(dB_t, dB);

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(dBr_t, dBr);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(dBr_t, dBr);

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(dBm_t, dBm);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(dBm_t, dBm);

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(mWatt_t, mWatt);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(mWatt_t, mWatt);

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(Watt_t, Watt);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(Watt_t, Watt);

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(dBm_per_Hz_t, dBm_per_Hz);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(dBm_per_Hz_t, dBm_per_Hz);

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(dBm_per_MHz_t, dBm_per_MHz);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(dBm_per_MHz_t, dBm_per_MHz);
/// @endcond
} // namespace ns3

// clang-format on
