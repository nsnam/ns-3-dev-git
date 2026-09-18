/*
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef THREE_GPP_PROPAGATION_LOSS_MODEL_H
#define THREE_GPP_PROPAGATION_LOSS_MODEL_H

#include "channel-condition-model.h"
#include "propagation-loss-model.h"

namespace ns3
{

/**
 * @ingroup propagation
 *
 * @brief Base class for the 3GPP propagation models
 */
class ThreeGppPropagationLossModel : public PropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    ThreeGppPropagationLossModel();

    /**
     * Destructor
     */
    ~ThreeGppPropagationLossModel() override;

    // Delete copy constructor and assignment operator to avoid misuse
    ThreeGppPropagationLossModel(const ThreeGppPropagationLossModel&) = delete;
    ThreeGppPropagationLossModel& operator=(const ThreeGppPropagationLossModel&) = delete;

    /**
     * @brief Set the channel condition model used to determine the channel
     *        state (e.g., the LOS/NLOS condition)
     * @param model pointer to the channel condition model
     */
    void SetChannelConditionModel(Ptr<ChannelConditionModel> model);

    /**
     * @brief Returns the associated channel condition model
     * @return the channel condition model
     */
    Ptr<ChannelConditionModel> GetChannelConditionModel() const;

    /**
     * @brief Set the central frequency of the model
     * @param f the central frequency in the range in Hz, between 500.0e6 and 100.0e9 Hz
     */
    void SetFrequency(double f);

    /**
     * @brief Return the current central frequency
     * @return The current central frequency
     */
    double GetFrequency() const;

    /**
     * @brief Return true if the O2I Building Penetration loss
     *        corresponds to a low loss condition.
     * @param cond The ptr to the channel condition model
     * @return True for low loss, false for high
     */
    bool IsO2iLowPenetrationLoss(Ptr<const ChannelCondition> cond) const;

    /**
     * @brief Clear cached O2I Building Penetration loss.
     *        Triggers a recomputation of losses.
     *        Used for testing.
     */
    void ClearO2iLossCacheMap();

  private:
    /**
     * Computes the received power by applying the pathloss model described in
     * 3GPP TR 38.901
     *
     * @param txPowerDbm tx power in dBm
     * @param a tx mobility model
     * @param b rx mobility model
     * @return the rx power in dBm
     */
    double DoCalcRxPower(double txPowerDbm,
                         Ptr<MobilityModel> a,
                         Ptr<MobilityModel> b) const override;

    /**
     * @brief Computes the pathloss between a and b
     * @param cond the channel condition
     * @param a tx mobility model
     * @param b rx mobility model
     * @return pathloss value in dB
     */
    double GetLoss(Ptr<ChannelCondition> cond, Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;

    /**
     * @brief Computes the pathloss between a and b considering that the line of
     *        sight is not obstructed
     * @param a tx mobility model
     * @param b rx mobility model
     * @return pathloss value in dB
     */
    virtual double GetLossLos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const = 0;

    /**
     * @brief Computes the pathloss between a and b considering that the line of
     *        sight is obstructed
     * @param a tx mobility model
     * @param b rx mobility model
     * @return pathloss value in dB
     */
    virtual double GetLossNlos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const = 0;

    /**
     * @brief Returns the minimum of the two independently generated distances
     *        according to the uniform distribution between the minimum and the maximum
     *        value depending on the specific 3GPP scenario (UMa, UMi-Street Canyon, RMa),
     *        i.e., between 0 and 25 m for UMa and UMi-Street Canyon, and between 0 and 10 m
     *        for RMa.
     *        According to 3GPP TR 38.901 this 2D−in distance shall be UT-specifically
     *        generated. 2D−in distance is used for the O2I penetration losses
     *        calculation according to 3GPP TR 38.901 7.4.3.
     *        See GetO2iLowPenetrationLoss/GetO2iHighPenetrationLoss functions.
     * @return Returns 02i 2D distance (in meters) used to calculate low/high losses.
     */
    virtual double GetO2iDistance2dIn() const = 0;

    /**
     * @brief Retrieves the o2i building penetration loss value by looking at m_o2iLossMap.
     *        If not found or if the channel condition changed it generates a new
     *        independent realization and stores it in the map, otherwise it calculates
     *        a new value as defined in 3GPP TR 38.901 7.4.3.1.
     *
     *
     * @param a tx mobility model (used for the key calculation)
     * @param b rx mobility model (used for the key calculation)
     * @param cond the LOS/NLOS channel condition
     * @return o2iLoss
     */
    double GetO2iSub6GhzPenetrationLoss(Ptr<MobilityModel> a,
                                        Ptr<MobilityModel> b,
                                        ChannelCondition::LosConditionValue cond) const;

    /**
     * @brief Retrieves the o2i building penetration loss value by looking at m_o2iLossMap.
     *        If not found or if the channel condition changed it generates a new
     *        independent realization and stores it in the map, otherwise it calculates
     *        a new value as defined in 3GPP TR 38.901 7.4.3.1.
     *
     *        Note that all child classes should implement this function to support
     *        low losses calculation. As such, this function should be purely virtual.
     *
     * @param a tx mobility model (used for the key calculation)
     * @param b rx mobility model (used for the key calculation)
     * @param cond the LOS/NLOS channel condition
     * @return o2iLoss
     */
    virtual double GetO2iLowPenetrationLoss(Ptr<MobilityModel> a,
                                            Ptr<MobilityModel> b,
                                            ChannelCondition::LosConditionValue cond) const;

    /**
     * @brief Retrieves the o2i building penetration loss value by looking at m_o2iLossMap.
     *        If not found or if the channel condition changed it generates a new
     *        independent realization and stores it in the map, otherwise it calculates
     *        a new value as defined in 3GPP TR 38.901 7.4.3.1.
     *
     *        Note that all child classes should implement this function to support
     *        high losses calculation. As such, this function should be purely virtual.
     *
     * @param a tx mobility model (used for the key calculation)
     * @param b rx mobility model (used for the key calculation)
     * @param cond the LOS/NLOS channel condition
     * @return o2iLoss
     */
    virtual double GetO2iHighPenetrationLoss(Ptr<MobilityModel> a,
                                             Ptr<MobilityModel> b,
                                             ChannelCondition::LosConditionValue cond) const;

    /**
     * @brief Computes the o2i vehicular penetration loss by looking at m_o2iVehicularUtLossMap.
     *        The loss is composed by the sum of losses for each UT, if vehicles,
     *        as implied by their speed (30-120 km/h).
     *        The loss for each UT is defined in 3GPP TR 38.901 7.4.3.2.
     *
     * @param a tx mobility model (used for the key calculation)
     * @param b rx mobility model (used for the key calculation)
     * @param cond the O2I channel condition (O2I, O2O)
     * @return vehicular o2iLoss
     */
    double GetO2iVehicularLoss(Ptr<MobilityModel> a,
                               Ptr<MobilityModel> b,
                               ChannelCondition::O2iConditionValue cond) const;
    /**
     * @brief Indicates the condition of the o2i building penetration loss
     *        (defined in 3GPP TR 38.901 7.4.3.1).
     *        The default implementation returns the condition as set
     *        (either based on the buildings materials, or if the probabilistic
     *        model is used in the ThreeGppChannelConditionModel, then
     *        based on the result of a random variable).
     *        The derived classes can change the default behavior by overriding
     *        this method.
     * @param cond the ptr to the channel condition model
     * @return True for low losses, false for high losses
     */
    virtual bool DoIsO2iLowPenetrationLoss(Ptr<const ChannelCondition> cond) const;

    /**
     * @brief Computes the pathloss between a and b considering that the line of
     *        sight is obstructed by a vehicle. By default it raises an error to
     *        avoid misuse.
     * @param a tx mobility model
     * @param b rx mobility model
     * @return pathloss value in dB
     */
    virtual double GetLossNlosv(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const;

    /**
     * @brief Retrieves the shadowing value by looking at m_shadowingMap.
     *        If not found or if the channel condition changed it generates a new
     *        independent realization and stores it in the map, otherwise it correlates
     *        the new value with the previous one using the autocorrelation function
     *        defined in 3GPP TR 38.901, Sec. 7.4.4.
     * @param a tx mobility model
     * @param b rx mobility model
     * @param cond the LOS/NLOS channel condition
     * @return shadowing loss in dB
     */
    double GetShadowing(Ptr<MobilityModel> a,
                        Ptr<MobilityModel> b,
                        ChannelCondition::LosConditionValue cond) const;

    /**
     * @brief Sample the SpatialGaussianField of shadow fading owned by a
     *        (site, condition slot) pair, see the InterUeSpatialConsistency
     *        attribute.
     *
     * @param siteNodeId Node id of the site endpoint owning the field.
     * @param condSlot Channel condition slot (0 = LOS, 1 = NLOS, 2 = O2I).
     * @param position Sampling position (only x and y are used).
     * @param corrDist Correlation distance in meters; non-positive values
     *        degrade to a single deterministic draw at the position.
     * @return A sample of the field with N(0,1) marginal distribution.
     */
    double SampleSpatiallyCorrelatedNormal(uint32_t siteNodeId,
                                           uint8_t condSlot,
                                           const Vector& position,
                                           double corrDist) const;

    /**
     * @brief Returns the shadow fading standard deviation
     * @param a tx mobility model
     * @param b rx mobility model
     * @param cond the LOS/NLOS channel condition
     * @return shadowing std in dB
     */
    virtual double GetShadowingStd(Ptr<MobilityModel> a,
                                   Ptr<MobilityModel> b,
                                   ChannelCondition::LosConditionValue cond) const = 0;

    /**
     * @brief Returns the shadow fading correlation distance
     * @param cond the LOS/NLOS channel condition
     * @return shadowing correlation distance in meters
     */
    virtual double GetShadowingCorrelationDistance(
        ChannelCondition::LosConditionValue cond) const = 0;

    /**
     * @brief Returns the shadow fading correlation distance of O2I links (3GPP
     *        TR 38.901 Table 7.5-6, O2I column); scenarios without an O2I
     *        column reuse the NLOS distance.
     * @return shadowing correlation distance in meters
     */
    virtual double GetO2iShadowingCorrelationDistance() const;

    /**
     * @brief Returns an unique key for the channel between a and b.
     *
     * The key is the value of the Cantor function calculated by using as
     * first parameter the lowest node ID, and as a second parameter the highest
     * node ID.
     *
     * @param a tx mobility model
     * @param b rx mobility model
     * @return channel key
     */
    static uint32_t GetKey(Ptr<MobilityModel> a, Ptr<MobilityModel> b);

    /**
     * @brief Get the difference between the node position
     *
     * The difference is calculated as (b-a) if Id(a) < Id (b), or
     * (a-b) if Id(b) <= Id(a).
     *
     * @param a First node
     * @param b Second node
     * @return the difference between the node vector position
     */
    static Vector GetVectorDifference(Ptr<MobilityModel> a, Ptr<MobilityModel> b);

  protected:
    void DoDispose() override;
    void NotifyConstructionCompleted() override;
    /**
     * @brief Returns a a single, link-specific, uniformly distributed variable
     *        value depending on the specific 3GPP scenario (UMa, UMi-Street Canyon, RMa),
     *        i.e., between 0 and 25 m for UMa and UMi-Street Canyon.
     *        According to 3GPP TR 38.901 this 2D−in distance shall be UT-specifically
     *        generated. 2D−in distance is used for the O2I penetration losses
     *        calculation according to 3GPP TR 38.901 7.4.3.
     *        See GetO2iLowPenetrationLoss/GetO2iHighPenetrationLoss functions.
     * @return Returns 02i 2D distance (in meters) used to calculate low/high losses.
     */
    virtual double GetO2iDistance2dInSub6Ghz() const;

    /**
     * @brief Returns the upper bound of the uniform variables of the O2I 2D-in
     *        distance of GetO2iDistance2dIn(), i.e., 25 m for UMa and
     *        UMi-Street Canyon and 10 m for RMa. The base implementation
     *        returns 0 (no indoor distance).
     * @return The upper bound in meters.
     */
    virtual double GetO2iDistance2dInMax() const;

    /**
     * @brief Returns the upper bound of the uniform variable of the O2I 2D-in
     *        distance of GetO2iDistance2dInSub6Ghz(), i.e., 25 m for UMa and
     *        UMi-Street Canyon. The base implementation returns 0 (no indoor
     *        distance).
     * @return The upper bound in meters.
     */
    virtual double GetO2iDistance2dInSub6GhzMax() const;

    /**
     * @brief Computes the 2D distance between two 3D vectors
     * @param a the first 3D vector
     * @param b the second 3D vector
     * @return the 2D distance between a and b
     */
    static double Calculate2dDistance(Vector a, Vector b);

    Ptr<ChannelConditionModel> m_channelConditionModel; //!< pointer to the channel condition model
    double m_frequency;                                 //!< operating frequency in Hz
    bool m_shadowingEnabled;                            //!< enable/disable shadowing
    bool m_enforceRanges;            //!< strictly enforce TR 38.901 parameter ranges
    bool m_buildingPenLossesEnabled; //!< enable/disable building penetration losses
    double m_meanVehicleO2iLoss; //!< normal cars (9dB), cars with metal coated glass panels (20dB)
    Ptr<NormalRandomVariable> m_normRandomVariable; //!< normal random variable

    /** Define a struct for the m_shadowingMap entries */
    struct ShadowingMapItem
    {
        double m_shadowing;                              //!< the shadowing loss in dB
        ChannelCondition::LosConditionValue m_condition; //!< the LOS/NLOS condition
        Vector m_distance;                               //!< the vector AB
    };

    mutable std::unordered_map<uint32_t, ShadowingMapItem>
        m_shadowingMap; //!< map to store the shadowing values

    /** Define a struct for the m_o2iLossMap entries */
    struct O2iLossMapItem
    {
        double m_o2iLoss;                                //!< the o2i loss in dB
        ChannelCondition::LosConditionValue m_condition; //!< the LOS/NLOS condition
    };

    mutable std::unordered_map<uint32_t, O2iLossMapItem>
        m_o2iLossMap; //!< map to store the o2i Loss values

    mutable std::unordered_map<uint32_t, double>
        m_o2iVehicularUtLossMap; //!< vehicular O2I loss for each individual UT with speed x, such
                                 //!< that 30 < x <= 120 km/h.
                                 //!< UT is addressed by NodeId

    Ptr<UniformRandomVariable> m_randomO2iVar1; //!< a uniform random variable for the calculation
                                                //!< of the indoor loss, see TR38.901 Table 7.4.3-2
    Ptr<UniformRandomVariable> m_randomO2iVar2; //!< a uniform random variable for the calculation
                                                //!< of the indoor loss, see TR38.901 Table 7.4.3-2
    Ptr<NormalRandomVariable>
        m_normalO2iLowLossVar; //!< a normal random variable for the calculation of 02i low loss,
                               //!< see TR38.901 Table 7.4.3-2
    Ptr<NormalRandomVariable>
        m_normalO2iHighLossVar; //!< a normal random variable for the calculation of 02i high loss,
                                //!< see TR38.901 Table 7.4.3-2

    Ptr<NormalRandomVariable>
        m_normalO2iVehicularLossVar; //!< a normal random variable for the calculation of
                                     //!< penetration loss for vehicles see TR38.901 section 7.4.3.2
    int64_t DoAssignStreams(int64_t stream) override;
};

/**
 * @ingroup propagation
 *
 * @brief Implements the pathloss model defined in 3GPP TR 38.901, Table 7.4.1-1
 *        for the RMa scenario.
 */
class ThreeGppRmaPropagationLossModel : public ThreeGppPropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    ThreeGppRmaPropagationLossModel();

    /**
     * Destructor
     */
    ~ThreeGppRmaPropagationLossModel() override;

    // Delete copy constructor and assignment operator to avoid misuse
    ThreeGppRmaPropagationLossModel(const ThreeGppRmaPropagationLossModel&) = delete;
    ThreeGppRmaPropagationLossModel& operator=(const ThreeGppRmaPropagationLossModel&) = delete;

  private:
    /**
     * @brief Computes the pathloss between a and b considering that the line of
     *        sight is not obstructed
     * @param a tx mobility model
     * @param b rx mobility model
     * @return pathloss value in dB
     */
    double GetLossLos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;

    /**
     * @brief Returns the minimum of the two independently generated distances
     *        according to the uniform distribution between the minimum and the maximum
     *        value depending on the specific 3GPP scenario (UMa, UMi-Street Canyon, RMa),
     *        i.e., between 0 and 25 m for UMa and UMi-Street Canyon, and between 0 and 10 m
     *        for RMa.
     *        According to 3GPP TR 38.901 this 2D−in distance shall be UT-specifically
     *        generated. 2D−in distance is used for the O2I penetration losses
     *        calculation according to 3GPP TR 38.901 7.4.3.
     *        See GetO2iLowPenetrationLoss/GetO2iHighPenetrationLoss functions.
     * @return Returns 02i 2D distance (in meters) used to calculate low/high losses.
     */
    double GetO2iDistance2dIn() const override;
    double GetO2iDistance2dInMax() const override;

    /**
     * @brief Indicates the condition of the o2i building penetration loss
     *        (defined in 3GPP TR 38.901 7.4.3.1).
     * @param cond the ptr to the channel condition model
     * @return True for low losses, false for high losses
     */
    bool DoIsO2iLowPenetrationLoss(Ptr<const ChannelCondition> cond) const override;

    /**
     * @brief Computes the pathloss between a and b considering that the line of
     *        sight is obstructed
     * @param a tx mobility model
     * @param b rx mobility model
     * @return pathloss value in dB
     */
    double GetLossNlos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;

    /**
     * @brief Returns the shadow fading standard deviation
     * @param a tx mobility model
     * @param b rx mobility model
     * @param cond the LOS/NLOS channel condition
     * @return shadowing std in dB
     */
    double GetShadowingStd(Ptr<MobilityModel> a,
                           Ptr<MobilityModel> b,
                           ChannelCondition::LosConditionValue cond) const override;

    /**
     * @brief Returns the shadow fading correlation distance
     * @param cond the LOS/NLOS channel condition
     * @return shadowing correlation distance in meters
     */
    double GetShadowingCorrelationDistance(ChannelCondition::LosConditionValue cond) const override;
    double GetO2iShadowingCorrelationDistance() const override;

    /**
     * @brief Computes the PL1 formula for the RMa scenario
     * @param frequency the operating frequency in Hz
     * @param distance3D the 3D distance between the tx and the rx nodes in meters
     * @param h the average building height in meters
     * @param w the average street width in meters
     * @return result of the PL1 formula
     */
    static double Pl1(double frequency, double distance3D, double h, double w);

    /**
     * @brief Computes the breakpoint distance for the RMa scenario
     * @param frequency the operating frequency in Hz
     * @param hA height of the tx node in meters
     * @param hB height of the rx node in meters
     * @return the breakpoint distance in meters
     */
    static double GetBpDistance(double frequency, double hA, double hB);

    double m_h; //!< average building height in meters
    double m_w; //!< average street width in meters
};

