/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

#include "ns3/object.h"
#include "ns3/random-variable-stream.h"
#include "ns3/vector.h"
#include "ns3/nstime.h"
#include <unordered_map>

namespace ns3 {

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
    LOS, //!< Line of Sight
    NLOS //!< Non Line of Sight
  };

  /**
   * Get the type ID.
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Constructor for the ChannelCondition class
   */
  ChannelCondition ();

  /**
   * Destructor for the ChannelCondition class
   */
  virtual ~ChannelCondition ();

  /**
   * Get the LosConditionValue contaning the information about the LOS/NLOS
   * state of the channel
   *
   * \return the LosConditionValue
   */
  LosConditionValue GetLosCondition () const;

  /**
   * Set the LosConditionValue with the information about the LOS/NLOS
   * state of the channel
   *
   * \param the LosConditionValue
   */
  void SetLosCondition (LosConditionValue losCondition);

private:
  LosConditionValue m_losCondition; //!< contains the information about the LOS/NLOS state of the channel

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
  static TypeId GetTypeId (void);

  /**
   * Constructor for the ChannelConditionModel class
   */
  ChannelConditionModel ();

  /**
   * Destructor for the ChannelConditionModel class
   */
  virtual ~ChannelConditionModel ();

  /**
   * Computes the condition of the channel between a and b

   *
   * \param a mobility model
   * \param b mobility model
   * \return the condition of the channel between a and b
   */
  virtual Ptr<ChannelCondition> GetChannelCondition (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const = 0;

  /**
   * If this  model uses objects of type RandomVariableStream,
   * set the stream numbers to the integers starting with the offset
   * 'stream'. Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream
   * \return the number of stream indices assigned by this model
   */
  virtual int64_t AssignStreams (int64_t stream) = 0;

  /**
  * \brief Copy constructor
  *
  * Defined and unimplemented to avoid misuse
  */
  ChannelConditionModel (const ChannelConditionModel&) = delete;

  /**
  * \brief Copy constructor
  *
  * Defined and unimplemented to avoid misuse
  * \returns the ChannelConditionModel instance
  */
  ChannelConditionModel &operator = (const ChannelConditionModel &) = delete;
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
  static TypeId GetTypeId (void);

  /**
   * Constructor for the ChannelConditionModel class
   */
  AlwaysLosChannelConditionModel ();

  /**
   * Destructor for the ChannelConditionModel class
   */
  virtual ~AlwaysLosChannelConditionModel ();

  /**
   * Computes the condition of the channel between a and b, that will be always LoS
   *
   * \param a mobility model
   * \param b mobility model
   * \return the condition of the channel between a and b, that will be always LoS
   */
  virtual Ptr<ChannelCondition> GetChannelCondition (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;

  /**
  * \brief Copy constructor
  *
  * Defined and unimplemented to avoid misuse
  */
  AlwaysLosChannelConditionModel (const AlwaysLosChannelConditionModel&) = delete;

  /**
  * \brief Copy constructor
  *
  * Defined and unimplemented to avoid misuse
  * \returns
  */
  AlwaysLosChannelConditionModel &operator = (const AlwaysLosChannelConditionModel &) = delete;

  /**
   * If this  model uses objects of type RandomVariableStream,
   * set the stream numbers to the integers starting with the offset
   * 'stream'. Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream
   * \return the number of stream indices assigned by this model
   */
  virtual int64_t AssignStreams (int64_t stream) override;
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
  static TypeId GetTypeId (void);

  /**
   * Constructor for the ChannelConditionModel class
   */
  NeverLosChannelConditionModel ();

  /**
   * Destructor for the ChannelConditionModel class
   */
  virtual ~NeverLosChannelConditionModel ();

  /**
   * Computes the condition of the channel between a and b, that will be always non-LoS
   *
   * \param a mobility model
   * \param b mobility model
   * \return the condition of the channel between a and b, that will be always non-LoS
   */
  virtual Ptr<ChannelCondition> GetChannelCondition (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;

  /**
  * \brief Copy constructor
  *
  * Defined and unimplemented to avoid misuse
  */
  NeverLosChannelConditionModel (const NeverLosChannelConditionModel&) = delete;

  /**
  * \brief Copy constructor
  *
  * Defined and unimplemented to avoid misuse
  * \returns
  */
  NeverLosChannelConditionModel &operator = (const NeverLosChannelConditionModel &) = delete;

  /**
   * If this  model uses objects of type RandomVariableStream,
   * set the stream numbers to the integers starting with the offset
   * 'stream'. Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream
   * \return the number of stream indices assigned by this model
   */
  virtual int64_t AssignStreams (int64_t stream) override;
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
  static TypeId GetTypeId (void);

  /**
   * Constructor for the ThreeGppRmaChannelConditionModel class
   */
  ThreeGppChannelConditionModel ();

  /**
   * Destructor for the ThreeGppRmaChannelConditionModel class
   */
  virtual ~ThreeGppChannelConditionModel () override;

  /**
   * \brief Retrieve the condition of the channel between a and b.
   *
   * If the channel condition does not exists, the method creates it and
   * store it in a local cache, that will be updated following the "UpdatePeriod"
   * parameter.
   *
   * \param a mobility model
   * \param b mobility model
   * \return the condition of the channel between a and b
   */
  virtual Ptr<ChannelCondition> GetChannelCondition (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;

  /**
   * If this  model uses objects of type RandomVariableStream,
   * set the stream numbers to the integers starting with the offset
   * 'stream'. Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream
   * \return the number of stream indices assigned by this model
   */
  virtual int64_t AssignStreams (int64_t stream) override;

protected:
  virtual void DoDispose () override;
  
  /**
  * \brief Computes the 2D distance between two 3D vectors
  * \param a the first 3D vector
  * \param b the second 3D vector
  * \return the 2D distance between a and b
  */
  static double Calculate2dDistance (const Vector &a, const Vector &b);

private:
  /**
   * Compute the LOS probability as specified in Table 7.4.2-1 of 3GPP TR 38.901.
   *
   * \param a tx mobility model
   * \param b rx mobility model
   * \return the LOS probability
   */
  virtual double ComputePlos (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const = 0;

  /**
   * \brief Returns a unique and reciprocal key for the channel between a and b.
   * \param a tx mobility model
   * \param b rx mobility model
   * \return channel key
   */
  static uint32_t GetKey (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b);

  /**
   * Struct to store the channel condition in the m_channelConditionMap
   */
  struct Item
  {
    Ptr<ChannelCondition> m_condition; //!< the channel condition
    Time m_generatedTime; //!< the time when the condition was generated
  };

  std::unordered_map<uint32_t, Item> m_channelConditionMap; //!< map to store the channel conditions
  Time m_updatePeriod; //!< the update period for the channel condition
  Ptr<UniformRandomVariable> m_uniformVar; //!< uniform random variable
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
  static TypeId GetTypeId (void);

  /**
   * Constructor for the ThreeGppRmaChannelConditionModel class
   */
  ThreeGppRmaChannelConditionModel ();

  /**
   * Destructor for the ThreeGppRmaChannelConditionModel class
   */
  virtual ~ThreeGppRmaChannelConditionModel () override;

private:
  /**
   * Compute the LOS probability as specified in Table 7.4.2-1 of 3GPP TR 38.901
   * for the RMa scenario.
   *
   * \param a tx mobility model
   * \param b rx mobility model
   * \return the LOS probability
   */
  virtual double ComputePlos (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;
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
  static TypeId GetTypeId (void);

  /**
   * Constructor for the ThreeGppUmaChannelConditionModel class
   */
  ThreeGppUmaChannelConditionModel ();

  /**
   * Destructor for the ThreeGppUmaChannelConditionModel class
   */
  virtual ~ThreeGppUmaChannelConditionModel () override;

private:
  /**
   * Compute the LOS probability as specified in Table 7.4.2-1 of 3GPP TR 38.901
   * for the UMa scenario.
   *
   * \param a tx mobility model
   * \param b rx mobility model
   * \return the LOS probability
   */
  virtual double ComputePlos (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;
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
  static TypeId GetTypeId (void);

  /**
   * Constructor for the ThreeGppUmiStreetCanyonChannelConditionModel class
   */
  ThreeGppUmiStreetCanyonChannelConditionModel ();

  /**
   * Destructor for the ThreeGppUmiStreetCanyonChannelConditionModel class
   */
  virtual ~ThreeGppUmiStreetCanyonChannelConditionModel () override;

private:
  /**
   * Compute the LOS probability as specified in Table 7.4.2-1 of 3GPP TR 38.901
   * for the UMi-Street Canyon scenario.
   *
   * \param a tx mobility model
   * \param b rx mobility model
   * \return the LOS probability
   */
  virtual double ComputePlos (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;
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
  static TypeId GetTypeId (void);

  /**
   * Constructor for the ThreeGppIndoorMixedOfficeChannelConditionModel class
   */
  ThreeGppIndoorMixedOfficeChannelConditionModel ();

  /**
   * Destructor for the ThreeGppIndoorMixedOfficeChannelConditionModel class
   */
  virtual ~ThreeGppIndoorMixedOfficeChannelConditionModel () override;

private:
  /**
   * Compute the LOS probability as specified in Table 7.4.2-1 of 3GPP TR 38.901
   * for the Indoor Mixed Office scenario.
   *
   * \param a tx mobility model
   * \param b rx mobility model
   * \return the LOS probability
   */
  virtual double ComputePlos (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;
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
  static TypeId GetTypeId (void);

  /**
   * Constructor for the ThreeGppIndoorOpenOfficeChannelConditionModel class
   */
  ThreeGppIndoorOpenOfficeChannelConditionModel ();

  /**
   * Destructor for the ThreeGppIndoorOpenOfficeChannelConditionModel class
   */
  virtual ~ThreeGppIndoorOpenOfficeChannelConditionModel () override;

private:
  /**
   * Compute the LOS probability as specified in Table 7.4.2-1 of 3GPP TR 38.901
   * for the Indoor Open Office scenario.
   *
   * \param a tx mobility model
   * \param b rx mobility model
   * \return the LOS probability
   */
  virtual double ComputePlos (Ptr<const MobilityModel> a, Ptr<const MobilityModel> b) const override;
};

} // end ns3 namespace

#endif /* CHANNEL_CONDITION_MODEL_H */
