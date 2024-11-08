/*
 * Copyright (c) 2011 Bucknell University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Tiago G. Rodrigues (tgr002@bucknell.edu)
 */

#ifndef DATA_COLLECTION_OBJECT_H
#define DATA_COLLECTION_OBJECT_H

#include "ns3/object.h"

#include <string>

namespace ns3
{

/**
 * @ingroup aggregator
 *
 * Base class for data collection framework objects.
 *
 * All data collection objects have 1) a string name, and 2) enabled
 * or disabled status.
 */
class DataCollectionObject : public Object
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    DataCollectionObject();
    ~DataCollectionObject() override;

    /// Set the status of an individual object.
    void Enable();
    /// Unset the status of an individual object.
    void Disable();

    /**
     * Check the status of an individual object.
     * @return true if the object is enabled
     */
    virtual bool IsEnabled() const;

    /**
     * Get the object's name.
     * @return the object's name
     */
    std::string GetName() const;

    /**
     * Set the object's name.
     * @param name the object's name
     */
    void SetName(std::string name);

  protected:
    /// Object's activation state.
    bool m_enabled;

    /// Name of the object within the data collection framework
    std::string m_name;
};

} // namespace ns3

#endif // DATA_COLLECTION_OBJECT_H
