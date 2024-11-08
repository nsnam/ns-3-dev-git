/*
 * Copyright (c) 2018 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#ifndef HE_CONFIGURATION_H
#define HE_CONFIGURATION_H

#include "ns3/deprecated.h"
#include "ns3/nstime.h"
#include "ns3/object.h"

namespace ns3
{

/**
 * @brief HE configuration
 * @ingroup wifi
 *
 * This object stores HE configuration information, for use in modifying
 * AP or STA behavior and for constructing HE-related information elements.
 *
 */
class HeConfiguration : public Object
{
  public:
    HeConfiguration();

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @param guardInterval the supported HE guard interval
     */
    void SetGuardInterval(Time guardInterval);

    /**
     * @return the supported HE guard interval
     */
    Time GetGuardInterval() const;

    /**
     * @param bssColor the BSS color
     */
    NS_DEPRECATED_3_44("Set the m_bssColor member variable instead")
    void SetBssColor(uint8_t bssColor);

    /**
     * @return the BSS color
     */
    NS_DEPRECATED_3_44("Get the m_bssColor member variable instead")
    uint8_t GetBssColor() const;

    /**
     * @param maxTbPpduDelay the maximum TB PPDU delay
     */
    NS_DEPRECATED_3_44("Set the m_maxTbPpduDelay member variable instead")
    void SetMaxTbPpduDelay(Time maxTbPpduDelay);

    /**
     * @return the maximum TB PPDU delay
     */
    NS_DEPRECATED_3_44("Get the m_maxTbPpduDelay member variable instead")
    Time GetMaxTbPpduDelay() const;

    uint8_t m_bssColor;    //!< BSS color
    Time m_maxTbPpduDelay; //!< Max TB PPDU delay
    uint8_t m_muBeAifsn;   //!< AIFSN for BE in MU EDCA Parameter Set
    uint8_t m_muBkAifsn;   //!< AIFSN for BK in MU EDCA Parameter Set
    uint8_t m_muViAifsn;   //!< AIFSN for VI in MU EDCA Parameter Set
    uint8_t m_muVoAifsn;   //!< AIFSN for VO in MU EDCA Parameter Set
    uint16_t m_muBeCwMin;  //!< CWmin for BE in MU EDCA Parameter Set
    uint16_t m_muBkCwMin;  //!< CWmin for BK in MU EDCA Parameter Set
    uint16_t m_muViCwMin;  //!< CWmin for VI in MU EDCA Parameter Set
    uint16_t m_muVoCwMin;  //!< CWmin for VO in MU EDCA Parameter Set
    uint16_t m_muBeCwMax;  //!< CWmax for BE in MU EDCA Parameter Set
    uint16_t m_muBkCwMax;  //!< CWmax for BK in MU EDCA Parameter Set
    uint16_t m_muViCwMax;  //!< CWmax for VI in MU EDCA Parameter Set
    uint16_t m_muVoCwMax;  //!< CWmax for VO in MU EDCA Parameter Set
    Time m_beMuEdcaTimer;  //!< Timer for BE in MU EDCA Parameter Set
    Time m_bkMuEdcaTimer;  //!< Timer for BK in MU EDCA Parameter Set
    Time m_viMuEdcaTimer;  //!< Timer for VI in MU EDCA Parameter Set
    Time m_voMuEdcaTimer;  //!< Timer for VO in MU EDCA Parameter Set

  private:
    Time m_guardInterval; //!< Supported HE guard interval
};

} // namespace ns3

#endif /* HE_CONFIGURATION_H */
