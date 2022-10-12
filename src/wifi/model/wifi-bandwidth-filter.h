/*
 * Copyright (c) 2022 University of Washington
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
 */

#ifndef WIFI_BANDWIDTH_FILTER_H
#define WIFI_BANDWIDTH_FILTER_H

#include <ns3/spectrum-transmit-filter.h>

namespace ns3
{

class WifiBandwidthFilter : public SpectrumTransmitFilter
{
  public:
    WifiBandwidthFilter();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Ignore the signal being received if it is a Wi-Fi PPDU whose TX band
     *        (including guard bands) does not overlap the current operating channel.
     *
     * \param params the parameters of the signals being received
     * \param receiverPhy the SpectrumPhy of the receiver
     * \return whether the signal being received should be ignored
     */

    bool DoFilter(Ptr<const SpectrumSignalParameters> params,
                  Ptr<const SpectrumPhy> receiverPhy) override;
};

} // namespace ns3

#endif
