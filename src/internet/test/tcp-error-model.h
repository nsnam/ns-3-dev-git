/*
 * Copyright (c) 2015 Natale Patriciello <natale.patriciello@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */
#ifndef TCPERRORCHANNEL_H
#define TCPERRORCHANNEL_H

#include "ns3/error-model.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-header.h"

namespace ns3
{

/**
 * @ingroup internet-test
 *
 * @brief A general (TCP-aware) error model.
 *
 * The class is responsible to take away the IP and TCP header from the packet,
 * and then to interrogate the method ShouldDrop, dropping the packet accordingly
 * to the returned value.
 */
class TcpGeneralErrorModel : public ErrorModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TcpGeneralErrorModel();

    /**
     * @brief Set the drop callback.
     * @param cb The callback to be set.
     */
    void SetDropCallback(Callback<void, const Ipv4Header&, const TcpHeader&, Ptr<const Packet>> cb)
    {
        m_dropCallback = cb;
    }

  protected:
    /**
     * @brief Check if the packet should be dropped.
     * @param ipHeader The packet IPv4 header.
     * @param tcpHeader The packet TCP header.
     * @param packetSize The packet size.
     * @returns True if the packet should be dropped.
     */
    virtual bool ShouldDrop(const Ipv4Header& ipHeader,
                            const TcpHeader& tcpHeader,
                            uint32_t packetSize) = 0;

  private:
    bool DoCorrupt(Ptr<Packet> p) override;
    Callback<void, const Ipv4Header&, const TcpHeader&, Ptr<const Packet>>
        m_dropCallback; //!< Drop callback.
};

/**
 * @ingroup internet-test
 *
 * @brief An error model TCP aware: it drops the sequence number declared.
 *
 * @see AddSeqToKill
 */
class TcpSeqErrorModel : public TcpGeneralErrorModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    TcpSeqErrorModel()
        : TcpGeneralErrorModel()
    {
    }

    /**
     * @brief Add the sequence number to the list of segments to be killed
     *
     * Calling x times this function indicates that you want to kill
     * the segment x times.
     *
     * @param seq sequence number to be killed
     */
    void AddSeqToKill(const SequenceNumber32& seq)
    {
        m_seqToKill.insert(m_seqToKill.end(), seq);
    }

  protected:
    bool ShouldDrop(const Ipv4Header& ipHeader,
                    const TcpHeader& tcpHeader,
                    uint32_t packetSize) override;

  protected:
    std::list<SequenceNumber32> m_seqToKill; //!< List of the sequence numbers to be dropped.

  private:
    void DoReset() override;
};

/**
 * @ingroup internet-test
 *
 * @brief Error model which drop packets with specified TCP flags.
 *
 * Set the flags with SetFlagToKill and the number of the packets with such flags
 * which should be killed.
 *
 * @see SetFlagToKill
 * @see SetKillRepeat
 *
 */
class TcpFlagErrorModel : public TcpGeneralErrorModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TcpFlagErrorModel();

    /**
     * @brief Set the flags of the segment that should be killed
     *
     * @param flags Flags
     */
    void SetFlagToKill(TcpHeader::Flags_t flags)
    {
        m_flagsToKill = flags;
    }

    /**
     * @brief Set how many packets should be killed
     *
     * If the flags are the same, this specified the numbers of drops:
     *
     * # -1 for infinite drops
     * # 0  for no drops
     * # >1 the number of drops
     *
     * @param killNumber Specifies the number of times the packet should be killed
     */
    void SetKillRepeat(int16_t killNumber)
    {
        m_killNumber = killNumber;
    }

  protected:
    bool ShouldDrop(const Ipv4Header& ipHeader,
                    const TcpHeader& tcpHeader,
                    uint32_t packetSize) override;

  protected:
    TcpHeader::Flags_t m_flagsToKill; //!< Flags a packet should have to be dropped.
    int16_t m_killNumber;             //!< The number of times the packet should be killed.

  private:
    void DoReset() override;
};

} // namespace ns3

#endif // TCPERRORCHANNEL_H
