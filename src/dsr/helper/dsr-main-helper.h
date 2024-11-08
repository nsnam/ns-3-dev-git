/*
 * Copyright (c) 2011 Yufei Cheng
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Yufei Cheng   <yfcheng@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

#ifndef DSR_MAIN_HELPER_H
#define DSR_MAIN_HELPER_H

#include "dsr-helper.h"

#include "ns3/dsr-routing.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/object-factory.h"

namespace ns3
{

/**
 * @ingroup dsr
 *
 * @brief Helper class that adds DSR routing to nodes.
 */
class DsrMainHelper
{
  public:
    /**
     * Create an DsrMainHelper that makes life easier for people who want to install
     * DSR routing to nodes.
     */
    DsrMainHelper();
    ~DsrMainHelper();
    /**
     * @brief Construct an DsrMainHelper from another previously initialized instance
     * (Copy Constructor).
     * @param o object to copy from
     */
    DsrMainHelper(const DsrMainHelper& o);
    /**
     * Install routing to the nodes
     * @param dsrHelper The DSR helper class
     * @param nodes the collection of nodes
     */
    void Install(DsrHelper& dsrHelper, NodeContainer nodes);
    /**
     * Set the helper class
     * @param dsrHelper the DSR helper class
     */
    void SetDsrHelper(DsrHelper& dsrHelper);

  private:
    /**
     * Install routing to a node
     * @param node the node to install DSR routing
     */
    void Install(Ptr<Node> node);
    /**
     * @brief Assignment operator declared private and not implemented to disallow
     * assignment and prevent the compiler from happily inserting its own.
     * @param o source object to assign
     * @return DsrHelper object
     */
    DsrMainHelper& operator=(const DsrMainHelper& o);
    const DsrHelper* m_dsrHelper; ///< helper class
};

} // namespace ns3

#endif /* DSR_MAIN_HELPER_H */
