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

#ifndef UNIFORM_PLANAR_ARRAY_H
#define UNIFORM_PLANAR_ARRAY_H

#include "phased-array-model.h"

#include <ns3/object.h>

namespace ns3
{

/**
 * \ingroup antenna
 *
 * \brief Class implementing Uniform Planar Array (UPA) model.
 *
 * \note the current implementation supports the modeling of antenna arrays
 * composed of a single panel and with single or dual polarization.
 */
class UniformPlanarArray : public PhasedArrayModel
{
  public:
    /**
     * Constructor
     */
    UniformPlanarArray();

    /**
     * Destructor
     */
    ~UniformPlanarArray() override;

    /**
     * \brief Get the type ID.
     * \return The object TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Returns the horizontal and vertical components of the antenna element field
     * pattern at the specified direction and for the specified polarization.
     * \param a the angle indicating the interested direction
     * \param polIndex the index of the polarization for which will be retrieved the field
     * pattern
     * \return a pair in which the first element is the horizontal component
     *         of the field pattern and the second element is the vertical
     *         component of the field pattern
     */
    std::pair<double, double> GetElementFieldPattern(Angles a, uint8_t polIndex = 0) const override;

    /**
     * Returns the location of the antenna element with the specified
     * index assuming the left bottom corner is (0,0,0), normalized
     * with respect to the wavelength.
     * Antenna elements are scanned row by row, left to right and bottom to top.
     * For example, an antenna with 2 rows and 3 columns will be ordered as follows:
     * ^ z
     * |  3 4 5
     * |  0 1 2
     * ----------> y
     * In case of dual-polarized antennas, the antennas of the first and the second polarization
     * are overlapped in space.
     *
     * \param index index of the antenna element
     * \return the 3D vector that
     * represents the position of the element
     */
    Vector GetElementLocation(uint64_t index) const override;

    /**
     *  Check if an antenna array contains dual-polarized elements
     *
     * \return true if antenna has two polarization otherwise false
     */
    bool IsDualPol() const override;

    /**
     *  Returns polarization angle of first polarization
     *  \return polarization angle in radians
     */
    double GetPolSlant() const override;

    /**
     * Returns the number of polarizations, 2 in the case that
     * the antenna is dual-polarized, otherwise 1
     * \return
     */
    uint8_t GetNumPols() const override;

    /**
     * Returns the number of total antenna elements. Note that if the antenna
     * is dual-polarized the number of total antenna elements is doubled.
     * \return the number of antenna elements
     */
    size_t GetNumElems() const override;

    /**
     * Set the number of columns of the phased array
     * This method resets the stored beamforming vector to a ComplexVector
     * of the correct size, but zero-filled
     * \param n the number of columns
     */
    void SetNumColumns(uint32_t n) override;

    /**
     * Get the number of columns of the phased array
     * \return the number of columns
     */
    uint32_t GetNumColumns() const override;

    /**
     * Set the number of rows of the phased array
     * This method resets the stored beamforming vector to a ComplexVector
     * of the correct size, but zero-filled
     * \param n the number of rows
     */
    void SetNumRows(uint32_t n) override;

    /**
     * Get the number of rows of the phased array
     * \return the number of rows
     */
    uint32_t GetNumRows() const override;

    /**
     * Set the number of vertical antenna ports
     * \param nPorts The number of vertical ports to be configured
     */
    void SetNumVerticalPorts(uint16_t nPorts) override;

    /**
     * Set the number of horizontal antenna ports
     * \param nPorts
     */
    void SetNumHorizontalPorts(uint16_t nPorts) override;

    /**
     * Get the number of vertical antenna ports
     * \return the number of vertical antenna ports
     */
    uint16_t GetNumVerticalPorts() const override;

    /**
     * Get the number of horizontal antenna ports
     * \return the number of horizontal antenna ports
     */
    uint16_t GetNumHorizontalPorts() const override;

    /**
     * Get the total number of antenna ports
     * \return the number of antenna ports
     */
    uint16_t GetNumPorts() const override;

