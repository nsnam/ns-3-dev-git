/*
 * Copyright (c) 2011-2019 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *   Jaume Nin <jnin@cttc.es>
 *   Nicola Baldo <nbaldo@cttc.es>
 *   Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef EMU_EPC_HELPER_H
#define EMU_EPC_HELPER_H

#include "no-backhaul-epc-helper.h"

namespace ns3
{

/**
 * @ingroup lte
 *
 * @brief Create an EPC network using EmuFdNetDevice
 *
 * This Helper will create an EPC network topology comprising of a
 * single node that implements both the SGW and PGW functionality, and
 * an MME node. The S1-U, X2-U and X2-C interfaces are realized using
 * EmuFdNetDevice; in particular, one device is used to send all the
 * traffic related to these interfaces.
 */
class EmuEpcHelper : public NoBackhaulEpcHelper
{
  public:
    /**
     * Constructor
     */
    EmuEpcHelper();

    /**
     * Destructor
     */
    ~EmuEpcHelper() override;

    // inherited from Object
    /**
     * Register this type.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void DoDispose() override;

    // inherited from EpcHelper
    void AddEnb(Ptr<Node> enbNode,
                Ptr<NetDevice> lteEnbNetDevice,
                std::vector<uint16_t> cellIds) override;
    void AddX2Interface(Ptr<Node> enbNode1, Ptr<Node> enbNode2) override;

  private:
    /**
     * helper to assign addresses to S1-U NetDevices
     */
    Ipv4AddressHelper m_epcIpv4AddressHelper;

    /**
     * Container for Ipv4Interfaces of the SGW
     */
    Ipv4InterfaceContainer m_sgwIpIfaces;

    /**
     * The name of the device used for the S1-U interface of the SGW
     */
    std::string m_sgwDeviceName;

    /**
     * The name of the device used for the S1-U interface of the eNB
     */
    std::string m_enbDeviceName;

    /**
     * MAC address used for the SGW
     */
    std::string m_sgwMacAddress;

    /**
     * First 5 bytes of the Enb MAC address base
     */
    std::string m_enbMacAddressBase;
};

} // namespace ns3

#endif // EMU_EPC_HELPER_H
