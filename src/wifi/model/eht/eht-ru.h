/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef EHT_RU_H
#define EHT_RU_H

#include "ns3/wifi-phy-common.h"

#include <cstdint>
#include <map>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

namespace ns3
{

/**
 * This class stores the subcarrier groups of all the available EHT RUs.
 */
class EhtRu
{
  public:
    /**
     * RU Specification.
     */
    class RuSpec
    {
      public:
        /**
         * Default constructor
         */
        RuSpec() = default;
        /**
         * Constructor
         *
         * @param ruType the RU type
         * @param index the RU index (starting at 1) within the 80 MHz segment
         * @param primary160MHz whether the RU is allocated in the primary 160MHz channel
         * @param primary80MHzOrLower80MHz if the RU is allocated in the primary 160MHz channel,
         * whether the RU is allocated in the primary 80MHz channel, otherwise whether the RU is
         * allocated in the lower 80MHz channel of the secondary 160 MHz channel
         */
        RuSpec(RuType ruType, std::size_t index, bool primary160MHz, bool primary80MHzOrLower80MHz);

        /**
         * Get the RU type
         *
         * @return the RU type
         */
        RuType GetRuType() const;

        /**
         * Get the RU index within the 80 MHz segment
         *
         * @return the RU index within the 80 MHz segment
         */
        std::size_t GetIndex() const;

        /**
         * Get the RU PHY index
         *
         * @param bw the width of the channel of which the RU is part
         * @param p20Index the index of the primary20 channel
         * @return the RU PHY index
         */
        std::size_t GetPhyIndex(MHz_t bw, uint8_t p20Index) const;

        /**
         * Get whether the RU is allocated in the primary 160MHz channel
         *
         * @return true if the RU is in the primary 160 MHz channel and false otherwise
         */
        bool GetPrimary160MHz() const;

        /**
         * @return if the RU is allocated in the primary 160MHz channel, true if the RU is allocated
         * in the primary 80MHz channel, otherwise true if the RU is allocated in the lower 80MHz
         * channel
         */
        bool GetPrimary80MHzOrLower80MHz() const;

        /**
         * Default three-way comparison
         *
         * @param rhs right hand side
         * @return deduced comparison type
         */
        auto operator<=>(const RuSpec& rhs) const = default;

      private:
        RuType m_ruType{};      //!< RU type
        std::size_t m_index{};  /**< RU index (starting at 1) within the 80 MHz segment */
        bool m_primary160MHz{}; //!< true if the RU is allocated in the primary 160MHz channel
        bool m_primary80MHzOrLower80MHz{}; //!< if the RU is allocated in the primary 160MHz
                                           //!< channel, true if the RU is allocated in the primary
                                           //!< 80MHz channel, otherwise true if the RU is allocated
                                           //!< in the lower 80MHz channel
    };

    /// definition of the primary flags <primary160MHz, primary80MHzOrLower80MHz>. The first flag
    /// identifies whether the RU is in the primary 160 MHz. The second flag identifies whether the
    /// RU is allocated in the primary 80MHz channel if the RU is allocated in the primary 160MHz
    /// channel, or whether the RU is allocated in the lower 80MHz channel if the RU is allocated in
    /// the secondary 160MHz channel.
    using PrimaryFlags = std::pair<bool, bool>;

    /// Identifier for an RU located in the primary 80 MHz channel
    static constexpr PrimaryFlags PRIMARY_80_FLAGS{true, true};
    /// Identifier for an RU located in the secondary 80 MHz channel
    static constexpr PrimaryFlags SECONDARY_80_FLAGS{true, false};
    /// Identifier for an RU occupying the whole primary 160 MHz channel
    static constexpr PrimaryFlags PRIMARY_160_FLAGS{true, true};
    /// Identifier for an RU occupying the whole secondary 160 MHz channel
    static constexpr PrimaryFlags SECONDARY_160_FLAGS{false, true};
    /// Identifier for an RU located in the lower 80 MHz channel of the secondary 160 MHz channel
    static constexpr PrimaryFlags SECONDARY_160_LOW_FLAGS{false, true};
    /// Identifier for an RU located in the upper 80 MHz channel of the secondary 160 MHz channel
    static constexpr PrimaryFlags SECONDARY_160_HIGH_FLAGS{false, false};

    /**
     * Get the primary flags of a given RU transmitted in a PPDU.
     *
     * @param bw the bandwidth of the PPDU (20, 40, 80, 160, 320)
     * @param ruType the RU type (number of tones)
     * @param phyIndex the PHY index (starting at 1) of the RU
     * @param p20Index the index of the primary20 channel
     * @return the primary flags
     */
    static PrimaryFlags GetPrimaryFlags(MHz_t bw,
                                        RuType ruType,
                                        std::size_t phyIndex,
                                        uint8_t p20Index);

    /**
     * Get the index of a given RU transmitted in a PPDU within its 80 MHz segment.
     *
     * @param bw the bandwidth of the PPDU (20, 40, 80, 160, 320)
     * @param ruType the RU type (number of tones)
     * @param phyIndex the PHY index (starting at 1) of the RU
     * @return the index within the 80 MHz segment
     */
    static std::size_t GetIndexIn80MHzSegment(MHz_t bw, RuType ruType, std::size_t phyIndex);

