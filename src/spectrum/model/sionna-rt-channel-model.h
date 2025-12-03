/*
 * Copyright (c) 2025 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Author: Amir Ashtari <aashtari@cttc.es>
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef SIONNA_RT_CHANNEL_MODEL_H
#define SIONNA_RT_CHANNEL_MODEL_H

#include "matrix-based-channel-model.h"

#include "ns3/angles.h"
#include "ns3/antenna-model.h"
#include "ns3/boolean.h"
#include "ns3/vector.h"

#include <complex.h>
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <unordered_map>

static const std::set<std::string> builtInSceneSionna = {"box",
                                                         "box_one_screen",
                                                         "box_two_screens",
                                                         "double_reflector",
                                                         "triple_reflector",
                                                         "simple_wedge",
                                                         "etoile",
                                                         "floor_wall",
                                                         "florence",
                                                         "munich",
                                                         "san_francisco",
                                                         "simple_street_canyon",
                                                         "simple_street_canyon_with_cars",
                                                         "simple_reflector",
                                                         "street_4b"};

namespace py = pybind11;

namespace ns3
{
class MobilityModel;

/**
 * @ingroup spectrum
 * @brief High-level interface for generating MIMO channel matrices using Sionna RT
 *
 * This class bridges ns-3's MatrixBasedChannelModel framework and the
 * Sionna RT Python library (sionna.rt) to generate realistic, geometry-based
 * MIMO channels. It embeds a Python interpreter (via pybind11), constructs
 * scenes (terrain/building geometry), computes propagation paths (LOS,
 * specular/diffuse reflections, refraction, diffraction, synthetic arrays),
 * and converts the resulting paths into ns-3 ChannelMatrix and ChannelParams
 * objects for use by spectrum and wireless modules like nr, wifi, lte, etc
 *
 * Design highlights:
 *  - Scenes and path computations are performed with sionna.rt through pybind11.
 *  - Computed CIRs are represented using Complex3DVector (rx x tx x taps).
 *  - Caching: channel matrices and parameters are stored per-Node pair using a
 *    reciprocal uint64_t key derived from antenna/mobility instances.
 *  - The class exposes solver configuration options (RtPathSolverConfig) and
 *    scene configuration (RtSceneConfig).
 *  - Supported Sionna RT versions: 1.1.0, 1.2.2, 2.0.1.
 *
 * Threading/Concurrency:
 *  - The Python interpreter is created via pybind11::embed, which must be used
 *    carefully when ns-3 is executed in multi-threaded contexts. The current
 *    implementation assumes single-threaded execution (typical for ns-3).
 */
class SionnaRtChannelModel : public MatrixBasedChannelModel
{
  public:
    /**
     * @brief Constructor.
     */
    SionnaRtChannelModel();

    /**
     * @brief Destructor.
     */
    ~SionnaRtChannelModel() override;

    /**
     * @brief Release references and perform cleanup.
     */
    void DoDispose() override;

    /**
     * @brief Get the ns-3 TypeId for this class.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Enable or disable shape merging when loading scenes from sionna.rt.
     *
     * Controls whether the scene geometry loader merges individual mesh shapes
     * into larger primitives during scene creation. Merging reduces the
     * complexity of geometric primitives passed to the path solver and can
     * improve performance and memory usage for complex scenes at the potential
     * cost of small geometric inaccuracies.
     *
     * Important notes:
     *  - Default behaviour is defined by RtSceneConfig::mergeShapes (default true).
     *  - Changing this setting only affects future scene loads / creations; it
     *    does not retroactively modify already-loaded scenes or cached channel
     *    matrices. To apply this change immediately, existing cached scenes and
     *    channel matrices must be invalidated or regenerated.
     *  - The setting is forwarded to sionna.rt via the "merge_shapes" argument
     *    when calling rt.load_scene(sceneFactory, merge_shapes=...).
     *
     * @param cond True to enable shape merging (reduce geometry complexity),
     *             false to preserve original, detailed shapes.
     */
    void SetMergeShapeEnable(bool cond);

