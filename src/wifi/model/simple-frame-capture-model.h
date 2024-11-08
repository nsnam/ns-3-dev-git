/*
 * Copyright (c) 2017
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef SIMPLE_FRAME_CAPTURE_MODEL_H
#define SIMPLE_FRAME_CAPTURE_MODEL_H

#include "frame-capture-model.h"
#include "wifi-units.h"

namespace ns3
{
/**
 * @ingroup wifi
 *
 * A simple threshold-based model for frame capture effect.
 * If the new incoming frame arrives while the receiver is
 * receiving the preamble of another frame and the SIR of
 * the new incoming frame is above a fixed margin, then
 * the current frame is dropped and the receiver locks
 * onto the new incoming frame.
 */
class SimpleFrameCaptureModel : public FrameCaptureModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    SimpleFrameCaptureModel();
    ~SimpleFrameCaptureModel() override;

    /**
     * Sets the frame capture margin.
     *
     * @param margin the frame capture margin
     */
    void SetMargin(dB_u margin);
    /**
     * Return the frame capture margin.
     *
     * @return the frame capture margin
     */
    dB_u GetMargin() const;

    /**
     * This method returns whether the reception should be switched to a
     * new incoming frame.
     *
     * @param currentEvent the event of the current frame
     * @param newEvent the event of the new incoming frame
     *
     * @return true if the reception should be switched to a new incoming frame,
     *         false otherwise
     */
    bool CaptureNewFrame(Ptr<Event> currentEvent, Ptr<Event> newEvent) const override;

  private:
    dB_u m_margin; ///< margin for determining if a new frame
};

} // namespace ns3

#endif /* SIMPLE_FRAME_CAPTURE_MODEL_H */
