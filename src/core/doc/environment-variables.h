/*
 * Copyright (c) 2022 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

// This file supplies Doxygen documentation only.
// It should *NOT* be included in the build.

/**
 * @ingroup system
 * @defgroup core-environ Environment Variables
 */

/**
 * @ingroup core-environ
 * @brief Initialize an ns3::Attribute
 *
 * Sets a new default \pname{value} for an Attribute.
 * This is invoked if the ns3::Object being constructed has an Attribute
 * matching \pname{name}, and that Attribute didn't appear in the
 * ns3::AttributeConstructionList argument to ns3::ObjectBase::ConstructSelf.
 * The ns3::AttributeConstructionList is typically createad by an ns3::ObjectFactory
 * and populated by ns3::ObjectFactory::Set.
 *
 * All objects with an Attribute matching \pname{name} will be matched,
 * so name collisions could wreak havoc.
 *
 * <dl class="params">
 *   <dt>%Parameters</dt>
 *     <dd>
 *       <table class="params">
 *         <tr>
 *           <td class="paramname">name</td>
 *           <td>The name of the Attribute to set.</td>
 *         </tr>
 *         <tr>
 *           <td class="paramname">value</td>
 *           <td>The value to set the Attribute to.</td>
 *         </tr>
 *         <tr>
 *           <td class="paramname">;</td>
 *           <td>Multiple \pname{name}=\pname{value} pairs should be delimited by ';'</td>
 *         </tr>
 *       </table>
 *     </dd>
 * </dl>
 *
 * Referenced by ns3::ObjectBase::ConstructSelf.
 */
const char* NS_ATTRIBUTE_DEFAULT = "name=value[;...]";

/**
 * @ingroup core-environ
 * @brief Write the ns3::CommandLine::Usage message, in Doxygen format,
 * to the referenced location.
 *
 * Set the directory where ns3::CommandLine instances should write their Usage
 * message, used when building documentation.
 *
 * This is used primarily by the documentation builds, which execute each
 * registered example to gather their ns3::CommandLine::Usage information.
 * This wouldn't normally be useful to users.
 *
 * <dl class="params">
 *   <dt>%Parameters</dt>
 *   <dd>
 *     <table class="params">
 *       <tr>
 *         <td class="paramname">path</td>
 *         <td> The directory where ns3::CommandLine should write its Usage message.</td>
 *       </tr>
 *     </table>
 *   </dd>
 * </dl>
 *
 * Referenced by ns3::CommandLine::PrintDoxygenUsage.
 */
const char* NS_COMMANDLINE_INTROSPECTION = "path";

/**
 * @ingroup core-environ
 * @brief Initialize a ns3::GlobalValue.
 *
 * Initialize the ns3::GlobalValue \pname{name} from \pname{value}.
 *
 * This overrides the initial value set in the corresponding
 * ns3::GlobalValue constructor.
 *
 * <dl class="params">
 *   <dt>%Parameters</dt>
 *   <dd>
 *     <table class="params">
 *       <tr>
 *         <td class="paramname">name</td>
 *         <td>The name of the ns3::GlobalValue to set.</td>
 *       </tr>
 *       <tr>
 *         <td class="paramname">value</td>
 *         <td>The value to set the ns3::GlobalValue to.</td>
 *       </tr>
 *       <tr>
 *         <td class="paramname">;</td>
 *         <td>Multiple \pname{name}=\pname{value} pairs should be delimited by ';'</td>
 *       </tr>
 *     </table>
 *   </dd>
 * </dl>
 *
 * Referenced by ns3::GlobalValue::InitializeFromEnv.
 */
const char* NS_GLOBAL_VALUE = "name=value[;...]";

/**
 * @ingroup core-environ
 * @brief Control which logging components are enabled.
 *
 * Enable logging from specific \pname{component}s, with specified \pname{option}s.
 * See the \ref logging module, or the Logging chapter
 * in the manual for details on what symbols can be used for specifying
 * log level and prefix options.
 *
 * <dl class="params">
 *   <dt>%Parameters</dt>
 *   <dd>
 *     <table class="params">
 *       <tr>
 *         <td class="paramname">component</td>
 *         <td>The logging component to enable.</td>
 *       </tr>
 *       <tr>
 *         <td class="paramname">option</td>
 *         <td>Log level and or prefix to enable. Multiple options should be delimited with '|'</td>
 *       </tr>
 *       <tr>
 *         <td class="paramname">:</td>
 *         <td>Multiple logging components can be enabled simultaneously, delimited by ':'</td>
 *       </tr>
 *     </table>
 *   </dd>
 * </dl>
 *
 * Referenced by ns3::PrintList, ns3::LogComponent::EnvVarCheck and
 * ns3::CheckEnvironmentVariables(), all in \ref log.cc.
 */
const char* NS_LOG = "component=option[|option...][:...]";

/**
 * @ingroup core-environ
 * @brief Where to make temporary directories.
 *
 * The absolute path where ns-3 should make temporary directories.
 *
 * See ns3::SystemPath::MakeTemporaryDirectoryName for details on how this environment
 * variable is used to create a temporary directory, and how that directory
 * will be named.
 *
 * <dl class="params">
 *   <dt>%Parameters</dt>
 *   <dd>
 *     <table class="params">
 *       <tr>
 *         <td class="paramname">path</td>
 *         <td>The absolute path in which to create the temporary directory.</td>
 *       </tr>
 *     </table>
 *   </dd>
 * </dl>
 *
 * Referenced by ns3::SystemPath::MakeTemporaryDirectoryName.
 *
 * @{
 */
const char* TMP = "path";
const char* TEMP = "path";
/**@}*/
