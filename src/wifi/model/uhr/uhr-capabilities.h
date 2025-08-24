/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef UHR_CAPABILITIES_H
#define UHR_CAPABILITIES_H

#include "ns3/wifi-information-element.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * The IEEE 802.11bn UHR Capabilities
 */
class UhrCapabilities : public WifiInformationElement
{
  public:
    /**
     * UHR MAC Capabilities Info subfield (section 9.4.2.aa2.2 of 802.11bn/D1.0)
     */
    struct UhrMacCapabilities
    {
        uint8_t dpsSupport : 1 {0};                 //!< DPS Support
        uint8_t dpsAssistingSupport : 1 {0};        //!< DPS Assisting Support
        uint8_t dpsApStaticHcmSupport : 1 {0};      //!< DPS AP Static HCM Support
        uint8_t mlPowerManagement : 1 {0};          //!< Multi-Link Power Management
        uint8_t npcaSupport : 1 {0};                //!< NPCA Support
        uint8_t enhancedBsrSupport : 1 {0};         //!< Enhanced BSR Support
        uint8_t additionalMappedTidSupport : 1 {0}; //!< Additional Mapped TID Support
        uint8_t eotspSupport : 1 {0};               //!< EOTSP support
        uint8_t dsoSupport : 1 {0};                 //!< DSO support
        uint8_t pEdcaSupport : 1 {0};               //!< P-EDCA support
        uint8_t dbeSupport : 1 {0};                 //!< DBE support
        uint8_t ulLliSupport : 1 {0};               //!< UL LLI support
        uint8_t p2pLliSupport : 1 {0};              //!< Peer-to-Peer LLI support
        uint8_t puoSupport : 1 {0};                 //!< PUO support
        uint8_t apPuoSupport : 1 {0};               //!< AP PUO support
        uint8_t duoSupport : 1 {0};                 //!< DUO support
        uint8_t omCtrlUlMuDataDisableRxSupport : 1 {
            0};                                 //!< OM Control UL MU Data Disable RX Support
        uint8_t aomSupport : 1 {0};             //!< AOM support
        uint8_t ifcsLocationSupport : 1 {0};    //!< IFCS Location support
        uint8_t uhrTrsSupport : 1 {0};          //!< UHR TRS support
        uint8_t txspgSupport : 1 {0};           //!< TXSPG support
        uint8_t txspgTxopReturnSupport : 1 {0}; //!< TXOP Return Support In TXSPG
        uint8_t uhrOpModeAndParamsUpdateTimeout : 4 {
            0}; //!< UHR Operating Mode And Parameters Update Timeout
        uint8_t paramUpdateAdvNotificationInterval : 3 {
            0}; //!< Parameter Update Adv Notification Interval
        uint8_t updateIndicationTimIntervals : 5 {0}; //!< Update Indication In TIM Interval
        uint8_t boundedEss : 1 {0};                   //!< Bounded ESS
        uint8_t btmAssurance : 1 {0};                 //!< BTM Assurance

        /**
         * Get the size of the serialized UHR MAC capabilities subfield
         *
         * @return the size of the serialized UHR MAC capabilities subfield
         */
        uint16_t GetSize() const;

        /**
         * Serialize the UHR MAC capabilities subfield
         *
         * @param start iterator pointing to where the UHR MAC capabilities subfield should be
         * written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the UHR MAC capabilities subfield
         *
         * @param start iterator pointing to where the UHR MAC capabilities subfield should be read
         * from
         * @return the number of bytes read
         */
        uint16_t Deserialize(Buffer::Iterator start);
    };

    /**
     * UHR PHY Capabilities Info subfield.
     */
    struct UhrPhyCapabilities
    {
        /**
         * Get the size of the serialized UHR PHY capabilities subfield
         *
         * @return the size of the serialized UHR PHY capabilities subfield
         */
        uint16_t GetSize() const;

        /**
         * Serialize the UHR PHY capabilities subfield
         *
         * @param start iterator pointing to where the UHR PHY capabilities subfield should be
         * written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the UHR PHY capabilities subfield
         *
         * @param start iterator pointing to where the UHR PHY capabilities subfield should be read
         * from
         * @return the number of bytes read
         */
        uint16_t Deserialize(Buffer::Iterator start);
    };

    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;
    void Print(std::ostream& os) const override;

    UhrMacCapabilities m_macCapabilities; //!< UHR MAC Capabilities Info subfield
    UhrPhyCapabilities m_phyCapabilities; //!< UHR PHY Capabilities Info subfield

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

} // namespace ns3

#endif /* UHR_CAPABILITIES_H */
