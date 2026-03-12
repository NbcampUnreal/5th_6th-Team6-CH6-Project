#include "LevelAreaNode.h"


IGridNode* LevelAreaNode::GetNeighborPointerGraph(
	int32 Index,
	IGridGraph* Graph) const
{
	if (!Graph) return nullptr;

	if (!Connections.IsValidIndex(Index))
		return nullptr;

	int32 TargetID = Connections[Index].TargetNodeID;

	return Graph->FindNodeByID(TargetID);
}

IGridNode* LevelAreaNode::GetNeighborIndexGraph(
	int32 Index,
	IGridGraph* Graph) const
{
	return GetNeighborPointerGraph(Index, Graph);
}

float LevelAreaNode::GetCostTo(const IGridNode* TargetNode) const
{
	if (!TargetNode) return 0.f;

	return FVector::Dist(RoomCenter, TargetNode->GetWorldLocation());
}

float LevelAreaNode::GetHeuristicCost(const IGridNode* TargetNode) const
{
	if (!TargetNode) return 0.f;

	return FVector::Dist(RoomCenter, TargetNode->GetWorldLocation());
}