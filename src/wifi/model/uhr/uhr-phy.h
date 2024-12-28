/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef UHR_PHY_H
#define UHR_PHY_H

#include "ns3/eht-phy.h"

/**
 * @file
 * @ingroup wifi
 * Declaration of ns3::UhrPhy class.
 */

namespace ns3
{

/**
 * This defines the BSS membership value for UHR PHY.
 */
#define UHR_PHY 120 // FIXME: not defined yet

/**
 * @brief PHY entity for UHR (11bn)
 * @ingroup wifi
 *
 * UHR PHY is based on EHT PHY.
 */
class UhrPhy : public EhtPhy
{
  public:
    /**
     * Constructor for UHR PHY
     *
     * @param buildModeList flag used to add UHR modes to list (disabled
     *                      by child classes to only add child classes' modes)
     */
    UhrPhy(bool buildModeList = true);
    /**
     * Destructor for UHR PHY
     */
    ~UhrPhy() override;

    const PpduFormats& GetPpduFormats() const override;
    Time GetDuration(WifiPpduField field, const WifiTxVector& txVector) const override;
    Ptr<WifiPpdu> BuildPpdu(const WifiConstPsduMap& psdus,
                            const WifiTxVector& txVector,
                            Time ppduDuration) override;
    Time CalculateNonHeDurationForHeMu(const WifiTxVector& txVector) const override;

    /**
     * Initialize all UHR modes.
     */
    static void InitializeModes();

    /**
     * Return the UHR MCS corresponding to
     * the provided index.
     *
     * @param index the index of the MCS
     * @return an UHR MCS
     */
    static WifiMode GetUhrMcs(uint8_t index);

    /**
     * Return MCS 0 from UHR MCS values.
     *
     * @return MCS 0 from UHR MCS values
     */
    static WifiMode GetUhrMcs0();
    /**
     * Return MCS 1 from UHR MCS values.
     *
     * @return MCS 1 from UHR MCS values
     */
    static WifiMode GetUhrMcs1();
    /**
     * Return MCS 2 from UHR MCS values.
     *
     * @return MCS 2 from UHR MCS values
     */
    static WifiMode GetUhrMcs2();
    /**
     * Return MCS 3 from UHR MCS values.
     *
     * @return MCS 3 from UHR MCS values
     */
    static WifiMode GetUhrMcs3();
    /**
     * Return MCS 4 from UHR MCS values.
     *
     * @return MCS 4 from UHR MCS values
     */
    static WifiMode GetUhrMcs4();
    /**
     * Return MCS 5 from UHR MCS values.
     *
     * @return MCS 5 from UHR MCS values
     */
    static WifiMode GetUhrMcs5();
    /**
     * Return MCS 6 from UHR MCS values.
     *
     * @return MCS 6 from UHR MCS values
     */
    static WifiMode GetUhrMcs6();
    /**
     * Return MCS 7 from UHR MCS values.
     *
     * @return MCS 7 from UHR MCS values
     */
    static WifiMode GetUhrMcs7();
    /**
     * Return MCS 8 from UHR MCS values.
     *
     * @return MCS 8 from UHR MCS values
     */
    static WifiMode GetUhrMcs8();
    /**
     * Return MCS 9 from UHR MCS values.
     *
     * @return MCS 9 from UHR MCS values
     */
    static WifiMode GetUhrMcs9();
    /**
     * Return MCS 10 from UHR MCS values.
     *
     * @return MCS 10 from UHR MCS values
     */
    static WifiMode GetUhrMcs10();
    /**
     * Return MCS 11 from UHR MCS values.
     *
     * @return MCS 11 from UHR MCS values
     */
    static WifiMode GetUhrMcs11();
    /**
     * Return MCS 12 from UHR MCS values.
     *
     * @return MCS 12 from UHR MCS values
     */
    static WifiMode GetUhrMcs12();
    /**
     * Return MCS 13 from UHR MCS values.
     *
     * @return MCS 13 from UHR MCS values
     */
    static WifiMode GetUhrMcs13();

  protected:
    void BuildModeList() override;
    WifiMode GetSigMode(WifiPpduField field, const WifiTxVector& txVector) const override;
    PhyFieldRxStatus DoEndReceiveField(WifiPpduField field, Ptr<Event> event) override;
    PhyFieldRxStatus ProcessSig(Ptr<Event> event,
                                PhyFieldRxStatus status,
                                WifiPpduField field) override;
    WifiPhyRxfailureReason GetFailureReason(WifiPpduField field) const override;

    /**
     * Create and return the UHR MCS corresponding to
     * the provided index.
     * This method binds all the callbacks used by WifiMode.
     *
     * @param index the index of the MCS
     * @return an UHR MCS
     */
    static WifiMode CreateUhrMcs(uint8_t index);

    static const PpduFormats m_uhrPpduFormats; //!< UHR PPDU formats
};

} // namespace ns3

#endif /* UHR_PHY_H */
