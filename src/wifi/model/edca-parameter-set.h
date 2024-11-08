/*
 * Copyright (c) 2016 Sébastien Deronne
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef EDCA_PARAMETER_SET_H
#define EDCA_PARAMETER_SET_H

#include "wifi-information-element.h"

namespace ns3
{

/**
 * @brief The EDCA Parameter Set
 * @ingroup wifi
 *
 * This class knows how to serialise and deserialise the EDCA Parameter Set.
 */
class EdcaParameterSet : public WifiInformationElement
{
  public:
    EdcaParameterSet();

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;

    /**
     * Set the QoS Info field in the EdcaParameterSet information element.
     *
     * @param qosInfo the QoS Info field in the EdcaParameterSet information element
     */
    void SetQosInfo(uint8_t qosInfo);
    /**
     * Set the AC_BE AIFSN field in the EdcaParameterSet information element.
     *
     * @param aifsn the AC_BE AIFSN field in the EdcaParameterSet information element
     */
    void SetBeAifsn(uint8_t aifsn);
    /**
     * Set the AC_BE ACI field in the EdcaParameterSet information element.
     *
     * @param aci the AC_BE ACI field in the EdcaParameterSet information element
     */
    void SetBeAci(uint8_t aci);
    /**
     * Set the AC_BE CWmin field in the EdcaParameterSet information element.
     *
     * @param cwMin the AC_BE CWmin field in the EdcaParameterSet information element
     */
    void SetBeCWmin(uint32_t cwMin);
    /**
     * Set the AC_BE CWmax field in the EdcaParameterSet information element.
     *
     * @param cwMax the AC_BE CWmax field in the EdcaParameterSet information element
     */
    void SetBeCWmax(uint32_t cwMax);
    /**
     * Set the AC_BE TXOP Limit field in the EdcaParameterSet information element.
     *
     * @param txop the AC_BE TXOP Limit field in the EdcaParameterSet information element
     */
    void SetBeTxopLimit(uint16_t txop);
    /**
     * Set the AC_BK AIFSN field in the EdcaParameterSet information element.
     *
     * @param aifsn the AC_BB AIFSN field in the EdcaParameterSet information element
     */
    void SetBkAifsn(uint8_t aifsn);
    /**
     * Set the AC_BK ACI field in the EdcaParameterSet information element.
     *
     * @param aci the AC_BK ACI field in the EdcaParameterSet information element
     */
    void SetBkAci(uint8_t aci);
    /**
     * Set the AC_BK CWmin field in the EdcaParameterSet information element.
     *
     * @param cwMin the AC_BK CWmin field in the EdcaParameterSet information element
     */
    void SetBkCWmin(uint32_t cwMin);
    /**
     * Set the AC_BK CWmax field in the EdcaParameterSet information element.
     *
     * @param cwMax the AC_BK CWmax field in the EdcaParameterSet information element
     */
    void SetBkCWmax(uint32_t cwMax);
    /**
     * Set the AC_BK TXOP Limit field in the EdcaParameterSet information element.
     *
     * @param txop the AC_BK TXOP Limit field in the EdcaParameterSet information element
     */
    void SetBkTxopLimit(uint16_t txop);
    /**
     * Set the AC_VI AIFSN field in the EdcaParameterSet information element.
     *
     * @param aifsn the AC_VI AIFSN field in the EdcaParameterSet information element
     */
    void SetViAifsn(uint8_t aifsn);
    /**
     * Set the AC_VI ACI field in the EdcaParameterSet information element.
     *
     * @param aci the AC_VI ACI field in the EdcaParameterSet information element
     */
    void SetViAci(uint8_t aci);
    /**
     * Set the AC_VI CWmin field in the EdcaParameterSet information element.
     *
     * @param cwMin the AC_VI CWmin field in the EdcaParameterSet information element
     */
    void SetViCWmin(uint32_t cwMin);
    /**
     * Set the AC_VI CWmax field in the EdcaParameterSet information element.
     *
     * @param cwMax the AC_VI CWmax field in the EdcaParameterSet information element
     */
    void SetViCWmax(uint32_t cwMax);
    /**
     * Set the AC_VI TXOP Limit field in the EdcaParameterSet information element.
     *
     * @param txop the AC_VI TXOP Limit field in the EdcaParameterSet information element
     */
    void SetViTxopLimit(uint16_t txop);
    /**
     * Set the AC_VO AIFSN field in the EdcaParameterSet information element.
     *
     * @param aifsn the AC_VO AIFSN field in the EdcaParameterSet information element
     */
    void SetVoAifsn(uint8_t aifsn);
    /**
     * Set the AC_VO ACI field in the EdcaParameterSet information element.
     *
     * @param aci the AC_VO ACI field in the EdcaParameterSet information element
     */
    void SetVoAci(uint8_t aci);
    /**
     * Set the AC_VO CWmin field in the EdcaParameterSet information element.
     *
     * @param cwMin the AC_VO CWmin field in the EdcaParameterSet information element
     */
    void SetVoCWmin(uint32_t cwMin);
    /**
     * Set the AC_VO CWmax field in the EdcaParameterSet information element.
     *
     * @param cwMax the AC_VO CWmax field in the EdcaParameterSet information element
     */
    void SetVoCWmax(uint32_t cwMax);
    /**
     * Set the AC_VO TXOP Limit field in the EdcaParameterSet information element.
     *
     * @param txop the AC_VO TXOP Limit field in the EdcaParameterSet information element
     */
    void SetVoTxopLimit(uint16_t txop);

