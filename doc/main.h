/**
 * @file
 * Main page of the Doxygen-generated documentation.
 */

/**
 * @mainpage ns-3 Documentation
 *
 * @section intro-sec Introduction
 * <a href="http://www.nsnam.org/">ns-3</a> documentation is maintained using
 * <a href="http://www.doxygen.org">Doxygen</a>.
 * Doxygen is typically used for
 * API documentation, and organizes such documentation across different
 * modules.   This project uses Doxygen for building the definitive
 * maintained API documentation.  Additional ns-3 project documentation
 * can be found at the
 * <a href="http://www.nsnam.org/documentation/latest">project web site</a>.
 *
 * @section install-sec Building the Documentation
 *
 * Building ns-3 Doxygen requires Doxygen version 1.11
 *
 * Type "./ns3 docs doxygen" or "./ns3 docs doxygen-no-build" to build the
 *  documentation.  The doc/ directory contains
 * configuration for Doxygen (doxygen.conf) and main.h.  The Doxygen
 * build process puts html files into the doc/html/ directory, and latex
 * filex into the doc/latex/ directory.
 *
 * @section topics-sec Doxygen groups
 *
 * The ns-3 library is split across many modules, and the Doxygen for these modules is typically
 * added to <a href ="https://www.doxygen.nl/manual/grouping.html">Doxygen groups</a>.
 * These groupings can be browsed under the <b><a href="topics.html">Topics</a></b> tab.
 */

/**
 * @namespace ns3
 * @brief Every class exported by the ns3 library is enclosed in the
 * ns3 namespace.
 */

/**
 * @name Macros defined by the build system.
 *
 * These have to be visible for doxygen to document them,
 * so we put them here in a file only seen by doxygen, not the compiler.
 *
 * @{
 */
/**
 * @ingroup assert
 *
 * @def NS3_ASSERT_ENABLE
 *
 * Enable asserts at compile time.
 *
 * This is normally set by `./ns3 configure --build-profile=debug`.
 */
#define NS3_ASSERT_ENABLE

/**
 * @ingroup logging
 *
 * @def NS3_LOG_ENABLE
 *
 * Enable logging at compile time.
 *
 * This is normally set by `./ns3 configure --build-profile=debug`.
 */
#define NS3_LOG_ENABLE

/**@}*/

/**
 * @page EnvironVar All Environment Variables
 *
 * All environment variables used by ns-3 are documented by module.
 *
 * @section environcore Core Environment Variables
 * See \ref core-environ
 */
