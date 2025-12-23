/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef SMD_BSS_TRANSITION_PARAMETERS_H
#define SMD_BSS_TRANSITION_PARAMETERS_H

#include "ns3/addba-extension.h"
#include "ns3/block-ack-parameter-set.h"
#include "ns3/wifi-export.h"
#include "ns3/wifi-information-element.h"
#include "ns3/wifi-opt-field.h"
#include "ns3/wifi-types.h"
#include "ns3/wifi-utils.h"

#include <map>
#include <optional>
#include <variant>
#include <vector>

namespace ns3
{

/**
 * @brief SMD BSS Transition Parameters element
 * @ingroup wifi
 *
 * This class serializes and deserializes the SMD BSS Transition Parameters defined in IEEE 802.11bn
 * D2.0 Section 9.4.2.364.
 */
class WIFI_EXPORT SmdBssTransitionParameters : public WifiInformationElement
{
  public:
    /**
     * @ingroup wifi
     *
     * Types of SMD BSS Transition Parameters element, based on the frame containing the SMD BSS
     * Transition Parameters element.
     */
    enum ContainingFrameType : int
    {
        UNSET = -1,
        ST_PREPARATION_REQUEST = 0,
        ST_PREPARATION_RESPONSE = 1,
        ST_EXECUTION_REQUEST = 2,
        ST_EXECUTION_RESPONSE = 3,
    };

    /**
     * SCS List field structure
     * IEEE 802.11bn D2.0 Figure 9-aa48
     */
    struct ScsList
    {
        std::vector<uint8_t> m_scsIds{}; ///< SCS Info field (list of SCS ID fields)

        /**
         * Serialize the SCS List field
         *
         * @param start iterator pointing to where the field should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the SCS List field
         *
         * @param start iterator pointing to where the field should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the SCS List field
         *
         * @return the serialized size in bytes of the SCS List field
         */
        uint16_t GetSerializedSize() const;

        /**
         * Print the SCS List field
         *
         * @param os the output stream
         */
        void Print(std::ostream& os) const;
    };

    /**
     * ST Info field structure in an ST preparation request
     * IEEE 802.11bn D2.0 Figure 9-aa46
     */
    struct StPrepReqStInfo
    {
        /**
         * ST Control field structure in an ST preparation request
         * IEEE 802.11bn D2.0 Figure 9-aa47
         */
        struct StControl
        {
            uint8_t m_reqDlSnNotTransferred : 1 {0}; ///< Request DL SN Not Transferred field
            uint8_t m_reqUlSnNotTransferred : 1 {0}; ///< Request UL SN Not Transferred field
            bool m_scsListPresent{false}; ///< SCS List Present (do not set, it is set by the
                                          ///< OptFieldWithPresenceInd)

            /**
             * Serialize the ST Control field
             *
             * @param start iterator pointing to where the field should be written to
             */
            void Serialize(Buffer::Iterator& start) const;

            /**
             * Deserialize the ST Control field
             *
             * @param start iterator pointing to where the field should be read from
             * @return the number of octets read
             */
            uint16_t Deserialize(Buffer::Iterator start);

            /**
             * Get the serialized size of the ST Control field
             *
             * @return the serialized size in bytes of the ST Control field
             */
            uint16_t GetSerializedSize() const;
        };

        StControl m_stControl{};      ///< ST Control field
        uint16_t m_listenInterval{0}; ///< Listen Interval field
        OptFieldWithPresenceInd<ScsList> m_scsList{
            m_stControl.m_scsListPresent}; ///< SCS List field

        /**
         * Constructor
         */
        StPrepReqStInfo();

        /**
         * Copy constructor
         *
         * @param other the object to copy from
         */
        StPrepReqStInfo(const StPrepReqStInfo& other);

        /**
         * Copy assignment operator
         * @param other the object to assign from
         * @return a reference to this object
         */
        StPrepReqStInfo& operator=(const StPrepReqStInfo& other);

        /**
         * Serialize the ST Info field
         *
         * @param start iterator pointing to where the field should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the ST Info field
         *
         * @param start iterator pointing to where the field should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the ST Info field
         *
         * @return the serialized size in bytes of the ST Info field
         */
        uint16_t GetSerializedSize() const;

        /**
         * Print the ST Info field
         *
         * @param os the output stream
         */
        void Print(std::ostream& os) const;
    };