    /**
     * @brief Create a new ChannelMatrix object wrapping a precomputed CIR tensor.
     *
     * This function converts a 3D array of complex samples into an ns-3
     * MatrixBasedChannelModel::ChannelMatrix instance. The expected format of
     * hUsn is a Complex3DVector shaped as:
     *   [numRxPorts][numTxPorts][numTaps],
     * where each complex entry represents a channel tap (CIR sample) for the
     * corresponding Rx/Tx element pair for a given delay tap index.
     *
     * The function will populate the ChannelMatrix's internal metadata (ports,
     * tap delays, samplerate assumptions) and will return a Ptr to the created
     * wrapper object. The mobility and antenna pointers are provided primarily
     * to capture metadata and orientation for consistent caching and book-keeping.
     *
     * @param hUsn 3D complex CIR vector (rx ports x tx ports x taps)
     * @param sMob Mobility model of the transmitter endpoint
     * @param uMob Mobility model of the receiver endpoint
     * @param sAntenna Transmitter PhasedArrayModel
     * @param uAntenna Receiver PhasedArrayModel
     * @return A pointer to the created ChannelMatrix instance
     */
    Ptr<MatrixBasedChannelModel::ChannelMatrix> CreateNewMatrixChannel(
        const Complex3DVector hUsn,
        const Ptr<const MobilityModel> sMob,
        const Ptr<const MobilityModel> uMob,
        Ptr<const PhasedArrayModel> sAntenna,
        Ptr<const PhasedArrayModel> uAntenna) const;

    /**
     * @brief Set the propagation scenario name used by sionna.rt scene factories.
     *
     * The scenario string selects a pre-defined scene factory within sionna.rt
     * (for example "munich", "simple_street_canyon_with_cars", etc.). The scene
     * name is used by LoadScene/CreateScene to instantiate the geometry.
     *
     * @param scenario The scene factory name to call in sionna.rt.scene
     */
    void SetScenario(const std::string& scenario);

    /**
     * @brief Return the configured propagation scenario name.
     *
     * @return the current propagation scenario name
     */
    std::string GetScenario() const;

    /**
     * @brief Set the center frequency used by the model and scene factories.
     *
     * The frequency affects antenna patterns, wavelength-dependent computations,
     * and where applicable, factory-specific scene generation (e.g., building
     * materials that might be frequency-specific).
     *
     * @param f Center frequency in Hz
     */
    void SetFrequency(double f);

    /**
     * @brief Get the configured center frequency.
     * @return Center frequency in Hz.
     */
    double GetFrequency() const;

    /**
     * @brief Set the maximum number of propagation paths to retain.
     *
     * Controls the upper limit on the number of paths that will be accepted from
     * the Sionna RT path solver. If the solver generates more paths than this
     * threshold, the CalculatePaths() method will recursively reduce the solver's
     * max_depth and recompute until the path count falls within bounds. This
     * mechanism ensures that channel matrix complexity remains tractable.
     *
     * @param p Maximum number of paths (typically 1 to 1000 depending on antenna
     *          arrays and simulation requirements)
     *
     * @see GetMaxNumberOfPaths, DoesNumberOfPathsExceedMaximum
     */
    void SetMaxNumberOfPaths(double p);

    /**
     * @brief Get the configured maximum number of propagation paths.
     *
     * @return Maximum number of paths allowed in computed channel matrices
     *
     * @see SetMaxNumberOfPaths
     */
    double GetMaxNumberOfPaths() const;

    /**
     * @brief Enable or disable normalization of path delays in Sionna RT.
     *
     * When true, Sionna RT normalizes delays so the first path is at zero
     * delay. When false, raw path delays are returned.
     *
     * @param cond True to normalize delays, false to preserve raw delays.
     */
    void SetNormalizeDelays(bool cond);

    /**
     * @brief Query whether path delay normalization is enabled.
     *
     * @return true if Sionna RT delay normalization is enabled.
     */
    bool GetNormalizeDelays() const;

    /**
     * @brief Return a string describing the antenna element pattern for Sionna.
     *
     * This method inspects the ns-3 AntennaModel instance and returns a string
     * that represents the Sionna element pattern (e.g., "isotropic", "3gpp_antenna").
     * Sionna expects the element description when creating PlanarArray objects
     * for element-level responses.
     *
     * @param element Pointer to the ns-3 AntennaModel for a single element
     * @return A string representation of the element pattern suitable for Sionna
     */
    std::string GetAntennaElementPattern(Ptr<const AntennaModel> element) const;

    /** Scene high-level configuration */
    struct RtSceneConfig
    {
        /// Which scene factory in sionna.rt.scene to call. Example: "munich"
        std::string sceneName = "munich";

