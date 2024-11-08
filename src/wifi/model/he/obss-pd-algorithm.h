/*
 * Copyright (c) 2018 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef OBSS_PD_ALGORITHM_H
#define OBSS_PD_ALGORITHM_H

#include "he-configuration.h"

#include "ns3/object.h"
#include "ns3/traced-callback.h"
#include "ns3/wifi-units.h"

namespace ns3
{

struct HeSigAParameters;

class WifiNetDevice;

/**
 * @brief OBSS PD algorithm interface
 * @ingroup wifi
 *
 * This object provides the interface for all OBSS_PD algorithms
 * and is designed to be subclassed.
 *
 * OBSS_PD stands for Overlapping Basic Service Set Preamble-Detection.
 * OBSS_PD is an 802.11ax feature that allows a STA, under specific
 * conditions, to ignore an inter-BSS PPDU.
 */
class ObssPdAlgorithm : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Connect the WifiNetDevice and setup eventual callbacks.
     *
     * @param device the WifiNetDevice
     */
    virtual void ConnectWifiNetDevice(const Ptr<WifiNetDevice> device);

    /**
     * Reset PHY to IDLE.
     * @param params HeSigAParameters causing PHY reset
     */
    void ResetPhy(HeSigAParameters params);

    /**
     * @param params the HE-SIG-A parameters
     *
     * Evaluate the receipt of HE-SIG-A.
     */
    virtual void ReceiveHeSigA(HeSigAParameters params) = 0;

    /**
     * TracedCallback signature for OBSS_PD reset events.
     *
     * @param [in] bssColor The BSS color of frame triggering the reset
     * @param [in] rssiDbm The RSSI (dBm) of frame triggering the reset
     * @param [in] powerRestricted Whether a TX power restriction is triggered
     * @param [in] txPowerMaxDbmSiso The SISO TX power restricted level (dBm)
     * @param [in] txPowerMaxDbmMimo The MIMO TX power restricted level (dBm)
     */
    typedef void (*ResetTracedCallback)(uint8_t bssColor,
                                        double rssiDbm,
                                        bool powerRestricted,
                                        double txPowerMaxDbmSiso,
                                        double txPowerMaxDbmMimo);

    /**
     * @param level the current OBSS PD level
     */
    void SetObssPdLevel(dBm_u level);
    /**
     * @return the current OBSS PD level
     */
    dBm_u GetObssPdLevel() const;

  protected:
    void DoDispose() override;

    Ptr<WifiNetDevice> m_device; ///< Pointer to the WifiNetDevice

  private:
    dBm_u m_obssPdLevel;    ///< Current OBSS PD level
    dBm_u m_obssPdLevelMin; ///< Minimum OBSS PD level
    dBm_u m_obssPdLevelMax; ///< Maximum OBSS PD level
    dBm_u m_txPowerRefSiso; ///< SISO reference TX power level
    dBm_u m_txPowerRefMimo; ///< MIMO reference TX power level

    /**
     * TracedCallback signature for PHY reset events.
     */
    TracedCallback<uint8_t, double, bool, double, double> m_resetEvent;
};

} // namespace ns3

#endif /* OBSS_PD_ALGORITHM_H */
