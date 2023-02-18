/*
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
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
 */

#ifndef CHANNEL_CONDITION_MODEL_H
#define CHANNEL_CONDITION_MODEL_H

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/random-variable-stream.h"
#include "ns3/vector.h"

#include <unordered_map>

namespace ns3
{

class MobilityModel;

/**
 * \ingroup propagation
 *
 * \brief Carries information about the LOS/NLOS channel state
 *
 * Additional information about the channel condition can be aggregated to instances of
 * this class.
 */
class ChannelCondition : public Object
{
  public:
    /**
     * Possible values for Line-of-Sight condition.
     */
    enum LosConditionValue
    {
        LOS,   //!< Line of Sight
        NLOS,  //!< Non Line of Sight
        NLOSv, //!< Non Line of Sight due to a vehicle
        LC_ND  //!< Los condition not defined
    };

    /**
     * Possible values for Outdoor to Indoor condition.
     */
    enum O2iConditionValue
    {
        O2O,   //!< Outdoor to Outdoor
        O2I,   //!< Outdoor to Indoor
        I2I,   //!< Indoor to Indoor
        O2I_ND //!< Outdoor to Indoor condition not defined
    };

    /**
     * Possible values for Low-High Penetration Loss condition.
     */
    enum O2iLowHighConditionValue
    {
        LOW,      //!< Low Penetration Losses
        HIGH,     //!< High Penetration Losses
        LH_O2I_ND //!< Low-High Penetration Losses not defined
    };

    /**
     * Get the type ID.
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor for the ChannelCondition class
     */
    ChannelCondition();

    /**
     * Constructor for the ChannelCondition class
     * \param losCondition the LOS condition value
     * \param o2iCondition the O2I condition value (by default is set to O2O)
     * \param o2iLowHighCondition the O2I Low-High Building Penetration loss condition value (by
     * default is set to LOW)
     */
    ChannelCondition(LosConditionValue losCondition,
                     O2iConditionValue o2iCondition = O2O,
                     O2iLowHighConditionValue o2iLowHighCondition = LOW);

    /**
     * Destructor for the ChannelCondition class
     */
    ~ChannelCondition() override;

    /**
     * Get the LosConditionValue containing the information about the LOS/NLOS
     * state of the channel
     *
     * \return the LosConditionValue
     */
    LosConditionValue GetLosCondition() const;

    /**
     * Set the LosConditionValue with the information about the LOS/NLOS
     * state of the channel
     *
     * \param losCondition the LosConditionValue
     */
    void SetLosCondition(LosConditionValue losCondition);

    /**
     * Get the O2iConditionValue containing the information about the O2I
     * state of the channel
     *
     * \return the O2iConditionValue
     */
    O2iConditionValue GetO2iCondition() const;

    /**
     * Set the O2iConditionValue containing the information about the O2I
     * state of the channel
     *
     * \param o2iCondition the O2iConditionValue
     */
    void SetO2iCondition(O2iConditionValue o2iCondition);

    /**
     * Get the O2iLowHighConditionValue containing the information about the O2I
     * building penetration losses (low or high)
     *
     * \return the O2iLowHighConditionValue
     */
    O2iLowHighConditionValue GetO2iLowHighCondition() const;

    /**
     * Set the O2iLowHighConditionValue containing the information about the O2I
     * building penetration losses (low or high)
     *
     * \param o2iLowHighCondition the O2iLowHighConditionValue
     */
    void SetO2iLowHighCondition(O2iLowHighConditionValue o2iLowHighCondition);

    /**
     * Return true if the channel condition is LOS
     *
     * \return true if the channel condition is LOS
     */
    bool IsLos() const;

    /**
     * Return true if the channel condition is NLOS
     *
     * It does not consider the case in which the LOS path is obstructed by a
     * vehicle. This case is represented as a separate channel condition (NLOSv),
     * use the method IsNlosv instead.
     *
     * \return true if the channel condition is NLOS
     */
    bool IsNlos() const;