        /// Whether to merge shapes during load_scene(...) to reduce geometry complexity
        bool mergeShapes = true;

        // Global arrays (installed onto scene.tx_array / scene.rx_array)
        double frequency; //!< the operating frequency
    };

    /**
     * @brief ChannelParams extension to carry Sionna-specific metadata.
     *
     * This struct extends MatrixBasedChannelModel::ChannelParams with additional
     * fields derived from Sionna RT paths (e.g., per-cluster Doppler shifts).
     */
    struct SionnaRtChannelParams : public ChannelParams
    {
        /**
         * Doppler shifts per cluster/path (complex vector indexed by path).
         */
        std::vector<double> m_doppler;
    };

    /** Configuration for the Sionna RT PathSolver */
    struct RtPathSolverConfig
    {
        /**
         * Maximum number of interactions (reflection/refraction depth)
         */
        int maxDepth = 2;

        /**
         * Include direct line-of-sight (LOS) path when true ::It only controls whether the solver
         * should include the direct path if it exists. When false, only non-LOS paths are
         * considered.
         */
        bool los = true;

        /**
         * Enable specular reflections (mirror-like)
         */
        bool specularReflection = true;

        /**
         * Enable diffuse reflections (scattering)
         */
        bool diffuseReflection = true;

        /**
         * Enable refraction through transparent bodies / materials
         */
        bool refraction = true;

        /**
         * Use synthetic array processing (if supported by scene factory)
         */
        bool syntheticArray = true;

        /**
         * Enable diffraction
         */
        bool diffraction = false;

        /**
         * Enable edge diffraction (alternative diffraction handling)
         */
        bool edgeDiffraction = false;

        /**
         * Random seed for stochastic components (e.g., diffuse scattering)
         */
        int seed = 41;
    };

    /**
     * @brief Retrieve or generate the channel matrix for the node pair.
     *
     * Given mobility and antenna pointers for endpoints A/B, this method returns
     * a cached ChannelMatrix if available and up-to-date; otherwise it invokes
     * GetNewChannel to compute a new one using Sionna RT. The cache key is a
     * reciprocal uint64_t encoding of endpoint identifiers.
     *
     * @param aMob Mobility model of node A (transmitter or one endpoint).
     * @param bMob Mobility model of node B (receiver or the other endpoint).
     * @param aAntenna Antenna array of node A.
     * @param bAntenna Antenna array of node B.
     * @return A pointer to a const ChannelMatrix instance representing the channel.
     */
    Ptr<const ChannelMatrix> GetChannel(Ptr<const MobilityModel> aMob,
                                        Ptr<const MobilityModel> bMob,
                                        Ptr<const PhasedArrayModel> aAntenna,
                                        Ptr<const PhasedArrayModel> bAntenna) override;

    /**
     * @brief Get channel parameters (path info) for a node pair if available.
     *
     * If the pair has associated ChannelParams in m_channelParamsMap this
     * returns it; otherwise a null pointer is returned.
     *
     * @param aMob Mobility model of node A.
     * @param bMob Mobility model of node B.
     * @return A pointer to const ChannelParams or nullptr when not available.
     */
    Ptr<const ChannelParams> GetParams(Ptr<const MobilityModel> aMob,
                                       Ptr<const MobilityModel> bMob) const override;

    /**
     * @brief Assign stream indices to any random variables used internally.
     *
     * This is used to make simulations reproducible by assigning deterministic
     * RNG stream indices.
     *
     * @param stream First stream index to use.
     * @return Number of stream indices assigned.
     */
    int64_t AssignStreams(int64_t stream);

    /**
     * @brief Compute a new channel matrix for the given node pair using Sionna RT.
     *
     * This method performs the entire flow:
     * 1. Create or load an sionna.rt scene
     * 2. Run the path solver to compute propagation paths
     * 3. Compute CIRs (per Rx/Tx element and path delays)
     * 4. Package CIRs into a ChannelMatrix instance and return it
     *
     * The function accepts the 'paths' object returned by CalculatePaths rather
     * than the Python scene directly to allow callers to separate scene and path
     * computation if desired.
     *
     * @param paths Python object describing the computed paths (from CalculatePaths)
     * @param sMob Mobility model of the transmitter endpoint
     * @param uMob Mobility model of the receiver endpoint
     * @param sAntenna Transmitter PhasedArrayModel
     * @param uAntenna Receiver PhasedArrayModel
     * @return A pointer to the newly created ChannelMatrix instance
     */
    virtual Ptr<ChannelMatrix> GetNewChannel(py::object paths,
                                             const Ptr<const MobilityModel> sMob,
                                             const Ptr<const MobilityModel> uMob,
                                             Ptr<const PhasedArrayModel> sAntenna,
                                             Ptr<const PhasedArrayModel> uAntenna) const;

