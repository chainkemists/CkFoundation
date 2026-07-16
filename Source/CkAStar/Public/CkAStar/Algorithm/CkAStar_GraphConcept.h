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

} // namespace ck::astar
