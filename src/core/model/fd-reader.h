/*
 * Copyright (c) 2010 The Boeing Company
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
 * Author: Tom Goff <thomas.goff@boeing.com>
 */

#ifndef FD_READER_H
#define FD_READER_H

#include "callback.h"
#include "event-id.h"

#include <cstdint>
#include <thread>

#ifdef __WIN32__
#include <BaseTsd.h>

/**
 * Signed version of size_t
 */
typedef SSIZE_T ssize_t;
#endif

/**
 * \file
 * \ingroup system
 * ns3::FdReader declaration.
 */

namespace ns3
{

/**
 * \ingroup system
 * \brief A class that asynchronously reads from a file descriptor.
 *
 * This class can be used to start a system thread that reads from a
 * given file descriptor and invokes a given callback when data is
 * received.  This class handles thread management automatically but
 * the \pname{DoRead()} method must be implemented by a subclass.
 */
class FdReader : public SimpleRefCount<FdReader>
{
  public:
    /** Constructor. */
    FdReader();
    /** Destructor. */
    virtual ~FdReader();

    /**
     * Start a new read thread.
     *
     * \param [in] fd A valid file descriptor open for reading.
     *
     * \param [in] readCallback A callback to invoke when new data is
     * available.
     */
    void Start(int fd, Callback<void, uint8_t*, ssize_t> readCallback);

    /**
     * Stop the read thread and reset internal state.  This does not
     * close the file descriptor used for reading.
     */
    void Stop();

#ifdef __WIN32__
    /**
     * Keeps track if the Winsock library has been initialized.
     */
    static bool winsock_initialized;
#endif
  protected:
    /**
     * \brief A structure representing data read.
     */
    struct Data
    {
        /** Default constructor, with null buffer and zero length. */
        Data()
            : m_buf(nullptr),
              m_len(0)
        {
        }

        /**
         * Construct from a buffer of a given length.
         *
         * \param [in] buf The buffer.
         * \param [in] len The size of the buffer, in bytes.
         */
        Data(uint8_t* buf, ssize_t len)
            : m_buf(buf),
              m_len(len)
        {
        }

        /** The read data buffer. */
        uint8_t* m_buf;
        /** The size of the read data buffer, in bytes. */
        ssize_t m_len;
    };

    /**
     * \brief The read implementation.
     *
     * The value of \pname{m_len} returned controls further processing.  The
     * callback function is only invoked when \pname{m_len} is positive; any
     * data read is not processed when \pname{m_len} is negative; reading
     * stops when \pname{m_len} is zero.
     *
     * The management of memory associated with \pname{m_buf} must be
     * compatible with the read callback.
     *
     * \return A structure representing what was read.
     */
    virtual FdReader::Data DoRead() = 0;

    /**
     * \brief The file descriptor to read from.
     */
    int m_fd;

  private:
    /** The asynchronous function which performs the read. */
    void Run();
    /** Event handler scheduled for destroy time to halt the thread. */
    void DestroyEvent();

    /** The main thread callback function to invoke when we have data. */
    Callback<void, uint8_t*, ssize_t> m_readCallback;

    /** The thread doing the read, created and launched by Start(). */
    std::thread m_readThread;

    /** Pipe used to signal events between threads. */
    int m_evpipe[2];
    /** Signal the read thread to stop. */
    bool m_stop;

    /**
     * The event scheduled for destroy time which will invoke DestroyEvent
     * and halt the thread.
     */
    EventId m_destroyEvent;
};

} // namespace ns3

#endif /* FD_READER_H */