    /**
     * ST Info field structure in an ST preparation response
     * IEEE 802.11bn D2.0 Figure 9-aa50
     */
    struct StPrepRespStInfo
    {
        /**
         * ST Control field structure in an ST preparation response
         * IEEE 802.11bn D2.0 Figure 9-aa51
         */
        struct StControl
        {
            bool m_dlBaInfoPresent{false}; ///< DL BA Info Present (do not set, it is set by the
                                           ///< OptFieldWithPresenceInd)
            bool m_ulBaInfoPresent{false}; ///< UL BA Info Present (do not set, it is set by the
                                           ///< OptFieldWithPresenceInd)
            bool m_scsListPresent{
                false}; ///< SCS List Present (do not set, it is set by the OptFieldWithPresenceInd)

            /**
             * Serialize the ST Control field
             *
             * @param start iterator pointing to where the field should be written to
             */
            void Serialize(Buffer::Iterator& start) const;

            /**
             * Deserialize the ST Control field
             *
             * @param start iterator pointing to where the field should be read from
             * @return the number of octets read
             */
            uint16_t Deserialize(Buffer::Iterator start);

            /**
             * Get the serialized size of the ST Control field
             * @return the serialized size in bytes of the ST Control field
             */
            uint16_t GetSerializedSize() const;
        };

        /**
         * Extended BA Parameters Info field structure in DL/UL BA Info
         * IEEE 802.11bn D2.0 Figure 9-aa53
         */
        struct ExtBaParamsInfo
        {
            BlockAckParameterSet m_baParameterSet{}; ///< BA Parameter Set field
            AddbaExtension::ExtParamSet
                m_addBaExtParameterSet{}; ///< ADDBA Extended Parameter Set field

            /**
             * Serialize the Extended BA Parameters Info field
             *
             * @param start iterator pointing to where the field should be written to
             */
            void Serialize(Buffer::Iterator& start) const;

            /**
             * Deserialize the Extended BA Parameters Info field
             *
             * @param start iterator pointing to where the field should be read from
             * @return the number of octets read
             */
            uint16_t Deserialize(Buffer::Iterator start);

            /**
             * Get the serialized size of the Extended BA Parameters Info field
             *
             * @return the serialized size in bytes of the Extended BA Parameters Info field
             */
            uint16_t GetSerializedSize() const;

            /**
             * Print the Extended BA Parameters Info field
             *
             * @param os the output stream
             */
            void Print(std::ostream& os) const;
        };

        /**
         * DL/UL BA Info field structure in an ST preparation response
         * IEEE 802.11bn D2.0 Figure 9-aa52 and 9-aa54
         */
        struct BaInfo
        {
            /**
             * Set the Extended BA Parameters Info for a given TID
             *
             * @param tid the TID for which to set the Extended BA Parameters Info
             * @param info the Extended BA Parameters Info to set
             */
            void Set(tid_t tid, const ExtBaParamsInfo& info);

            /**
             * Get the Extended BA Parameters Info for a given TID
             *
             * @param tid the TID for which to get the Extended BA Parameters Info
             * @return an optional containing the Extended BA Parameters Info if present,
             * std::nullopt otherwise
             */
            std::optional<ExtBaParamsInfo> Get(tid_t tid) const;

            /**
             * @return the DL/UL TID Bitmap field value
             */
            uint8_t GetTidBitmap() const;

            /**
             * @return the map of Extended BA Parameters Info per TID
             */
            const std::map<tid_t, ExtBaParamsInfo>& GetExtBaParamsInfos() const;

            /**
             * Serialize the DL/UL BA Info field
             *
             * @param start iterator pointing to where the field should be written to
             */
            void Serialize(Buffer::Iterator& start) const;

            /**
             * Deserialize the DL/UL BA Info field
             *
             * @param start iterator pointing to where the field should be read from
             * @return the number of octets read
             */
            uint16_t Deserialize(Buffer::Iterator start);

            /**
             * Get the serialized size of the DL/UL BA Info field
             *
             * @return the serialized size in bytes of the DL/UL BA Info field
             */
            uint16_t GetSerializedSize() const;

            /**
             * Print the DL/UL BA Info field
             *
             * @param os the output stream
             */
            void Print(std::ostream& os) const;

          private:
            uint8_t m_tidBitmap{0}; ///< DL/UL TID Bitmap field
            std::map<tid_t, ExtBaParamsInfo>
                m_extBaParamsInfos{}; ///< Extended BA Parameters Info per TID fields
        };

        StControl m_stControl{}; ///< ST Control field
        OptFieldWithPresenceInd<BaInfo> m_dlBaInfo{m_stControl.m_dlBaInfoPresent}; ///< DL BA Info
                                                                                   ///< field
        OptFieldWithPresenceInd<BaInfo> m_ulBaInfo{m_stControl.m_ulBaInfoPresent}; ///< UL BA Info
                                                                                   ///< field
        OptFieldWithPresenceInd<ScsList> m_scsList{m_stControl.m_scsListPresent};  ///< SCS List
                                                                                   ///< field

