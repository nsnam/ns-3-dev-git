/*
 * Copyright (c) 2026 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef STRUCT_H
#define STRUCT_H

#include "abort.h"
#include "tuple.h"

#include <concepts>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/**
 * @file
 * @ingroup attribute_Struct
 * Guidelines for defining attributes represented by structures.
 *
 * StructValue allows a structure to use the attribute system's accessors,
 * checkers, serialization, and deserialization facilities. To define a
 * StructValue, provide one StructField descriptor for every field, in the same
 * order in which the field values are serialized. Choose one of the following
 * forms according to how the structure exposes and initializes its fields
 * ( @see StructValue for more information):
 *
 * 1. **Public data members.** If all fields are publicly accessible, each
 *    StructField specifies the field's AttributeValue type and a pointer to the
 *    data member, for example
 *    `StructField<UintegerValue, &Data::count>`. StructValue reconstructs the
 *    structure directly from the field values. If an AttributeValue returns a
 *    different type, its GetAccessor() method converts the value to the data
 *    member type.
 * 2. **Private data members with getters and a factory.** Each private field
 *    must have a getter, and its StructField specifies the AttributeValue type
 *    and a pointer to that getter, for example
 *    `StructField<UintegerValue, &Data::GetCount>`. For any public data member, a
 *    getter is not required, as its StructField descriptor can specify the field's
 *    AttributeValue type and a pointer to the data member. A StructConstructor
 *    descriptor must precede the StructField descriptors and identify a static member
 *    or free factory function that accepts all field values in order and returns
 *    the structure, for example `StructConstructor<&Data::Create>`.
 * 3. **Private data members with getters and setters.** Each private field must
 *    have both a getter and a setter, and its StructField specifies the
 *    AttributeValue type and pointers to both methods, for example
 *    `StructField<UintegerValue, &Data::GetCount, &Data::SetCount>`. For any public
 *    data member, getter and setter are not required, as its StructField descriptor
 *    can specify the field's AttributeValue type and a pointer to the data member.
 *    The structure must be default constructible; StructValue reconstructs it by
 *    default-constructing an instance and invoking the setter, for private data
 *    members, or accessing the pointer, for public data members.
 *
 * StructValue stores the fields internally as a TupleValue. MakeStructAccessor()
 * and MakeStructChecker() can then be used to register the structure as an
 * attribute and validate its individual fields.
 *
 * @see StructValueTestSuite for examples of all three forms.
 */

namespace ns3
{

/**
 * @ingroup attributes
 * @defgroup attribute_Struct Struct Attribute
 * AttributeValue implementation for structures.
 */

namespace internal
{

template <typename T, typename... Descriptors>
struct StructValueTraits;

} // namespace internal

/**
 * @ingroup attribute_Struct
 *
 * AttributeValue implementation for structures.
 *
 * The fields of the structure are stored internally as a tuple of AttributeValue
 * objects. StructField instances associate every structure field with the
 * AttributeValue type used to represent it and specify how to access it.
 *
 * @code
 * struct Data
 * {
 *     uint16_t count;
 *     double value;
 * };
 *
 * using DataValue =
 *     StructValue<Data,
 *                 StructField<UintegerValue, &Data::count>,
 *                 StructField<DoubleValue, &Data::value>>;
 * @endcode
 *
 * Getter and setter methods can be used for private fields. In this case, the
 * represented type must be default constructible because Get() reconstructs it
 * by invoking each setter on a default-constructed instance.
 * @code
 * using DataValue =
 *     StructValue<Data,
 *                 StructField<UintegerValue, &Data::GetCount, &Data::SetCount>,
 *                 StructField<DoubleValue, &Data::GetValue, &Data::SetValue>>;
 * @endcode
 *
 * A constructor function can be used when fields have getters but no setters.
 * The StructConstructor descriptor, when present, must precede the StructField
 * descriptors and identify a static member or free factory function that accepts
 * all field values in order and returns the structure.
 * @code
 * using DataValue =
 *     StructValue<Data,
 *                 StructConstructor<&Data::Create>,
 *                 StructField<UintegerValue, &Data::GetCount>,
 *                 StructField<DoubleValue, &Data::GetValue>>;
 * @endcode
 *
 * Get() constructs the represented structure by using, in order of priority,
 * direct aggregate or constructor initialization when all fields are data
 * members, the provided constructor function, or default construction followed
 * by the field setters. If a data member is not directly assignable from the
 * value returned by its AttributeValue, GetAccessor() performs the conversion.
 *
 * @tparam T
 *         The structure represented by this AttributeValue.
 * @tparam Descriptors
 *         An optional StructConstructor followed by StructField types describing
 *         the structure fields in declaration order.
 *
 * @see AttributeValue
 */
template <typename T, typename... Descriptors>
class StructValue : public internal::StructValueTraits<T, Descriptors...>::base_type
{
  public:
    /** The represented structure type. */
    using result_type = T;
    /** Tuple of the values returned by the field AttributeValue objects. */
    using tuple_type = typename internal::StructValueTraits<T, Descriptors...>::tuple_type;
    /** Tuple of the field AttributeValue objects. */
    using value_type = typename internal::StructValueTraits<T, Descriptors...>::value_type;

