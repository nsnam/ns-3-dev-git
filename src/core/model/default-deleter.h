/*
 * Copyright (c) 2010 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#ifndef DEFAULT_DELETER_H
#define DEFAULT_DELETER_H

/**
 * @file
 * @ingroup ptr
 * ns3::DefaultDeleter declaration and template implementation,
 * for reference-counted smart pointers.
 */

namespace ns3
{

/**
 * @ingroup ptr
 * @brief A template used to delete objects
 *        by the ns3::SimpleRefCount templates when the
 *        last reference to an object they manage
 *        disappears.
 *
 * @tparam T \deduced The object type being deleted.
 * \sa ns3::SimpleRefCount
 */
template <typename T>
struct DefaultDeleter
{
    /**
     * The default deleter implementation, which just does a normal
     * @code
     *   delete object;
     * @endcode
     * @tparam T \deduced The object type being deleted.
     * @param [in] object The object to delete.
     */
    inline static void Delete(T* object)
    {
        delete object;
    }
};

} // namespace ns3

#endif /* DEFAULT_DELETER_H */