    /**
     * @brief Determine whether an existing ChannelMatrix must be updated.
     *
     * Checks if the cached ChannelMatrix is stale because of configuration changes
     * (update period expired or mobility changed). If the update period has passed
     * or the ChannelMatrix metadata differs from the provided inputs, it will
     * report that regeneration is required.
     *
     * @param channelMatrix The existing channel matrix to check
     * @return true if the matrix must be updated; false otherwise
     */
    bool ChannelMatrixNeedsUpdate(Ptr<const ChannelMatrix> channelMatrix) const;

    /**
     * @brief Check whether antenna configuration changes require regenerating the channel matrix.
     *
     * This compares the provided antenna arrays with the stored ChannelMatrix
     * metadata to decide if the matrix is stale (for example if number of ports,
     * element locations, or element types changed).
     *
     * @param aAntenna Antenna array of node A
     * @param bAntenna Antenna array of node B
     * @param channelMatrix The existing channel matrix to compare against
     * @return true if the channel matrix must be updated; false otherwise
     */
    bool AntennaSetupChanged(Ptr<const PhasedArrayModel> aAntenna,
                             Ptr<const PhasedArrayModel> bAntenna,
                             Ptr<const ChannelMatrix> channelMatrix) const;

    /**
     * @brief Build a Sionna PlanarArray description from an ns-3 PhasedArrayModel.
     *
     * This converts antenna element locations, patterns and polarization into a
     * Python PlanarArray object suitable for installation into an sionna.rt scene.
     *
     * @param rt sionna.rt module used to construct Python objects
     * @param antenna The ns-3 PhasedArrayModel to convert
     * @return A Python PlanarArray object (py::object)
     */
    py::object MakePlannarArray(const py::module_ rt, Ptr<const PhasedArrayModel> antenna) const;

    /**
     * @brief Create a sionna.rt scene object for the provided endpoints and antennas.
     *
     * The returned py::object is the Python scene instance and ownership is
     * managed by pybind11. The method uses the RtSceneConfig and frequency set
     * in this class to configure the scene creation.
     *
     * @param rt sionna.rt module used to construct Python objects
     * @param sMob Mobility model of the transmitter
     * @param uMob Mobility model of the receiver
     * @param sAntenna Transmitter phased array model
     * @param uAntenna Receiver phased array model
     * @return A Python scene object (py::object)
     */
    py::object CreateScene(const py::module_ rt,
                           const Ptr<const MobilityModel> sMob,
                           const Ptr<const MobilityModel> uMob,
                           Ptr<const PhasedArrayModel> sAntenna,
                           Ptr<const PhasedArrayModel> uAntenna) const;

    /**
     * @brief Configure the Sionna RT path solver behavior.
     *
     * Sets all options that control how the path solver traces rays through the
     * scene, including maximum interaction depth, which phenomena to model
     * (LOS, reflections, diffractions, etc.), and whether to use synthetic array
     * processing. These settings remain in effect for all subsequent path
     * computations until changed again.
     *
     * @param configs RtPathSolverConfig structure containing solver options
     *
     * @see GetRtPathSolverConfig, RtPathSolverConfig
     */
    void SetRtPathSolverConfig(RtPathSolverConfig configs);

    /**
     * @brief Retrieve the current path solver configuration.
     *
     * @return A copy of the RtPathSolverConfig currently in use
     *
     * @see SetRtPathSolverConfig
     */
    RtPathSolverConfig GetRtPathSolverConfig() const;

  protected:
    /**
     * @brief Load a Sionna RT scene using the configured scenario name.
     *
     * Instantiates the scene factory selected by m_sceneConfigs.sceneName. The
     * method detects whether the scenario is:
     *   - A built-in Sionna scenario name (e.g., "munich", "simple_street_canyon"),
     *     in which case rt.load_scene() is invoked with the scenario name; or
     *   - A custom XML file path (filename ending with ".xml"), in which case
     *     rt.Scene(filename=...) is called to load from file.
     *
     * The merge_shapes parameter from m_sceneConfigs is forwarded to control
     * whether the scene loader should merge individual shapes.
     *
     * @param rt The imported sionna.rt Python module for scene creation
     * @return A Python Scene object (py::object) ready for transmitter/receiver
     *         and path solver configuration
     *
     * @see SetScenario, RtSceneConfig
     */
    py::object LoadScene(const py::module_ rt) const;

