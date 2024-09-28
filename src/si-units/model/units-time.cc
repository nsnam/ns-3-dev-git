#include "units-time.h"

// clang-format off
namespace ns3
{

/// User defined literals for nanoseconds
/// @param v value in nanoseconds
/// @return nSEC_t object
nSEC_t
operator""_nSEC(unsigned long long v)
{
    return nSEC_t{static_cast<long long>(v)};
}

/// User defined literals for nanoseconds
/// @param v value in nanoseconds
/// @return nSEC_t object
nSEC_t
operator""_nSEC(long double v)
{
    return nSEC_t{static_cast<long long>(v)};
}

/// User defined literals for microseconds
/// @param v value in microseconds
/// @return nSEC_t object
nSEC_t
operator""_uSEC(unsigned long long v)
{
    return nSEC_t{static_cast<long long>(v * ONE_KILO)};
}

/// User defined literals for microseconds
/// @param v value in microseconds
/// @return nSEC_t object
nSEC_t
operator""_uSEC(long double v)
{
    return nSEC_t{static_cast<long long>(v * ONE_KILO)};
}

/// User defined literals for milliseconds
/// @param v value in milliseconds
/// @return nSEC_t object
nSEC_t
operator""_mSEC(unsigned long long v)
{
    return nSEC_t{static_cast<long long>(v * ONE_MEGA)};
}

/// User defined literals for milliseconds
/// @param v value in milliseconds
/// @return nSEC_t object
nSEC_t
operator""_mSEC(long double v)
{
    return nSEC_t{static_cast<long long>(v * ONE_MEGA)};
}

/// User defined literals for seconds
/// @param v value in seconds
/// @return nSEC_t object
nSEC_t
operator""_SEC(unsigned long long v)
{
    return nSEC_t{static_cast<long long>(v * ONE_GIGA)};
}

/// User defined literals for seconds
/// @param v value in seconds
/// @return nSEC_t object
nSEC_t
operator""_SEC(long double v)
{
    return nSEC_t{static_cast<long long>(v * ONE_GIGA)};
}

/// Output stream operator for nSEC_t
/// @param os output stream
/// @param q nSEC_t object
/// @return output stream
std::ostream&
operator<<(std::ostream& os, const nSEC_t& q)
{
    return os << q.str();
}

/// Input stream operator for nSEC_t
/// @param is input stream
/// @param q nSEC_t object
/// @return input stream
std::istream&
operator>>(std::istream& is, nSEC_t& q)
{
    is >> q.v;
    return is;
}

/// @cond Doxygen warning against macro internalsk
ATTRIBUTE_CHECKER_IMPLEMENT_WITH_NAME(nSEC, "nSEC_t");
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(nSEC_t, nSEC);
/// @endcond

} // namespace ns3

// clang-format on
