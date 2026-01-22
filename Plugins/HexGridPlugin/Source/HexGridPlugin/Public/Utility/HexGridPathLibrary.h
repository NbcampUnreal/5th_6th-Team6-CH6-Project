#pragma once

#include "CoreMinimal.h"
#include "Hex/HexAdapter.h"

/**
 * Static utility library for hex grid pathfinding and range calculations
 * - Graph / Adapter must be created externally
 * - Dynamic blocking is handled via CanVisit()
 */

class HEXGRIDPLUGIN_API FHexGridPathLibrary
{
public:
	// ---------------- A* ----------------
	static bool Hex_FindPath(
		UHexGridAdapter& Graph,// PRE-BUILT graph, no more rebuilding path every time when the path finding is used
		const FHexCoord& Start,
		const FHexCoord& End,
		TFunction<bool(const IGridNode*)> CanVisit,
		// out
		TArray<FHexCoord>& OutPath
	);

	// ---------------- Flood Fill ----------------
	static bool FloodFill(
		UHexGridAdapter& Graph,// PRE-BUILT graph
		const FHexCoord& StartHex,
		TFunction<bool(const IGridNode*)> CanVisit,
		// out
		TSet<FHexCoord>& OutArea,
		TArray<FHexCoord>& OutRing
	);
};
