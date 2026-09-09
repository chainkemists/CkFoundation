#pragma once

#include "CoreMinimal.h"

#include <concepts>
#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::astar
{

// --------------------------------------------------------------------------------------------------------------------

template <typename T>
concept AStarNodeId =
	std::copyable<T> &&
	std::equality_comparable<T>;

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Graph, typename T_NodeId>
concept AStarGraph =
	AStarNodeId<T_NodeId> &&
	requires(const T_Graph& InGraph, const T_NodeId& InA, const T_NodeId& InB)
	{
		InGraph.Neighbors(InA);
		{ InGraph.Cost(InA, InB) } -> std::convertible_to<float>;
		{ InGraph.Heuristic(InA, InB) } -> std::convertible_to<float>;
		{ InGraph.IsGoal(InA) } -> std::convertible_to<bool>;
	};

// --------------------------------------------------------------------------------------------------------------------

// Optional graph hook used only to order otherwise exactly equal FScore entries. Smaller values win the tie.
// Graphs without TieBreak remain valid A* graphs and use a zero secondary score.
template <typename T_Graph, typename T_NodeId>
concept AStarGraphWithTieBreak =
	AStarGraph<T_Graph, T_NodeId> &&
	requires(const T_Graph& InGraph, const T_NodeId& InNode, const T_NodeId& InGoal)
	{
		{ InGraph.TieBreak(InNode, InGoal) } -> std::convertible_to<float>;
	};

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck::astar
