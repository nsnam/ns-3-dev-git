/*
 *   Copyright (c) 2020 University of Padova, Dep. of Information Engineering, SIGNET lab.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PHASED_ARRAY_MODEL_H
#define PHASED_ARRAY_MODEL_H

#include <ns3/angles.h>
#include <ns3/antenna-model.h>
#include <ns3/matrix-array.h>
#include <ns3/object.h>

#include <complex>

namespace ns3
{

/**
 * \ingroup antenna
 *
 * \brief Class implementing the phased array model virtual base class.
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
     * \brief Get the type ID.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();

    //!< type definition for complex vectors
    using ComplexVector = ComplexMatrixArray; //!< the underlying Valarray

    /**
     * \brief Computes the Frobenius norm of the complex vector
     *
     * \param complexVector on which to calculate Frobenius norm
     * \return the Frobenius norm of the complex vector
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
     * Returns the horizontal and vertical components of the antenna element field
     * pattern at the specified direction. Single polarization is considered.
     * \param a the angle indicating the interested direction
     * \return a pair in which the first element is the horizontal component
     *         of the field pattern and the second element is the vertical
     *         component of the field pattern
     */
    virtual std::pair<double, double> GetElementFieldPattern(Angles a) const = 0;

    /**
     * Returns the location of the antenna element with the specified
     * index, normalized with respect to the wavelength.
     * \param index the index of the antenna element
     * \return the 3D vector that represents the position of the element
     */
    virtual Vector GetElementLocation(uint64_t index) const = 0;

    /**
     * Returns the number of antenna elements
     * \return the number of antenna elements
     */
    virtual size_t GetNumberOfElements() const = 0;

    /**
     * Sets the beamforming vector to be used
     * \param beamformingVector the beamforming vector
     */
    void SetBeamformingVector(const ComplexVector& beamformingVector);

    /**
     * Returns the beamforming vector that is currently being used
     * \return the current beamforming vector
     */
    ComplexVector GetBeamformingVector() const;

    /**
     * Returns the beamforming vector that points towards the specified position
     * \param a the beamforming angle
     * \return the beamforming vector
     */
    ComplexVector GetBeamformingVector(Angles a) const;

    /**
     * Returns the steering vector that points toward the specified position
     * \param a the steering angle
     * \return the steering vector
     */
    ComplexVector GetSteeringVector(Angles a) const;

    /**
     * Sets the antenna model to be used
     * \param antennaElement the antenna model
     */
    void SetAntennaElement(Ptr<AntennaModel> antennaElement);

    /**
     * Returns a pointer to the AntennaModel instance used to model the elements of the array
     * \return pointer to the AntennaModel instance
     */
    Ptr<const AntennaModel> GetAntennaElement() const;

    /**
     * Returns the ID of this antenna array instance
     * \return the ID value
     */
    uint32_t GetId() const;

  protected:
    /**
     * Utility method to compute the euclidean norm of a ComplexVector
     * \param vector the ComplexVector
     * \return the euclidean norm
     */
    static double ComputeNorm(const ComplexVector& vector);

    ComplexVector m_beamformingVector;  //!< the beamforming vector in use
    Ptr<AntennaModel> m_antennaElement; //!< the model of the antenna element in use
    bool m_isBfVectorValid;             //!< ensures the validity of the beamforming vector
    static uint32_t
        m_idCounter;  //!< the ID counter that is used to determine the unique antenna array ID
    uint32_t m_id{0}; //!< the ID of this antenna array instance
};

/**
 * \brief Stream insertion operator.
 *
 * \param [in] os The reference to the output stream.
 * \param [in] cv A vector of complex values.
 * \returns The reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, const PhasedArrayModel::ComplexVector& cv);

} /* namespace ns3 */

#endif /* PHASED_ARRAY_MODEL_H */
