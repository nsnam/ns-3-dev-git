/*
 * Copyright (c) 2015 Danilo Abrignani
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Danilo Abrignani <danilo.abrignani@unibo.it>
 */

#ifndef CC_HELPER_H
#define CC_HELPER_H

#include "ns3/component-carrier.h"
#include "ns3/config.h"
#include "ns3/names.h"
#include "ns3/net-device-container.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/simulator.h"

#include <map>

namespace ns3
{

/**
 * @ingroup lte
 *
 * Creation and configuration of Component Carrier entities. One CcHelper instance is
 * typically enough for an LTE simulation. To create it:
 *
 *     Ptr<CcHelper> ccHelper = CreateObject<CcHelper> ();
 *
 * The general responsibility of the helper is to create various Component Carrier objects
 * and arrange them together to set the eNodeB. The overall
 * arrangement would look like the following:
 * - Ul Bandwidths
 * - Dl Bandwidths
 * - Ul Earfcn
 * - Dl Earfcn
 *
 *
 * This helper it is also used within the LteHelper in order to maintain backwards compatibility
 * with previous user simulation script.
 */
class CcHelper : public Object
{
  public:
    CcHelper();
    ~CcHelper() override;

    /**
     * Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();
    void DoDispose() override;

    /**
     * Create single CC.
     *
     * @param ulBandwidth the UL bandwidth
     * @param dlBandwidth the DL bandwidth
     * @param ulEarfcn the UL EARFCN
     * @param dlEarfcn the DL EARFCN
     * @param isPrimary true if primary
     * @returns the component carrier
     */
    ComponentCarrier DoCreateSingleCc(uint16_t ulBandwidth,
                                      uint16_t dlBandwidth,
                                      uint32_t ulEarfcn,
                                      uint32_t dlEarfcn,
                                      bool isPrimary);

    /**
     * Set an attribute for the Component Carrier to be created.
     *
     * @param n the name of the attribute.
     * @param v the value of the attribute
     */
    void SetCcAttribute(std::string n, const AttributeValue& v);

    /**
     * EquallySpacedCcs() create a valid std::map< uint8_t, Ptr<ComponentCarrier> >
     * The Primary Component Carrier it is at the position 0 in the map
     * The number of Component Carrier created depend on m_noOfCcs
     * Currently it is limited to maximum 2 ComponentCarrier
     * Since, only a LteEnbPhy object is available just symmetric Carrier Aggregation scheme
     * are allowed, i.e. 2 Uplink Component Carrier and 2 Downlink Component Carrier
     * Using this method, each CC will have the same characteristics (bandwidth)
     * while they are spaced by exactly the bandwidth. Hence, using this method,
     * you will create a intra-channel Carrier Aggregation scheme.
     *
     * @returns std::map< uint8_t, Ptr<ComponentCarrier> >
     */

    std::map<uint8_t, ComponentCarrier> EquallySpacedCcs();

    /**
     * Set number of CCs.
     *
     * @param nCc the number of CCs
     */
    void SetNumberOfComponentCarriers(uint16_t nCc);
    /**
     * Set UL EARFCN.
     *
     * @param ulEarfcn the UL EARFCN
     */
    void SetUlEarfcn(uint32_t ulEarfcn);
    /**
     * Set DL EARFCN.
     *
     * @param dlEarfcn the DL EARFCN
     */
    void SetDlEarfcn(uint32_t dlEarfcn);
    /**
     * Set DL bandwidth.
     *
     * @param dlBandwidth the DL bandwidth
     */
    void SetDlBandwidth(uint16_t dlBandwidth);
    /**
     * Set UL bandwidth.
     *
     * @param ulBandwidth the UL bandwidth
     */
    void SetUlBandwidth(uint16_t ulBandwidth);
    /**
     * Get number of component carriers.
     *
     * @returns the number of component carriers
     */
    uint16_t GetNumberOfComponentCarriers() const;
    /**
     * Get UL EARFCN.
     *
     * @returns the UL EARFCN
     */
    uint32_t GetUlEarfcn() const;
    /**
     * Get DL EARFCN.
     *
     * @returns the DL EARFCN
     */
    uint32_t GetDlEarfcn() const;
    /**
     * Get DL bandwidth.
     *
     * @returns the DL bandwidth
     */
    uint16_t GetDlBandwidth() const;
    /**
     * Get UL bandwidth.
     *
     * @returns the UL bandwidth
     */
    uint16_t GetUlBandwidth() const;

  protected:
    // inherited from Object
    void DoInitialize() override;

  private:
    /**
     * Create a single component carrier.
     *
     * @param ulBandwidth uplink bandwidth for the current CC
     * @param dlBandwidth downlink bandwidth for the current CC
     * @param ulEarfcn uplink EARFCN - not control on the validity at this point
     * @param dlEarfcn downlink EARFCN - not control on the validity at this point
     * @param isPrimary identify if this is the Primary Component Carrier (PCC) - only one
     * PCC is allowed
     * @return the component carrier
     */
    ComponentCarrier CreateSingleCc(uint16_t ulBandwidth,
                                    uint16_t dlBandwidth,
                                    uint32_t ulEarfcn,
                                    uint32_t dlEarfcn,
                                    bool isPrimary) const;

    /// Factory for each Carrier Component.
    ObjectFactory m_ccFactory;

    uint32_t m_ulEarfcn;                  ///< Uplink EARFCN
    uint32_t m_dlEarfcn;                  ///< Downlink EARFCN
    uint16_t m_dlBandwidth;               ///< Downlink Bandwidth
    uint16_t m_ulBandwidth;               ///< Uplink Bandwidth
    uint16_t m_numberOfComponentCarriers; ///< Number of component carriers
};

} // namespace ns3

#endif // LTE_HELPER_H
