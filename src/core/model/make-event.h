/*
 * Copyright (c) 2008 INRIA
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
 */

#ifndef MAKE_EVENT_H
#define MAKE_EVENT_H

#include "warnings.h"

#include <functional>
#include <tuple>
#include <type_traits>

/**
 * \file
 * \ingroup events
 * ns3::MakeEvent function declarations and template implementation.
 */

namespace ns3
{

class EventImpl;

/**
 * \ingroup events
 * Make an EventImpl from class method members which take
 * varying numbers of arguments.
 *
 * \tparam MEM \deduced The class method function signature.
 * \tparam OBJ \deduced The class type holding the method.
 * \tparam Ts \deduced Type template parameter pack.
 * \param [in] mem_ptr Class method member function pointer
 * \param [in] obj Class instance.
 * \param [in] args Arguments to be bound to the underlying function.
 * \returns The constructed EventImpl.
 */
template <typename MEM, typename OBJ, typename... Ts>
std::enable_if_t<std::is_member_pointer_v<MEM>, EventImpl*> MakeEvent(MEM mem_ptr,
                                                                      OBJ obj,
                                                                      Ts... args);

/**
 * \ingroup events
 * \defgroup makeeventfnptr MakeEvent from Function Pointers and Lambdas.
 *
 * Create EventImpl instances from function pointers or lambdas which take
 * varying numbers of arguments.
 *
 * @{
 */
/**
 * Make an EventImpl from a function pointer taking varying numbers
 * of arguments.
 *
 * \tparam Us \deduced Formal types of the arguments to the function.
 * \tparam Ts \deduced Actual types of the arguments to the function.
 * \param [in] f The function pointer.
 * \param [in] args Arguments to be bound to the function.
 * \returns The constructed EventImpl.
 */
template <typename... Us, typename... Ts>
EventImpl* MakeEvent(void (*f)(Us...), Ts... args);

/**
 * Make an EventImpl from a lambda.
 *
 * \param [in] function The lambda
 * \returns The constructed EventImpl.
 */
template <typename T>
EventImpl* MakeEvent(T function);

/**@}*/

} // namespace ns3

/********************************************************************
 *  Implementation of the templates declared above.
 ********************************************************************/

#include "event-impl.h"

namespace ns3
{

namespace internal
{

/**
 * \ingroup events
 * Helper for the MakeEvent functions which take a class method.
 *
 * This helper converts a pointer to a reference.
 *
 * This is the generic template declaration (with empty body).
 *
 * \tparam T \explicit The class type.
 */
template <typename T>
struct EventMemberImplObjTraits;

/**
 * \ingroup events
 * Helper for the MakeEvent functions which take a class method.
 *
 * This helper converts a pointer to a reference.
 *
 * This is the specialization for pointer types.
 *
 * \tparam T \explicit The class type.
 */
template <typename T>
struct EventMemberImplObjTraits<T*>
{
    /**
     * \param [in] p Object pointer.
     * \return A reference to the object pointed to by p.
     */
    static T& GetReference(T* p)
    {
        return *p;
    }
};

} // namespace internal

template <typename MEM, typename OBJ, typename... Ts>
std::enable_if_t<std::is_member_pointer_v<MEM>, EventImpl*>
MakeEvent(MEM mem_ptr, OBJ obj, Ts... args)
{
    class EventMemberImpl : public EventImpl
    {
      public:
        EventMemberImpl() = delete;

        EventMemberImpl(OBJ obj, MEM function, Ts... args)
            : m_function(std::bind(function, obj, args...))
        {
        }

      protected:
        ~EventMemberImpl() override
        {
        }

      private:
        void Notify() override
        {
            m_function();
        }

        std::function<void()> m_function;
    }* ev = new EventMemberImpl(obj, mem_ptr, args...);

    return ev;
}

template <typename... Us, typename... Ts>
EventImpl*
MakeEvent(void (*f)(Us...), Ts... args)
{
    class EventFunctionImpl : public EventImpl
    {
      public:
        EventFunctionImpl(void (*function)(Us...), Ts... args)
            : m_function(function),
              m_arguments(args...)
        {
        }

      protected:
        ~EventFunctionImpl() override
        {
        }

      private:
        void Notify() override
        {
            std::apply([this](Ts... args) { (*m_function)(args...); }, m_arguments);
        }

        void (*m_function)(Us...);
        std::tuple<std::remove_reference_t<Ts>...> m_arguments;
    }* ev = new EventFunctionImpl(f, args...);

    return ev;
}

template <typename T>
EventImpl*
MakeEvent(T function)
{
    class EventImplFunctional : public EventImpl
    {
      public:
        EventImplFunctional(T function)
            : m_function(function)
        {
        }

        ~EventImplFunctional() override
        {
        }

      private:
        void Notify() override
        {
            m_function();
        }

        T m_function;
    }* ev = new EventImplFunctional(function);

    return ev;
}

} // namespace ns3

#endif /* MAKE_EVENT_H */