    /**
     * Get the number of vertical elements belonging to each port
     * \return the number of vertical elements belonging of each port
     */
    size_t GetVElemsPerPort() const override;

    /**
     * Get the number of horizontal elements belonging to each port
     * \return the number of horizontal elements belonging to each port
     */
    size_t GetHElemsPerPort() const override;

    /**
     * Get the total number of elements belonging to each port
     * \return the number of elements per port
     */
    size_t GetNumElemsPerPort() const override;

    /**
     * Maps element within a port to an index of element within the antenna array
     * \param portIndex the port index
     * \param subElementIndex the element index within the port
     * \return the element index in the antenna array
     */
    uint16_t ArrayIndexFromPortIndex(uint16_t portIndex, uint16_t subElementIndex) const override;

    /**
     * \brief Set the bearing angle
     * This method sets the bearing angle and
     * computes its cosine and sine
     * \param alpha the bearing angle in radians
     */
    void SetAlpha(double alpha);

    /**
     * \brief Set the downtilt angle
     * This method sets the downtilt angle and
     * computes its cosine and sine
     * \param beta the downtilt angle in radians
     */
    void SetBeta(double beta);

    /**
     * \brief Set the polarization slant angle
     * This method sets the polarization slant angle and
     * computes its cosine and sine
     * \param polSlant the polarization slant angle in radians
     */
    void SetPolSlant(double polSlant);

    /**
     * Set the horizontal spacing for the antenna elements of the phased array
     * This method resets the stored beamforming vector to a ComplexVector
     * of the correct size, but zero-filled
     * \param s the horizontal spacing in multiples of wavelength
     */
    void SetAntennaHorizontalSpacing(double s);

    /**
     * Get the horizontal spacing for the antenna elements of the phased array
     * \return the horizontal spacing in multiples of wavelength
     */
    double GetAntennaHorizontalSpacing() const;

    /**
     * Set the vertical spacing for the antenna elements of the phased array
     * This method resets the stored beamforming vector to a ComplexVector
     * of the correct size, but zero-filled
     * \param s the vertical spacing in multiples of wavelength
     */
    void SetAntennaVerticalSpacing(double s);

    /**
     * Get the vertical spacing for the antenna elements of the phased array
     * \return the vertical spacing in multiples of wavelength
     */
    double GetAntennaVerticalSpacing() const;

    /**
     * Set the polarization
     * \param isDualPol whether to set the antenna array to be dual-polarized,
     * if true, antenna will be dual-polarized
     */
    void SetDualPol(bool isDualPol);

    /**
     * Returns the index of polarization to which belongs the antenna element with a specific index
     * \param elemIndex the antenna element index
     * \return the polarization index
     */
    uint8_t GetElemPol(size_t elemIndex) const override;

  private:
    uint32_t m_numColumns{1}; //!< number of columns
    uint32_t m_numRows{1};    //!< number of rows
    double m_disV{0.5}; //!< antenna spacing in the vertical direction in multiples of wave length
    double m_disH{0.5}; //!< antenna spacing in the horizontal direction in multiples of wave length
    double m_alpha{0.0};                          //!< the bearing angle in radians
    double m_cosAlpha{1.0};                       //!< the cosine of alpha
    double m_sinAlpha{0.0};                       //!< the sine of alpha
    double m_beta{0.0};                           //!< the downtilt angle in radians
    double m_cosBeta{1.0};                        //!< the cosine of Beta
    double m_sinBeta{0.0};                        //!< the sine of Beta
    double m_polSlant{0.0};                       //!< the polarization slant angle in radians
    bool m_isDualPolarized{false};                //!< if true antenna elements are dual-polarized
    uint16_t m_numVPorts{1};                      //!< Number of vertical ports
    uint16_t m_numHPorts{1};                      //!< Number of horizontal ports
    std::vector<double> m_cosPolSlant{1.0, 0.0};  //!< the cosine of polarization slant angle
    std::vector<double> m_sinPolSlant{0.0, -1.0}; //!< the sine polarization slant angle
};

} /* namespace ns3 */

#endif /* UNIFORM_PLANAR_ARRAY_H */