    /** Number of represented structure fields. */
    static constexpr std::size_t field_count =
        internal::StructValueTraits<T, Descriptors...>::field_count;

    /** Construct a StructValue containing default-initialized field values. */
    StructValue() = default;

    /**
     * Construct a StructValue from a structure.
     *
     * @param value Value with which to construct this object.
     */
    StructValue(const result_type& value);

    /**
     * @returns a deep copy of this class, wrapped into an Attribute object.
     */
    Ptr<AttributeValue> Copy() const override;

    /**
     * Get the stored fields as a structure.
     *
     * @return the structure containing the stored field values
     */
    result_type Get() const;

    /**
     * Set the stored fields from a structure.
     *
     * @param value the structure containing the field values to store
     */
    void Set(const result_type& value);

    /**
     * Set the given variable to the structure represented by this object.
     *
     * @tparam U
     *         The type of the given variable.
     * @param value The variable to set.
     * @return true if the given variable was set
     */
    template <typename U>
    bool GetAccessor(U& value) const;

  private:
    /** Traits describing the constructor and fields. */
    using Traits = internal::StructValueTraits<T, Descriptors...>;
    /** TupleValue type used to store the structure fields. */
    using Base = typename Traits::base_type;
};

/**
 * Describe a structure field and the AttributeValue used to represent it.
 *
 * A field can be accessed either directly through a pointer to a data member:
 * @code
 * StructField<UintegerValue, &Data::count>
 * @endcode
 * or through a getter and a setter:
 * @code
 * StructField<UintegerValue, &Data::GetCount, &Data::SetCount>
 * @endcode
 *
 * @tparam Value
 *         The AttributeValue type used to represent the field.
 * @tparam Getter
 *         Pointer to the represented data member or to its getter.
 * @tparam Setter
 *         Pointer to the setter associated with a getter. It must be omitted when
 *         Getter points to a data member and may be omitted when a StructConstructor
 *         is provided.
 */
template <typename Value, auto Getter, auto Setter = nullptr>
struct StructField
{
    /** AttributeValue type used to represent the field. */
    using value_type = Value;
    /** Type of the data member pointer or getter. */
    using getter_type = decltype(Getter);
    /** Type of the setter. */
    using setter_type = decltype(Setter);

    /** Pointer to the represented data member or to its getter. */
    static constexpr auto getter = Getter;
    /** Pointer to the setter associated with a getter. */
    static constexpr auto setter = Setter;
};

/**
 * Describe a function that constructs a structure from its field values.
 *
 * C++ does not allow taking the address of a constructor. Therefore, Function
 * must point to a static or free factory function that returns the represented
 * structure and accepts its field values in declaration order.
 *
 * @code
 * struct Data
 * {
 *     static Data Create(uint16_t count, double value);
 * };
 *
 * StructConstructor<&Data::Create>
 * @endcode
 *
 * @tparam Function
 *         Pointer to the function that constructs the structure.
 */
template <auto Function>
struct StructConstructor
{
    /** Type of the constructor function pointer. */
    using function_type = decltype(Function);

    /** Pointer to the function that constructs the structure. */
    static constexpr auto function = Function;
};

namespace internal
{

/** Concept matching a valid structure field descriptor. */
template <typename Field>
concept StructFieldDescriptor =
    requires {
        typename Field::value_type;
        typename Field::getter_type;
        typename Field::setter_type;
    } && ((std::is_member_object_pointer_v<typename Field::getter_type> &&
           std::same_as<typename Field::setter_type, std::nullptr_t>) ||
          (std::is_member_function_pointer_v<typename Field::getter_type> &&
           (std::same_as<typename Field::setter_type, std::nullptr_t> ||
            std::is_member_function_pointer_v<typename Field::setter_type>)));

/** Concept matching a structure field that can be read from the represented structure. */
template <typename Field, typename T>
concept StructFieldFor =
    StructFieldDescriptor<Field> && std::invocable<typename Field::getter_type, const T&>;

/** Concept matching a structure field that can be set from the given value type. */
template <typename Field, typename T, typename Value>
concept SettableStructField =
    StructFieldDescriptor<Field> && (std::is_member_object_pointer_v<typename Field::getter_type> ||
                                     std::invocable<typename Field::setter_type, T&, Value>);

/**
 * Provide the implementation details of StructValue for a set of fields.
 *
 * @tparam T
 *         The represented structure type.
 * @tparam Constructor
 *         Function used to construct the structure, or nullptr.
 * @tparam Fields
 *         Fields represented by StructValue.
 */
template <typename T, auto Constructor, typename... Fields>
    requires(StructFieldFor<Fields, T> && ...)
struct StructValueTraitsBase
{
    /** TupleValue type used to store the structure fields. */
    using base_type = TupleValue<typename Fields::value_type...>;
    /** Tuple of the values returned by the field AttributeValue objects. */
    using tuple_type = typename base_type::result_type;
    /** Tuple of the field AttributeValue objects. */
    using value_type = typename base_type::value_type;