        /**
         * Constructor
         */
        StPrepRespStInfo();

        /**
         * Copy constructor
         *
         * @param other the object to copy from
         */
        StPrepRespStInfo(const StPrepRespStInfo& other);

        /**
         * Copy assignment operator
         * @param other the object to assign from
         * @return a reference to this object
         */
        StPrepRespStInfo& operator=(const StPrepRespStInfo& other);

        /**
         * Serialize the ST Info field
         *
         * @param start iterator pointing to where the field should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the ST Info field
         *
         * @param start iterator pointing to where the field should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the ST Info field
         *
         * @return the serialized size in bytes of the ST Info field
         */
        uint16_t GetSerializedSize() const;

        /**
         * Print the ST Info field
         *
         * @param os the output stream
         */
        void Print(std::ostream& os) const;
    };

    /**
     * ST Info field structure in an ST execution request
     * IEEE 802.11bn D2.0 Figure 9-aa55
     */
    struct StExecReqStInfo
    {
        /**
         * ST Control field structure in an ST execution request
         * IEEE 802.11bn D2.0 Figure 9-aa56
         */
        struct StControl
        {
            bool m_dlTidBmPresent{false};  ///< DL TID Bitmap Present (do not set, it is set by the
                                           ///< OptFieldWithPresenceInd)
            bool m_dlBaInfoPresent{false}; ///< DL BA Info Present (do not set, it is set by the
                                           ///< OptFieldWithPresenceInd)

            /**
             * Serialize the ST Control field
             *
             * @param start iterator pointing to where the field should be written to
             */
            void Serialize(Buffer::Iterator& start) const;

            /**
             * Deserialize the ST Control field
             *
             * @param start iterator pointing to where the field should be read from
             * @return the number of octets read
             */
            uint16_t Deserialize(Buffer::Iterator start);

            /**
             * Get the serialized size of the ST Control field
             * @return the serialized size in bytes of the ST Control field
             */
            uint16_t GetSerializedSize() const;
        };

        StControl m_stControl{}; ///< ST Control field
        OptFieldWithPresenceInd<uint8_t> m_dlTidBitmap{
            m_stControl.m_dlTidBmPresent}; ///< DL TID Bitmap field
        OptFieldWithPresenceInd<StPrepRespStInfo::BaInfo> m_dlBaInfo{
            m_stControl.m_dlBaInfoPresent}; ///< DL BA Info field

        /**
         * Constructor
         */
        StExecReqStInfo();

        /**
         * Copy constructor
         *
         * @param other the object to copy from
         */
        StExecReqStInfo(const StExecReqStInfo& other);

        /**
         * Copy assignment operator
         * @param other the object to assign from
         * @return a reference to this object
         */
        StExecReqStInfo& operator=(const StExecReqStInfo& other);

        /**
         * Serialize the ST Info field
         *
         * @param start iterator pointing to where the field should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the ST Info field
         *
         * @param start iterator pointing to where the field should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the ST Info field
         *
         * @return the serialized size in bytes of the ST Info field
         */
        uint16_t GetSerializedSize() const;

        /**
         * Print the ST Info field
         *
         * @param os the output stream
         */
        void Print(std::ostream& os) const;
    };

    /**
     * ST Info field structure in an ST execution response
     * IEEE 802.11bn D2.0 Figure 9-aa57
     */
    struct StExecRespStInfo
    {
        /**
         * ST Control field structure in an ST execution response
         * IEEE 802.11bn D2.0 Figure 9-aa58
         */
        struct StControl
        {
            bool m_nomMaxDlDrainPeriodDurPresent{
                false}; ///< Nominal Maximum DL Draining Period Duration Present
                        ///< (do not set, it is set by the OptFieldWithPresenceInd)
            bool m_latestUlSnListPresent{false}; ///< Latest UL SN List Present (do not set, it is
                                                 ///< set by the OptFieldWithPresenceInd)

            /**
             * Serialize the ST Control field
             *
             * @param start iterator pointing to where the field should be written to
             */
            void Serialize(Buffer::Iterator& start) const;

            /**
             * Deserialize the ST Control field
             *
             * @param start iterator pointing to where the field should be read from
             * @return the number of octets read
             */
            uint16_t Deserialize(Buffer::Iterator start);

