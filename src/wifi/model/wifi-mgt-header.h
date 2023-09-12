/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_MGT_HEADER_H
#define WIFI_MGT_HEADER_H

#include "non-inheritance.h"
#include "supported-rates.h"

#include "ns3/eht-capabilities.h"
#include "ns3/header.h"
#include "ns3/multi-link-element.h"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

namespace ns3
{

namespace internal
{

/**
 * \ingroup object
 * \tparam T \explicit An Information Element type
 *
 * Provides the type used to store Information Elements in the tuple held by WifiMgtHeader:
 * - a mandatory Information Element of type T is stored as std::optional\<T\>
 * - an optional Information Element of type T is stored as std::optional\<T\>
 * - an Information Element of type T that can appear 0 or more times is stored as std::vector\<T\>
 */
template <class T>
struct GetStoredIe
{
    /// typedef for the resulting optional type
    typedef std::optional<T> type;
};

/** \copydoc GetStoredIe */
template <class T>
struct GetStoredIe<std::optional<T>>
{
    /// typedef for the resulting optional type
    typedef std::optional<T> type;
};

/** \copydoc GetStoredIe */
template <class T>
struct GetStoredIe<std::vector<T>>
{
    /// typedef for the resulting optional type
    typedef std::vector<T> type;
};

/** \copydoc GetStoredIe */
template <class T>
using GetStoredIeT = typename GetStoredIe<T>::type;

} // namespace internal

/**
 * \ingroup wifi
 * Implement the header for management frames.
 * \tparam Derived \explicit the type of derived management frame
 * \tparam Tuple \explicit A tuple of the types of Information Elements included in the mgt frame
 */
template <typename Derived, typename Tuple>
class WifiMgtHeader;

/**
 * \ingroup wifi
 * Base class for implementing management frame headers. This class adopts the CRTP idiom,
 * mainly to allow subclasses to specialize the method used to initialize Information
 * Elements before deserialization (<i>InitForDeserialization</i>).
 *
 * The sorted list of Information Elements that can be included in the management frame implemented
 * as a subclass of this class is provided as the template parameter pack. Specifically:
 * - the type of a mandatory Information Element IE is IE
 * - the type of an optional Information Element IE is std::optional\<IE\>
 * - the type of an Information Element IE that can appear zero or more times is std::vector\<IE\>
 *
 * \tparam Derived \explicit the type of derived management frame
 * \tparam Elems \explicit sorted list of Information Elements that can be included in mgt frame
 */
template <typename Derived, typename... Elems>
class WifiMgtHeader<Derived, std::tuple<Elems...>> : public Header
{
  public:
    /**
     * Access a (mandatory or optional) Information Element.
     *
     * \tparam T \explicit the type of the Information Element to return
     * \return a reference to the Information Element of the given type
     */
    template <typename T,
              std::enable_if_t<(std::is_same_v<std::vector<T>, Elems> + ...) == 0, int> = 0>
    std::optional<T>& Get();

    /**
     * Access a (mandatory or optional) Information Element.
     *
     * \tparam T \explicit the type of the Information Element to return
     * \return a const reference to the Information Element of the given type
     */
    template <typename T,
              std::enable_if_t<(std::is_same_v<std::vector<T>, Elems> + ...) == 0, int> = 0>
    const std::optional<T>& Get() const;

    /**
     * Access an Information Element that can be present zero or more times.
     *
     * \tparam T \explicit the type of the Information Element to return
     * \return a reference to the Information Element of the given type
     */
    template <typename T,
              std::enable_if_t<(std::is_same_v<std::vector<T>, Elems> + ...) == 1, int> = 0>
    std::vector<T>& Get();

    /**
     * Access an Information Element that can be present zero or more times.
     *
     * \tparam T \explicit the type of the Information Element to return
     * \return a reference to the Information Element of the given type
     */
    template <typename T,
              std::enable_if_t<(std::is_same_v<std::vector<T>, Elems> + ...) == 1, int> = 0>
    const std::vector<T>& Get() const;

