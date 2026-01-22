// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Templates/Function.h"// for TFunction


/*
 * This is for CPU-Based Path finding function library
*/


//Library(static Cpp)

class FCPU_PathFindingLibrary 
{
public:
	//Generic Helpers
	//Template based
	template<typename NodeType>
		static float GetDistance(NodeType* NodeA, NodeType* NodeB);


	template<typename NodeType>
	static void ResetNodeFlags(TArray<NodeType*>& Nodes, uint8 ResetMask = NodeType::Blocked);

	

// ---------------- A* Template ----------------
	template<typename NodeType, typename GraphType>
	static bool FindPath_AStar(
		GraphType* Graph,
		NodeType* StartNode,
		NodeType* GoalNode,
		TArray<NodeType*>& OutPath,
		TFunction<bool(NodeType*)> CanVisit = nullptr);

// ---------------- Dijkstra Template ----------------
	template<typename NodeType, typename GraphType>
	static bool FindPath_Dijkstra(
		GraphType* Graph,
		NodeType* StartNode,
		NodeType* GoalNode,
		TArray<NodeType*>& OutPath,
		TFunction<bool(NodeType*)> CanVisit = nullptr);
	
// ---------------- Dijkstra Template ----------------
	template<typename NodeType, typename GraphType>
	static bool BFS(// not just for the pathfinding, but for other purposes, like area defining, floodfill
		GraphType* Graph,
		NodeType* StartNode,
		TArray<NodeType*>& OutConnectedNodes,
		TFunction<bool(NodeType*)> CanVisit = nullptr);

};


//add this for template body Fuck yeah for inl !!!!!
#if CPP
#include "PathFindingLibrary/Private/CPU/CPU_PathFindingLibrary.inl"
#endif
