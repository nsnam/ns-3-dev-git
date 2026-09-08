// Copyright (c) 2026 Tom Henderson
//
// SPDX-License-Identifier: NIST-Software

#include "ns3/constant-position-mobility-model.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/net-device.h"
#include "ns3/phased-array-spectrum-propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-phy.h"
#include "ns3/spectrum-signal-parameters.h"
#include "ns3/spectrum-value.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"
#include "ns3/uniform-planar-array.h"

#include <complex>
#include <valarray>

using namespace ns3;

/**
 * @ingroup spectrum-tests
 *
 * Phased-array loss model whose gain is the squared magnitude of the sum of the
 * transmitter's beamforming weights it is given, so that the received PSD reveals
 * which beamforming vector the channel passed.
 */
class TxBeamGainLossModel : public PhasedArraySpectrumPropagationLossModel
{
  private:
    Ptr<SpectrumSignalParameters> DoCalcRxPowerSpectralDensity(
        Ptr<const SpectrumSignalParameters> params,
        Ptr<const MobilityModel> /* a */,
        Ptr<const MobilityModel> /* b */,
        Ptr<const PhasedArrayModel> /* aPhasedArrayModel */,
        Ptr<const PhasedArrayModel> /* bPhasedArrayModel */,
        const PhasedArrayModel::ComplexVector& aBeamformingVector,
        const PhasedArrayModel::ComplexVector& /* bBeamformingVector */) const override
    {
        std::complex<double> sum{0.0, 0.0};
        for (size_t i = 0; i < aBeamformingVector.GetSize(); ++i)
        {
            sum += aBeamformingVector(i);
        }
        auto rxParams = params->Copy();
        *(rxParams->psd) *= std::norm(sum);
        return rxParams;
    }

    int64_t DoAssignStreams(int64_t /* stream */) override
    {
        return 0;
    }
};

/**
 * @ingroup spectrum-tests
 *
 * Minimal SpectrumPhy that records the PSD of the last received signal.
 */
class TxBeamTestPhy : public SpectrumPhy
{
  public:
    /**
     * Constructor.
     *
     * @param spectrumModel The spectrum model used for reception.
     * @param antenna The phased array of this PHY.
     */
    TxBeamTestPhy(Ptr<const SpectrumModel> spectrumModel, Ptr<PhasedArrayModel> antenna)
        : m_spectrumModel(spectrumModel),
          m_antenna(antenna)
    {
    }

    void SetDevice(Ptr<NetDevice> d) override
    {
        m_device = d;
    }

    Ptr<NetDevice> GetDevice() const override
    {
        return m_device;
    }

    void SetMobility(Ptr<MobilityModel> m) override
    {
        m_mobility = m;
    }

    Ptr<MobilityModel> GetMobility() const override
    {
        return m_mobility;
    }

    void SetChannel(Ptr<SpectrumChannel> c) override
    {
        m_channel = c;
    }

    Ptr<const SpectrumModel> GetRxSpectrumModel() const override
    {
        return m_spectrumModel;
    }

    Ptr<Object> GetAntenna() const override
    {
        return m_antenna;
    }

    void StartRx(Ptr<SpectrumSignalParameters> params) override
    {
        m_rxPsd = params->psd;
        ++m_rxCount;
    }

    Ptr<SpectrumValue> m_rxPsd; ///< PSD of the last received signal
    uint32_t m_rxCount{0};      ///< Number of received signals

  private:
    Ptr<const SpectrumModel> m_spectrumModel; ///< Receive spectrum model
    Ptr<PhasedArrayModel> m_antenna;          ///< Phased array
    Ptr<NetDevice> m_device;                  ///< Device
    Ptr<MobilityModel> m_mobility;            ///< Mobility model
    Ptr<SpectrumChannel> m_channel;           ///< Channel
};

/**
 * @ingroup spectrum-tests
 *
 * Check that a signal is received with the gain of the beamforming vector the
 * transmitter had when the transmission started, even if that vector is
 * changed before the signal arrives at the receiver.
 *
 * Two configurations are run. With a propagation delay model, the receiver's
 * StartRx event is scheduled after a positive delay and the transmitter's array
 * is re-steered while the signal is in flight. Without a propagation delay
 * model, StartRx is scheduled with zero delay but is still a separate event; the
 * array is re-steered by another event at the same timestamp that runs between
 * StartTx and StartRx. Both cases fail if the channel reads the transmitter's
 * array at reception time instead of using the vector captured at transmission.
 *
 * The loss model used here returns |sum of the transmitter's weights|^2 as the
 * gain, so the vector in use at transmission (all ones, gain numElems^2) and the
 * re-steered vector (alternating +1/-1, gain 0) are distinguishable from the
 * received PSD.
 */
class SpectrumPhasedArrayTxBeamTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param withDelay Whether a propagation delay model is installed on the channel.
     */
    SpectrumPhasedArrayTxBeamTestCase(bool withDelay);

  private:
    void DoRun() override;

    bool m_withDelay; ///< Whether a propagation delay model is installed on the channel
};