            /**
             * Get the serialized size of the ST Control field
             * @return the serialized size in bytes of the ST Control field
             */
            uint16_t GetSerializedSize() const;
        };

        /**
         * Latest UL SN List field structure in an ST execution response
         * IEEE 802.11bn D2.0 Figure 9-aa59
         */
        struct LatestUlSnList
        {
            /**
             * Set the Latest UL SN for a given TID
             *
             * @param tid the TID
             * @param sn the Latest UL SN value
             */
            void Set(tid_t tid, uint16_t sn);

            /**
             * Get the Latest UL SN for a given TID
             *
             * @param tid the TID
             * @return the Latest UL SN value, or std::nullopt if not present
             */
            std::optional<uint16_t> Get(tid_t tid) const;

            /**
             * Get the TID Bitmap field
             *
             * @return the TID Bitmap field
             */
            uint8_t GetTidBitmap() const;

            /**
             * Get the Latest UL SN per TID fields
             *
             * @return the Latest UL SN per TID fields
             */
            const std::map<tid_t, uint16_t>& GetLatestUlSnPerTid() const;

            /**
             * Serialize the Latest UL SN List field
             *
             * @param start iterator pointing to where the field should be written to
             */
            void Serialize(Buffer::Iterator& start) const;

            /**
             * Deserialize the Latest UL SN List field
             *
             * @param start iterator pointing to where the field should be read from
             * @return the number of octets read
             */
            uint16_t Deserialize(Buffer::Iterator start);

            /**
             * Get the serialized size of the Latest UL SN List field
             * @return the serialized size in bytes of the Latest UL SN List field
             */
            uint16_t GetSerializedSize() const;

            /**
             * Print the Latest UL SN List field
             *
             * @param os the output stream
             */
            void Print(std::ostream& os) const;

          private:
            uint8_t m_tidBitmap{0};                         ///< TID Bitmap field
            std::map<tid_t, uint16_t> m_latestUlSnPerTid{}; ///< Latest UL SN per TID fields
        };

        StControl m_stControl{}; ///< ST Control field
        OptFieldWithPresenceInd<uint16_t> m_nomMaxDlDrainPeriodDur{
            m_stControl.m_nomMaxDlDrainPeriodDurPresent}; ///< Nominal Maximum DL Draining
                                                          ///< Period Duration field
        OptFieldWithPresenceInd<LatestUlSnList> m_latestUlSnList{
            m_stControl.m_latestUlSnListPresent}; ///< Latest UL SN List field

        /**
         * Constructor
         */
        StExecRespStInfo();

        /**
         * Copy constructor
         *
         * @param other the object to copy from
         */
        StExecRespStInfo(const StExecRespStInfo& other);

        /**
         * Copy assignment operator
         * @param other the object to assign from
         * @return a reference to this object
         */
        StExecRespStInfo& operator=(const StExecRespStInfo& other);

        /**
         * Serialize the ST Info field
         *
         * @param start iterator pointing to where the field should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the ST Info field
         *
         * @param start iterator pointing to where the field should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the ST Info field
         *
         * @return the serialized size in bytes of the ST Info field
         */
        uint16_t GetSerializedSize() const;

        /**
         * Print the ST Info field
         *
         * @param os the output stream
         */
        void Print(std::ostream& os) const;
    };

    /// Typedef for structure holding a reference to the containing frame
    using StInfo = std::variant<std::monostate,
                                StPrepReqStInfo,
                                StPrepRespStInfo,
                                StExecReqStInfo,
                                StExecRespStInfo>;

    /**
     * Constructor
     *
     * @param stInfo the ST Info field
     */
    SmdBssTransitionParameters(const StInfo& stInfo = {});
    /**
     * Constructor
     *
     * @param containingFrameType the type of the frame containing the SMD BSS Transition Parameters
     * element, used to initialize the ST Info field with the appropriate structure
     */
    explicit SmdBssTransitionParameters(ContainingFrameType containingFrameType);
    ~SmdBssTransitionParameters() override = default;

    /**
     * @return the type of the frame containing this SMD BSS Transition Parameters element
     */
    ContainingFrameType GetContainingFrameType() const;

    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
    void Print(std::ostream& os) const override;

    StInfo m_stInfo; ///< ST Info field

  private:
    /**
     * Set the type of the frame containing this SMD BSS Transition Parameters element. This is used
     * to initialize the ST Info field with the appropriate structure.
     *
     * @param type the type of the frame containing this SMD BSS Transition Parameters element
     */
    void SetContainingFrameType(ContainingFrameType type);
};

} // namespace ns3

#endif /* SMD_BSS_TRANSITION_PARAMETERS_H */
