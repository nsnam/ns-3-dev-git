/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2021 University of Washington: Group mobility changes
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Adapted from 'mobility-helper.h' for group mobility by Tom Henderson
 */

#ifndef GROUP_MOBILITY_HELPER_H
#define GROUP_MOBILITY_HELPER_H

#include "ns3/attribute.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

#include <vector>

namespace ns3
{

class PositionAllocator;
class MobilityModel;

/**
 * @ingroup mobility
 * @brief Helper class used to assign positions and mobility models to nodes
 * for a group mobility configuration.
 *
 * This helper can be used for group mobility configuration and installation
 * onto a group of nodes, in which there is a reference (parent) mobility
 * model that is the same for all nodes, and similarly configured (but
 * distinct) child mobility models.
 */
class GroupMobilityHelper
{
  public:
    /**
     * Construct a group mobility helper
     */
    GroupMobilityHelper();

    /**
     * Destroy a group mobility helper
     */
    ~GroupMobilityHelper();

    /**
     * Set the position allocator which will be used to allocate the initial
     * position of the reference mobility model.
     *
     * @param allocator allocate initial reference mobility model position
     */
    void SetReferencePositionAllocator(Ptr<PositionAllocator> allocator);

    /**
     * Configure the position allocator which will be used to allocate
     * the initial position of the reference mobility model.
     *
     * @tparam Ts \deduced Argument types
     * @param type the type of position allocator to use.
     * @param [in] args Name and AttributeValue pairs to set.
     */
    template <typename... Ts>
    void SetReferencePositionAllocator(std::string type, Ts&&... args);

    /**
     * Set the position allocator which will be used to allocate the initial
     * position of the member mobility models.
     *
     * @param allocator allocate initial member mobility model positions
     */
    void SetMemberPositionAllocator(Ptr<PositionAllocator> allocator);

    /**
     * Configure the position allocator which will be used to allocate the
     * initial position of the member mobility models.
     *
     * @tparam Ts \deduced Argument types
     * @param type the type of position allocator to use.
     * @param [in] args Name and AttributeValue pairs to set.
     */
    template <typename... Ts>
    void SetMemberPositionAllocator(std::string type, Ts&&... args);

    /**
     * Set the reference mobility model which will be installed as the parent
     * mobility model during GroupMobilityModel::Install.
     *
     * @param mobility reference mobility model
     */
    void SetReferenceMobilityModel(Ptr<MobilityModel> mobility);

    /**
     * Configure the reference mobility model which will be installed as the
     * parent mobility model during GroupMobilityModel::Install.
     *
     * @tparam Ts \deduced Argument types
     * @param type the type of mobility model to use.
     * @param [in] args Name and AttributeValue pairs to set.
     */
    template <typename... Ts>
    void SetReferenceMobilityModel(std::string type, Ts&&... args);

    /**
     * Configure the mobility model which will be installed as the
     * member (child) mobility model during GroupMobilityModel::Install.
     *
     * Calls to MobilityHelper::Install will create an instance of a matching
     * mobility model for each node.
     *
     * @tparam Ts \deduced Argument types
     * @param type the type of mobility model to use.
     * @param [in] args Name and AttributeValue pairs to set.
     */
    template <typename... Ts>
    void SetMemberMobilityModel(std::string type, Ts&&... args);

    /**
     * @brief Install and configure a hierarchical mobility model to the
     * given node, based on the configured reference and member models.
     *
     * If position allocators are configured, they will be invoked to
     * set the initial position.
     *
     * @param node The node to configure
     */
    void Install(Ptr<Node> node);
    /**
     * @brief Install and configure a hierarchical mobility model to the
     * given node, based on the configured reference and member models.
     *
     * If position allocators are configured, they will be invoked to
     * set the initial position.
     *
     * @param nodeName The name of the node to configure
     */
    void Install(std::string nodeName);

    /**
     * @brief Install and configure a hierarchical mobility model to all nodes
     * in the container, based on the configured reference and member models.
     *
     * If position allocators are configured, they will be invoked to
     * set the initial positions.
     *
     * @param container The set of nodes to configure
     */
    void Install(NodeContainer container);

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by the mobility models on these nodes. Return the number of
     * streams (possibly zero) that have been assigned. The Install()
     * method should have previously been called by the user.
     *
     * @note If the PositionAllocator used contains random variables, they
     * will not be affected by this call to AssignStreams because they are
     * used earlier during Install() time.  If the user needs to assign a fixed
     * stream number to a PositionAllocator used with this helper, the user
     * should instantiate it outside of the helper, call AssignStreams() on
     * it, and then pass the pointer of it to this helper.
     *
     * @param c NodeContainer of the set of nodes containing the MobilityModels
     * that should be modified to use a fixed stream
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this helper
     */
    int64_t AssignStreams(NodeContainer c, int64_t stream);

  private:
    // Enable logging from template instantiations
    NS_LOG_TEMPLATE_DECLARE; //!< the log component

    bool m_referencePositionSet{
        false}; //!< flag for avoiding multiple SetPosition calls on the reference model
    Ptr<MobilityModel> m_referenceMobility; //!< Reference mobility model
    Ptr<PositionAllocator>
        m_referencePosition; //!< Position allocator for use as reference position allocator
    ObjectFactory m_memberMobilityFactory; //!< Object factory to create member mobility models
    Ptr<PositionAllocator>
        m_memberPosition; //!< Position allocator for use as member position allocator
};

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

template <typename... Ts>
void
GroupMobilityHelper::SetReferencePositionAllocator(std::string type, Ts&&... args)
{
    ObjectFactory pos(type, std::forward<Ts>(args)...);
    m_referencePosition = pos.Create()->GetObject<PositionAllocator>();
    NS_ABORT_MSG_IF(!m_referencePosition, "Unable to create allocator from TypeId " << type);
}

template <typename... Ts>
void
GroupMobilityHelper::SetMemberPositionAllocator(std::string type, Ts&&... args)
{
    ObjectFactory pos(type, std::forward<Ts>(args)...);
    m_memberPosition = pos.Create()->GetObject<PositionAllocator>();
    NS_ABORT_MSG_IF(!m_memberPosition, "Unable to create allocator from TypeId " << type);
}

template <typename... Ts>
void
GroupMobilityHelper::SetReferenceMobilityModel(std::string type, Ts&&... args)
{
    NS_LOG_FUNCTION(this << type);
    ObjectFactory mob(type, std::forward<Ts>(args)...);
    m_referenceMobility = mob.Create()->GetObject<MobilityModel>();
    NS_ABORT_MSG_IF(!m_referenceMobility, "Unable to create mobility from TypeId " << type);
}

template <typename... Ts>
void
GroupMobilityHelper::SetMemberMobilityModel(std::string type, Ts&&... args)
{
    NS_LOG_FUNCTION(this << type);
    m_memberMobilityFactory.SetTypeId(type);
    m_memberMobilityFactory.Set(std::forward<Ts>(args)...);
}

} // namespace ns3

#endif /* GROUP_MOBILITY_HELPER_H */