/**
 * @ingroup propagation
 *
 * @brief Implements the pathloss model defined in 3GPP TR 38.901, Table 7.4.1-1
 *        for the UMa scenario.
 */
class ThreeGppUmaPropagationLossModel : public ThreeGppPropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    ThreeGppUmaPropagationLossModel();

    /**
     * Destructor
     */
    ~ThreeGppUmaPropagationLossModel() override;

    // Delete copy constructor and assignment operator to avoid misuse
    ThreeGppUmaPropagationLossModel(const ThreeGppUmaPropagationLossModel&) = delete;
    ThreeGppUmaPropagationLossModel& operator=(const ThreeGppUmaPropagationLossModel&) = delete;

  private:
    int64_t DoAssignStreams(int64_t stream) override;

    /**
     * @brief Computes the pathloss between a and b considering that the line of
     *        sight is not obstructed
     * @param a tx mobility model
     * @param b rx mobility model
     * @return pathloss value in dB
     */
    double GetLossLos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;

    /**
     * @brief Returns the minimum of the two independently generated distances
     *        according to the uniform distribution between the minimum and the maximum
     *        value depending on the specific 3GPP scenario (UMa, UMi-Street Canyon, RMa),
     *        i.e., between 0 and 25 m for UMa and UMi-Street Canyon, and between 0 and 10 m
     *        for RMa.
     *        According to 3GPP TR 38.901 this 2D−in distance shall be UT-specifically
     *        generated. 2D−in distance is used for the O2I penetration losses
     *        calculation according to 3GPP TR 38.901 7.4.3.
     *        See GetO2iLowPenetrationLoss/GetO2iHighPenetrationLoss functions.
     * @return Returns 02i 2D distance (in meters) used to calculate low/high losses.
     */
    double GetO2iDistance2dIn() const override;

    /**
     * @brief Returns a a single, link-specific, uniformly distributed variable
     *        value depending on the specific 3GPP scenario (UMa, UMi-Street Canyon, RMa),
     *        i.e., between 0 and 25 m for UMa and UMi-Street Canyon.
     *        According to 3GPP TR 38.901 this 2D−in distance shall be UT-specifically
     *        generated. 2D−in distance is used for the O2I penetration losses
     *        calculation according to 3GPP TR 38.901 7.4.3.
     *        See GetO2iLowPenetrationLoss/GetO2iHighPenetrationLoss functions.
     * @return Returns 02i 2D distance (in meters) used to calculate low/high losses.
     */
    double GetO2iDistance2dInSub6Ghz() const override;
    double GetO2iDistance2dInMax() const override;
    double GetO2iDistance2dInSub6GhzMax() const override;

    /**
     * @brief Computes the pathloss between a and b considering that the line of
     *        sight is obstructed.
     * @param a tx mobility model
     * @param b rx mobility model
     * @return pathloss value in dB
     */
    double GetLossNlos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;

    /**
     * @brief Returns the shadow fading standard deviation
     * @param a tx mobility model
     * @param b rx mobility model
     * @param cond the LOS/NLOS channel condition
     * @return shadowing std in dB
     */
    double GetShadowingStd(Ptr<MobilityModel> a,
                           Ptr<MobilityModel> b,
                           ChannelCondition::LosConditionValue cond) const override;

    /**
     * @brief Returns the shadow fading correlation distance
     * @param cond the LOS/NLOS channel condition
     * @return shadowing correlation distance in meters
     */
    double GetShadowingCorrelationDistance(ChannelCondition::LosConditionValue cond) const override;
    double GetO2iShadowingCorrelationDistance() const override;

    /**
     * @brief Computes the breakpoint distance
     * @param hUt height of the UT in meters
     * @param hBs height of the BS in meters
     * @param distance2D distance between the two nodes in meters
     * @return the breakpoint distance in meters
     */
    double GetBpDistance(double hUt, double hBs, double distance2D) const;

    Ptr<UniformRandomVariable> m_uniformVar; //!< a uniform random variable used for the computation
                                             //!< of the breakpoint distance
};

