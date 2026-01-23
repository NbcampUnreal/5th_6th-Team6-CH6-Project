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

	// --- 1. Find start / end nodes ---
	FHexNodeAdapter* StartNode = Graph.GetNode(Start);
	FHexNodeAdapter* EndNode   = Graph.GetNode(End);

	if (!StartNode || !EndNode)
	{
		UE_LOG(HexGridUtilityLog, Warning,
			TEXT("Hex_FindPath >> Invalid start or end hex"));
		return false;
	}

	// --- 2. Reset pathfinding state ---
	Graph.ResetAllNodes();

	// --- 3. Run generic A* ---
	TArray<IGridNode*> NodePath;

	const bool bFound =
		FCPU_PathFindingLibrary::FindPath_AStar<IGridNode, UHexGridAdapter>(// fuck, make the grid adapter to be a uobject 
			&Graph,
			StartNode,
			EndNode,
			NodePath,
			[&](IGridNode* Node)
			{
				return CanVisit ? CanVisit(Node) : true;
			});

	if (!bFound || NodePath.IsEmpty())
        return false;

    // 4. Translate node → hex
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

	// 1. Translate hex → node
	FHexNodeAdapter* StartNode = Graph.GetNode(StartHex);
	if (!StartNode)
		return false;

	TQueue<IGridNode*> Queue;
	Queue.Enqueue(StartNode);
	OutArea.Add(StartHex);

	// 2. Generic BFS over IGridNode
	while (!Queue.IsEmpty())
	{
		IGridNode* Current = nullptr;
		Queue.Dequeue(Current);

		const FHexNodeAdapter* CurrentHex =
			static_cast<const FHexNodeAdapter*>(Current);

		for (int32 i = 0; i < Current->GetNumNeighbors(); ++i)
		{
			IGridNode* Neighbor =
				Current->GetNeighborPointerGraph(i, nullptr);

			if (!Neighbor)
				continue;

			const FHexNodeAdapter* NeighborHex =
				static_cast<const FHexNodeAdapter*>(Neighbor);

			const FHexCoord& Coord = NeighborHex->GetCoord();

			if (OutArea.Contains(Coord))
				continue;

			// blocked → ring
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
