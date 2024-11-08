/*
 * Copyright (c) 2010 NICTA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Quincy Tse <quincy.tse@nicta.com.au>
 */
#include "fatal-impl.h"

#include "log.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <list>

#ifdef __WIN32__
struct sigaction
{
    void (*sa_handler)(int);
    int sa_flags;
    int sa_mask;
};

int
sigaction(int sig, struct sigaction* action, struct sigaction* old)
{
    if (sig == -1)
    {
        return 0;
    }
    if (old == nullptr)
    {
        if (signal(sig, SIG_DFL) == SIG_ERR)
        {
            return -1;
        }
    }
    else
    {
        if (signal(sig, action->sa_handler) == SIG_ERR)
        {
            return -1;
        }
    }
    return 0;
}
#endif

/**
 * @file
 * @ingroup fatalimpl
 * @brief ns3::FatalImpl::RegisterStream(), ns3::FatalImpl::UnregisterStream(),
 * and ns3::FatalImpl::FlushStreams() implementations;
 * see Implementation note!
 *
 * @note Implementation.
 *
 * The singleton pattern we use here is tricky because we have to ensure:
 *
 *   - RegisterStream() succeeds, even if called before \c main() enters and
 *     before any constructor run in this file.
 *
 *   - UnregisterStream() succeeds, whether or not FlushStreams() has
 *     been called.
 *
 *   - All memory allocated with \c new is deleted properly before program exit.
 *
 * This is why we go through all the painful hoops below.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FatalImpl");

namespace FatalImpl
{

/**
 * @ingroup fatalimpl
 * Unnamed namespace for fatal streams memory implementation
 * and signal handler.
 */
namespace
{

/**
 * @ingroup fatalimpl
 * @brief Static variable pointing to the list of output streams
 * to be flushed on fatal errors.
 *
 * @returns The address of the static pointer.
 */
std::list<std::ostream*>**
PeekStreamList()
{
    NS_LOG_FUNCTION_NOARGS();
    static std::list<std::ostream*>* streams = nullptr;
    return &streams;
}

/**
 * @ingroup fatalimpl
 * @brief Get the stream list, initializing it if necessary.
 *
 * @returns The stream list.
 */
std::list<std::ostream*>*
GetStreamList()
{
    NS_LOG_FUNCTION_NOARGS();
    std::list<std::ostream*>** pstreams = PeekStreamList();
    if (*pstreams == nullptr)
    {
        *pstreams = new std::list<std::ostream*>();
    }
    return *pstreams;
}

} // unnamed namespace

void
RegisterStream(std::ostream* stream)
{
    NS_LOG_FUNCTION(stream);
    GetStreamList()->push_back(stream);
}

void
UnregisterStream(std::ostream* stream)
{
    NS_LOG_FUNCTION(stream);
    std::list<std::ostream*>** pl = PeekStreamList();
    if (*pl == nullptr)
    {
        return;
    }
    (*pl)->remove(stream);
    if ((*pl)->empty())
    {
        delete *pl;
        *pl = nullptr;
    }
}

/**
 * @ingroup fatalimpl
 * Unnamed namespace for fatal streams signal handler.
 *
 * This is private to the fatal implementation.
 */
namespace
{

/**
 * @ingroup fatalimpl
 * @brief Overrides normal SIGSEGV handler once the HandleTerminate
 * function is run.
 *
 * This is private to the fatal implementation.
 *
 * @param [in] sig The signal condition.
 */
void
sigHandler(int sig)
{
    NS_LOG_FUNCTION(sig);
    FlushStreams();
    std::abort();
}
} // unnamed namespace

void
FlushStreams()
{
    NS_LOG_FUNCTION_NOARGS();
    std::list<std::ostream*>** pl = PeekStreamList();
    if (*pl == nullptr)
    {
        return;
    }

    /* Override default SIGSEGV handler - will flush subsequent
     * streams even if one of the stream pointers is bad.
     * The SIGSEGV override should only be active for the
     * duration of this function. */
    struct sigaction hdl;
    hdl.sa_handler = sigHandler;
    sigaction(SIGSEGV, &hdl, nullptr);

    std::list<std::ostream*>* l = *pl;

    /* Need to do it this way in case any of the ostream* causes SIGSEGV */
    while (!l->empty())
    {
        std::ostream* s(l->front());
        l->pop_front();
        s->flush();
    }

    /* Restore default SIGSEGV handler (Not that it matters anyway) */
    hdl.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &hdl, nullptr);

    /* Flush all opened FILE* */
    std::fflush(nullptr);

    /* Flush stdandard streams - shouldn't be required (except for clog) */
    std::cout.flush();
    std::cerr.flush();
    std::clog.flush();

    delete l;
    *pl = nullptr;
}

} // namespace FatalImpl

} // namespace ns3
