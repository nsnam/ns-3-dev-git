/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef ADDBA_EXTENSION_H
#define ADDBA_EXTENSION_H

#include "wifi-information-element.h"

namespace ns3
{

/**
 * @ingroup wifi
 *
 * The IEEE 802.11 ADDBA Extension Element (Sec. 9.4.2.139 of 802.11-2020)
 */
class AddbaExtension : public WifiInformationElement
{
  public:
    AddbaExtension() = default;

    /**
     * ADDBA Extended Parameter Set
     */
    struct ExtParamSet
    {
        uint8_t noFragment : 1;    //!< reserved when transmitted by HE STA to HE STA
        uint8_t heFragmentOp : 2;  //!< indicates level of HE dynamic fragmentation (unsupported)
        uint8_t : 2;               //!< reserved
        uint8_t extBufferSize : 3; //!< extended buffer size
    };

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;
    void Print(std::ostream& os) const override;

    ExtParamSet m_extParamSet{}; //!< ADDBA Extended Parameter Set field

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

} // namespace ns3

#endif /* ADDBA_EXTENSION_H */