    /**
     * Return the QoS Info field in the EdcaParameterSet information element.
     *
     * @return the QoS Info field in the EdcaParameterSet information element
     */
    uint8_t GetQosInfo() const;
    /**
     * Return the AC_BE AIFSN field in the EdcaParameterSet information element.
     *
     * @return the AC_BE AIFSN field in the EdcaParameterSet information element
     */
    uint8_t GetBeAifsn() const;
    /**
     * Return the AC_BE CWmin field in the EdcaParameterSet information element.
     *
     * @return the AC_BE CWmin field in the EdcaParameterSet information element
     */
    uint32_t GetBeCWmin() const;
    /**
     * Return the AC_BE CWmax field in the EdcaParameterSet information element.
     *
     * @return the AC_BE CWmax field in the EdcaParameterSet information element
     */
    uint32_t GetBeCWmax() const;
    /**
     * Return the AC_BE TXOP Limit field in the EdcaParameterSet information element.
     *
     * @return the AC_BE TXOP Limit field in the EdcaParameterSet information element
     */
    uint16_t GetBeTxopLimit() const;
    /**
     * Return the AC_BK AIFSN field in the EdcaParameterSet information element.
     *
     * @return the AC_BK AIFSN field in the EdcaParameterSet information element
     */
    uint8_t GetBkAifsn() const;
    /**
     * Return the AC_BK CWmin field in the EdcaParameterSet information element.
     *
     * @return the AC_BK CWmin field in the EdcaParameterSet information element
     */
    uint32_t GetBkCWmin() const;
    /**
     * Return the AC_BK CWmax field in the EdcaParameterSet information element.
     *
     * @return the AC_BK CWmax field in the EdcaParameterSet information element
     */
    uint32_t GetBkCWmax() const;
    /**
     * Return the AC_BK TXOP Limit field in the EdcaParameterSet information element.
     *
     * @return the AC_BK TXOP Limit field in the EdcaParameterSet information element
     */
    uint16_t GetBkTxopLimit() const;
    /**
     * Return the AC_VI AIFSN field in the EdcaParameterSet information element.
     *
     * @return the AC_VI AIFSN field in the EdcaParameterSet information element
     */
    uint8_t GetViAifsn() const;
    /**
     * Return the AC_VI CWmin field in the EdcaParameterSet information element.
     *
     * @return the AC_VI CWmin field in the EdcaParameterSet information element
     */
    uint32_t GetViCWmin() const;
    /**
     * Return the AC_VI CWmax field in the EdcaParameterSet information element.
     *
     * @return the AC_VI CWmax field in the EdcaParameterSet information element
     */
    uint32_t GetViCWmax() const;
    /**
     * Return the AC_VI TXOP Limit field in the EdcaParameterSet information element.
     *
     * @return the AC_VI TXOP Limit field in the EdcaParameterSet information element
     */
    uint16_t GetViTxopLimit() const;
    /**
     * Return the AC_VO AIFSN field in the EdcaParameterSet information element.
     *
     * @return the AC_VO AIFSN field in the EdcaParameterSet information element
     */
    uint8_t GetVoAifsn() const;
    /**
     * Return the AC_VO CWmin field in the EdcaParameterSet information element.
     *
     * @return the AC_VO CWmin field in the EdcaParameterSet information element
     */
    uint32_t GetVoCWmin() const;
    /**
     * Return the AC_VO CWmax field in the EdcaParameterSet information element.
     *
     * @return the AC_VO CWmax field in the EdcaParameterSet information element
     */
    uint32_t GetVoCWmax() const;
    /**
     * Return the AC_VO TXOP Limit field in the EdcaParameterSet information element.
     *
     * @return the AC_VO TXOP Limit field in the EdcaParameterSet information element
     */
    uint16_t GetVoTxopLimit() const;

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    uint8_t m_qosInfo;  ///< QOS info
    uint8_t m_reserved; ///< reserved
    uint32_t m_acBE;    ///< AC_BE
    uint32_t m_acBK;    ///< AC_BK
    uint32_t m_acVI;    ///< AC_VI
    uint32_t m_acVO;    ///< AC_VO
};

} // namespace ns3

#endif /* EDCA_PARAMETER_SET_H */
