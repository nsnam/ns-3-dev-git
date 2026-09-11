/*
 * Copyright (c) 2015 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#ifndef TRACED_CALLBACK_TYPEDEF_TEST_H
#define TRACED_CALLBACK_TYPEDEF_TEST_H

#include "ns3/test.h"
#include "ns3/traced-callback.h"

#include <cstddef>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>

/**
 * @file
 * @ingroup core-tests
 *
 * Scaffolding shared by the per-module TracedCallback typedef test suites.
 *
 * Each module declaring TracedCallback typedefs owns a test suite deriving
 * from TracedCallbackTypedefTestCase, which uses the TRACED_CALLBACK_CHECK() and
 * TRACED_CALLBACK_DUPE() macros to verify that its typedefs are invoked with the right type and
 * number of arguments.
 */

namespace ns3
{

/**
 * @ingroup core-tests
 *
 * Number of arguments the last invoked TracedCallback passed to its sink.
 *
 * Since the sink function is outside the invoking class we can't use
 * the test macros directly.  Instead, we cache the argument count here,
 * then inspect it in TracedCallbackTypedefTestCase::Checker::Cleanup().
 */
inline std::size_t g_nArgs = 0;

/**
 * @ingroup core-tests
 *
 * Stringify the known TracedCallback type names.
 *
 * @tparam T @explicit The typedef name.
 * @param [in] N The number of arguments expected.
 * @returns The \c TracedCallback type name.
 */
template <typename T>
inline std::string
TracedCallbackTypeName(int N)
{
    return "unknown";
}

/**
 * @ingroup core-tests
 *
 * Declare the stringified name of a TracedCallback typedef.
 *
 * Must be used at namespace scope, inside namespace ns3.
 */
#define TRACED_CALLBACK_TYPENAME(T)                                                                \
    template <>                                                                                    \
    inline std::string TracedCallbackTypeName<T>(int N)                                            \
    {                                                                                              \
        std::stringstream ss;                                                                      \
        ss << #T << "(" << N << ")";                                                               \
        return ss.str();                                                                           \
    }

/**
 * @ingroup core-tests
 *
 * Sink functions.
 */
template <typename... Ts>
class TracedCbSink
{
  public:
    /**
     * @brief Sink function, called by a TracedCallback.
     * @tparam Ts parameters of the TracedCallback.
     */
    static void Sink(Ts...)
    {
        std::cout << "with " << sizeof...(Ts) << " args." << std::endl;
        g_nArgs = sizeof...(Ts);
    }
};

/**
 * @ingroup core-tests
 *
 * Base class for the per-module TracedCallback typedef test cases.
 *
 * This verifies that a TracedCallback is called with the right type
 * and number of arguments.
 */
class TracedCallbackTypedefTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param [in] name Name of the test case.
     */
    TracedCallbackTypedefTestCase(std::string name)
        : TestCase(name)
    {
    }

  protected:
    /** Typedefs which are identical to previously declared ones. */
    std::set<std::string> m_dupes;

    /** Callback checkers. */
    template <typename... Ts>
    class Checker
    {
        /// TracedCallback to be called.
        TracedCallback<Ts...> m_cb;

        /// Arguments of the TracedCallback.
        std::tuple<std::remove_pointer_t<std::remove_cvref_t<Ts>>...> m_items;

        /// Number of arguments of the TracedCallback.
        static constexpr std::size_t m_nItems = sizeof...(Ts);

      public:
        /**
         * Invoke a TracedCallback.
         *
         * @tparam U @explicit The TracedCallback typedef under test.
         */
        template <typename U>
        void Invoke()
        {
            U sink = TracedCbSink<Ts...>::Sink;
            Callback<void, Ts...> cb = MakeCallback(sink);

            std::cout << TracedCallbackTypeName<U>(m_nItems) << " invoked ";
            m_cb.ConnectWithoutContext(cb);
            std::apply(m_cb, m_items);
            Cleanup();
        }

      private:
        /**
         * Cleanup the test.
         */
        void Cleanup()
        {
            if (g_nArgs == 0)
            {
                std::cout << std::endl;
            }
            NS_ASSERT_MSG(g_nArgs == m_nItems,
                          "failed, g_nArgs: " << g_nArgs << " N: " << m_nItems);
            g_nArgs = 0;
        }
    };
};

/**
 * @ingroup core-tests
 *
 * Check the TracedCallback by calling its Invoke function.
 */
#define TRACED_CALLBACK_CHECK(U, ...) Checker<__VA_ARGS__>().Invoke<U>()

/**
 * @ingroup core-tests
 *
 * Check the TracedCallback duplicate by checking if it matches the TracedCallback
 * it is supposed to be equal to.
 */
#define TRACED_CALLBACK_DUPE(U, T1)                                                                \
    if (m_dupes.find(#U) == m_dupes.end())                                                         \
    {                                                                                              \
        NS_TEST_ASSERT_MSG_NE(0, 1, "expected to find " << #U << " in dupes.");                    \
    }                                                                                              \
    if (TracedCallbackTypeName<U>(0) == TracedCallbackTypeName<T1>(0))                             \
    {                                                                                              \
        std::cout << #U << " matches " << #T1 << std::endl;                                        \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        NS_TEST_ASSERT_MSG_EQ(                                                                     \
            TracedCallbackTypeName<U>(0),                                                          \
            TracedCallbackTypeName<T1>(0),                                                         \
            "the typedef "                                                                         \
                << #U << " used to match the typedef " << #T1                                      \
                << " but no longer does.  Please add a new TRACED_CALLBACK_CHECK call.");          \
    }

} // namespace ns3

#endif /* TRACED_CALLBACK_TYPEDEF_TEST_H */
