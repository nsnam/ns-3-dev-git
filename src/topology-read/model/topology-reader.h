/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Universita' di Firenze, Italy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Tommaso Pecorella (tommaso.pecorella@unifi.it)
 * Author: Valerio Sartini (valesar@gmail.com)
 */

#ifndef TOPOLOGY_READER_H
#define TOPOLOGY_READER_H

#include <string>
#include <map>
#include <list>

#include "ns3/object.h"
#include "ns3/node-container.h"


namespace ns3 {

/**
 * \ingroup topology
 *
 * \brief Interface for input file readers management.
 *
 * This interface perform the shared tasks among all possible input file readers.
 * Each different file format is handled by its own topology reader.
 */
class TopologyReader : public Object
{

public:
  /**
   * \brief Inner class holding the details about a link between two nodes.
   *
   * The link is not described in terms of technology. Rather it is only stating
   * an association between two nodes. The nodes are characterized also with names
   * reflecting how the nodes are called in the original topology file.
   */
  class Link
  {
public:
    /**
     * \brief Constant iterator to scan the map of link attributes.
     */
    typedef std::map<std::string, std::string>::const_iterator ConstAttributesIterator;

    /**
     * \brief Constructor
     * \param fromPtr Ptr to the node the link is orginating from
     * \param fromName name of the node the link is orginating from
     * \param toPtr Ptr to the node the link is directed to
     * \param toName name of the node the link is directed to
     */
    Link ( Ptr<Node> fromPtr, const std::string &fromName, Ptr<Node> toPtr, const std::string &toName );

    /**
     * \brief Returns a Ptr<Node> to the "from" node of the link
     * \return a Ptr<Node> to the "from" node of the link
     */
    Ptr<Node> GetFromNode (void) const;
    /**
     * \brief Returns the name of the "from" node of the link
     * \return the name of the "from" node of the link
     */
    std::string GetFromNodeName (void) const;
    /**
     * \brief Returns a Ptr<Node> to the "to" node of the link
     * \return a Ptr<Node> to the "to" node of the link
     */
    Ptr<Node> GetToNode (void) const;
    /**
     * \brief Returns the name of the "to" node of the link
     * \return the name of the "to" node of the link
     */
    std::string GetToNodeName (void) const;
    /**
     * \brief Returns the value of a link attribute. The attribute must exist.
     *
     * \param name the name of the attribute
     *
     * \return the value of the attribute
     */
    std::string GetAttribute (const std::string &name) const;
    /**
     * \brief Returns the value of a link attribute.
     * \param name the name of the attribute
     * \param value the value of the attribute
     *
     * \return true if the attribute was defined, false otherwise.
     */
    bool GetAttributeFailSafe (const std::string &name, std::string &value) const;
    /**
     * \brief Sets an arbitrary link attribute.
     * \param name the name of the attribute
     * \param value the value of the attribute
     */
    void SetAttribute (const std::string &name, const std::string &value);
    /**
     * \brief Returns an iterator to the begin of the attributes.
     * \return a const iterator to the first attribute of a link.
     */
    ConstAttributesIterator AttributesBegin (void) const;
    /**
     * \brief Returns an iterator to the end of the attributes.
     * \return a const iterator to the last attribute of a link.
     */
    ConstAttributesIterator AttributesEnd (void) const;

private:
    Link ();
    std::string m_fromName; //!< Name of the node the links originates from
    Ptr< Node > m_fromPtr;  //!< The node the links originates from
    std::string m_toName;   //!< Name of the node the links is directed to
    Ptr< Node > m_toPtr;    //!< The node the links is directed to
    std::map<std::string, std::string> m_linkAttr;  ///< Container of the link attributes (if any)
  };

  /**
   * \brief Constant iterator to the list of the links.
   */
  typedef std::list< Link >::const_iterator ConstLinksIterator;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TopologyReader ();
  virtual ~TopologyReader ();

  /**
   * \brief Main topology reading function.
   *
   * The data is parsed and the results are returned in the passed lists.
   * The rationale behind this choice is to allow non-progressive node IDs
   * in the topology files, as well as to separate the topology
   * reader from the choices about actual IP number assignment and
   * kind of links between nodes.
   *
   * \return the container of the nodes created (or null if there was an error)
   */
  virtual NodeContainer Read (void) = 0;

  /**
   * \brief Sets the input file name.
   * \param fileName the input file name.
   */
  void SetFileName (const std::string &fileName);

  /**
   * \brief Returns the input file name.
   * \return the input file name.
   */
  std::string GetFileName (void) const;

  /**
   * \brief Returns an iterator to the the first link in this block.
   * \return a const iterator to the first link in this block.
   */
  ConstLinksIterator LinksBegin (void) const;

  /**
   * \brief Returns an iterator to the the last link in this block.
   * \return a const iterator to the last link in this block.
   */
  ConstLinksIterator LinksEnd (void) const;

  /**
   * \brief Returns the number of links in this block.
   * \return the number of links in this block.
   */
  int LinksSize (void) const;

  /**
   * \brief Checks if the block contains any links.
   * \return true if there are no links in this block, false otherwise.
   */
  bool LinksEmpty (void) const;

  /**
   * \brief Adds a link to the topology.
   * \param link the link to be added.
   */
  void AddLink (Link link);

private:

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  TopologyReader (const TopologyReader&);
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  TopologyReader& operator= (const TopologyReader&);

  /**
   * the name of the input file
   */
  std::string m_fileName;

  /**
   * the container of the links between the nodes
   */
  std::list<Link> m_linksList;

  // end class TopologyReader
};

// end namespace ns3
};


#endif /* TOPOLOGY_READER_H */
