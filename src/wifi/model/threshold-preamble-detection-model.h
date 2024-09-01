/*
 * Copyright (c) 2018 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef THRESHOLD_PREAMBLE_DETECTION_MODEL_H
#define THRESHOLD_PREAMBLE_DETECTION_MODEL_H

#include "preamble-detection-model.h"

namespace ns3
{
/**
 * \ingroup wifi
 *
 * A threshold-based model for detecting PHY preamble.
 * This model assumes that a preamble is successfully detected if SNR is at or above a given
 * threshold (set to 4 dB by default). However, if RSSI is below a minimum RSSI (set to -82 dBm by
 * default), the PHY preamble is not detected.
 */
class ThresholdPreambleDetectionModel : public PreambleDetectionModel
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    ThresholdPreambleDetectionModel();
    ~ThresholdPreambleDetectionModel() override;
    bool IsPreambleDetected(double rssi, double snr, MHz_u channelWidth) const override;

  private:
    double m_threshold; ///< SNR threshold in dB used to decide whether a preamble is successfully
                        ///< received
    double m_rssiMin;   ///< Minimum RSSI in dBm that shall be received to start the decision
};

} // namespace ns3

#endif /* THRESHOLD_PREAMBLE_DETECTION_MODEL_H */