    /**
     * @brief Render a visualization of the scene and paths to a PNG image file.
     *
     * If m_isImageRendered is true, this method invokes the Sionna RT rendering
     * pipeline using the specified camera parameters and writes the output image
     * to the file specified by m_outputImageDirectory and m_outputImageName.
     *
     * The image provides visual debugging of scene geometry and path interactions,
     * which can be useful for validating scenario definitions and understanding
     * propagation mechanisms.
     *
     * @param rt The sionna.rt module providing rendering functionality
     * @param paths The Python Paths object containing computed propagation paths
     * @param scene The Python Scene object describing the simulation environment
     */
    void SceneRenderImageToFile(const py::module_ rt,
                                const py::object paths,
                                const py::object scene) const;

    /**
     * @brief Convert a polarization slant angle into Sionna RT string representation.
     *
     * Maps the polarization configuration (defined by slant angle and element count)
     * into one of Sionna's standard polarization strings: "V" (vertical), "H"
     * (horizontal), "VH" (dual polarization), or "cross" (cross-polarization).
     *
     * @param polSlantAngle Polarization slant angle in radians (typically 0 or pi/2)
     * @param numpol Number of polarizations per element (1 = single-pol, 2 = dual-pol)
     * @return Sionna polarization string suitable for PlanarArray configuration
     */
    std::string GetPolarizationFromPolSlantAngle(const double polSlantAngle,
                                                 const uint8_t numpol) const;

    /**
     * @brief Compute propagation paths between transmitter and receiver.
     *
     * Invokes the Sionna RT PathSolver with the configured RtPathSolverConfig
     * options (max_depth, los, specular_reflection, etc.). The solver recursively
     * traces rays through the scene to identify all significant paths up to the
     * configured maximum depth. If the number of paths exceeds the configured
     * maximum (m_maxNumberOfPaths), this method automatically adjusts the solver
     * depth and retries until the path count is acceptable.
     *
     * @param rt The imported sionna.rt Python module
     * @param scene A Python Scene object (from CreateScene or LoadScene)
     * @return A Python Paths object containing delays, angles, powers, and
     *         other per-path metadata
     *
     * @see SetRtPathSolverConfig, GetNumberOfPathsFromCir
     */
    py::object CalculatePaths(const py::module_ rt, const py::object scene);

    /**
     * @brief Convert calculated paths into channel impulse responses (CIRs).
     *
     * The returned Complex3DVector is shaped as:
     *   [numRxPorts][numTxPorts][path]
     * where each path is the complex channel coefficient for a given delay tap.
     * Delay and power normalization are handled according to the scene and
     * solver semantics. The function uses internal utilities to extract angles,
     * Doppler and delays from the paths object.
     *
     * @param paths Python object representing computed paths
     * @return Complex3DVector containing CIRs for Rx/Tx element pairs
     */
    Complex3DVector CalculateCirFromPaths(const py::object paths) const;

    /**
     * @brief Extract delays (tau) per path from RS paths object.
     *
     * Returns a vector of delay values for each path aligned with the CIR.
     *
     * @param np The numpy_python module reference (py::module_)
     * @param paths Python paths object computed by Sionna RT
     * @return Vector of delays [s], ordered to match CIR taps
     */
    MatrixBasedChannelModel::DoubleVector CalculateTauFromPaths(const py::module_ np,
                                                                py::object paths) const;
    /**
     * @brief Extract Doppler shifts per path from the Python paths object.
     *
     * The Doppler shifts are returned in a PhasedArrayModel::ComplexVector where
     * each entry is e^{j*2*pi*fd*t} or similar, depending on the calling convention.
     *
     * @param np The numpy_python module reference (py::module_)
     * @param paths Python paths object computed by Sionna RT
     * @return Complex vector of per-path Doppler shifts (one per path)
     */
    std::vector<double> CalculateDopplerFromPaths(const py::module_ np, py::object paths) const;
    /**
     * @brief Extract angular measurements (AOD/AOA/ZOA/ZOD) from the paths.
     *
     * Returns the requested angle measure for each path in radians. The Angle
     * parameter is case-sensitive and should be one of:
     *   - "theta_r" (Zenith angles of arrival, ZOA)
     *   - "theta_t" (Zenith angles of departure, ZOD)
     *   - "phi_t"   (Azimuth angles of departure, AOD)
     *   - "phi_r"   (Azimuth angles of arrival, AOA)
     *
     * @param np The numpy_python module reference (py::module_)
     * @param paths Python paths object computed by Sionna RT
     * @param Angle String indicating the desired angle type
     * @return Vector of angle values (radians) per path/tap
     */
    MatrixBasedChannelModel::DoubleVector CalculateAnglesFromPaths(const py::module_ np,
                                                                   py::object paths,
                                                                   std::string Angle) const;

