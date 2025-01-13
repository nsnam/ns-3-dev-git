/*
 * Copyright (c) 2013 Fraunhofer FKIE
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Sascha Alexander Jopen <jopen@cs.uni-bonn.de>
 */
#ifndef LR_WPAN_INTERFERENCE_HELPER_H
#define LR_WPAN_INTERFERENCE_HELPER_H

#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"

#include <set>

namespace ns3
{

class SpectrumValue;
class SpectrumModel;

namespace lrwpan
{

/**
 * @ingroup lr-wpan
 *
 * @brief This class provides helper functions for LrWpan interference handling.
 */
class LrWpanInterferenceHelper : public SimpleRefCount<LrWpanInterferenceHelper>
{
  public:
    /**
     * Create a new interference helper for the given SpectrumModel.
     *
     * @param spectrumModel the SpectrumModel to be used
     */
    LrWpanInterferenceHelper(Ptr<const SpectrumModel> spectrumModel);

    ~LrWpanInterferenceHelper();

    /**
     * Add the given signal to the set of accumulated signals. Never add the same
     * signal more than once. The SpectrumModels of the signal and the one used
     * for instantiation of the helper have to be the same.
     *
     * @param signal the signal to be added
     * @return false, if the signal was not added because the SpectrumModel of the
     * signal does not match the one of the helper, true otherwise.
     */
    bool AddSignal(Ptr<const SpectrumValue> signal);

    /**
     * Remove the given signal to the set of accumulated signals.
     *
     * @param signal the signal to be removed
     * @return false, if the signal was not removed (because it was not added
     * before), true otherwise.
     */
    bool RemoveSignal(Ptr<const SpectrumValue> signal);

    /**
     * Remove all currently accumulated signals.
     */
    void ClearSignals();

    /**
     * Get the sum of all accumulated signals.
     *
     * @return the sum of the signals
     */
    Ptr<SpectrumValue> GetSignalPsd() const;

    /**
     * Get the SpectrumModel used by the helper.
     *
     * @return the helpers SpectrumModel
     */
    Ptr<const SpectrumModel> GetSpectrumModel() const;

  private:
    // Disable implicit copy constructors
    /**
     * @brief Copy constructor - defined and not implemented.
     */
    LrWpanInterferenceHelper(const LrWpanInterferenceHelper&);
    /**
     * @brief Copy constructor - defined and not implemented.
     * @returns
     */
    LrWpanInterferenceHelper& operator=(const LrWpanInterferenceHelper&);
    /**
     * The helpers SpectrumModel.
     */
    Ptr<const SpectrumModel> m_spectrumModel;

    /**
     * The set of accumulated signals.
     */
    std::set<Ptr<const SpectrumValue>> m_signals;

    /**
     * The precomputed sum of all accumulated signals.
     */
    mutable Ptr<SpectrumValue> m_signal;

    /**
     * Mark m_signal as dirty, whenever a signal is added or removed. m_signal has
     * to be recomputed before next use.
     */
    mutable bool m_dirty;
};

} // namespace lrwpan
} // namespace ns3

#endif /* LR_WPAN_INTERFERENCE_HELPER_H */
