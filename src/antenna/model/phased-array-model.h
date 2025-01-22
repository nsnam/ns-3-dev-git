/*
 *   Copyright (c) 2020 University of Padova, Dep. of Information Engineering, SIGNET lab.
 *
 *   SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef PHASED_ARRAY_MODEL_H
#define PHASED_ARRAY_MODEL_H

#include "angles.h"
#include "antenna-model.h"

#include "ns3/matrix-array.h"
#include "ns3/object.h"
#include "ns3/symmetric-adjacency-matrix.h"

#include <complex>

namespace ns3
{

/**
 * @ingroup antenna
 *
 * @brief Class implementing the phased array model virtual base class.
 */
class PhasedArrayModel : public Object
{
  public:
    /**
     * Constructor
     */
    PhasedArrayModel();

    /**
     * Destructor
     */
    ~PhasedArrayModel() override;

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    //!< type definition for complex vectors
    using ComplexVector = ComplexMatrixArray; //!< the underlying Valarray

    /**
     * @brief Computes the Frobenius norm of the complex vector
     *
     * @param complexVector on which to calculate Frobenius norm
     * @return the Frobenius norm of the complex vector
     */
    double norm(const ComplexVector& complexVector) const
    {
        double norm = 0;
        for (size_t i = 0; i < complexVector.GetSize(); i++)
        {
            norm += std::norm(complexVector[i]);
        }
        return std::sqrt(norm);
    }

    /**
     * @brief Returns the location of the antenna element with the specified
     * index, normalized with respect to the wavelength.
     * @param index the index of the antenna element
     * @return the 3D vector that represents the position of the element
     */
    virtual Vector GetElementLocation(uint64_t index) const = 0;

    /**
     * @brief Returns the number of antenna elements
     * @return the number of antenna elements
     */
    virtual size_t GetNumElems() const = 0;

    /**
     * @brief Returns the horizontal and vertical components of the antenna element field
     * pattern at the specified direction. Single polarization is considered.
     * @param a the angle indicating the interested direction
     * @param polIndex the index of the polarization for which will be retrieved the field
     * pattern
     * @return a pair in which the first element is the horizontal component of the field
     * pattern and the second element is the vertical component of the field pattern
     */
    virtual std::pair<double, double> GetElementFieldPattern(Angles a,
                                                             uint8_t polIndex = 0) const = 0;

    /**
     * @brief Set the vertical number of ports
     * @param nPorts the vertical number of ports
     */
    virtual void SetNumVerticalPorts(uint16_t nPorts) = 0;

    /**
     * @brief Set the horizontal number of ports
     * @param nPorts the horizontal number of ports
     */
    virtual void SetNumHorizontalPorts(uint16_t nPorts) = 0;

    /**
     * @brief Get the vertical number of ports
     * @return the vertical number of ports
     */
    virtual uint16_t GetNumVerticalPorts() const = 0;

    /**
     * @brief Get the horizontal number of ports
     * @return the horizontal number of ports
     */
    virtual uint16_t GetNumHorizontalPorts() const = 0;

    /**
     * @brief Get the number of ports
     * @return the number of ports
     */
    virtual uint16_t GetNumPorts() const = 0;

    /**
     * @brief Get the number of polarizations
     * @return the number of polarizations
     */
    virtual uint8_t GetNumPols() const = 0;

    /**
     * @brief Get the vertical number of antenna elements per port
     * @return the vertical number of antenna elements per port
     */
    virtual size_t GetVElemsPerPort() const = 0;
    /**
     * @brief Get the horizontal number of antenna elements per port
     * @return the horizontal number of antenna elements per port
     */
    virtual size_t GetHElemsPerPort() const = 0;

    /**
     * @brief Get the number of elements per port
     * @return the number of elements per port
     */
    virtual size_t GetNumElemsPerPort() const = 0;

    /**
     * @brief Set the number of columns
     * @param nColumns the number of columns to be set
     */
    virtual void SetNumColumns(uint32_t nColumns) = 0;

