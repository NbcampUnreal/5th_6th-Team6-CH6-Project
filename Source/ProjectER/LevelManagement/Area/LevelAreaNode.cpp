#include "LevelAreaNode.h"

IGridNode* LevelAreaNode::GetNeighborPointerGraph(
	int32 Index, IGridGraph* Graph) const
{
	if (!Graph) return nullptr;
	if (!NeighborIDs.IsValidIndex(Index)) return nullptr;

	return Graph->FindNodeByID(NeighborIDs[Index]);
}

IGridNode* LevelAreaNode::GetNeighborIndexGraph(
	int32 Index, IGridGraph* Graph) const
{
	return GetNeighborPointerGraph(Index, Graph);
}