/**
 * @ingroup propagation
 *
 * @brief Implements the pathloss model defined in 3GPP TR 38.901, Table 7.4.1-1
 *        for the UMi-Street Canyon scenario.
 */
class ThreeGppUmiStreetCanyonPropagationLossModel : public ThreeGppPropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    ThreeGppUmiStreetCanyonPropagationLossModel();

    /**
     * Destructor
     */
    ~ThreeGppUmiStreetCanyonPropagationLossModel() override;

    // Delete copy constructor and assignment operator to avoid misuse
    ThreeGppUmiStreetCanyonPropagationLossModel(
        const ThreeGppUmiStreetCanyonPropagationLossModel&) = delete;
    ThreeGppUmiStreetCanyonPropagationLossModel& operator=(
        const ThreeGppUmiStreetCanyonPropagationLossModel&) = delete;

  private:
    /**
     * @brief Computes the pathloss between a and b considering that the line of
     *        sight is not obstructed
     * @param a tx mobility model
     * @param b rx mobility model
     * @return pathloss value in dB
     */
    double GetLossLos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;

    /**
     * @brief Returns the minimum of the two independently generated distances
     *        according to the uniform distribution between the minimum and the maximum
     *        value depending on the specific 3GPP scenario (UMa, UMi-Street Canyon, RMa),
     *        i.e., between 0 and 25 m for UMa and UMi-Street Canyon, and between 0 and 10 m
     *        for RMa.
     *        According to 3GPP TR 38.901 this 2D−in distance shall be UT-specifically
     *        generated. 2D−in distance is used for the O2I penetration losses
     *        calculation according to 3GPP TR 38.901 7.4.3.
     *        See GetO2iLowPenetrationLoss/GetO2iHighPenetrationLoss functions.
     * @return Returns 02i 2D distance (in meters) used to calculate low/high losses.
     */
    double GetO2iDistance2dIn() const override;

    /**
     * @brief Returns a a single, link-specific, uniformly distributed variable
     *        value depending on the specific 3GPP scenario (UMa, UMi-Street Canyon, RMa),
     *        i.e., between 0 and 25 m for UMa and UMi-Street Canyon.
     *        According to 3GPP TR 38.901 this 2D−in distance shall be UT-specifically
     *        generated. 2D−in distance is used for the O2I penetration losses
     *        calculation according to 3GPP TR 38.901 7.4.3.
     *        See GetO2iLowPenetrationLoss/GetO2iHighPenetrationLoss functions.
     * @return Returns 02i 2D distance (in meters) used to calculate low/high losses.
     */
    double GetO2iDistance2dInSub6Ghz() const override;
    double GetO2iDistance2dInMax() const override;
    double GetO2iDistance2dInSub6GhzMax() const override;

    /**
     * @brief Computes the pathloss between a and b considering that the line of
     *        sight is obstructed.
     * @param a tx mobility model
     * @param b rx mobility model
     * @return pathloss value in dB
     */
    double GetLossNlos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;

    /**
     * @brief Returns the shadow fading standard deviation
     * @param a tx mobility model
     * @param b rx mobility model
     * @param cond the LOS/NLOS channel condition
     * @return shadowing std in dB
     */
    double GetShadowingStd(Ptr<MobilityModel> a,
                           Ptr<MobilityModel> b,
                           ChannelCondition::LosConditionValue cond) const override;

    /**
     * @brief Returns the shadow fading correlation distance
     * @param cond the LOS/NLOS channel condition
     * @return shadowing correlation distance in meters
     */
    double GetShadowingCorrelationDistance(ChannelCondition::LosConditionValue cond) const override;
    double GetO2iShadowingCorrelationDistance() const override;

    /**
     * @brief Computes the breakpoint distance
     * @param hUt height of the UT node in meters
     * @param hBs height of the BS node in meters
     * @param distance2D distance between the two nodes in meters
     * @return the breakpoint distance in meters
     */
    double GetBpDistance(double hUt, double hBs, double distance2D) const;
};

