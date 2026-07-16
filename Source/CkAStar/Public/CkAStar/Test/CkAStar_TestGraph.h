#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::astar
{

// --------------------------------------------------------------------------------------------------------------------

// Each cell is identified by int32 index = Y * Width + X.
// 4-directional movement (up/down/left/right). Cells can be blocked.
// Cost = 1.0f per step. Heuristic = Manhattan distance.

struct FGridGraph
{
public:
	FGridGraph() = default;

	FGridGraph(int32 InWidth, int32 InHeight, int32 InGoalNode)
		: _Width{InWidth}
		, _Height{InHeight}
		, _GoalNode{InGoalNode}
	{
		_BlockedCells.Reserve(16);
	}

	// ----------------------------------------------------------------------------------------------------------------

	auto
	Neighbors(int32 InNode) const -> TArray<int32>
	{
		auto Result = TArray<int32>{};
		Result.Reserve(4);

		const auto X = InNode % _Width;
		const auto Y = InNode / _Width;

		auto TryAdd = [&](int32 InNx, int32 InNy)
		{
			if (InNx >= 0 && InNx < _Width && InNy >= 0 && InNy < _Height)
			{
				const auto NeighborIndex = InNy * _Width + InNx;
				if (NOT _BlockedCells.Contains(NeighborIndex))
				{
					Result.Add(NeighborIndex);
				}
			}
		};

		TryAdd(X + 1, Y);
		TryAdd(X - 1, Y);
		TryAdd(X, Y + 1);
		TryAdd(X, Y - 1);

		return Result;
	}

	auto
	Cost(int32 InFrom, int32 InTo) const -> float
	{
		return 1.0f;
	}

	auto
	Heuristic(int32 InCurrent, int32 InGoal) const -> float
	{
		const auto Cx = InCurrent % _Width;
		const auto Cy = InCurrent / _Width;
		const auto Gx = InGoal % _Width;
		const auto Gy = InGoal / _Width;
		return static_cast<float>(FMath::Abs(Cx - Gx) + FMath::Abs(Cy - Gy));
	}

	auto
	IsGoal(int32 InNode) const -> bool
	{
		return InNode == _GoalNode;
	}

	// ----------------------------------------------------------------------------------------------------------------

	auto
	BlockCell(int32 InCellIndex) -> void
	{
		_BlockedCells.Add(InCellIndex);
	}

	auto
	UnblockCell(int32 InCellIndex) -> void
	{
		_BlockedCells.Remove(InCellIndex);
	}

	auto
	IsCellBlocked(int32 InCellIndex) const -> bool
	{
		return _BlockedCells.Contains(InCellIndex);
	}

	// ----------------------------------------------------------------------------------------------------------------

	auto
	Get_Width() const -> int32 { return _Width; }

	auto
	Get_Height() const -> int32 { return _Height; }

	auto
	Get_GoalNode() const -> int32 { return _GoalNode; }

	auto
	Set_GoalNode(int32 InGoalNode) -> void { _GoalNode = InGoalNode; }

	auto
	CellToXY(int32 InCell, int32& OutX, int32& OutY) const -> void
	{
		OutX = InCell % _Width;
		OutY = InCell / _Width;
	}

	auto
	XYToCell(int32 InX, int32 InY) const -> int32
	{
		return InY * _Width + InX;
	}

private:
	int32 _Width = 0;
	int32 _Height = 0;
	int32 _GoalNode = 0;
	TSet<int32> _BlockedCells;
};

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck::astar
