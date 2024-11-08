/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_DEFAULT_PROTECTION_MANAGER_H
#define WIFI_DEFAULT_PROTECTION_MANAGER_H

#include "wifi-protection-manager.h"

namespace ns3
{

class WifiTxParameters;
class WifiMpdu;
class WifiMacHeader;

/**
 * @ingroup wifi
 *
 * WifiDefaultProtectionManager is the default protection manager, which selects
 * the protection method for a frame based on its size.
 */
class WifiDefaultProtectionManager : public WifiProtectionManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    WifiDefaultProtectionManager();
    ~WifiDefaultProtectionManager() override;

    std::unique_ptr<WifiProtection> TryAddMpdu(Ptr<const WifiMpdu> mpdu,
                                               const WifiTxParameters& txParams) override;
    std::unique_ptr<WifiProtection> TryAggregateMsdu(Ptr<const WifiMpdu> msdu,
                                                     const WifiTxParameters& txParams) override;

  protected:
    /**
     * Select the protection method for a single PSDU.
     *
     * @param hdr the MAC header of the PSDU
     * @param txParams the TX parameters describing the PSDU
     * @return the selected protection method
     */
    virtual std::unique_ptr<WifiProtection> GetPsduProtection(
        const WifiMacHeader& hdr,
        const WifiTxParameters& txParams) const;

  private:
    /**
     * Calculate the protection method to use if the given MPDU is added to the
     * current DL MU PPDU (represented by the given TX parameters). If the computed
     * protection method is the same as the current one, a null pointer is
     * returned. Otherwise, the computed protection method is returned.
     * The TX width of the PPDU containing the MU-RTS is the same as the DL MU PPDU
     * being protected. Each non-AP station is solicited to transmit a CTS occupying a
     * bandwidth equal to the minimum between the TX width of the DL MU PPDU and the
     * maximum channel width supported by the non-AP station.
     *
     * @param mpdu the given MPDU
     * @param txParams the TX parameters describing the current DL MU PPDU
     * @return the new protection method or a null pointer if the protection method
     *         is unchanged
     */
    virtual std::unique_ptr<WifiProtection> TryAddMpduToMuPpdu(Ptr<const WifiMpdu> mpdu,
                                                               const WifiTxParameters& txParams);

    /**
     * Calculate the protection method for the UL MU transmission solicited by the given
     * Trigger Frame.
     * The TX width of the PPDU containing the MU-RTS is the same as the TB PPDUs being
     * solicited by the given Trigger Frame. Each non-AP station is solicited to transmit a CTS
     * occupying a bandwidth equal to the minimum between the TX width of the PPDU containing
     * the MU-RTS and the maximum channel width supported by the non-AP station.
     *
     * @param mpdu the given Trigger Frame
     * @param txParams the current TX parameters (just the TXVECTOR needs to be set)
     * @return the protection method for the UL MU transmission solicited by the given Trigger Frame
     */
    virtual std::unique_ptr<WifiProtection> TryUlMuTransmission(Ptr<const WifiMpdu> mpdu,
                                                                const WifiTxParameters& txParams);

    bool m_sendMuRts;           //!< true for sending an MU-RTS to protect DL MU PPDUs
    bool m_singleRtsPerTxop;    //!< true for using protection only once in a TXOP
    bool m_skipMuRtsBeforeBsrp; //!< whether to skip MU-RTS before BSRP TF
};

} // namespace ns3

#endif /* WIFI_DEFAULT_PROTECTION_MANAGER_H */
