/*
 * Copyright (c) 2017
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef FRAME_CAPTURE_MODEL_H
#define FRAME_CAPTURE_MODEL_H

#include "ns3/nstime.h"
#include "ns3/object.h"

namespace ns3
{

class Event;
class Time;

/**
 * @ingroup wifi
 * @brief the interface for Wifi's frame capture models
 *
 */
class FrameCaptureModel : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * A pure virtual method that must be implemented in the subclass.
     * This method returns whether the reception should be switched to a
     * new incoming frame.
     *
     * @param currentEvent the event of the current frame
     * @param newEvent the event of the new incoming frame
     *
     * @return true if the reception should be switched to a new incoming frame,
     *         false otherwise
     */
    virtual bool CaptureNewFrame(Ptr<Event> currentEvent, Ptr<Event> newEvent) const = 0;

    /**
     * This method returns true if the capture window duration has not elapsed yet,
     *                     false otherwise.
     *
     * @param timePreambleDetected the time the preamble was detected
     *
     * @return true if the capture window duration has not elapsed yet,
     *         false otherwise
     */
    virtual bool IsInCaptureWindow(Time timePreambleDetected) const;

  private:
    Time m_captureWindow; //!< Capture window duration
};

} // namespace ns3

#endif /* FRAME_CAPTURE_MODEL_H */