/**
 * @ingroup propagation
 *
 * @brief Implements the pathloss model defined in 3GPP TR 38.901, Table 7.4.1-1
 *        for the Indoor Office scenario.
 */
class ThreeGppIndoorOfficePropagationLossModel : public ThreeGppPropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    ThreeGppIndoorOfficePropagationLossModel();

    /**
     * Destructor
     */
    ~ThreeGppIndoorOfficePropagationLossModel() override;

    // Delete copy constructor and assignment operator to avoid misuse
    ThreeGppIndoorOfficePropagationLossModel(const ThreeGppIndoorOfficePropagationLossModel&) =
        delete;
    ThreeGppIndoorOfficePropagationLossModel& operator=(
        const ThreeGppIndoorOfficePropagationLossModel&) = delete;

  private:
    /**
     * @brief Computes the pathloss between a and b considering that the line of
     *        sight is not obstructed
     * @param a tx mobility model
     * @param b rx mobility model
     * @return pathloss value in dB
     */
    double GetLossLos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;

    /**
     * @brief Returns the minimum of the two independently generated distances
     *        according to the uniform distribution between the minimum and the maximum
     *        value depending on the specific 3GPP scenario (UMa, UMi-Street Canyon, RMa),
     *        i.e., between 0 and 25 m for UMa and UMi-Street Canyon, and between 0 and 10 m
     *        for RMa.
     *        According to 3GPP TR 38.901 this 2D−in distance shall be UT-specifically
     *        generated. 2D−in distance is used for the O2I penetration losses
     *        calculation according to 3GPP TR 38.901 7.4.3.
     *        See GetO2iLowPenetrationLoss/GetO2iHighPenetrationLoss functions.
     * @return Returns 02i 2D distance (in meters) used to calculate low/high losses.
     */
    double GetO2iDistance2dIn() const override;

    /**
     * @brief Computes the pathloss between a and b considering that the line of
     *        sight is obstructed
     * @param a tx mobility model
     * @param b rx mobility model
     * @return pathloss value in dB
     */
    double GetLossNlos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;

    /**
     * @brief Returns the shadow fading standard deviation
     * @param a tx mobility model
     * @param b rx mobility model
     * @param cond the LOS/NLOS channel condition
     * @return shadowing std in dB
     */
    double GetShadowingStd(Ptr<MobilityModel> a,
                           Ptr<MobilityModel> b,
                           ChannelCondition::LosConditionValue cond) const override;

    /**
     * @brief Returns the shadow fading correlation distance
     * @param cond the LOS/NLOS channel condition
     * @return shadowing correlation distance in meters
     */
    double GetShadowingCorrelationDistance(ChannelCondition::LosConditionValue cond) const override;
};

