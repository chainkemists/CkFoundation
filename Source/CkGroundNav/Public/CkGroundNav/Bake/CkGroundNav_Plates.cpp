#include "CkGroundNav_Plates.h"

#include "CkGroundNav/Bake/CkGroundNav_MarkupMask.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace plates_private
    {
        struct FSeedPlane
        {
            FVector _Normal = FVector::UpVector;
            double _OriginX = 0.0;
            double _OriginY = 0.0;
            double _OriginZ = 0.0;
        };

        /** Height the seed's plane predicts at a cell centre. */
        auto Get_PlaneHeightAt(
            const FSeedPlane& InPlane,
            double            InCellX,
            double            InCellY) -> double
        {
            const auto DeltaX = InCellX - InPlane._OriginX;
            const auto DeltaY = InCellY - InPlane._OriginY;

            // A walkable normal is never near-horizontal — the slope filter rejected those long
            // before a plate could seed on one — so this division is safe by construction.
            return InPlane._OriginZ -
                (((InPlane._Normal.X * DeltaX) + (InPlane._Normal.Y * DeltaY)) / InPlane._Normal.Z);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_PlateField::
        Get_PlateIndexAt(
            int32 InX,
            int32 InY,
            int32 InLayer) const
        -> int32
    {
        const auto IsValidCell = InX >= 0 && InY >= 0 && InLayer >= 0 &&
                                 InX < _SizeX && InY < _SizeY && InLayer < _LayerCount;

        return IsValidCell
            ? _CellToPlate[(InLayer * _SizeX * _SizeY) + (InY * _SizeX) + InX]
            : FCk_GroundNav_Plate::kNoPlate;
    }

    auto
        FCk_GroundNav_PlateField::
        Get_AreaPolicy(
            int32 InIndex) const
        -> const FGameplayTagContainer&
    {
        static const auto NoPolicy = FGameplayTagContainer{};

        return _AreaPolicies.IsValidIndex(InIndex) ? _AreaPolicies[InIndex] : NoPolicy;
    }

    auto
        FCk_GroundNav_PlateField::
        Get_MaxPlaneResidualUu() const
        -> float
    {
        auto Max = 0.0f;

        for (const auto& Plate : _Plates)
        { Max = FMath::Max(Max, Plate._MaxPlaneResidualUu); }

        return Max;
    }

    auto
        FCk_GroundNav_PlateField::
        Get_MaxHeightRangeUu() const
        -> float
    {
        auto Max = 0.0f;

        for (const auto& Plate : _Plates)
        { Max = FMath::Max(Max, Plate._HeightRangeUu); }

        return Max;
    }

    auto
        FCk_GroundNav_PlateField::
        Get_CollapseRatio() const
        -> float
    {
        if (_Plates.IsEmpty())
        { return 0.0f; }

        auto CoveredCells = 0;

        for (const auto& Plate : _Plates)
        { CoveredCells += Plate.Get_CellCount(); }

        return static_cast<float>(CoveredCells) / static_cast<float>(_Plates.Num());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_CellSurface(
            const FCk_GroundNav_SpanField&  InSpans,
            const FCk_GroundNav_LayerField& InLayers,
            int32                           InX,
            int32                           InY,
            int32                           InLayer,
            float&                          OutTopZ,
            FVector&                        OutNormal)
        -> bool
    {
        if (NOT InSpans.Get_IsValidColumn(InX, InY))
        { return false; }

        const auto& LayerColumn = InLayers.Get_Column(InX, InY);
        const auto& SpanColumn = InSpans.Get_Column(InX, InY);

        for (auto Index = 0; Index < LayerColumn.Num(); ++Index)
        {
            if (LayerColumn[Index] != InLayer)
            { continue; }

            OutTopZ = SpanColumn[Index]._MaxZ;
            OutNormal = SpanColumn[Index]._Normal.Get_Normal();

            return true;
        }

        return false;
    }

    auto
        Get_SpanIndexForLayer(
            const FCk_GroundNav_LayerField& InLayers,
            int32                           InX,
            int32                           InY,
            int32                           InLayer) -> int32
    {
        const auto& LayerColumn = InLayers.Get_Column(InX, InY);

        for (auto SpanIndex = 0; SpanIndex < LayerColumn.Num(); ++SpanIndex)
        {
            if (LayerColumn[SpanIndex] == InLayer)
            { return SpanIndex; }
        }

        return INDEX_NONE;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoDecompose_Plates(
            const FCk_GroundNav_SpanField&       InSpans,
            const FCk_GroundNav_LayerField&      InLayers,
            const FCk_GroundNav_ConnectionField& InConnections,
            const FCk_GroundNav_MergeTunables&   InTunables,
            FCk_GroundNav_PlateField&            OutPlates,
            TConstArrayView<int32>               InCellPolicy)
        -> FCk_GroundNav_BakeStageResult
    {
        using namespace plates_private;

        auto Result = FCk_GroundNav_BakeStageResult{};

        const auto TunablesAreValid =
            InTunables.Get_PlaneFitToleranceUu() >= 0.0f &&
            InTunables.Get_NormalConeDegrees() >= 0.0f &&
            InTunables.Get_NormalConeDegrees() <= 90.0f;

        const auto DimensionsArePositive = InLayers._SizeX > 0 && InLayers._SizeY > 0;
        const auto ExpectedColumnCount64 = static_cast<int64>(InLayers._SizeX) * InLayers._SizeY;
        const auto ColumnCountIsRepresentable = ExpectedColumnCount64 > 0 &&
            ExpectedColumnCount64 <= TNumericLimits<int32>::Max();
        const auto ColumnCount = ColumnCountIsRepresentable
            ? static_cast<int32>(ExpectedColumnCount64)
            : 0;
        const auto ExpectedPlateCellCount64 = ColumnCountIsRepresentable && InLayers._LayerCount >= 0
            ? ExpectedColumnCount64 * InLayers._LayerCount
            : -1;
        const auto PlateCellCountIsRepresentable =
            ExpectedPlateCellCount64 >= 0 && ExpectedPlateCellCount64 <= TNumericLimits<int32>::Max();
        const auto DimensionsMatch =
            InSpans._SizeX == InLayers._SizeX && InSpans._SizeY == InLayers._SizeY &&
            InConnections._SizeX == InLayers._SizeX && InConnections._SizeY == InLayers._SizeY;
        const auto CellPolicyIsWellFormed = InCellPolicy.IsEmpty() ||
            (PlateCellCountIsRepresentable &&
                static_cast<int64>(InCellPolicy.Num()) == ExpectedPlateCellCount64);
        auto ColumnsAreWellFormed = DimensionsArePositive && ColumnCountIsRepresentable && DimensionsMatch &&
            InSpans._Columns.Num() == ColumnCount && InLayers._Columns.Num() == ColumnCount &&
            InConnections._Columns.Num() == ColumnCount;

        if (ColumnsAreWellFormed)
        {
            for (auto ColumnIndex = 0; ColumnIndex < ColumnCount && ColumnsAreWellFormed; ++ColumnIndex)
            {
                const auto SpanCount = InSpans._Columns[ColumnIndex].Num();
                ColumnsAreWellFormed = InLayers._Columns[ColumnIndex].Num() == SpanCount &&
                    InConnections._Columns[ColumnIndex].Num() == SpanCount;

                for (auto SpanIndex = 0; SpanIndex < SpanCount && ColumnsAreWellFormed; ++SpanIndex)
                {
                    const auto X = ColumnIndex % InLayers._SizeX;
                    const auto Y = ColumnIndex / InLayers._SizeX;
                    const auto& Connections = InConnections._Columns[ColumnIndex][SpanIndex];

                    for (auto Direction = 0; Direction < kDirectionCount; ++Direction)
                    {
                        const auto NeighbourSpan = Connections._Neighbours[Direction];

                        if (NeighbourSpan == FCk_GroundNav_SpanConnections::kNoConnection)
                        { continue; }

                        const auto Offset = Get_DirectionOffset(Direction);
                        const auto NeighbourX = X + Offset.X;
                        const auto NeighbourY = Y + Offset.Y;

                        if (NeighbourSpan < 0 || NeighbourX < 0 || NeighbourY < 0 ||
                            NeighbourX >= InLayers._SizeX || NeighbourY >= InLayers._SizeY ||
                            NeighbourSpan >= InSpans.Get_Column(NeighbourX, NeighbourY).Num())
                        {
                            ColumnsAreWellFormed = false;
                            break;
                        }
                    }
                }
            }
        }

        if (NOT TunablesAreValid || NOT PlateCellCountIsRepresentable || NOT CellPolicyIsWellFormed ||
            NOT ColumnsAreWellFormed)
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        const auto SizeX = InLayers._SizeX;
        const auto SizeY = InLayers._SizeY;
        const auto CellCount = ColumnCount;
        const auto CellSize = static_cast<double>(InSpans._CellSizeUu);

        OutPlates = FCk_GroundNav_PlateField{};
        OutPlates._SizeX = SizeX;
        OutPlates._SizeY = SizeY;
        OutPlates._LayerCount = InLayers._LayerCount;
        OutPlates._CellToPlate.Init(
            FCk_GroundNav_Plate::kNoPlate, static_cast<int32>(ExpectedPlateCellCount64));

        const auto Tolerance = static_cast<double>(InTunables.Get_PlaneFitToleranceUu());

        // Quantized normals mean two mathematically identical surfaces can differ in their last bits,
        // so a zero-degree cone still has to admit an exact match.
        const auto MinNormalDot =
            FMath::Cos(FMath::DegreesToRadians(InTunables.Get_NormalConeDegrees())) - UE_KINDA_SMALL_NUMBER;

        auto ProbesSpent = 0;

        for (auto LayerIndex = 0; LayerIndex < InLayers._LayerCount; ++LayerIndex)
        {
            const auto PlaneOffset = LayerIndex * CellCount;

            const auto Get_CellPolicy = [&](int32 InX, int32 InY) -> int32
            {
                return InCellPolicy.IsEmpty()
                    ? INDEX_NONE
                    : InCellPolicy[PlaneOffset + (InY * SizeX) + InX];
            };

            const auto Get_IsReciprocallyConnected = [&](int32 InX, int32 InY, int32 InNeighbourX,
                int32 InNeighbourY, int32 InDirection) -> bool
            {
                const auto SpanIndex = Get_SpanIndexForLayer(InLayers, InX, InY, LayerIndex);
                const auto NeighbourSpanIndex = Get_SpanIndexForLayer(
                    InLayers, InNeighbourX, InNeighbourY, LayerIndex);

                if (SpanIndex == INDEX_NONE || NeighbourSpanIndex == INDEX_NONE)
                { return false; }

                const auto& Connections = InConnections.Get_Column(InX, InY);
                const auto& NeighbourConnections = InConnections.Get_Column(InNeighbourX, InNeighbourY);

                return Connections[SpanIndex]._Neighbours[InDirection] == NeighbourSpanIndex &&
                    NeighbourConnections[NeighbourSpanIndex]._Neighbours[Get_OppositeDirection(InDirection)] == SpanIndex;
            };

            const auto Get_IsMergeable = [&](int32 InX, int32 InY, int32 InNeighbourX, int32 InNeighbourY,
                int32 InDirection, const FSeedPlane& InSeed, int32 InSeedPolicy) -> bool
            {
                ++ProbesSpent;

                if (OutPlates._CellToPlate[PlaneOffset + (InY * SizeX) + InX] != FCk_GroundNav_Plate::kNoPlate)
                { return false; }

                if (Get_CellPolicy(InX, InY) != InSeedPolicy)
                { return false; }

                if (NOT Get_IsReciprocallyConnected(InX, InY, InNeighbourX, InNeighbourY, InDirection))
                { return false; }

                auto TopZ = 0.0f;
                auto Normal = FVector::UpVector;

                if (NOT Get_CellSurface(InSpans, InLayers, InX, InY, LayerIndex, TopZ, Normal))
                { return false; }

                if (FVector::DotProduct(Normal, InSeed._Normal) < MinNormalDot)
                { return false; }

                const auto PlaneZ = Get_PlaneHeightAt(InSeed,
                    (static_cast<double>(InX) + 0.5) * CellSize,
                    (static_cast<double>(InY) + 0.5) * CellSize);

                return FMath::Abs(static_cast<double>(TopZ) - PlaneZ) <= Tolerance;
            };

            for (auto Y = 0; Y < SizeY; ++Y)
            {
                for (auto X = 0; X < SizeX; ++X)
                {
                    ++ProbesSpent;

                    if (OutPlates._CellToPlate[PlaneOffset + (Y * SizeX) + X] != FCk_GroundNav_Plate::kNoPlate)
                    { continue; }

                    auto SeedZ = 0.0f;
                    auto SeedNormal = FVector::UpVector;

                    if (NOT Get_CellSurface(InSpans, InLayers, X, Y, LayerIndex, SeedZ, SeedNormal))
                    { continue; }

                    const auto SeedPolicy = Get_CellPolicy(X, Y);

                    auto Seed = FSeedPlane{};
                    Seed._Normal = SeedNormal;
                    Seed._OriginX = (static_cast<double>(X) + 0.5) * CellSize;
                    Seed._OriginY = (static_cast<double>(Y) + 0.5) * CellSize;
                    Seed._OriginZ = static_cast<double>(SeedZ);

                    auto MaxX = X;

                    while (MaxX + 1 < SizeX && Get_IsMergeable(MaxX + 1, Y, MaxX, Y, 2, Seed, SeedPolicy))
                    { ++MaxX; }

                    auto MaxY = Y;

                    for (auto CandidateY = Y + 1; CandidateY < SizeY; ++CandidateY)
                    {
                        auto WholeRowJoins = true;

                        for (auto CandidateX = X; CandidateX <= MaxX && WholeRowJoins; ++CandidateX)
                        {
                            WholeRowJoins = Get_IsMergeable(
                                CandidateX, CandidateY, CandidateX, CandidateY - 1, 3, Seed, SeedPolicy);

                            if (WholeRowJoins && CandidateX > X)
                            {
                                WholeRowJoins = Get_IsReciprocallyConnected(
                                    CandidateX, CandidateY, CandidateX - 1, CandidateY, 2);
                            }
                        }

                        if (NOT WholeRowJoins)
                        { break; }

                        MaxY = CandidateY;
                    }

                    auto Plate = FCk_GroundNav_Plate{};
                    Plate._LayerIndex = LayerIndex;
                    Plate._MinX = X;
                    Plate._MinY = Y;
                    Plate._MaxX = MaxX;
                    Plate._MaxY = MaxY;

                    const auto PlateIndex = OutPlates._Plates.Num();

                    auto LowestZ = TNumericLimits<double>::Max();
                    auto HighestZ = TNumericLimits<double>::Lowest();
                    auto SummedOffset = 0.0;
                    auto MemberCount = 0;

                    for (auto MemberY = Y; MemberY <= MaxY; ++MemberY)
                    {
                        for (auto MemberX = X; MemberX <= MaxX; ++MemberX)
                        {
                            ++ProbesSpent;

                            OutPlates._CellToPlate[PlaneOffset + (MemberY * SizeX) + MemberX] = PlateIndex;

                            auto TopZ = 0.0f;
                            auto Normal = FVector::UpVector;
                            Get_CellSurface(InSpans, InLayers, MemberX, MemberY, LayerIndex, TopZ, Normal);

                            const auto PlaneZ = Get_PlaneHeightAt(Seed,
                                (static_cast<double>(MemberX) + 0.5) * CellSize,
                                (static_cast<double>(MemberY) + 0.5) * CellSize);

                            LowestZ = FMath::Min(LowestZ, static_cast<double>(TopZ));
                            HighestZ = FMath::Max(HighestZ, static_cast<double>(TopZ));

                            SummedOffset += static_cast<double>(TopZ) - PlaneZ;
                            ++MemberCount;
                        }
                    }

                    // Re-centre the plane on the plate's mean before measuring, so the residual
                    // describes the plate rather than how far the seed happened to sit from it.
                    const auto MeanOffset = SummedOffset / static_cast<double>(MemberCount);
                    auto MaxResidual = 0.0;

                    for (auto MemberY = Y; MemberY <= MaxY; ++MemberY)
                    {
                        for (auto MemberX = X; MemberX <= MaxX; ++MemberX)
                        {
                            ++ProbesSpent;

                            auto TopZ = 0.0f;
                            auto Normal = FVector::UpVector;
                            Get_CellSurface(InSpans, InLayers, MemberX, MemberY, LayerIndex, TopZ, Normal);

                            const auto PlaneZ = Get_PlaneHeightAt(Seed,
                                (static_cast<double>(MemberX) + 0.5) * CellSize,
                                (static_cast<double>(MemberY) + 0.5) * CellSize) + MeanOffset;

                            MaxResidual = FMath::Max(MaxResidual,
                                FMath::Abs(static_cast<double>(TopZ) - PlaneZ));
                        }
                    }

                    Plate._MaxPlaneResidualUu = static_cast<float>(MaxResidual);
                    Plate._HeightRangeUu = static_cast<float>(HighestZ - LowestZ);

                    OutPlates._Plates.Emplace(Plate);
                }
            }
        }

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);

        // A probe here is one cell surface read: a seed candidacy test, a mergeability test while the
        // rectangle grows (the row that fails to join included), a member assignment, or a member's
        // re-measure against the re-centred plane. Connection admission reads are not probes.
        Result.Set_ProbesSpent(ProbesSpent);

        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Stamp_PlateCostPolicies(
            const FCk_GroundNav_PlateLattice&           InLattice,
            TConstArrayView<FCk_GroundNav_MarkupRecord> InMarkups,
            FCk_GroundNav_PlateField&                   InOutPlates)
        -> void
    {
        InOutPlates._AreaPolicies.Reset();

        for (auto& Plate : InOutPlates._Plates)
        {
            Plate._AreaPolicyIndex = INDEX_NONE;
            Plate._CostMultiplier = 1.0f;
        }

        if (NOT InLattice.Get_IsValid())
        { return; }

        for (auto PlateIndex = 0; PlateIndex < InOutPlates._Plates.Num(); ++PlateIndex)
        {
            auto& Plate = InOutPlates._Plates[PlateIndex];

            auto Policy = FGameplayTagContainer{};
            auto Multiplier = TOptional<float>{};

            for (const auto& Markup : InMarkups)
            {
                const auto MarkupApplies = Markup.Get_Enable() == ECk_EnableDisable::Enable &&
                                           Markup.Get_Kind() == ECk_GroundNav_MarkupKind::Cost;

                if (NOT MarkupApplies)
                { continue; }

                const auto CellRect = Get_MarkupCellRect(
                    Markup, InLattice._OriginXY, InLattice._CellSizeUu, InLattice._SizeX, InLattice._SizeY);

                if (NOT CellRect.IsSet())
                { continue; }

                const auto MinX = FMath::Max(CellRect->_MinX, Plate._MinX);
                const auto MinY = FMath::Max(CellRect->_MinY, Plate._MinY);
                const auto MaxX = FMath::Min(CellRect->_MaxX, Plate._MaxX);
                const auto MaxY = FMath::Min(CellRect->_MaxY, Plate._MaxY);

                auto CoversAnyCell = false;

                for (auto Y = MinY; Y <= MaxY && NOT CoversAnyCell; ++Y)
                {
                    for (auto X = MinX; X <= MaxX && NOT CoversAnyCell; ++X)
                    {
                        if (InOutPlates.Get_PlateIndexAt(X, Y, Plate._LayerIndex) != PlateIndex)
                        { continue; }

                        CoversAnyCell = Get_IsMarkupCoveringCell(
                            Markup,
                            InLattice.Get_CellMinXY(X, Y),
                            InLattice._CellSizeUu,
                            InLattice.Get_SurfaceZ(X, Y, Plate._LayerIndex));
                    }
                }

                if (NOT CoversAnyCell)
                { continue; }

                Policy.AddTag(Markup.Get_AreaTag());

                Multiplier = Multiplier.IsSet()
                    ? FMath::Max(*Multiplier, Markup.Get_CostMultiplier())
                    : Markup.Get_CostMultiplier();
            }

            Plate._CostMultiplier = Multiplier.Get(1.0f);

            if (Policy.IsEmpty())
            { continue; }

            const auto Interned = InOutPlates._AreaPolicies.IndexOfByPredicate(
                [&](const FGameplayTagContainer& InCandidate) -> bool { return InCandidate == Policy; });

            Plate._AreaPolicyIndex = Interned != INDEX_NONE
                ? Interned
                : InOutPlates._AreaPolicies.Emplace(Policy);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
