#pragma once

#include "CoreMinimal.h"

#include <concepts>
#include <ranges>

// ====================================================================================================================

namespace ck::astar
{

// ====================================================================================================================
// NODE ID CONCEPT — Constrains what can be used as a node identifier in A*
// ====================================================================================================================

template <typename T>
concept AStarNodeId =
	std::copyable<T> &&
	std::equality_comparable<T> &&
	requires(const T& InNode)
	{
		{ GetTypeHash(InNode) } -> std::convertible_to<uint32>;
	};

// ====================================================================================================================
// GRAPH CONCEPT — Constrains what can be used as a graph adapter for A*
// ====================================================================================================================

template <typename T_Graph, typename T_NodeId>
concept AStarGraph =
	AStarNodeId<T_NodeId> &&
	requires(const T_Graph& InGraph, const T_NodeId& InA, const T_NodeId& InB)
	{
		{ InGraph.Neighbors(InA) } -> std::ranges::input_range;
		{ InGraph.Cost(InA, InB) } -> std::convertible_to<float>;
		{ InGraph.Heuristic(InA, InB) } -> std::convertible_to<float>;
		{ InGraph.IsGoal(InA) } -> std::convertible_to<bool>;
	};

// ====================================================================================================================

} // namespace ck::astar