SpectrumPhasedArrayTxBeamTestCase::SpectrumPhasedArrayTxBeamTestCase(bool withDelay)
    : TestCase(std::string("TX beamforming vector at transmission time, ") +
               (withDelay ? "with" : "without") + " propagation delay"),
      m_withDelay(withDelay)
{
}

void
SpectrumPhasedArrayTxBeamTestCase::DoRun()
{
    BandInfo bandInfo;
    bandInfo.fl = 1.99e9;
    bandInfo.fc = 2.0e9;
    bandInfo.fh = 2.01e9;
    Bands bands{bandInfo};
    Ptr<SpectrumModel> spectrumModel = Create<SpectrumModel>(bands);

    auto txAntenna = CreateObjectWithAttributes<UniformPlanarArray>("NumRows",
                                                                    UintegerValue(2),
                                                                    "NumColumns",
                                                                    UintegerValue(2));
    auto rxAntenna = CreateObjectWithAttributes<UniformPlanarArray>("NumRows",
                                                                    UintegerValue(2),
                                                                    "NumColumns",
                                                                    UintegerValue(2));
    const size_t numElems = txAntenna->GetNumElems();

    // Weights summing to numElems (loss model gain numElems^2) for the transmission, and
    // weights summing to zero (gain 0) for the re-steered array.
    PhasedArrayModel::ComplexVector txBeam(
        std::valarray<std::complex<double>>(std::complex<double>(1.0, 0.0), numElems));
    std::valarray<std::complex<double>> alternating(numElems);
    for (size_t i = 0; i < numElems; ++i)
    {
        alternating[i] = (i % 2 == 0) ? 1.0 : -1.0;
    }
    PhasedArrayModel::ComplexVector laterBeam(alternating);
    txAntenna->SetBeamformingVector(txBeam);
    rxAntenna->SetBeamformingVector(txBeam);

    auto txMobility = CreateObject<ConstantPositionMobilityModel>();
    txMobility->SetPosition(Vector(0, 0, 0));
    auto rxMobility = CreateObject<ConstantPositionMobilityModel>();
    rxMobility->SetPosition(Vector(3000, 0, 0)); // 10 us at the speed of light

    auto txPhy = CreateObject<TxBeamTestPhy>(spectrumModel, txAntenna);
    txPhy->SetMobility(txMobility);
    auto rxPhy = CreateObject<TxBeamTestPhy>(spectrumModel, rxAntenna);
    rxPhy->SetMobility(rxMobility);

    auto channel = CreateObject<MultiModelSpectrumChannel>();
    channel->AddPhasedArraySpectrumPropagationLossModel(CreateObject<TxBeamGainLossModel>());
    if (m_withDelay)
    {
        channel->SetPropagationDelayModel(CreateObject<ConstantSpeedPropagationDelayModel>());
    }
    channel->AddRx(rxPhy);
    txPhy->SetChannel(channel);
    rxPhy->SetChannel(channel);

    auto txPsd = Create<SpectrumValue>(spectrumModel);
    (*txPsd)[0] = 1.0;
    auto params = Create<SpectrumSignalParameters>();
    params->psd = txPsd;
    params->duration = MicroSeconds(1);
    params->txPhy = txPhy;

    const Time txTime = MilliSeconds(1);
    Simulator::Schedule(txTime, [&]() { channel->StartTx(params); });
    // Re-steer the transmitter's array while the signal is in flight: halfway through
    // the propagation delay, or in the same instant when there is no delay (the
    // reception is then a separate event at the same timestamp).
    const Time steerTime = m_withDelay ? txTime + MicroSeconds(5) : txTime;
    Simulator::Schedule(steerTime, [&]() { txAntenna->SetBeamformingVector(laterBeam); });
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ(rxPhy->m_rxCount, 1, "Expected one reception");
    NS_TEST_ASSERT_MSG_EQ_TOL((*rxPhy->m_rxPsd)[0],
                              static_cast<double>(numElems * numElems),
                              1e-9,
                              "Received PSD does not reflect the beamforming vector in use "
                              "when the transmission started");
    NS_TEST_ASSERT_MSG_EQ(txAntenna->GetBeamformingVector() == laterBeam,
                          true,
                          "The channel modified the transmitter's current beamforming vector");
    Simulator::Destroy();
}

/**
 * @ingroup spectrum-tests
 *
 * Test suite for the evaluation of the transmitter's beamforming vector at transmission time.
 */
class SpectrumPhasedArrayTxBeamTestSuite : public TestSuite
{
  public:
    SpectrumPhasedArrayTxBeamTestSuite();
};

SpectrumPhasedArrayTxBeamTestSuite::SpectrumPhasedArrayTxBeamTestSuite()
    : TestSuite("spectrum-phased-array-tx-beam", Type::UNIT)
{
    AddTestCase(new SpectrumPhasedArrayTxBeamTestCase(true), TestCase::Duration::QUICK);
    AddTestCase(new SpectrumPhasedArrayTxBeamTestCase(false), TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static SpectrumPhasedArrayTxBeamTestSuite g_spectrumPhasedArrayTxBeamTestSuite;