    /**
     * Return true if the channel condition is NLOSv
     *
     * \return true if the channel condition is NLOSv
     */
    bool IsNlosv() const;

    /**
     * Return true if the channel is outdoor-to-indoor
     *
     * \return true if the channel is outdoor-to-indoor
     */
    bool IsO2i() const;

    /**
     * Return true if the channel is outdoor-to-outdoor
     *
     * \return true if the channel is outdoor-to-outdoor
     */
    bool IsO2o() const;

    /**
     * Return true if the channel is indoor-to-indoor
     *
     * \return true if the channel is indoor-to-indoor
     */
    bool IsI2i() const;

    /**
     * Return true if this instance is equivalent to the one passed as argument
     *
     * \param losCondition the LOS condition of the other channel condition instance
     * \param o2iCondition the 02I condition of the other channel condition instance
     * \return true if the channel LOS and O2i conditions of the instance are equivalent to those
     * passed as arguments
     */
    bool IsEqual(LosConditionValue losCondition, O2iConditionValue o2iCondition) const;

  private:
    LosConditionValue
        m_losCondition; //!< contains the information about the LOS state of the channel
    O2iConditionValue
        m_o2iCondition; //!< contains the information about the O2I state of the channel
    O2iLowHighConditionValue m_o2iLowHighCondition; //!< contains the information about the O2I
                                                    //!< low-high building penetration losses

    /**
     * Prints a LosConditionValue to output
     * \param os the output stream
     * \param cond the LosConditionValue
     *
     * \return a reference to the output stream
     */
    friend std::ostream& operator<<(std::ostream& os, LosConditionValue cond);
};

/**
 * \ingroup propagation
 *
 * \brief Models the channel condition
 *
 * Computes the condition of the channel between the transmitter and the
 * receiver
 */
class ChannelConditionModel : public Object
{
  public:
    /**
     * Get the type ID.
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor for the ChannelConditionModel class
     */
    ChannelConditionModel();

    /**
     * Destructor for the ChannelConditionModel class
     */
    ~ChannelConditionModel() override;

    // Delete copy constructor and assignment operator to avoid misuse
    ChannelConditionModel(const ChannelConditionModel&) = delete;
    ChannelConditionModel& operator=(const ChannelConditionModel&) = delete;

    /**
     * Computes the condition of the channel between a and b
     *
     * \param a mobility model
     * \param b mobility model
     * \return the condition of the channel between a and b
     */
    virtual Ptr<ChannelCondition> GetChannelCondition(Ptr<const MobilityModel> a,
                                                      Ptr<const MobilityModel> b) const = 0;

    /**
     * If this  model uses objects of type RandomVariableStream,
     * set the stream numbers to the integers starting with the offset
     * 'stream'. Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * \param stream the offset used to set the stream numbers
     * \return the number of stream indices assigned by this model
     */
    virtual int64_t AssignStreams(int64_t stream) = 0;
};

/**
 * \ingroup propagation
 *
 * \brief Models an always in-LoS condition model
 */
class AlwaysLosChannelConditionModel : public ChannelConditionModel
{
  public:
    /**
     * Get the type ID.
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    AlwaysLosChannelConditionModel();

    /**
     * Destructor
     */
    ~AlwaysLosChannelConditionModel() override;

    // Delete copy constructor and assignment operator to avoid misuse
    AlwaysLosChannelConditionModel(const AlwaysLosChannelConditionModel&) = delete;
    AlwaysLosChannelConditionModel& operator=(const AlwaysLosChannelConditionModel&) = delete;

    /**
     * Computes the condition of the channel between a and b, that will be always LoS
     *
     * \param a mobility model
     * \param b mobility model
     * \return the condition of the channel between a and b, that will be always LoS
     */
    Ptr<ChannelCondition> GetChannelCondition(Ptr<const MobilityModel> a,
                                              Ptr<const MobilityModel> b) const override;