    // void CreateOnePath(py::object& paths);

    /**
     * @brief Build ChannelParams metadata from Sionna RT paths object.
     *
     * Produces an instance of SionnaRtChannelParams, populated with per-path
     * doppler terms (m_doppler) and any other per-channel metadata extracted
     * from the paths object.
     *
     * @param paths Python paths object returned by CalculatePaths
     * @param aMob Mobility model of node A
     * @param bMob Mobility model of node B
     * @return Ptr to an allocated SionnaRtChannelParams instance
     */

    Ptr<SionnaRtChannelParams> CalculateChannelParamsFromPaths(const py::object paths,
                                                               Ptr<const MobilityModel> aMob,
                                                               Ptr<const MobilityModel> bMob) const;

    /**
     * @brief Extract the number of propagation paths from the CIR tensor.
     *
     * Retrieves the channel impulse response (CIR) data from the Sionna RT paths
     * object and extracts the total number of propagation paths. The CIR is
     * organized as a 6-D numpy array with shape:
     *   [num_rx, num_rx_ant, num_tx, num_tx_ant, num_paths, num_time_steps]
     *
     * This method works identically for both synthetic array and standard array
     * configurations, always reading from dimension 4 (num_paths).
     *
     * @param paths Python Paths object computed by Sionna RT PathSolver
     * @return Number of propagation paths in the CIR; returns 0 if the array
     *         has unexpected dimensionality or is malformed
     *
     * @see DoesNumberOfPathsExceedMaximum
     */
    size_t GetNumberOfPathsFromCir(const py::object paths) const;

    /**
     * @brief Determine if the path count exceeds the configured maximum threshold.
     *
     * Compares a given path count against the m_maxNumberOfPaths attribute to
     * decide whether the channel matrix computation should be rejected or
     * recursively recomputed with reduced solver depth.
     *
     * @param numPaths Number of paths to check against the threshold
     * @return true if numPaths > m_maxNumberOfPaths; false otherwise
     */
    bool DoesNumberOfPathsExceedMaximum(size_t numPaths) const;

    // Attributes
    std::unordered_map<uint64_t, Ptr<ChannelMatrix>>
        m_channelMatrixMap; //!< map containing the channel realizations per pair of
                            //!< PhasedAntennaArray instances, the key of this map is reciprocal
                            //!< uniquely identifies a pair of PhasedAntennaArrays
    std::unordered_map<uint64_t, Ptr<SionnaRtChannelParams>>
        m_channelParamsMap; //!< map containing the common channel parameters per pair of nodes, the
                            //!< key of this map is reciprocal and uniquely identifies a pair of
                            //!< nodes
    /** Channel update period used to decide whether to re-run path computations */
    Time m_updatePeriod;

    /** Path solver configuration used when computing paths */
    RtPathSolverConfig m_RtPathSolverConfig;

    /** High-level scene configuration */
    RtSceneConfig m_sceneConfigs;

    /** Output image rendering options (file name/directory) */
    std::string m_outputImageName;      //!< Output file name
    std::string m_outputImageDirectory; //!< Output directory

    /** Control for whether we should render a scene image */
    bool m_isImageRendered;
    /** Whether Sionna RT should normalize path delays in CIR output. */
    bool m_normalizeDelays;
    double m_maxNumberOfPaths; //!< Maximum number of paths to be generated by the path solver
    /** Camera position/look-at used by the scene rendering pipeline */
    Vector m_cameraPosition; //!< Camera position for scene rendering
    Vector m_cameraLookAt;   //!< Camera look-at point for scene rendering
};

} // namespace ns3

#endif // SIONNA_RT_CHANNEL_MODEL_H
