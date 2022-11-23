/*
 * Copyright (c) 2005 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage.inria.fr>
 */

#ifndef SYSTEM_WALL_CLOCK_MS_H
#define SYSTEM_WALL_CLOCK_MS_H

#include <stdint.h>

/**
 * \file
 * \ingroup system
 * ns3::SystemWallClockMs declaration.
 */

namespace ns3
{

/**
 * \ingroup system
 * \brief Measure elapsed wall clock time in milliseconds.
 */
class SystemWallClockMs
{
  public:
    SystemWallClockMs();
    ~SystemWallClockMs();

    /**
     * Start a measure.
     */
    void Start();
    /**
     * \brief Stop measuring the time since Start() was called.
     * \returns the measured elapsed wall clock time (in milliseconds) since
     *          Start() was invoked.
     *
     * It is possible to start a new measurement with Start() after
     * this method returns.
     *
     * Returns \c int64_t to avoid dependency on \c clock_t in ns-3 code.
     */
    int64_t End();

    /**
     * \returns the measured elapsed wall clock time (in milliseconds) since
     *          Start() was invoked.
     *
     * Returns \c int64_t to avoid dependency on \c clock_t in ns-3 code.
     */
    int64_t GetElapsedReal() const;
    /**
     * \returns the measured elapsed 'user' wall clock time (in milliseconds)
     *          since Start() was invoked.
     *
     * Returns \c int64_t to avoid dependency on \c clock_t in ns-3 code.
     */
    int64_t GetElapsedUser() const;
    /**
     * \returns the measured elapsed 'system' wall clock time (in milliseconds)
     *          since Start() was invoked.
     *
     * Returns \c int64_t to avoid dependency on \c clock_t in ns-3 code.
     */
    int64_t GetElapsedSystem() const;

  private:
    class SystemWallClockMsPrivate* m_priv; //!< The implementation.
};

} // namespace ns3

#endif /* SYSTEM_WALL_CLOCK_MS_H */