/**
 * @ingroup propagation
 *
 * @brief Base class for the NTN pathloss models defined in 3GPP TR 38.811.
 *
 * This class holds the path loss computations common to all NTN scenarios.
 * The scenario subclasses provide the scenario-specific channel condition
 * model, shadow fading and clutter loss table, and shadowing correlation
 * distances.
 *
 * The model is defined for satellite-to-ground links with elevation angles
 * between 10 and 90 degrees. For links outside this domain, i.e., with a
 * non-positive elevation angle (such as between two ground terminals, or
 * with the satellite below the horizon) or an undefined elevation angle
 * (coincident positions), the elevation-dependent terms are not evaluated
 * and a fixed loss of 500 dB is returned instead.
 */
class ThreeGppNTNPropagationLossModel : public ThreeGppPropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @copydoc ThreeGppPropagationLossModel::GetO2iDistance2dIn
     *  Does nothing in NTN scenarios.
     */
    double GetO2iDistance2dIn() const override;

  protected:
    /**
     * @brief The nested map containing the Shadow Fading and Clutter Loss
     *        values for the scenario, set by the subclass constructor
     */
    const std::map<int, std::vector<float>>* m_SFCL{nullptr};
    double m_shadowingCorrDistanceLos{0};  //!< shadowing correlation distance for LOS, in meters
    double m_shadowingCorrDistanceNlos{0}; //!< shadowing correlation distance for NLOS, in meters
    bool m_ionoScintLossEnabled{true};  //!< whether the ionospheric scintillation loss is applied
    bool m_tropoScintLossEnabled{true}; //!< whether the tropospheric scintillation loss is applied

  private:
    // Inherited
    double GetLossLos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;
    double GetLossNlos(Ptr<MobilityModel> a, Ptr<MobilityModel> b) const override;
    double GetShadowingStd(Ptr<MobilityModel> a,
                           Ptr<MobilityModel> b,
                           ChannelCondition::LosConditionValue cond) const override;
    double GetShadowingCorrelationDistance(ChannelCondition::LosConditionValue cond) const override;
};