    /**
     * @brief Set the number of rows
     * @param nRows the number of rows to be set
     */
    virtual void SetNumRows(uint32_t nRows) = 0;

    /**
     * @brief Get the number of columns
     * @return the number of columns in the antenna array
     */
    virtual uint32_t GetNumColumns() const = 0;

    /**
     * @brief Get the number of rows
     * @return the number of rows in the antenna array
     */
    virtual uint32_t GetNumRows() const = 0;

    /**
     * @brief Get the polarization slant angle
     * @return the polarization slant angle
     */
    virtual double GetPolSlant() const = 0;

    /**
     * @brief Get the indication whether the antenna array is dual polarized
     * @return Returns true if the antenna array is dual polarized
     */
    virtual bool IsDualPol() const = 0;

    /**
     * @brief Calculate the index in the antenna array from the port index and the element in the
     * port
     * @param portIndex the port index
     * @param subElementIndex the element index in the port
     * @return the antenna element index in the antenna array
     */
    virtual uint16_t ArrayIndexFromPortIndex(uint16_t portIndex,
                                             uint16_t subElementIndex) const = 0;

    /**
     * Returns the index of the polarization to which belongs that antenna element
     * @param elementIndex the antenna element for which will be returned the polarization index
     * @return the polarization index
     */
    virtual uint8_t GetElemPol(size_t elementIndex) const = 0;

    /**
     * Sets the beamforming vector to be used
     * @param beamformingVector the beamforming vector
     */
    void SetBeamformingVector(const ComplexVector& beamformingVector);

    /**
     * Returns the beamforming vector that is currently being used
     * @return the current beamforming vector
     */
    ComplexVector GetBeamformingVector() const;

    /**
     * Returns the const reference of the beamforming vector that is currently being used
     * @return the const reference of the current beamforming vector
     */
    const PhasedArrayModel::ComplexVector& GetBeamformingVectorRef() const;

    /**
     * Returns the beamforming vector that points towards the specified position
     * @param a the beamforming angle
     * @return the beamforming vector
     */
    ComplexVector GetBeamformingVector(Angles a) const;

    /**
     * Returns the steering vector that points toward the specified position
     * @param a the steering angle
     * @return the steering vector
     */
    ComplexVector GetSteeringVector(Angles a) const;

    /**
     * Sets the antenna model to be used
     * @param antennaElement the antenna model
     */
    void SetAntennaElement(Ptr<AntennaModel> antennaElement);

    /**
     * Returns a pointer to the AntennaModel instance used to model the elements of the array
     * @return pointer to the AntennaModel instance
     */
    Ptr<const AntennaModel> GetAntennaElement() const;

    /**
     * Returns the ID of this antenna array instance
     * @return the ID value
     */
    uint32_t GetId() const;

    /**
     * Returns whether the channel needs to be updated due to antenna setting changes.
     * @param antennaB the second antenna of the pair for the channel we want to check
     * @return whether a channel update is needed due to antenna settings changes
     */
    bool IsChannelOutOfDate(Ptr<const PhasedArrayModel> antennaB) const;

  protected:
    /**
     * After changing the antenna settings, InvalidateChannels() should be called to mark
     * up-to-date channels as out-of-date
     */
    void InvalidateChannels() const;

    ComplexVector m_beamformingVector;  //!< the beamforming vector in use
    Ptr<AntennaModel> m_antennaElement; //!< the model of the antenna element in use
    bool m_isBfVectorValid;             //!< ensures the validity of the beamforming vector
    static uint32_t
        m_idCounter;  //!< the ID counter that is used to determine the unique antenna array ID
    uint32_t m_id{0}; //!< the ID of this antenna array instance
    static SymmetricAdjacencyMatrix<bool>
        m_outOfDateAntennaPairChannel; //!< matrix indicating whether a channel matrix between a
                                       //!< pair of antennas needs to be updated after a change in
                                       //!< one of the antennas configurations
};

} /* namespace ns3 */

#endif /* PHASED_ARRAY_MODEL_H */
