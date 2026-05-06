#pragma once

#include "core/ShapeNode.h"

#include <TopoDS_Shape.hxx>

#include <vector>

class ShapeDocument
{
public:
    void Clear();

    int AddNode(ShapeNode node);

    const ShapeNode* FindNode(int id) const;
    ShapeNode* FindNode(int id);

    const std::vector<ShapeNode>& Nodes() const { return m_nodes; }

    TopoDS_Shape RootShape() const { return m_rootShape; }
    void SetRootShape(const TopoDS_Shape& shape) { m_rootShape = shape; }

private:
    TopoDS_Shape m_rootShape;
    std::vector<ShapeNode> m_nodes;
};