    /** Number of structure fields. */
    static constexpr std::size_t field_count = sizeof...(Fields);

    /**
     * Construct the represented structure from field values.
     *
     * @param values The field values.
     * @return the constructed structure
     */
    static T Construct(tuple_type values);

    /**
     * Read the field values from a structure.
     *
     * @param value The structure to read.
     * @return the field values
     */
    static tuple_type Read(const T& value);

  private:
    /**
     * Check whether the structure can be constructed directly from field values.
     *
     * @tparam Is
     *         Indices of the field values.
     * @return true if the structure can be constructed directly
     */
    template <std::size_t... Is>
    static constexpr bool IsDirectlyConstructible(std::index_sequence<Is...>);

    /**
     * Read a field through its AttributeValue type.
     *
     * @tparam Field
     *         Descriptor of the field to read.
     * @param object The structure to read.
     * @return the field value returned by the AttributeValue
     */
    template <typename Field>
    static auto ReadField(const T& object);

    /**
     * Check whether the constructor function accepts all field values.
     *
     * @tparam Is
     *         Indices of the field values.
     * @return true if the constructor function can be invoked
     */
    template <std::size_t... Is>
    static constexpr bool IsConstructorInvocable(std::index_sequence<Is...>);

    /**
     * Set the fields of a structure from tuple elements.
     *
     * @tparam Is
     *         Indices of the fields to set.
     * @param object The structure whose fields are set.
     * @param values The tuple containing the field values.
     */
    template <std::size_t... Is>
    static void SetFields(T& object, tuple_type&& values, std::index_sequence<Is...>);

    /**
     * Set a structure field through its data member or setter.
     *
     * @tparam Field
     *         Descriptor of the field to set.
     * @tparam Value
     *         Type of the field value.
     * @param object The structure whose field is set.
     * @param value The value to set.
     */
    template <typename Field, typename Value>
        requires SettableStructField<Field, T, Value>
    static void SetField(T& object, Value&& value);
};

/**
 * Select the StructValue implementation when no constructor function is provided.
 *
 * @tparam T
 *         The represented structure type.
 * @tparam Fields
 *         Fields represented by StructValue.
 */
template <typename T, typename... Fields>
struct StructValueTraits : StructValueTraitsBase<T, nullptr, Fields...>
{
};

/**
 * Select the StructValue implementation when a constructor function is provided.
 *
 * @tparam T
 *         The represented structure type.
 * @tparam Constructor
 *         Function used to construct the structure.
 * @tparam Fields
 *         Fields represented by StructValue.
 */
template <typename T, auto Constructor, typename... Fields>
struct StructValueTraits<T, StructConstructor<Constructor>, Fields...>
    : StructValueTraitsBase<T, Constructor, Fields...>
{
};

} // namespace internal

/**
 * @ingroup attribute_Struct
 *
 * Checker for attribute values representing structures.
 */
class StructChecker : public TupleChecker
{
};

/**
 * @ingroup attribute_Struct
 *
 * Create a StructChecker from the AttributeCheckers associated with the fields.
 *
 * @tparam T
 *         The StructValue type to check.
 * @tparam Ts
 *         The AttributeChecker types.
 * @param checkers AttributeCheckers for the individual fields.
 * @return pointer to the StructChecker instance
 */
template <typename T, typename... Ts>
    requires(sizeof...(Ts) == T::field_count)
Ptr<const AttributeChecker> MakeStructChecker(Ts... checkers);

/**
 * @ingroup attribute_Struct
 *
 * Create an AttributeAccessor for a structure data member, or for a lone class
 * get functor or set method.
 *
 * @tparam T
 *         The StructValue type used by the attribute.
 * @tparam T1
 *         The type of the class data member, get functor, or set method.
 * @param a1 The address of the data member, get functor, or set method.
 * @return the AttributeAccessor
 */
template <typename T, typename T1>
Ptr<const AttributeAccessor> MakeStructAccessor(T1 a1);

/**
 * @ingroup attribute_Struct
 *
 * Create an AttributeAccessor using a pair of get functor and set methods.
 *
 * @tparam T
 *         The StructValue type used by the attribute.
 * @tparam T1
 *         The type of the first method.
 * @tparam T2
 *         The type of the second method.
 * @param a1 The address of the first method.
 * @param a2 The address of the second method.
 * @return the AttributeAccessor
 */
template <typename T, typename T1, typename T2>
Ptr<const AttributeAccessor> MakeStructAccessor(T1 a1, T2 a2);

} // namespace ns3

