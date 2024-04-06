/*
 * Copyright (c) 2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef TIMER_IMPL_H
#define TIMER_IMPL_H

#include "fatal-error.h"
#include "simulator.h"

#include <type_traits>

/**
 * \file
 * \ingroup timer
 * \ingroup timerimpl
 * ns3::TimerImpl declaration and implementation.
 */

namespace ns3
{

namespace internal
{

/**
 * \ingroup timer
 * The timer implementation underlying Timer and Watchdog.
 */
class TimerImpl
{
  public:
    /** Destructor. */
    virtual ~TimerImpl()
    {
    }

    /**
     * Set the arguments to be used when invoking the expire function.
     *
     * \tparam Args \deduced Type template parameter pack
     * \param [in] args The arguments to pass to the invoked method
     */
    template <typename... Args>
    void SetArgs(Args... args);

    /**
     * Schedule the callback for a future time.
     *
     * \param [in] delay The amount of time until the timer expires.
     * \returns The scheduled EventId.
     */
    virtual EventId Schedule(const Time& delay) = 0;
    /** Invoke the expire function. */
    virtual void Invoke() = 0;
};

/********************************************************************
 *  Implementation of TimerImpl implementation functions.
 ********************************************************************/

/**
 * \ingroup timer
 * \defgroup timerimpl TimerImpl Implementation
 * @{
 */
/** TimerImpl specialization class for varying numbers of arguments. */
template <typename... Args>
struct TimerImplX : public TimerImpl
{
    /**
     * Bind the arguments to be used when the callback function is invoked.
     *
     * \param [in] args The arguments to pass to the invoked method.
     */
    virtual void SetArguments(Args... args) = 0;
};

/**
 * Make a TimerImpl from a function pointer taking varying numbers of arguments.
 *
 * \tparam U \deduced Return type of the callback function.
 * \tparam Ts \deduced Argument types of the callback function.
 * \returns The TimerImpl.
 */
template <typename U, typename... Ts>
TimerImpl*
MakeTimerImpl(U(fn)(Ts...))
{
    struct FnTimerImpl : public TimerImplX<const std::remove_cvref_t<Ts>&...>
    {
        FnTimerImpl(void (*fn)(Ts...))
            : m_fn(fn)
        {
        }

        void SetArguments(const std::remove_cvref_t<Ts>&... args) override
        {
            m_arguments = std::tuple(args...);
        }

        EventId Schedule(const Time& delay) override
        {
            return std::apply(
                [&, this](Ts... args) { return Simulator::Schedule(delay, m_fn, args...); },
                m_arguments);
        }

        void Invoke() override
        {
            std::apply([this](Ts... args) { (m_fn)(args...); }, m_arguments);
        }

        decltype(fn) m_fn;
        std::tuple<std::remove_cvref_t<Ts>...> m_arguments;
    }* function = new FnTimerImpl(fn);

    return function;
}

/**
 * Make a TimerImpl from a class method pointer taking
 * a varying number of arguments.
 *
 * \tparam OBJ_PTR \deduced Class type.
 * \tparam U \deduced Class method function return type.
 * \tparam V \deduced Class method function class type.
 * \tparam Ts \deduced Class method function argument types.
 * \param [in] memPtr Class method to invoke when the timer expires.
 * \param [in] objPtr Object instance pointer.
 * \returns The TimerImpl.
 */
template <typename OBJ_PTR, typename U, typename V, typename... Ts>
TimerImpl*
MakeTimerImpl(U (V::*memPtr)(Ts...), OBJ_PTR objPtr)
{
    struct MemFnTimerImpl : public TimerImplX<const std::remove_cvref_t<Ts>&...>
    {
        MemFnTimerImpl(decltype(memPtr) memPtr, OBJ_PTR objPtr)
            : m_memPtr(std::bind_front(memPtr, objPtr))
        {
        }

        void SetArguments(const std::remove_cvref_t<Ts>&... args) override
        {
            m_arguments = std::tuple(args...);
        }

        EventId Schedule(const Time& delay) override
        {
            return std::apply(
                [&, this](Ts... args) {
                    return Simulator::Schedule(delay, std::bind(m_memPtr, args...));
                },
                m_arguments);
        }

        void Invoke() override
        {
            std::apply(m_memPtr, m_arguments);
        }

        std::function<U(Ts...)> m_memPtr;
        std::tuple<std::remove_cvref_t<Ts>...> m_arguments;
    }* function = new MemFnTimerImpl(memPtr, objPtr);

    return function;
}

/**@}*/ // \ingroup timer

/********************************************************************
 *  Implementation of TimerImpl itself.
 ********************************************************************/

template <typename... Args>
void
TimerImpl::SetArgs(Args... args)
{
    using TimerImplBase = TimerImplX<const std::remove_cvref_t<Args>&...>;
    auto impl = dynamic_cast<TimerImplBase*>(this);
    if (impl == nullptr)
    {
        NS_FATAL_ERROR("You tried to set Timer arguments incompatible with its function.");
        return;
    }
    impl->SetArguments(args...);
}

} // namespace internal

} // namespace ns3

#endif /* TIMER_IMPL_H */