    void Print(std::ostream& os) const final;
    uint32_t GetSerializedSize() const final;
    void Serialize(Buffer::Iterator start) const final;
    uint32_t Deserialize(Buffer::Iterator start) final;

  protected:
    /**
     * \tparam IE \deduced the type of the Information Element to initialize for deserialization
     * \param optElem the object to initialize for deserializing the information element into
     *
     * The Information Element object is constructed by calling the object's default constructor.
     */
    template <typename IE>
    void InitForDeserialization(std::optional<IE>& optElem);

    /**
     * \param optElem the EhtCapabilities object to initialize for deserializing the
     *                information element into
     */
    void InitForDeserialization(std::optional<EhtCapabilities>& optElem);

    /** \copydoc ns3::Header::Print */
    void PrintImpl(std::ostream& os) const;
    /** \copydoc ns3::Header::GetSerializedSize */
    uint32_t GetSerializedSizeImpl() const;
    /** \copydoc ns3::Header::Serialize */
    void SerializeImpl(Buffer::Iterator start) const;
    /** \copydoc ns3::Header::Deserialize */
    uint32_t DeserializeImpl(Buffer::Iterator start);

    /**
     * \tparam T \deduced the type of the Information Element
     * \param elem the optional Information Element
     * \param start the buffer iterator pointing to where deserialization starts
     * \return an iterator pointing to where deserialization terminated
     */
    template <typename T>
    Buffer::Iterator DoDeserialize(std::optional<T>& elem, Buffer::Iterator start);

    /**
     * \tparam T \deduced the type of the Information Elements
     * \param elems a vector of Information Elements
     * \param start the buffer iterator pointing to where deserialization starts
     * \return an iterator pointing to where deserialization terminated
     */
    template <typename T>
    Buffer::Iterator DoDeserialize(std::vector<T>& elems, Buffer::Iterator start);

    /// type of the Information Elements contained by this frame
    using Elements = std::tuple<internal::GetStoredIeT<Elems>...>;