/**
 * @ingroup propagation
 *
 * @brief Implements the pathloss model defined in 3GPP TR 38.811, Sec. 6.6.2,
 *        for the NTN Dense Urban scenario.
 */
class ThreeGppNTNDenseUrbanPropagationLossModel : public ThreeGppNTNPropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    ThreeGppNTNDenseUrbanPropagationLossModel();

    /**
     * Destructor
     */
    ~ThreeGppNTNDenseUrbanPropagationLossModel() override;

    /**
     * @brief Copy constructor
     *
     * Deleted in base class
     */
    ThreeGppNTNDenseUrbanPropagationLossModel(const ThreeGppNTNDenseUrbanPropagationLossModel&) =
        delete;

    /**
     * @brief Copy constructor
     *
     * Deleted in base class
     * @returns the ThreeGppNTNDenseUrbanPropagationLossModel instance
     */
    ThreeGppNTNDenseUrbanPropagationLossModel& operator=(
        const ThreeGppNTNDenseUrbanPropagationLossModel&) = delete;
};

/**
 * @ingroup propagation
 *
 * @brief Implements the pathloss model defined in 3GPP TR 38.811, Sec. 6.6.2,
 *        for the NTN Urban scenario.
 */
class ThreeGppNTNUrbanPropagationLossModel : public ThreeGppNTNPropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    ThreeGppNTNUrbanPropagationLossModel();

    /**
     * Destructor
     */
    ~ThreeGppNTNUrbanPropagationLossModel() override;

    /**
     * @brief Copy constructor
     *
     * Deleted in base class
     */
    ThreeGppNTNUrbanPropagationLossModel(const ThreeGppNTNUrbanPropagationLossModel&) = delete;

    /**
     * @brief Copy constructor
     *
     * Deleted in base class
     * @returns the ThreeGppNTNUrbanPropagationLossModel instance
     */
    ThreeGppNTNUrbanPropagationLossModel& operator=(const ThreeGppNTNUrbanPropagationLossModel&) =
        delete;
};

