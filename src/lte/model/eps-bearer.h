/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */


#ifndef EPS_BEARER
#define EPS_BEARER

#include <ns3/uinteger.h>
#include <unordered_map>

namespace ns3 {

/**
 * 3GPP TS 36.413 9.2.1.18 GBR QoS Information
 *
 */
struct GbrQosInformation
{
  /** 
   * Default constructor, initializes member variables to zero or equivalent
   */
  GbrQosInformation ();

  uint64_t gbrDl;  /**< Guaranteed Bit Rate (bit/s) in downlink */
  uint64_t gbrUl;  /**< Guaranteed Bit Rate (bit/s) in uplink */
  uint64_t mbrDl;  /**< Maximum Bit Rate (bit/s) in downlink */
  uint64_t mbrUl;  /**< Maximum Bit Rate (bit/s) in uplink */
};


/**
 * 3GPP 23.203 Section 6.1.7.3 Allocation and Retention Priority characteristics
 *
 */
struct AllocationRetentionPriority
{
  /** 
   * Default constructor, initializes member variables to zero or equivalent
   */
  AllocationRetentionPriority ();
  uint8_t priorityLevel;     ///< 1-15; 1 = highest
  bool preemptionCapability; ///< true if bearer can preempt others
  bool preemptionVulnerability; ///< true if bearer can be preempted by others
};

/**
 * This class contains the specification of EPS Bearers.
 *
 * See the following references:
 * 3GPP TS 23.203, Section 4.7.2 The EPS bearer
 * 3GPP TS 23.203, Section 4.7.3 Bearer level QoS parameters
 * 3GPP TS 36.413 Section 9.2.1.15 E-RAB Level QoS Parameters
 *
 */
struct EpsBearer
{

  /**
   * QoS Class Indicator. See 3GPP 23.203 Section 6.1.7.2 for standard values.
   * Updated to Release 15.
   */
  enum Qci : uint8_t
  {
    GBR_CONV_VOICE          = 1,    ///< GBR Conversational Voice
    GBR_CONV_VIDEO          = 2,    ///< GBR Conversational Video (Live streaming)
    GBR_GAMING              = 3,    ///< GBR Real Time Gaming
    GBR_NON_CONV_VIDEO      = 4,    ///< GBR Non-Conversational Video (Buffered Streaming)
    NGBR_IMS                = 5,    ///< Non-GBR IMS Signalling
    NGBR_VIDEO_TCP_OPERATOR = 6,    ///< Non-GBR TCP-based Video (Buffered Streaming, e.g., www, e-mail...)
    NGBR_VOICE_VIDEO_GAMING = 7,    ///< Non-GBR Voice, Video, Interactive Streaming
    NGBR_VIDEO_TCP_PREMIUM  = 8,    ///< Non-GBR TCP-based Video (Buffered Streaming, e.g., www, e-mail...)
    NGBR_VIDEO_TCP_DEFAULT  = 9,    ///< Non-GBR TCP-based Video (Buffered Streaming, e.g., www, e-mail...)
  } qci; ///< Qos class indicator

  GbrQosInformation gbrQosInfo; ///< GBR QOS information
  AllocationRetentionPriority arp; ///< allocation retention priority

  /**
   * Default constructor. QCI will be initialized to NGBR_VIDEO_TCP_DEFAULT
   * 
   */
  EpsBearer ();

  /**
   *
   * @param x the QoS Class Indicator
   *
   */
  EpsBearer (Qci x);

  /**
   *
   * @param x the QoS Class Indicator
   * @param y the GbrQosInformation
   *
   */
  EpsBearer (Qci x, GbrQosInformation y);

  /**
   *
   * @return true if the EPS Bearer is a Guaranteed Bit Rate bearer, false otherwise
   */
  bool IsGbr () const;

  /**
   *
   * @return the priority associated with the QCI of this bearer as per 3GPP 23.203 Section 6.1.7.2
   */
  uint8_t GetPriority () const;

  /**
   *
   *
   *
   * @return the packet delay budget associated with the QCI of this bearer as per 3GPP 23.203 Section 6.1.7.2
   */
  uint16_t GetPacketDelayBudgetMs () const;

  /**
   *
   *
   *
   * @return the packet error loss rate associated with the QCI of this bearer as per 3GPP 23.203 Section 6.1.7.2
   */
  double  GetPacketErrorLossRate () const;

private:
  /**
   * \brief Hashing QCI
   *
   * Qci are just uint8_t, so that's how we calculate the hash. Unfortunately,
   * we have to provide this struct because gcc 4.9 would not compile otherwise.
   */
  struct QciHash
  {
    /**
     * \brief Hash the QCI like a normal uint8_t
     * \param s Qci to hash
     * \return Hash of Qci
     */
    std::size_t
    operator () (Qci const& s) const noexcept
      {
        return std::hash<uint8_t> {} (s);
      }
  };

  /**
    * \brief Map between QCI and requirements
    *
    * The tuple is formed by: isGbr, priority, packet delay budget, packet error rate,
    *  default maximum data burst, default averaging window (0 when does not apply)
    */
  typedef std::unordered_map<Qci, std::tuple<bool, uint8_t, uint16_t, double, uint32_t, uint32_t>, QciHash > BearerRequirementsMap;

  /**
   * \brief Is the selected QCI GBR?
   * \param map Map between QCI and requirements
   * \param qci QCI to look for
   * \return GBR flag for the selected CQI
   */
  static uint32_t
  IsGbr (const BearerRequirementsMap &map, Qci qci) {return std::get<0> (map.at (qci));}

  /**
   * \brief Get priority for the selected QCI
   * \param map Map between QCI and requirements
   * \param qci QCI to look for
   * \return priority for the selected QCI
   */
  static uint8_t
  GetPriority (const BearerRequirementsMap &map, Qci qci) {return std::get<1> (map.at (qci));}
  /**
   * \brief Get packet delay in ms for the selected QCI
   * \param map Map between QCI and requirements
   * \param qci QCI to look for
   * \return packet delay in ms for the selected QCI
   */
  static uint16_t
  GetPacketDelayBudgetMs (const BearerRequirementsMap &map, Qci qci) {return std::get<2> (map.at (qci));}
  /**
   * \brief Get packet error rate for the selected QCI
   * \param map Map between QCI and requirements
   * \param qci QCI to look for
   * \return packet error rate for the selected QCI
   */
  static double
  GetPacketErrorLossRate (const BearerRequirementsMap &map, Qci qci) {return std::get<3> (map.at (qci));}
  /**
   * \brief Get maximum data burst for the selected QCI
   * \param map Map between QCI and requirements
   * \param qci QCI to look for
   * \return maximum data burst for the selected QCI
   */
  static uint32_t
  GetMaxDataBurst (const BearerRequirementsMap &map, Qci qci) {return std::get<4> (map.at (qci));}
  /**
   * \brief Get default averaging window for the selected QCI
   * \param map Map between QCI and requirements
   * \param qci QCI to look for
   * \return default averaging window for the selected QCI
   */
  static uint32_t
  GetAvgWindow (const BearerRequirementsMap &map, Qci qci) {return std::get<5> (map.at (qci));}

  /**
   * \brief Retrieve requirements for Rel. 11
   * \return the BearerRequirementsMap for Release 11
   */
  static BearerRequirementsMap GetRequirementsRel11 ();

  /**
    * \brief Requirements per bearer
    */
  BearerRequirementsMap m_requirements;
};

} // namespace ns3


#endif // EPS_BEARER
