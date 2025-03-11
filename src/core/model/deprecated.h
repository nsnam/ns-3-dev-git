/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef NS3_DEPRECATED_H
#define NS3_DEPRECATED_H

/**
 * @file
 * @ingroup deprecation
 * NS_DEPRECATED macro definition.
 */

/**
 * @defgroup deprecation Deprecation
 * @ingroup core
 */

/**
 * @ingroup deprecation
 * @def NS_DEPRECATED
 * Mark a function as deprecated.
 *
 * Users should expect deprecated features to be removed eventually.
 *
 * When deprecating a feature, please update the documentation
 * with information for users on how to update their code.
 *
 * The following snippet shows an example of how to deprecate the function SomethingUseful()
 * in favor of the new function TheNewWay().
 * Note: in the following snippet, the Doxygen blocks are not following the ns-3 style.
 * This allows the code to be safely embedded in the documentation.
 *
 * @code
 * /// Do something useful.
 * ///
 * /// \deprecated This method will go away in future versions of ns-3.
 * /// See instead TheNewWay().
 * NS_DEPRECATED_3_XX("see TheNewWay")
 * void SomethingUseful();
 *
 * /// Do something more useful.
 * void TheNewWay();
 * @endcode
 *
 * Please follow these two guidelines to ease future maintenance
 * (primarily the eventual removal of the deprecated code):
 *
 * 1.  Please use the versioned form `NS_DEPRECATED_3_XX`,
 *     not the generic `NS_DEPRECATED`.
 *
 * 2.  Typically only the declaration needs to be deprecated,
 *
 *     @code
 *     NS_DEPRECATED_3_XX("see TheNewWay")
 *     void SomethingUseful();
 *     @endcode
 *
 *     but it's helpful to put the same macro as a comment
 *     at the site of the definition, to make it easier to find
 *     all the bits which eventually have to be removed:
 *
 *     @code
 *     // NS_DEPRECATED_3_XX("see TheNewWay")
 *     void SomethingUseful() { ... }
 *     @endcode
 *
 * @note Sometimes it is necessary to silence a deprecation warning.
 * Even though this is highly discouraged, if necessary it is possible to use:
 * @code
 *   NS_WARNING_PUSH_DEPRECATED;
 *   // call to a function or class that has been deprecated.
 *   NS_WARNING_POP;
 * @endcode
 * These macros are defined in warnings.h
 *
 * @param msg Optional message to add to the compiler warning.
 *
 */
#define NS_DEPRECATED(msg) /** \deprecated msg */ [[deprecated(msg)]]

/**
 * @ingroup deprecation
 * @def NS_DEPRECATED_3_45
 * Tag for things deprecated in version ns-3.45.
 */
#define NS_DEPRECATED_3_45(msg) NS_DEPRECATED("Deprecated in ns-3.45: " msg)

/**
 * @ingroup deprecation
 * @def NS_DEPRECATED_3_44
 * Tag for things deprecated in version ns-3.44.
 */
#define NS_DEPRECATED_3_44(msg) NS_DEPRECATED("Deprecated in ns-3.44: " msg)

/**
 * @ingroup deprecation
 * @def NS_DEPRECATED_3_43
 * Tag for things deprecated in version ns-3.43.
 */
#define NS_DEPRECATED_3_43(msg) NS_DEPRECATED("Deprecated in ns-3.43: " msg)

/**
 * @ingroup deprecation
 * @def NS_DEPRECATED_3_42
 * Tag for things deprecated in version ns-3.42.
 */
#define NS_DEPRECATED_3_42(msg) NS_DEPRECATED("Deprecated in ns-3.42: " msg)

/**
 * @ingroup deprecation
 * @def NS_DEPRECATED_3_41
 * Tag for things deprecated in version ns-3.41.
 */
#define NS_DEPRECATED_3_41(msg) NS_DEPRECATED("Deprecated in ns-3.41: " msg)

/**
 * @ingroup deprecation
 * @def NS_DEPRECATED_3_40
 * Tag for things deprecated in version ns-3.40.
 */
#define NS_DEPRECATED_3_40(msg) NS_DEPRECATED("Deprecated in ns-3.40: " msg)

#endif /* NS3_DEPRECATED_H */
