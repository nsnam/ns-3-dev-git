/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Faker Moatamri <faker.moatamri@sophia.inria.fr>
 */

#include "model-typeid-creator.h"

namespace ns3
{

ModelTypeidCreator::ModelTypeidCreator()
{
}

void

ModelTypeidCreator::Build(GtkTreeStore* treestore)
{
    m_treestore = treestore;
    m_iters.push_back(nullptr);
    Iterate();
    NS_ASSERT(m_iters.size() == 1);
}

void
ModelTypeidCreator::Add(ModelTypeid* node)
{
    GtkTreeIter* parent = m_iters.back();
    auto current = g_new(GtkTreeIter, 1);
    gtk_tree_store_append(m_treestore, current, parent);
    gtk_tree_store_set(m_treestore, current, COL_TYPEID, node, -1);
    m_iters.push_back(current);
}

void
ModelTypeidCreator::Remove()
{
    GtkTreeIter* iter = m_iters.back();
    g_free(iter);
    m_iters.pop_back();
}

void
ModelTypeidCreator::VisitAttribute(TypeId tid,
                                   std::string name,
                                   std::string defaultValue,
                                   uint32_t index)
{
    auto node = new ModelTypeid();
    node->type = ModelTypeid::NODE_ATTRIBUTE;
    node->tid = tid;
    node->name = name;
    node->defaultValue = defaultValue;
    node->index = index;
    Add(node);
    Remove();
}

void
ModelTypeidCreator::StartVisitTypeId(std::string name)
{
    auto node = new ModelTypeid();
    node->type = ModelTypeid::NODE_TYPEID;
    node->tid = TypeId::LookupByName(name);
    Add(node);
}

void
ModelTypeidCreator::EndVisitTypeId()
{
    Remove();
}
} // end namespace ns3