    /**
     * If this  model uses objects of type RandomVariableStream,
     * set the stream numbers to the integers starting with the offset
     * 'stream'. Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * \param stream the offset used to set the stream numbers
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream) override;
};

/**
 * \ingroup propagation
 *
 * \brief Models a never in-LoS condition model
 */
class NeverLosChannelConditionModel : public ChannelConditionModel
{
  public:
    /**
     * Get the type ID.
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    NeverLosChannelConditionModel();

    /**
     * Destructor
     */
    ~NeverLosChannelConditionModel() override;

    // Delete copy constructor and assignment operator to avoid misuse
    NeverLosChannelConditionModel(const NeverLosChannelConditionModel&) = delete;
    NeverLosChannelConditionModel& operator=(const NeverLosChannelConditionModel&) = delete;

    /**
     * Computes the condition of the channel between a and b, that will be always non-LoS
     *
     * \param a mobility model
     * \param b mobility model
     * \return the condition of the channel between a and b, that will be always non-LoS
     */
    Ptr<ChannelCondition> GetChannelCondition(Ptr<const MobilityModel> a,
                                              Ptr<const MobilityModel> b) const override;

    /**
     * If this  model uses objects of type RandomVariableStream,
     * set the stream numbers to the integers starting with the offset
     * 'stream'. Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * \param stream the offset used to set the stream numbers
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream) override;
};

/**
 * \ingroup propagation
 *
 * \brief Models a never in-LoS condition model caused by a blocking vehicle
 */
class NeverLosVehicleChannelConditionModel : public ChannelConditionModel
{
  public:
    /**
     * Get the type ID.
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    NeverLosVehicleChannelConditionModel();

    /**
     * Destructor
     */
    ~NeverLosVehicleChannelConditionModel() override;

    // Delete copy constructor and assignment operator to avoid misuse
    NeverLosVehicleChannelConditionModel(const NeverLosVehicleChannelConditionModel&) = delete;
    NeverLosVehicleChannelConditionModel& operator=(const NeverLosVehicleChannelConditionModel&) =
        delete;

    /**
     * Computes the condition of the channel between a and b, that will be always NLOSv
     *
     * \param a mobility model
     * \param b mobility model
     * \return the condition of the channel between a and b, that will be always NLOSv
     */
    Ptr<ChannelCondition> GetChannelCondition(Ptr<const MobilityModel> a,
                                              Ptr<const MobilityModel> b) const override;

    /**
     * If this  model uses objects of type RandomVariableStream,
     * set the stream numbers to the integers starting with the offset
     * 'stream'. Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * \param stream the offset used to set the stream numbers
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream) override;
};

/**
 * \ingroup propagation
 *
 * \brief Base class for the 3GPP channel condition models
 *
 */
class ThreeGppChannelConditionModel : public ChannelConditionModel
{
  public:
    /**
     * Get the type ID.
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor for the ThreeGppRmaChannelConditionModel class
     */
    ThreeGppChannelConditionModel();

    /**
     * Destructor for the ThreeGppRmaChannelConditionModel class
     */
    ~ThreeGppChannelConditionModel() override;

    /**
     * \brief Retrieve the condition of the channel between a and b.
     *
     * If the channel condition does not exists, the method computes it by calling
     * ComputeChannelCondition and stores it in a local cache, that will be updated
     * following the "UpdatePeriod" parameter.
     *
     * \param a mobility model
     * \param b mobility model
     * \return the condition of the channel between a and b
     */
    Ptr<ChannelCondition> GetChannelCondition(Ptr<const MobilityModel> a,
                                              Ptr<const MobilityModel> b) const override;

    /**
     * If this  model uses objects of type RandomVariableStream,
     * set the stream numbers to the integers starting with the offset
     * 'stream'. Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * \param stream the offset used to set the stream numbers
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream) override;

  protected:
    void DoDispose() override;

    /**
     * Determine the density of vehicles in a V2V scenario.
     */
    enum VehicleDensity
    {
        LOW,
        MEDIUM,
        HIGH,
        INVALID
    };