/*****************************************************************************
 * Implementation below
 *****************************************************************************/

namespace ns3
{

namespace internal
{

template <typename T, auto Constructor, typename... Fields>
    requires(StructFieldFor<Fields, T> && ...)
T
StructValueTraitsBase<T, Constructor, Fields...>::Construct(tuple_type values)
{
    if constexpr ((std::is_member_object_pointer_v<typename Fields::getter_type> && ...))
    {
        if constexpr (IsDirectlyConstructible(std::index_sequence_for<Fields...>{}))
        {
            return std::make_from_tuple<T>(std::move(values));
        }
        else
        {
            static_assert(std::is_default_constructible_v<T>,
                          "A structure requiring AttributeValue conversion must be default "
                          "constructible");
            T object{};
            SetFields(object, std::move(values), std::index_sequence_for<Fields...>{});
            return object;
        }
    }
    else if constexpr (!std::is_same_v<decltype(Constructor), std::nullptr_t>)
    {
        static_assert(IsConstructorInvocable(std::index_sequence_for<Fields...>{}),
                      "StructConstructor must accept the values returned by the field "
                      "AttributeValues and return the represented structure");
        return std::apply(Constructor, std::move(values));
    }
    else
    {
        static_assert(std::is_default_constructible_v<T>,
                      "A structure accessed through getters and setters must be default "
                      "constructible");
        static_assert(((std::is_member_object_pointer_v<typename Fields::getter_type> ||
                        std::is_member_function_pointer_v<typename Fields::setter_type>) &&
                       ...),
                      "Every getter requires either a StructConstructor or a setter");
        T object{};
        SetFields(object, std::move(values), std::index_sequence_for<Fields...>{});
        return object;
    }
}

template <typename T, auto Constructor, typename... Fields>
    requires(StructFieldFor<Fields, T> && ...)
typename StructValueTraitsBase<T, Constructor, Fields...>::tuple_type
StructValueTraitsBase<T, Constructor, Fields...>::Read(const T& value)
{
    return tuple_type(ReadField<Fields>(value)...);
}

template <typename T, auto Constructor, typename... Fields>
    requires(StructFieldFor<Fields, T> && ...)
template <std::size_t... Is>
constexpr bool
StructValueTraitsBase<T, Constructor, Fields...>::IsDirectlyConstructible(
    std::index_sequence<Is...>)
{
    return std::is_constructible_v<T, std::tuple_element_t<Is, tuple_type>...>;
}

template <typename T, auto Constructor, typename... Fields>
    requires(StructFieldFor<Fields, T> && ...)
template <typename Field>
auto
StructValueTraitsBase<T, Constructor, Fields...>::ReadField(const T& object)
{
    typename Field::value_type attributeValue;
    attributeValue.Set(std::invoke(Field::getter, object));
    return attributeValue.Get();
}

template <typename T, auto Constructor, typename... Fields>
    requires(StructFieldFor<Fields, T> && ...)
template <std::size_t... Is>
constexpr bool
StructValueTraitsBase<T, Constructor, Fields...>::IsConstructorInvocable(std::index_sequence<Is...>)
{
    using constructor_type = decltype(Constructor);
    using is_invocable =
        std::is_invocable_r<T, constructor_type, std::tuple_element_t<Is, tuple_type>&&...>;
    return is_invocable::value;
}

template <typename T, auto Constructor, typename... Fields>
    requires(StructFieldFor<Fields, T> && ...)
template <std::size_t... Is>
void
StructValueTraitsBase<T, Constructor, Fields...>::SetFields(T& object,
                                                            tuple_type&& values,
                                                            std::index_sequence<Is...>)
{
    (SetField<Fields>(object, std::get<Is>(std::move(values))), ...);
}

template <typename T, auto Constructor, typename... Fields>
    requires(StructFieldFor<Fields, T> && ...)
template <typename Field, typename Value>
    requires SettableStructField<Field, T, Value>
void
StructValueTraitsBase<T, Constructor, Fields...>::SetField(T& object, Value&& value)
{
    if constexpr (std::is_member_object_pointer_v<typename Field::getter_type>)
    {
        if constexpr (std::is_assignable_v<decltype(std::invoke(Field::getter, object)), Value>)
        {
            std::invoke(Field::getter, object) = std::forward<Value>(value);
        }
        else
        {
            typename Field::value_type attributeValue(std::forward<Value>(value));
            NS_ABORT_MSG_UNLESS(
                attributeValue.GetAccessor(std::invoke(Field::getter, object)),
                "The AttributeValue could not be converted to the structure field type");
        }
    }
    else
    {
        std::invoke(Field::setter, object, std::forward<Value>(value));
    }
}

} // namespace internal

template <typename T, typename... Fields>
StructValue<T, Fields...>::StructValue(const result_type& value)
{
    Set(value);
}

template <typename T, typename... Fields>
Ptr<AttributeValue>
StructValue<T, Fields...>::Copy() const
{
    return Create<StructValue<T, Fields...>>(Get());
}

template <typename T, typename... Fields>
typename StructValue<T, Fields...>::result_type
StructValue<T, Fields...>::Get() const
{
    return Traits::Construct(Base::Get());
}

template <typename T, typename... Fields>
void
StructValue<T, Fields...>::Set(const result_type& value)
{
    Base::Set(Traits::Read(value));
}

template <typename T, typename... Fields>
template <typename U>
bool
StructValue<T, Fields...>::GetAccessor(U& value) const
{
    value = U(Get());
    return true;
}

namespace internal
{

/**
 * @ingroup attribute_Struct
 *
 * Internal StructChecker implementation.
 *
 * @tparam T
 *         The StructValue type to check.
 */
template <typename T>
class StructChecker;

/**
 * @ingroup attribute_Struct
 *
 * StructChecker specialization for a StructValue.
 *
 * @tparam T
 *         The represented structure type.
 * @tparam Fields
 *         The fields represented by the StructValue.
 */
template <typename T, typename... Fields>
class StructChecker<StructValue<T, Fields...>> : public ns3::StructChecker
{
  public:
    /**
     * Construct a checker from the checkers for the individual fields.
     *
     * @tparam Ts
     *         The AttributeChecker types.
     * @param checkers The checkers for the individual fields.
     */
    template <typename... Ts>
    StructChecker(Ts... checkers)
        : m_checkers{checkers...}
    {
    }