    /**
     * Get the number of distinct RUs of the given type (number of tones)
     * available in a PPDU of the given bandwidth.
     *
     * @param bw the bandwidth of the PPDU (20, 40, 80, 160, 320)
     * @param ruType the RU type (number of tones)
     * @param includeUndefinedRus whether undefined RUs should be taken account.
     * @return the number of distinct RUs available
     */
    static std::size_t GetNRus(MHz_t bw, RuType ruType, bool includeUndefinedRus = false);

    /**
     * Get the set of distinct RUs of the given type (number of tones)
     * available in an PPDU of the given bandwidth.
     * This does not take undefined RUs into account.
     *
     * @param bw the bandwidth of the PPDU (20, 40, 80, 160, 320)
     * @param ruType the RU type (number of tones)
     * @return the set of distinct RUs available
     */
    static std::vector<RuSpec> GetRusOfType(MHz_t bw, RuType ruType);

    /**
     * Get the set of 26-tone RUs that can be additionally allocated if the given
     * bandwidth is split in RUs of the given type.
     *
     * @param bw the bandwidth of the PPDU (20, 40, 80, 160, 320)
     * @param ruType the RU type (number of tones)
     * @return the set of 26-tone RUs that can be additionally allocated
     */
    static std::vector<RuSpec> GetCentral26TonesRus(MHz_t bw, RuType ruType);

    /**
     * Get the subcarrier group of the RU having the given PHY index among all the RUs of the given
     * type (number of tones) available in an PPDU of the given bandwidth. A subcarrier group is
     * defined as one or more pairs indicating the lowest frequency index and the highest frequency
     * index.
     *
     * @param bw the bandwidth of the PPDU (20, 40, 80, 160, 320)
     * @param ruType the RU type (number of tones)
     * @param phyIndex the PHY index (starting at 1) of the RU
     * @return the subcarrier range of the specified RU
     */
    static SubcarrierGroup GetSubcarrierGroup(MHz_t bw, RuType ruType, std::size_t phyIndex);

    /**
     * Check whether the given RU overlaps with the given set of RUs.
     * Note that for channel width of 160 MHz the returned range is relative to
     * the 160 MHz channel (i.e. -1012 to 1012).
     *
     * @param bw the bandwidth of the PPDU (20, 40, 80, 160, 320)
     * @param ru the given RU allocation
     * @param v the given set of RUs
     * @return true if the given RU overlaps with the given set of RUs.
     */
    static bool DoesOverlap(MHz_t bw, RuSpec ru, const std::vector<RuSpec>& v);

    /**
     * Find the RU allocation of the given RU type overlapping the given
     * reference RU allocation.
     * Note that an assert is generated if the RU allocation is not found.
     *
     * @param bw the bandwidth of the PPDU (20, 40, 80, 160, 320)
     * @param referenceRu the reference RU allocation
     * @param searchedRuType the searched RU type
     * @return the searched RU allocation.
     */
    static RuSpec FindOverlappingRu(MHz_t bw, RuSpec referenceRu, RuType searchedRuType);

    /**
     * Given the channel bandwidth and the number of stations candidate for being
     * assigned an RU, maximize the number of candidate stations that can be assigned
     * an RU subject to the constraint that all the stations must be assigned an RU
     * of the same size (in terms of number of tones).
     *
     * @param bandwidth the channel bandwidth
     * @param nStations the number of candidate stations. On return, it is set to
     *                  the number of stations that are assigned an RU
     * @param[out] nCentral26TonesRus the number of additional 26-tone RUs that can be
     *                                allocated if the returned RU size is greater than 26 tones
     * @return the RU type
     */
    static RuType GetEqualSizedRusForStations(MHz_t bandwidth,
                                              std::size_t& nStations,
                                              std::size_t& nCentral26TonesRus);

    /// Get the RU specs based on RU_ALLOCATION
    /// @param ruAllocation 9 bit RU_ALLOCATION value
    /// @return RU spec associated with the RU_ALLOCATION
    static std::vector<RuSpec> GetRuSpecs(uint16_t ruAllocation);

    /// Get the RU_ALLOCATION value for equal size RUs
    /// @param ruType equal size RU type (generated by GetEqualSizedRusForStations)
    /// @param isOdd if number of stations is an odd number
    /// @param hasUsers whether it contributes to User field(s) in the content channel this RU
    /// Allocation belongs to;
    /// @return RU_ALLOCATION value
    static uint16_t GetEqualizedRuAllocation(RuType ruType, bool isOdd, bool hasUsers);

  private:
    /**
     * Get the number of 26-tone RUs that can be allocated if returned RU size is greater than 26
     * tones.
     *
     * @param bw the bandwidth of the PPDU (20, 40, 80, 160, 320)
     * @param ruType the RU type (number of tones)
     * @return the number of 26-tone RUs that can be allocated
     */
    static uint8_t GetNumCentral26TonesRus(MHz_t bw, RuType ruType);

    /// Subcarrier groups for all EHT RUs
    static const SubcarrierGroups m_ruSubcarrierGroups;

    /// RU allocation map
    using RuAllocationMap = std::map<uint8_t, std::vector<RuSpec>>;

    /// Table 36-34 of IEEE802.11be-D7.0
    static const RuAllocationMap m_ruAllocations;
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param ru the RU
 * @returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const EhtRu::RuSpec& ru);

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param flags the primary flags
 * @returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, const EhtRu::PrimaryFlags& flags);

} // namespace ns3

#endif /* EHT_RU_H */