    /**
     * \brief Computes the 2D distance between two 3D vectors
     * \param a the first 3D vector
     * \param b the second 3D vector
     * \return the 2D distance between a and b
     */
    static double Calculate2dDistance(const Vector& a, const Vector& b);

    Ptr<UniformRandomVariable> m_uniformVar; //!< uniform random variable

  private:
    /**
     * This method computes the channel condition based on a probabilistic model
     * that is specific for the scenario of interest
     *
     * \param a tx mobility model
     * \param b rx mobility model
     * \return the channel condition
     */
    Ptr<ChannelCondition> ComputeChannelCondition(Ptr<const MobilityModel> a,
                                                  Ptr<const MobilityModel> b) const;

    /**
     * Compute the LOS probability.
     *
     * \param a tx mobility model
     * \param b rx mobility model
     * \return the LOS probability
     */
    virtual double ComputePlos(Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const = 0;

    /**
     * Determines whether the channel condition is O2I or O2O
     *
     * \param a tx mobility model
     * \param b rx mobility model
     * \return the O2I channelcondition
     */
    virtual ChannelCondition::O2iConditionValue ComputeO2i(Ptr<const MobilityModel> a,
                                                           Ptr<const MobilityModel> b) const;

    /**
     * Compute the NLOS probability. By default returns 1 - PLOS
     *
     * \param a tx mobility model
     * \param b rx mobility model
     * \return the LOS probability
     */
    virtual double ComputePnlos(Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const;

    /**
     * \brief Returns a unique and reciprocal key for the channel between a and b.
     * \param a tx mobility model
     * \param b rx mobility model
     * \return channel key
     */
    static uint32_t GetKey(Ptr<const MobilityModel> a, Ptr<const MobilityModel> b);

    /**
     * Struct to store the channel condition in the m_channelConditionMap
     */
    struct Item
    {
        Ptr<ChannelCondition> m_condition; //!< the channel condition
        Time m_generatedTime;              //!< the time when the condition was generated
    };

    std::unordered_map<uint32_t, Item>
        m_channelConditionMap; //!< map to store the channel conditions
    Time m_updatePeriod;       //!< the update period for the channel condition

    double m_o2iThreshold{
        0}; //!< the threshold for determining what is the ratio of channels with O2I
    double m_o2iLowLossThreshold{0}; //!< the threshold for determining what is the ratio of low -
                                     //!< high O2I building penetration losses
    double m_linkO2iConditionToAntennaHeight{
        false}; //!< the indicator that determines whether the O2I/O2O condition is determined based
                //!< on the UE height
    Ptr<UniformRandomVariable> m_uniformVarO2i; //!< uniform random variable that is used for the
                                                //!< generation of the O2i conditions
    Ptr<UniformRandomVariable>
        m_uniformO2iLowHighLossVar; //!< a uniform random variable for the calculation of the
                                    //!< low/high losses, see TR38.901 Table 7.4.3-2
};

/**
 * \ingroup propagation
 *
 * \brief Computes the channel condition for the RMa scenario
 *
 * Computes the channel condition following the specifications for the RMa
 * scenario reported in Table 7.4.2-1 of 3GPP TR 38.901
 */
class ThreeGppRmaChannelConditionModel : public ThreeGppChannelConditionModel
{
  public:
    /**
     * Get the type ID.
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor for the ThreeGppRmaChannelConditionModel class
     */
    ThreeGppRmaChannelConditionModel();

    /**
     * Destructor for the ThreeGppRmaChannelConditionModel class
     */
    ~ThreeGppRmaChannelConditionModel() override;

  private:
    /**
     * Compute the LOS probability as specified in Table 7.4.2-1 of 3GPP TR 38.901
     * for the RMa scenario.
     *
     * \param a tx mobility model
     * \param b rx mobility model
     * \return the LOS probability
     */
    double ComputePlos(Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;
};

/**
 * \ingroup propagation
 *
 * \brief Computes the channel condition for the UMa scenario
 *
 * Computes the channel condition following the specifications for the UMa
 * scenario reported in Table 7.4.2-1 of 3GPP TR 38.901
 */
class ThreeGppUmaChannelConditionModel : public ThreeGppChannelConditionModel
{
  public:
    /**
     * Get the type ID.
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor for the ThreeGppUmaChannelConditionModel class
     */
    ThreeGppUmaChannelConditionModel();

    /**
     * Destructor for the ThreeGppUmaChannelConditionModel class
     */
    ~ThreeGppUmaChannelConditionModel() override;

  private:
    /**
     * Compute the LOS probability as specified in Table 7.4.2-1 of 3GPP TR 38.901
     * for the UMa scenario.
     *
     * \param a tx mobility model
     * \param b rx mobility model
     * \return the LOS probability
     */
    double ComputePlos(Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;
};

/**
 * \ingroup propagation
 *
 * \brief Computes the channel condition for the UMi-Street canyon scenario
 *
 * Computes the channel condition following the specifications for the
 * UMi-Street canyon scenario reported in Table 7.4.2-1 of 3GPP TR 38.901
 */
class ThreeGppUmiStreetCanyonChannelConditionModel : public ThreeGppChannelConditionModel
{
  public:
    /**
     * Get the type ID.
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor for the ThreeGppUmiStreetCanyonChannelConditionModel class
     */
    ThreeGppUmiStreetCanyonChannelConditionModel();

    /**
     * Destructor for the ThreeGppUmiStreetCanyonChannelConditionModel class
     */
    ~ThreeGppUmiStreetCanyonChannelConditionModel() override;

  private:
    /**
     * Compute the LOS probability as specified in Table 7.4.2-1 of 3GPP TR 38.901
     * for the UMi-Street Canyon scenario.
     *
     * \param a tx mobility model
     * \param b rx mobility model
     * \return the LOS probability
     */
    double ComputePlos(Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;
};

/**
 * \ingroup propagation
 *
 * \brief Computes the channel condition for the Indoor Mixed Office scenario
 *
 * Computes the channel condition following the specifications for the
 * Indoor Mixed Office scenario reported in Table 7.4.2-1 of 3GPP TR 38.901
 */
class ThreeGppIndoorMixedOfficeChannelConditionModel : public ThreeGppChannelConditionModel
{
  public:
    /**
     * Get the type ID.
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor for the ThreeGppIndoorMixedOfficeChannelConditionModel class
     */
    ThreeGppIndoorMixedOfficeChannelConditionModel();

    /**
     * Destructor for the ThreeGppIndoorMixedOfficeChannelConditionModel class
     */
    ~ThreeGppIndoorMixedOfficeChannelConditionModel() override;

  private:
    /**
     * Compute the LOS probability as specified in Table 7.4.2-1 of 3GPP TR 38.901
     * for the Indoor Mixed Office scenario.
     *
     * \param a tx mobility model
     * \param b rx mobility model
     * \return the LOS probability
     */
    double ComputePlos(Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;
};

/**
 * \ingroup propagation
 *
 * \brief Computes the channel condition for the Indoor Open Office scenario
 *
 * Computes the channel condition following the specifications for the
 * Indoor Open Office scenario reported in Table 7.4.2-1 of 3GPP TR 38.901
 */
class ThreeGppIndoorOpenOfficeChannelConditionModel : public ThreeGppChannelConditionModel
{
  public:
    /**
     * Get the type ID.
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor for the ThreeGppIndoorOpenOfficeChannelConditionModel class
     */
    ThreeGppIndoorOpenOfficeChannelConditionModel();

    /**
     * Destructor for the ThreeGppIndoorOpenOfficeChannelConditionModel class
     */
    ~ThreeGppIndoorOpenOfficeChannelConditionModel() override;

  private:
    /**
     * Compute the LOS probability as specified in Table 7.4.2-1 of 3GPP TR 38.901
     * for the Indoor Open Office scenario.
     *
     * \param a tx mobility model
     * \param b rx mobility model
     * \return the LOS probability
     */
    double ComputePlos(Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;
};

} // namespace ns3

#endif /* CHANNEL_CONDITION_MODEL_H */
