#include "core/ShapeDocument.h"

void ShapeDocument::Clear()
{
    m_rootShape.Nullify();
    m_nodes.clear();
}

int ShapeDocument::AddNode(ShapeNode node)
{
    const int id = static_cast<int>(m_nodes.size());
    node.id = id;
    m_nodes.push_back(std::move(node));
    return id;
}

const ShapeNode* ShapeDocument::FindNode(int id) const
{
    if (id < 0 || id >= static_cast<int>(m_nodes.size()))
    {
        return nullptr;
    }
    return &m_nodes[static_cast<size_t>(id)];
}

ShapeNode* ShapeDocument::FindNode(int id)
{
    return const_cast<ShapeNode*>(static_cast<const ShapeDocument*>(this)->FindNode(id));
}