    const std::vector<Ptr<const AttributeChecker>>& GetCheckers() const override
    {
        return m_checkers;
    }

    bool Check(const AttributeValue& value) const override
    {
        const auto structValue = dynamic_cast<const StructValue<T, Fields...>*>(&value);
        if (structValue == nullptr)
        {
            return false;
        }

        std::size_t n{0};
        return std::apply(
            [this, &n](const auto&... values) { return (m_checkers[n++]->Check(values) && ...); },
            structValue->GetValue());
    }

    std::string GetValueTypeName() const override
    {
        return "ns3::StructValue";
    }

    bool HasUnderlyingTypeInformation() const override
    {
        return false;
    }

    std::string GetUnderlyingTypeInformation() const override
    {
        return "";
    }

    Ptr<AttributeValue> Create() const override
    {
        return ns3::Create<StructValue<T, Fields...>>();
    }

    bool Copy(const AttributeValue& source, AttributeValue& destination) const override
    {
        const auto src = dynamic_cast<const StructValue<T, Fields...>*>(&source);
        auto dst = dynamic_cast<StructValue<T, Fields...>*>(&destination);
        if (src == nullptr || dst == nullptr)
        {
            return false;
        }
        *dst = *src;
        return true;
    }

  private:
    std::vector<Ptr<const AttributeChecker>> m_checkers; //!< Field checkers
};

} // namespace internal

template <typename T, typename... Ts>
    requires(sizeof...(Ts) == T::field_count)
Ptr<const AttributeChecker>
MakeStructChecker(Ts... checkers)
{
    return Create<internal::StructChecker<T>>(checkers...);
}

template <typename T, typename T1>
Ptr<const AttributeAccessor>
MakeStructAccessor(T1 a1)
{
    return MakeAccessorHelper<T>(a1);
}

template <typename T, typename T1, typename T2>
Ptr<const AttributeAccessor>
MakeStructAccessor(T1 a1, T2 a2)
{
    return MakeAccessorHelper<T>(a1, a2);
}

} // namespace ns3

#endif // STRUCT_H
