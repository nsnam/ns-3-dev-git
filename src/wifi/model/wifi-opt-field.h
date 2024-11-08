/*
 * Copyright (c) 2024 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_OPT_FIELD_H
#define WIFI_OPT_FIELD_H

#include <functional>
#include <optional>

namespace ns3
{

/**
 * @ingroup wifi
 *
 * OptFieldWithPresenceInd is a class modeling an optional field (in an Information Element, a
 * management frame, etc.) having an associated Presence Indicator bit. This class is a wrapper
 * around std::optional (most of its functions are exposed, more can be added if needed) that
 * additionally sets the Presence Indicator flag appropriately when operations like reset or
 * assignment of a value are performed on the optional field.
 */
template <typename T>
class OptFieldWithPresenceInd
{
  public:
    /// @brief constructor
    /// @param presenceFlag the Presence Indicator flag
    OptFieldWithPresenceInd(bool& presenceFlag);

    /// @brief Destroy the value (if any) contained in the optional field.
    /// @return a reference to this object
    constexpr OptFieldWithPresenceInd& operator=(std::nullopt_t);

    /// @brief Assign the given value to the optional field
    /// @param other the given value
    /// @return a reference to this object
    constexpr OptFieldWithPresenceInd& operator=(const std::optional<T>& other);

    /// @brief Assign the given value to the optional field
    /// @param other the given value
    /// @return a reference to this object
    constexpr OptFieldWithPresenceInd& operator=(std::optional<T>&& other);

    /// @brief Operator bool
    /// @return whether this object contains a value
    constexpr explicit operator bool() const;

    /// @brief Check whether this object contains a value
    /// @return whether this object contains a value
    constexpr bool has_value() const;

    /// @return a pointer to the contained value
    constexpr const T* operator->() const;

    /// @return a pointer to the contained value
    constexpr T* operator->();

    /// @return a reference to the contained value
    constexpr const T& operator*() const;

    /// @return a reference to the contained value
    constexpr T& operator*();

    /// @brief Construct the contained value in-place
    /// @tparam Args the type of arguments to pass to the constructor
    /// @param args the arguments to pass to the constructor
    /// @return a reference to the new contained value
    template <class... Args>
    constexpr T& emplace(Args&&... args);

    /// @brief Destroy the value (if any) contained in the optional field
    constexpr void reset();

  private:
    std::optional<T> m_field;                    ///< the optional field
    std::reference_wrapper<bool> m_presenceFlag; ///< the Presence Indicator flag
};

} // namespace ns3

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

namespace ns3
{

template <typename T>
OptFieldWithPresenceInd<T>::OptFieldWithPresenceInd(bool& presenceFlag)
    : m_presenceFlag(presenceFlag)
{
    m_presenceFlag.get() = false;
}

template <typename T>
constexpr OptFieldWithPresenceInd<T>&
OptFieldWithPresenceInd<T>::operator=(std::nullopt_t)
{
    m_field.reset();
    m_presenceFlag.get() = false;
    return *this;
}

template <typename T>
constexpr OptFieldWithPresenceInd<T>&
OptFieldWithPresenceInd<T>::operator=(const std::optional<T>& other)
{
    m_field = other;
    m_presenceFlag.get() = other.has_value();
    return *this;
}

template <typename T>
constexpr OptFieldWithPresenceInd<T>&
OptFieldWithPresenceInd<T>::operator=(std::optional<T>&& other)
{
    m_field = std::move(other);
    m_presenceFlag.get() = other.has_value();
    return *this;
}

template <typename T>
constexpr OptFieldWithPresenceInd<T>::operator bool() const
{
    return m_field.has_value();
}

template <typename T>
constexpr bool
OptFieldWithPresenceInd<T>::has_value() const
{
    return m_field.has_value();
}

template <typename T>
constexpr const T*
OptFieldWithPresenceInd<T>::operator->() const
{
    return &(*m_field);
}

template <typename T>
constexpr T*
OptFieldWithPresenceInd<T>::operator->()
{
    return &(*m_field);
}

template <typename T>
constexpr const T&
OptFieldWithPresenceInd<T>::operator*() const
{
    return *m_field;
}

template <typename T>
constexpr T&
OptFieldWithPresenceInd<T>::operator*()
{
    return *m_field;
}

template <typename T>
template <class... Args>
constexpr T&
OptFieldWithPresenceInd<T>::emplace(Args&&... args)
{
    m_field.emplace(std::forward<Args>(args)...);
    m_presenceFlag.get() = true;
    return *m_field;
}

template <typename T>
constexpr void
OptFieldWithPresenceInd<T>::reset()
{
    m_field.reset();
    m_presenceFlag.get() = false;
}

} // namespace ns3

#endif /* WIFI_OPT_FIELD_H */
