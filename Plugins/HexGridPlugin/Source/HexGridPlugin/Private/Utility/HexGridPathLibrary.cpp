#include "Utility/HexGridPathLibrary.h"

#include "Utility/UtilityLog.h"
#include "CPU/CPU_PathFindingLibrary.h"


//A* path finding
bool FHexGridPathLibrary::Hex_FindPath(
	UHexGridAdapter& Graph,
	const FHexCoord& Start,
	const FHexCoord& End,
	TFunction<bool(const IGridNode*)> CanVisit,
	TArray<FHexCoord>& OutPath)
{
	OutPath.Empty();

	FHexNodeAdapter* StartNode = Graph.GetNode(Start);
	FHexNodeAdapter* EndNode   = Graph.GetNode(End);

	if (!StartNode || !EndNode)
	{
		UE_LOG(HexGridUtilityLog, Warning,
			TEXT("Hex_FindPath >> Invalid start or end hex"));
		return false;
	}

	Graph.ResetAllNodes();

	TArray<IGridNode*> NodePath;

	// GraphType is UHexGridAdapter which now implements IGridGraph — static_assert passes cleanly
	const bool bFound = FCPU_PathFindingLibrary::FindPath_AStar<IGridNode, UHexGridAdapter>(
		&Graph,
		StartNode,
		EndNode,
		NodePath,
		[&](IGridNode* Node) { return CanVisit ? CanVisit(Node) : true; });

	if (!bFound || NodePath.IsEmpty())
		return false;

	OutPath.Reserve(NodePath.Num());
	for (IGridNode* Node : NodePath)
	{
		const FHexNodeAdapter* HexNode = static_cast<const FHexNodeAdapter*>(Node);
		OutPath.Add(HexNode->GetCoord());
	}

	return true;
}

// Flood fill
bool FHexGridPathLibrary::FloodFill(
	UHexGridAdapter& Graph,
	const FHexCoord& StartHex,
	TFunction<bool(const IGridNode*)> CanVisit,
	TSet<FHexCoord>& OutArea,
	TArray<FHexCoord>& OutRing)
{
	OutArea.Reset();
	OutRing.Reset();

	FHexNodeAdapter* StartNode = Graph.GetNode(StartHex);
	if (!StartNode) return false;

	TQueue<IGridNode*> Queue;
	Queue.Enqueue(StartNode);
	OutArea.Add(StartHex);

	while (!Queue.IsEmpty())
	{
		IGridNode* Current = nullptr;
		Queue.Dequeue(Current);

		const FHexNodeAdapter* CurrentHex = static_cast<const FHexNodeAdapter*>(Current);

		for (int32 i = 0; i < Current->GetNumNeighbors(); ++i)
		{
			// Pass &Graph instead of nullptr — fixes the silent failure bug
			IGridNode* Neighbor = Current->GetNeighborPointerGraph(i, &Graph);
			if (!Neighbor) continue;

			const FHexNodeAdapter* NeighborHex = static_cast<const FHexNodeAdapter*>(Neighbor);
			const FHexCoord& Coord = NeighborHex->GetCoord();

			if (OutArea.Contains(Coord)) continue;

			if (CanVisit && !CanVisit(Neighbor))
			{
				OutRing.Add(CurrentHex->GetCoord());
				continue;
			}

			OutArea.Add(Coord);
			Queue.Enqueue(Neighbor);
		}
	}

	return OutArea.Num() > 0;
}