/**
 * @ingroup propagation
 *
 * @brief Implements the pathloss model defined in 3GPP TR 38.811, Sec. 6.6.2,
 *        for the NTN Suburban scenario.
 */
class ThreeGppNTNSuburbanPropagationLossModel : public ThreeGppNTNPropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    ThreeGppNTNSuburbanPropagationLossModel();

    /**
     * Destructor
     */
    ~ThreeGppNTNSuburbanPropagationLossModel() override;

    /**
     * @brief Copy constructor
     *
     * Deleted in base class
     */
    ThreeGppNTNSuburbanPropagationLossModel(const ThreeGppNTNSuburbanPropagationLossModel&) =
        delete;

    /**
     * @brief Copy constructor
     *
     * Deleted in base class
     * @returns the ThreeGppNTNSuburbanPropagationLossModel instance
     */
    ThreeGppNTNSuburbanPropagationLossModel& operator=(
        const ThreeGppNTNSuburbanPropagationLossModel&) = delete;
};

/**
 * @ingroup propagation
 *
 * @brief Implements the pathloss model defined in 3GPP TR 38.811, Sec. 6.6.2,
 *        for the NTN Rural scenario.
 */
class ThreeGppNTNRuralPropagationLossModel : public ThreeGppNTNPropagationLossModel
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    ThreeGppNTNRuralPropagationLossModel();

    /**
     * Destructor
     */
    ~ThreeGppNTNRuralPropagationLossModel() override;

    /**
     * @brief Copy constructor
     *
     * Deleted in base class
     */
    ThreeGppNTNRuralPropagationLossModel(const ThreeGppNTNRuralPropagationLossModel&) = delete;

    /**
     * @brief Copy constructor
     *
     * Deleted in base class
     * @returns the ThreeGppNTNRuralPropagationLossModel instance
     */
    ThreeGppNTNRuralPropagationLossModel& operator=(const ThreeGppNTNRuralPropagationLossModel&) =
        delete;
};

} // namespace ns3

#endif /* THREE_GPP_PROPAGATION_LOSS_MODEL_H */
