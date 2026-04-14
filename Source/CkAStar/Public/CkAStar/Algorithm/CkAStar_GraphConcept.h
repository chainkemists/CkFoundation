#pragma once

#include "CoreMinimal.h"

#include <concepts>
#include <type_traits>

// ====================================================================================================================

namespace ck::astar
{

// ====================================================================================================================
// NODE ID CONCEPT — Constrains what can be used as a node identifier in A*
// ====================================================================================================================

template <typename T>
concept AStarNodeId =
	std::copyable<T> &&
	std::equality_comparable<T>;

// ====================================================================================================================
// GRAPH CONCEPT — Constrains what can be used as a graph adapter for A*
// ====================================================================================================================

template <typename T_Graph, typename T_NodeId>
concept AStarGraph =
	AStarNodeId<T_NodeId> &&
	requires(const T_Graph& InGraph, const T_NodeId& InA, const T_NodeId& InB)
	{
		{ *InGraph.Neighbors(InA).begin() } -> std::convertible_to<T_NodeId>;
		{ InGraph.Neighbors(InA).end() };
		{ InGraph.Cost(InA, InB) } -> std::convertible_to<float>;
		{ InGraph.Heuristic(InA, InB) } -> std::convertible_to<float>;
		{ InGraph.IsGoal(InA) } -> std::convertible_to<bool>;
	};

// ====================================================================================================================

} // namespace ck::astar
