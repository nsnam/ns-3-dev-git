/*
 * Copyright (c) 2014 Universita' degli Studi di Napoli "Federico II"
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stefano.avallone@unina.it>
 */

#ifndef WIFI_TX_CURRENT_MODEL_H
#define WIFI_TX_CURRENT_MODEL_H

#include "wifi-units.h"

#include "ns3/object.h"

namespace ns3
{

/**
 * @ingroup energy
 *
 * @brief Model the transmit current as a function of the transmit power and mode
 *
 */
class WifiTxCurrentModel : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    WifiTxCurrentModel();
    ~WifiTxCurrentModel() override;

    /**
     * @param txPower the nominal TX power
     * @returns the transmit current
     */
    virtual ampere_u CalcTxCurrent(dBm_u txPower) const = 0;
};

/**
 * @ingroup energy
 *
 * @brief a linear model of the Wifi transmit current
 *
 * This model assumes that the transmit current is a linear function
 * of the nominal transmit power used to send the frame.
 * In particular, the power absorbed during the transmission of a frame \f$ W_{tx} \f$
 * is given by the power absorbed by the power amplifier \f$ W_{pa} \f$ plus the power
 * absorbed by the RF subsystem. The latter is assumed to be the same as the power
 * absorbed in the IDLE state \f$ W_{idle} \f$.
 *
 * The efficiency \f$ \eta \f$ of the power amplifier is given by
 * \f$ \eta = \frac{P_{tx}}{W_{pa}} \f$, where \f$ P_{tx} \f$ is the output power, i.e.,
 * the nominal transmit power. Hence, \f$ W_{pa} = \frac{P_{tx}}{\eta} \f$
 *
 * It turns out that \f$ W_{tx} = \frac{P_{tx}}{\eta} + W_{idle} \f$. By dividing both
 * sides by the supply voltage \f$ V \f$: \f$ I_{tx} = \frac{P_{tx}}{V \cdot \eta} + I_{idle} \f$,
 * where \f$ I_{tx} \f$ and \f$ I_{idle} \f$ are, respectively, the transmit current and
 * the idle current.
 *
 * For more information, refer to:
 * Francesco Ivan Di Piazza, Stefano Mangione, and Ilenia Tinnirello.
 * "On the Effects of Transmit Power Control on the Energy Consumption of WiFi Network Cards",
 * Proceedings of ICST QShine 2009, pp. 463--475
 *
 * If the TX current corresponding to a given nominal transmit power is known, the efficiency
 * of the power amplifier is given by the above formula:
 * \f$ \eta = \frac{P_{tx}}{(I_{tx}-I_{idle})\cdot V} \f$
 *
 */
class LinearWifiTxCurrentModel : public WifiTxCurrentModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    LinearWifiTxCurrentModel();
    ~LinearWifiTxCurrentModel() override;

    ampere_u CalcTxCurrent(dBm_u txPower) const override;

  private:
    double m_eta;           ///< ETA
    volt_u m_voltage;       ///< voltage
    ampere_u m_idleCurrent; ///< idle current
};

} // namespace ns3

#endif /* WIFI_TX_CURRENT_MODEL_H */
