#include "CkGroundNav_Layers.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace layers_private
    {
        auto Get_FlatSpanIndex(
            const FCk_GroundNav_SpanField& InSpans,
            const TArray<int32>&           InColumnOffsets,
            const FCk_GroundNav_SpanAddress& InAddress) -> int32
        {
            return InColumnOffsets[InSpans.Get_ColumnIndex(InAddress._X, InAddress._Y)] + InAddress._SpanIndex;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_LayerField::
        Get_OccupancyAt(
            int32 InX,
            int32 InY,
            int32 InLayer) const
        -> int32
    {
        auto Count = 0;

        for (const auto& Layer : Get_Column(InX, InY))
        {
            if (Layer == InLayer)
            { ++Count; }
        }

        return Count;
    }

    auto
        FCk_GroundNav_LayerField::
        Get_AssignedSpanCount() const
        -> int32
    {
        auto Count = 0;

        for (const auto& Column : _Columns)
        {
            for (const auto& Layer : Column)
            {
                if (Layer != kNoLayer)
                { ++Count; }
            }
        }

        return Count;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoFind_ConnectedComponents(
            const FCk_GroundNav_SpanField&       InSpans,
            const FCk_GroundNav_ConnectionField& InConnections,
            TArray<FCk_GroundNav_Component>&     OutComponents,
            int32&                               InOutProbes)
        -> int32
    {
        using namespace layers_private;

        OutComponents.Reset();

        const auto ColumnCount = InSpans._Columns.Num();

        // One flat id per span, so visitation is a bit array rather than a set of triples.
        auto ColumnOffsets = TArray<int32>{};
        ColumnOffsets.Reserve(ColumnCount);

        auto TotalSpans = 0;

        for (const auto& Column : InSpans._Columns)
        {
            ColumnOffsets.Emplace(TotalSpans);
            TotalSpans += Column.Num();
        }

        auto Visited = TBitArray<>{false, TotalSpans};
        auto Frontier = TArray<FCk_GroundNav_SpanAddress>{};

        for (auto Y = 0; Y < InSpans._SizeY; ++Y)
        {
            for (auto X = 0; X < InSpans._SizeX; ++X)
            {
                const auto& SeedColumn = InSpans.Get_Column(X, Y);

                for (auto SeedIndex = 0; SeedIndex < SeedColumn.Num(); ++SeedIndex)
                {
                    ++InOutProbes;

                    if (NOT SeedColumn[SeedIndex]._IsWalkable)
                    { continue; }

                    const auto Seed = FCk_GroundNav_SpanAddress{X, Y, SeedIndex};

                    if (Visited[Get_FlatSpanIndex(InSpans, ColumnOffsets, Seed)])
                    { continue; }

                    auto Component = FCk_GroundNav_Component{};
                    Component._Footprint.Init(false, ColumnCount);

                    Frontier.Reset();
                    Frontier.Emplace(Seed);
                    Visited[Get_FlatSpanIndex(InSpans, ColumnOffsets, Seed)] = true;

                    while (Frontier.Num() > 0)
                    {
                        const auto Current = Frontier.Pop();

                        const auto CurrentColumnIndex = InSpans.Get_ColumnIndex(Current._X, Current._Y);

                        if (Component._Footprint[CurrentColumnIndex])
                        { Component._OverlapsItself = true; }

                        Component._Footprint[CurrentColumnIndex] = true;
                        Component._Spans.Emplace(Current);

                        const auto& Connections = InConnections.Get_Column(Current._X, Current._Y)[Current._SpanIndex];

                        for (auto Direction = 0; Direction < kDirectionCount; ++Direction)
                        {
                            if (NOT Connections.Get_IsConnected(Direction))
                            { continue; }

                            const auto Offset = Get_DirectionOffset(Direction);
                            const auto Neighbour = FCk_GroundNav_SpanAddress{
                                Current._X + Offset.X,
                                Current._Y + Offset.Y,
                                Connections._Neighbours[Direction]};

                            ++InOutProbes;

                            const auto NeighbourFlat = Get_FlatSpanIndex(InSpans, ColumnOffsets, Neighbour);

                            if (Visited[NeighbourFlat])
                            { continue; }

                            Visited[NeighbourFlat] = true;
                            Frontier.Emplace(Neighbour);
                        }
                    }

                    // Scan order within the component, so layer assignment is reproducible.
                    Component._Spans.Sort([](const FCk_GroundNav_SpanAddress& InLeft,
                                             const FCk_GroundNav_SpanAddress& InRight) -> bool
                    {
                        if (InLeft._Y != InRight._Y)
                        { return InLeft._Y < InRight._Y; }

                        if (InLeft._X != InRight._X)
                        { return InLeft._X < InRight._X; }

                        return InLeft._SpanIndex < InRight._SpanIndex;
                    });

                    OutComponents.Emplace(MoveTemp(Component));
                }
            }
        }

        return OutComponents.Num();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoExtract_Layers(
            const FCk_GroundNav_SpanField&       InSpans,
            const FCk_GroundNav_ConnectionField& InConnections,
            FCk_GroundNav_LayerField&            OutLayers)
        -> FCk_GroundNav_BakeStageResult
    {
        auto Result = FCk_GroundNav_BakeStageResult{};

        if (InConnections._SizeX != InSpans._SizeX || InConnections._SizeY != InSpans._SizeY)
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        const auto ColumnCount = InSpans._Columns.Num();

        OutLayers = FCk_GroundNav_LayerField{};
        OutLayers._SizeX = InSpans._SizeX;
        OutLayers._SizeY = InSpans._SizeY;
        OutLayers._Columns.SetNum(ColumnCount);

        for (auto ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
        {
            OutLayers._Columns[ColumnIndex].Init(
                FCk_GroundNav_LayerField::kNoLayer, InSpans._Columns[ColumnIndex].Num());
        }

        auto ProbesSpent = 0;

        auto Components = TArray<FCk_GroundNav_Component>{};
        DoFind_ConnectedComponents(InSpans, InConnections, Components, ProbesSpent);

        auto LayerFootprints = TArray<TBitArray<>>{};

        const auto Get_OpenNewLayer = [&]() -> int32
        {
            LayerFootprints.Emplace(TBitArray<>{false, ColumnCount});
            return LayerFootprints.Num() - 1;
        };

        const auto Do_Place = [&](const FCk_GroundNav_SpanAddress& InAddress, int32 InLayer) -> void
        {
            const auto ColumnIndex = InSpans.Get_ColumnIndex(InAddress._X, InAddress._Y);

            LayerFootprints[InLayer][ColumnIndex] = true;
            OutLayers._Columns[ColumnIndex][InAddress._SpanIndex] = InLayer;
        };

        for (const auto& Component : Components)
        {
            if (NOT Component._OverlapsItself)
            {
                auto TargetLayer = FCk_GroundNav_LayerField::kNoLayer;

                for (auto LayerIndex = 0; LayerIndex < LayerFootprints.Num(); ++LayerIndex)
                {
                    // The test reads the whole layer footprint, so it costs one probe per column.
                    ProbesSpent += ColumnCount;

                    auto Intersection = TBitArray<>{LayerFootprints[LayerIndex]};
                    Intersection.CombineWithBitwiseAND(Component._Footprint, EBitwiseOperatorFlags::MaintainSize);

                    if (Intersection.Contains(true))
                    { continue; }

                    TargetLayer = LayerIndex;
                    break;
                }

                if (TargetLayer == FCk_GroundNav_LayerField::kNoLayer)
                { TargetLayer = Get_OpenNewLayer(); }

                for (const auto& Address : Component._Spans)
                { Do_Place(Address, TargetLayer); }

                continue;
            }

            // Self-overlapping: no single layer can hold it, so each span takes the lowest layer that
            // is free at its own column, opening one when every existing layer is taken.
            for (const auto& Address : Component._Spans)
            {
                const auto ColumnIndex = InSpans.Get_ColumnIndex(Address._X, Address._Y);
                auto TargetLayer = FCk_GroundNav_LayerField::kNoLayer;

                for (auto LayerIndex = 0; LayerIndex < LayerFootprints.Num(); ++LayerIndex)
                {
                    ++ProbesSpent;

                    if (LayerFootprints[LayerIndex][ColumnIndex])
                    { continue; }

                    TargetLayer = LayerIndex;
                    break;
                }

                if (TargetLayer == FCk_GroundNav_LayerField::kNoLayer)
                { TargetLayer = Get_OpenNewLayer(); }

                Do_Place(Address, TargetLayer);
            }
        }

        OutLayers._LayerCount = LayerFootprints.Num();

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);

        // A probe here is one span read in the flood fill — a seed candidacy test or a neighbour visit
        // — or one column of a layer footprint read while placing a component.
        Result.Set_ProbesSpent(ProbesSpent);

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