    Elements m_elements; //!< Information Elements contained by this frame
};

/**
 * \ingroup wifi
 *  Inspect a type to deduce whether it is an Information Element that can be included in a
 *  Per-STA Profile subelement of a Multi-Link Element.
 *  \tparam T \explicit The type to inspect.
 */
template <class T>
struct CanBeInPerStaProfile : std::true_type
{
};

/** \copydoc CanBeInPerStaProfile */
template <class T>
inline constexpr bool CanBeInPerStaProfileV = CanBeInPerStaProfile<T>::value;

/**
 * \ingroup wifi
 * Implement the header for management frames that can be included in a Per-STA Profile
 * subelement of a Multi-Link Element.
 * \tparam Derived \explicit the type of derived management frame
 * \tparam Tuple \explicit A tuple of the types of Information Elements included in the mgt frame
 */
template <typename Derived, typename Tuple>
class MgtHeaderInPerStaProfile;

/**
 * \ingroup wifi
 *
 * Add methods needed to serialize/deserialize a management header into a Per-STA Profile
 * subelement of a Multi-Link Element.
 *
 * \tparam Derived \explicit the type of derived management frame
 * \tparam Elems \explicit sorted list of Information Elements that can be included in mgt frame
 */
template <typename Derived, typename... Elems>
class MgtHeaderInPerStaProfile<Derived, std::tuple<Elems...>>
    : public WifiMgtHeader<Derived, std::tuple<Elems...>>
{
  public:
    /**
     * \param frame the frame containing the Multi-Link Element
     * \return the number of bytes that are needed to serialize this header into a Per-STA Profile
     *         subelement of the Multi-Link Element
     */
    uint32_t GetSerializedSizeInPerStaProfile(const Derived& frame) const;

    /**
     * Serialize this header into a Per-STA Profile subelement of a Multi-Link Element
     *
     * \param start an iterator which points to where the header should be written
     * \param frame the frame containing the Multi-Link Element
     */
    void SerializeInPerStaProfile(Buffer::Iterator start, const Derived& frame) const;

    /**
     * Deserialize this header from a Per-STA Profile subelement of a Multi-Link Element.
     *
     * \param start an iterator which points to where the header should be read from
     * \param length the expected number of bytes to read
     * \param frame the frame containing the Multi-Link Element
     * \return the number of bytes read
     */
    uint32_t DeserializeFromPerStaProfile(Buffer::Iterator start,
                                          uint16_t length,
                                          const Derived& frame);

    /**
     * Copy Information Elements inherited from the management frame containing the Multi-Link
     * Element into this header (which is stored in a Per-STA Profile subelement). This method
     * shall be invoked when the deserialization has been completed (i.e., the Non-Inheritance
     * element, if present, has been deserialized).
     *
     * \param frame the frame containing the Multi-Link Element
     */
    void CopyIesFromContainingFrame(const Derived& frame);

  protected:
    using WifiMgtHeader<Derived, std::tuple<Elems...>>::InitForDeserialization;

    /**
     * \param frame the frame containing the Multi-Link Element
     * \return the number of bytes that are needed to serialize this header into a Per-STA Profile
     *         subelement of the Multi-Link Element
     */
    uint32_t GetSerializedSizeInPerStaProfileImpl(const Derived& frame) const;

    /**
     * Serialize this header into a Per-STA Profile subelement of a Multi-Link Element
     *
     * \param start an iterator which points to where the header should be written
     * \param frame the frame containing the Multi-Link Element
     */
    void SerializeInPerStaProfileImpl(Buffer::Iterator start, const Derived& frame) const;

    /**
     * Deserialize this header from a Per-STA Profile subelement of a Multi-Link Element.
     *
     * \param start an iterator which points to where the header should be read from
     * \param length the expected number of bytes to read
     * \param frame the frame containing the Multi-Link Element
     * \return the number of bytes read
     */
    uint32_t DeserializeFromPerStaProfileImpl(Buffer::Iterator start,
                                              uint16_t length,
                                              const Derived& frame);

    /**
     * Pass a pointer to this frame to the Multi-Link Element (if any) included in this frame.
     */
    void SetMleContainingFrame() const;

    /**
     * \param optElem the MultiLinkElement object to initialize for deserializing the
     *                information element into
     */
    void InitForDeserialization(std::optional<MultiLinkElement>& optElem);

  private:
    using WifiMgtHeader<Derived, std::tuple<Elems...>>::DoDeserialize;
    using WifiMgtHeader<Derived, std::tuple<Elems...>>::m_elements;

    std::optional<NonInheritance> m_nonInheritance; /**< the Non-Inheritance IE possibly appended
                                                         to the Per-STA Profile subelement */
};

/**
 * Implementation of the templates declared above.
 */

template <typename Derived, typename... Elems>
template <typename T, std::enable_if_t<(std::is_same_v<std::vector<T>, Elems> + ...) == 0, int>>
std::optional<T>&
WifiMgtHeader<Derived, std::tuple<Elems...>>::Get()
{
    return std::get<std::optional<T>>(m_elements);
}

template <typename Derived, typename... Elems>
template <typename T, std::enable_if_t<(std::is_same_v<std::vector<T>, Elems> + ...) == 0, int>>
const std::optional<T>&
WifiMgtHeader<Derived, std::tuple<Elems...>>::Get() const
{
    return std::get<std::optional<T>>(m_elements);
}

template <typename Derived, typename... Elems>
template <typename T, std::enable_if_t<(std::is_same_v<std::vector<T>, Elems> + ...) == 1, int>>
std::vector<T>&
WifiMgtHeader<Derived, std::tuple<Elems...>>::Get()
{
    return std::get<std::vector<T>>(m_elements);
}

template <typename Derived, typename... Elems>
template <typename T, std::enable_if_t<(std::is_same_v<std::vector<T>, Elems> + ...) == 1, int>>
const std::vector<T>&
WifiMgtHeader<Derived, std::tuple<Elems...>>::Get() const
{
    return std::get<std::vector<T>>(m_elements);
}

template <typename Derived, typename... Elems>
template <typename IE>
void
WifiMgtHeader<Derived, std::tuple<Elems...>>::InitForDeserialization(std::optional<IE>& optElem)
{
    optElem.emplace();
}

template <typename Derived, typename... Elems>
void
WifiMgtHeader<Derived, std::tuple<Elems...>>::InitForDeserialization(
    std::optional<EhtCapabilities>& optElem)
{
    NS_ASSERT(Get<SupportedRates>());
    auto rates = AllSupportedRates{*Get<SupportedRates>(), std::nullopt};
    const bool is2_4Ghz = rates.IsSupportedRate(
        1000000 /* 1 Mbit/s */); // TODO: use presence of VHT capabilities IE and HE 6 GHz Band
                                 // Capabilities IE once the later is implemented
    auto& heCapabilities = Get<HeCapabilities>();
    if (heCapabilities)
    {
        optElem.emplace(is2_4Ghz, heCapabilities.value());
    }
    else
    {
        optElem.emplace();
    }
}

template <typename Derived, typename... Elems>
void
MgtHeaderInPerStaProfile<Derived, std::tuple<Elems...>>::InitForDeserialization(
    std::optional<MultiLinkElement>& optElem)
{
    optElem.emplace(*static_cast<const Derived*>(this));
}

namespace internal
{

/**
 * \tparam T \deduced the type of the Information Element
 * \param elem the optional Information Element
 * \return the serialized size of the Information Element, if present, or 0, otherwise
 */
template <typename T>
uint16_t
DoGetSerializedSize(const std::optional<T>& elem)
{
    return elem.has_value() ? elem->GetSerializedSize() : 0;
}

/**
 * \tparam T \deduced the type of the Information Elements
 * \param elems a vector of Information Elements
 * \return the serialized size of the Information Elements
 */
template <typename T>
uint16_t
DoGetSerializedSize(const std::vector<T>& elems)
{
    return std::accumulate(elems.cbegin(), elems.cend(), 0, [](uint16_t a, const auto& b) {
        return b.GetSerializedSize() + a;
    });
}

} // namespace internal

template <typename Derived, typename... Elems>
uint32_t
WifiMgtHeader<Derived, std::tuple<Elems...>>::GetSerializedSize() const
{
    return static_cast<const Derived*>(this)->GetSerializedSizeImpl();
}

template <typename Derived, typename... Elems>
uint32_t
WifiMgtHeader<Derived, std::tuple<Elems...>>::GetSerializedSizeImpl() const
{
    return std::apply([&](auto&... elems) { return (internal::DoGetSerializedSize(elems) + ...); },
                      m_elements);
}

namespace internal
{

/**
 * \tparam T \deduced the type of the Information Element
 * \param elem the optional Information Element
 * \param start the buffer iterator pointing to where serialization starts
 * \return an iterator pointing to where serialization terminated
 */
template <typename T>
Buffer::Iterator
DoSerialize(const std::optional<T>& elem, Buffer::Iterator start)
{
    return elem.has_value() ? elem->Serialize(start) : start;
}

/**
 * \tparam T \deduced the type of the Information Elements
 * \param elems a vector of Information Elements
 * \param start the buffer iterator pointing to where serialization starts
 * \return an iterator pointing to where serialization terminated
 */
template <typename T>
Buffer::Iterator
DoSerialize(const std::vector<T>& elems, Buffer::Iterator start)
{
    return std::accumulate(elems.cbegin(),
                           elems.cend(),
                           start,
                           [](Buffer::Iterator i, const auto& a) { return a.Serialize(i); });
}

} // namespace internal

template <typename Derived, typename... Elems>
void
WifiMgtHeader<Derived, std::tuple<Elems...>>::Serialize(Buffer::Iterator start) const
{
    static_cast<const Derived*>(this)->SerializeImpl(start);
}

template <typename Derived, typename... Elems>
void
WifiMgtHeader<Derived, std::tuple<Elems...>>::SerializeImpl(Buffer::Iterator start) const
{
    auto i = start;
    std::apply([&](auto&... elems) { ((i = internal::DoSerialize(elems, i)), ...); }, m_elements);
}

template <typename Derived, typename... Elems>
template <typename T>
Buffer::Iterator
WifiMgtHeader<Derived, std::tuple<Elems...>>::DoDeserialize(std::optional<T>& elem,
                                                            Buffer::Iterator start)
{
    auto i = start;
    static_cast<Derived*>(this)->InitForDeserialization(elem);
    i = elem->DeserializeIfPresent(i);
    if (i.GetDistanceFrom(start) == 0)
    {
        elem.reset(); // the element is not present
    }
    return i;
}

template <typename Derived, typename... Elems>
template <typename T>
Buffer::Iterator
WifiMgtHeader<Derived, std::tuple<Elems...>>::DoDeserialize(std::vector<T>& elems,
                                                            Buffer::Iterator start)
{
    auto i = start;
    do
    {
        auto tmp = i;
        std::optional<T> item;
        static_cast<Derived*>(this)->InitForDeserialization(item);
        i = item->DeserializeIfPresent(i);
        if (i.GetDistanceFrom(tmp) == 0)
        {
            break;
        }
        elems.push_back(std::move(*item));
    } while (true);
    return i;
}

template <typename Derived, typename... Elems>
uint32_t
WifiMgtHeader<Derived, std::tuple<Elems...>>::Deserialize(Buffer::Iterator start)
{
    return static_cast<Derived*>(this)->DeserializeImpl(start);
}

template <typename Derived, typename... Elems>
uint32_t
WifiMgtHeader<Derived, std::tuple<Elems...>>::DeserializeImpl(Buffer::Iterator start)
{
    auto i = start;

    std::apply(
        // auto cannot be used until gcc 10.4 due to gcc bug 97938
        // (see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97938)
        [&](internal::GetStoredIeT<Elems>&... elems) {
            (
                [&] {
                    if constexpr (std::is_same_v<std::remove_reference_t<decltype(elems)>, Elems>)
                    {
                        // optional IE or IE that can be present 0 or more times
                        i = DoDeserialize(elems, i);
                    }
                    else
                    {
                        // mandatory IE
                        static_cast<Derived*>(this)->InitForDeserialization(elems);
                        i = elems->Deserialize(i);
                    }
                }(),
                ...);
        },
        m_elements);

    return i.GetDistanceFrom(start);
}

namespace internal
{

/**
 * \tparam T \deduced the type of the Information Element
 * \param elem the optional Information Element
 * \param os the output stream
 */
template <typename T>
void
DoPrint(const std::optional<T>& elem, std::ostream& os)
{
    if (elem.has_value())
    {
        os << *elem << " , ";
    }
}

/**
 * \tparam T \deduced the type of the Information Elements
 * \param elems a vector of Information Elements
 * \param os the output stream
 */
template <typename T>
void
DoPrint(const std::vector<T>& elems, std::ostream& os)
{
    std::copy(elems.cbegin(), elems.cend(), std::ostream_iterator<T>(os, " , "));
}

} // namespace internal

template <typename Derived, typename... Elems>
void
WifiMgtHeader<Derived, std::tuple<Elems...>>::Print(std::ostream& os) const
{
    static_cast<const Derived*>(this)->PrintImpl(os);
}

template <typename Derived, typename... Elems>
void
WifiMgtHeader<Derived, std::tuple<Elems...>>::PrintImpl(std::ostream& os) const
{
    std::apply([&](auto&... elems) { ((internal::DoPrint(elems, os)), ...); }, m_elements);
}

namespace internal
{

/**
 * \tparam T \deduced the type of the given Information Element
 * \tparam Derived \deduced the type of the containing management frame
 * \param elem the given Information Element
 * \param frame the containing management frame
 * \return whether the given Information Element shall be serialized in a Per-STA Profile
 *         subelement of the Multi-Link Element included in the containing management frame
 */
template <typename T, typename Derived>
bool
MustBeSerializedInPerStaProfile(const std::optional<T>& elem, const Derived& frame)
{
    if (!CanBeInPerStaProfileV<T>)
    {
        return false;
    }

    if (auto& outsideIe = frame.template Get<T>();
        outsideIe.has_value() && elem.has_value() && !(outsideIe.value() == elem.value()))
    {
        // the IE is present both outside the Multi-Link Element and in the Per-STA Profile,
        // but they are different, hence the IE must be serialized in the Per-STA Profile
        return true;
    }

    if (!frame.template Get<T>().has_value() && elem.has_value())
    {
        // the IE is not present outside the Multi-Link Element and is present in the Per-STA
        // Profile, hence the IE must be serialized in the Per-STA Profile
        return true;
    }

    return false;
}

/**
 * \tparam T \deduced the type of the given vector of Information Elements
 * \tparam Derived \deduced the type of the containing management frame
 * \param elems the given vector of Information Elements
 * \param frame the containing management frame
 * \return whether the given Information Elements shall be serialized in a Per-STA Profile
 *         subelement of the Multi-Link Element included in the containing management frame
 */
template <typename T, typename Derived>
bool
MustBeSerializedInPerStaProfile(const std::vector<T>& elems, const Derived& frame)
{
    if (!CanBeInPerStaProfileV<T>)
    {
        return false;
    }

    if (auto& outsideIe = frame.template Get<T>();
        !outsideIe.empty() && !elems.empty() && !(outsideIe == elems))
    {
        // the IEs are present both outside the Multi-Link Element and in the Per-STA Profile,
        // but they are different, hence the IEs must be serialized in the Per-STA Profile
        return true;
    }

    if (frame.template Get<T>().empty() && !elems.empty())
    {
        // the IEs are not present outside the Multi-Link Element and is present in the Per-STA
        // Profile, hence the IEs must be serialized in the Per-STA Profile
        return true;
    }

    return false;
}

/**
 * \tparam T \deduced the type of the given Information Element
 * \tparam Derived \deduced the type of the containing management frame
 * \param elem the given Information Element
 * \param frame the containing management frame
 * \return a pair (Element ID, Element ID Extension) if the given Information Element shall be
 *         listed in the Non-Inheritance IE of the Per-STA Profile subelement of the Multi-Link
 *         Element included in the containing management frame
 */
template <typename T, typename Derived>
std::optional<std::pair<uint8_t, uint8_t>>
MustBeListedInNonInheritance(const std::optional<T>& elem, const Derived& frame)
{
    if (auto& outsideIe = frame.template Get<T>();
        CanBeInPerStaProfileV<T> && outsideIe.has_value() && !elem.has_value())
    {
        return {{outsideIe->ElementId(), outsideIe->ElementIdExt()}};
    }
    return std::nullopt;
}

/**
 * \tparam T \deduced the type of the given vector of Information Elements
 * \tparam Derived \deduced the type of the containing management frame
 * \param elems the given Information Elements
 * \param frame the containing management frame
 * \return a pair (Element ID, Element ID Extension) if the given Information Element shall be
 *         listed in the Non-Inheritance IE of the Per-STA Profile subelement of the Multi-Link
 *         Element included in the containing management frame
 */
template <typename T, typename Derived>
std::optional<std::pair<uint8_t, uint8_t>>
MustBeListedInNonInheritance(const std::vector<T>& elems, const Derived& frame)
{
    if (auto& outsideIe = frame.template Get<T>();
        CanBeInPerStaProfileV<T> && !outsideIe.empty() && elems.empty())
    {
        return {{outsideIe.front().ElementId(), outsideIe.front().ElementIdExt()}};
    }
    return std::nullopt;
}

} // namespace internal

template <typename Derived, typename... Elems>
uint32_t
MgtHeaderInPerStaProfile<Derived, std::tuple<Elems...>>::GetSerializedSizeInPerStaProfile(
    const Derived& frame) const
{
    return static_cast<const Derived*>(this)->GetSerializedSizeInPerStaProfileImpl(frame);
}

template <typename Derived, typename... Elems>
uint32_t
MgtHeaderInPerStaProfile<Derived, std::tuple<Elems...>>::GetSerializedSizeInPerStaProfileImpl(
    const Derived& frame) const
{
    uint32_t size = 0;
    std::optional<NonInheritance> nonInheritance;

    std::apply(
        [&](auto&... elems) {
            (
                [&] {
                    if (internal::MustBeSerializedInPerStaProfile(elems, frame))
                    {
                        size += internal::DoGetSerializedSize(elems);
                    }
                    else if (auto idPair = internal::MustBeListedInNonInheritance(elems, frame))
                    {
                        if (!nonInheritance)
                        {
                            nonInheritance.emplace();
                        }
                        nonInheritance->Add(idPair->first, idPair->second);
                    }
                }(),
                ...);
        },
        m_elements);

    if (nonInheritance)
    {
        size += nonInheritance->GetSerializedSize();
    }
    return size;
}

template <typename Derived, typename... Elems>
void
MgtHeaderInPerStaProfile<Derived, std::tuple<Elems...>>::SerializeInPerStaProfile(
    Buffer::Iterator start,
    const Derived& frame) const
{
    static_cast<const Derived*>(this)->SerializeInPerStaProfileImpl(start, frame);
}

template <typename Derived, typename... Elems>
void
MgtHeaderInPerStaProfile<Derived, std::tuple<Elems...>>::SerializeInPerStaProfileImpl(
    Buffer::Iterator start,
    const Derived& frame) const
{
    auto i = start;
    std::optional<NonInheritance> nonInheritance;

    std::apply(
        [&](auto&... elems) {
            (
                [&] {
                    if (internal::MustBeSerializedInPerStaProfile(elems, frame))
                    {
                        i = internal::DoSerialize(elems, i);
                    }
                    else if (auto idPair = internal::MustBeListedInNonInheritance(elems, frame))
                    {
                        if (!nonInheritance)
                        {
                            nonInheritance.emplace();
                        }
                        nonInheritance->Add(idPair->first, idPair->second);
                    }
                }(),
                ...);
        },
        m_elements);

    if (nonInheritance)
    {
        nonInheritance->Serialize(i);
    }
}

namespace internal
{

/**
 * \tparam T \deduced the type of the given Information Element
 * \tparam Derived \deduced the type of the containing management frame
 * \param elem the given Information Element
 * \param frame the containing management frame
 *
 * Copy the given Information Element from the containing frame to the Per-STA Profile subelement
 * of the Multi-Link Element, if the Information Element has been inherited (i.e., it is present
 * outside the Multi-Link Element and not present in the Per-STA Profile subelement)
 */
template <typename T, typename Derived>
void
DoCopyIeFromContainingFrame(std::optional<T>& elem, const Derived& frame)
{
    if (auto& outsideIe = frame.template Get<T>();
        CanBeInPerStaProfileV<T> && outsideIe.has_value() && !elem.has_value())
    {
        elem = outsideIe.value();
    }
}

/**
 * \tparam T \deduced the type of the given vector of Information Elements
 * \tparam Derived \deduced the type of the containing management frame
 * \param elems the given vector of Information Elements
 * \param frame the containing management frame
 *
 * Copy the given Information Element from the containing frame to the Per-STA Profile subelement
 * of the Multi-Link Element, if the Information Element has been inherited (i.e., it is present
 * outside the Multi-Link Element and not present in the Per-STA Profile subelement)
 */
template <typename T, typename Derived>
void
DoCopyIeFromContainingFrame(std::vector<T>& elems, const Derived& frame)
{
    if (auto& outsideIe = frame.template Get<T>();
        CanBeInPerStaProfileV<T> && !outsideIe.empty() && elems.empty())
    {
        elems = outsideIe;
    }
}

} // namespace internal

template <typename Derived, typename... Elems>
uint32_t
MgtHeaderInPerStaProfile<Derived, std::tuple<Elems...>>::DeserializeFromPerStaProfile(
    Buffer::Iterator start,
    uint16_t length,
    const Derived& frame)
{
    return static_cast<Derived*>(this)->DeserializeFromPerStaProfileImpl(start, length, frame);
}

template <typename Derived, typename... Elems>
uint32_t
MgtHeaderInPerStaProfile<Derived, std::tuple<Elems...>>::DeserializeFromPerStaProfileImpl(
    Buffer::Iterator start,
    uint16_t length,
    const Derived& frame)
{
    auto i = start;

    // deserialize the IEs in the Per-STA Profile subelement
    std::apply(
        [&](auto&... elems) {
            (
                [&] {
                    if (i.GetDistanceFrom(start) < length)
                    {
                        i = static_cast<Derived*>(this)->DoDeserialize(elems, i);
                        internal::DoCopyIeFromContainingFrame(elems, frame);
                    }
                }(),
                ...);
        },
        m_elements);

    // deserialize the Non-Inheritance IE, if present
    m_nonInheritance.reset();
    i = DoDeserialize(m_nonInheritance, i);

    auto distance = i.GetDistanceFrom(start);
    NS_ASSERT_MSG(distance == length,
                  "Bytes read (" << distance << ") not matching expected number (" << length
                                 << ")");
    return distance;
}

namespace internal
{

/**
 * \tparam T \deduced the type of the given Information Element
 * \param elem the given Information Element
 * \param nonInheritance the Non-Inheritance information element
 *
 * Remove the given Information Element from this header, if it is present and is listed in
 * the given Non-Inheritance element.
 */
template <typename T>
void
RemoveIfNotInherited(std::optional<T>& elem, const NonInheritance& nonInheritance)
{
    if (elem.has_value() && nonInheritance.IsPresent(elem->ElementId(), elem->ElementIdExt()))
    {
        elem.reset();
    }
}

/**
 * \tparam T \deduced the type of the given vector of Information Elements
 * \param elem the given Information Elements
 * \param nonInheritance the Non-Inheritance information element
 *
 * Remove the given Information Elements from this header, if they are present and are listed in
 * the given Non-Inheritance element.
 */
template <typename T>
void
RemoveIfNotInherited(std::vector<T>& elem, const NonInheritance& nonInheritance)
{
    if (!elem.empty() &&
        nonInheritance.IsPresent(elem.front().ElementId(), elem.front().ElementIdExt()))
    {
        elem.clear();
    }
}

} // namespace internal

template <typename Derived, typename... Elems>
void
MgtHeaderInPerStaProfile<Derived, std::tuple<Elems...>>::CopyIesFromContainingFrame(
    const Derived& frame)
{
    // copy inherited Information Elements that appear in the containing frame after the
    // MLE (those appearing before have been copied by DeserializeFromPerStaProfileImpl)
    std::apply(
        [&](auto&... elems) { ((internal::DoCopyIeFromContainingFrame(elems, frame)), ...); },
        m_elements);

    // we have possibly deserialized a Non-Inheritance element; remove IEs listed therein
    if (m_nonInheritance)
    {
        std::apply(
            [&](auto&... elems) {
                ((internal::RemoveIfNotInherited(elems, *m_nonInheritance)), ...);
            },
            m_elements);
    }
}

template <typename Derived, typename... Elems>
void
MgtHeaderInPerStaProfile<Derived, std::tuple<Elems...>>::SetMleContainingFrame() const
{
    if (auto& mle = WifiMgtHeader<Derived, std::tuple<Elems...>>::template Get<MultiLinkElement>())
    {
        mle->m_containingFrame = *static_cast<const Derived*>(this);
    }
}

} // namespace ns3

#endif /* WIFI_MGT_HEADER_H */
