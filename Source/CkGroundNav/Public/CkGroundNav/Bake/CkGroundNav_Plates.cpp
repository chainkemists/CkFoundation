#include "CkGroundNav_Plates.h"

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

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoDecompose_Plates(
            const FCk_GroundNav_SpanField&     InSpans,
            const FCk_GroundNav_LayerField&    InLayers,
            const FCk_GroundNav_MergeTunables& InTunables,
            FCk_GroundNav_PlateField&          OutPlates)
        -> FCk_GroundNav_BakeStageResult
    {
        using namespace plates_private;

        auto Result = FCk_GroundNav_BakeStageResult{};

        const auto TunablesAreValid =
            InTunables.Get_PlaneFitToleranceUu() >= 0.0f &&
            InTunables.Get_NormalConeDegrees() >= 0.0f &&
            InTunables.Get_NormalConeDegrees() <= 90.0f;

        if (NOT TunablesAreValid || InLayers._SizeX <= 0 || InLayers._SizeY <= 0)
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        const auto SizeX = InLayers._SizeX;
        const auto SizeY = InLayers._SizeY;
        const auto CellCount = SizeX * SizeY;
        const auto CellSize = static_cast<double>(InSpans._CellSizeUu);

        OutPlates = FCk_GroundNav_PlateField{};
        OutPlates._SizeX = SizeX;
        OutPlates._SizeY = SizeY;
        OutPlates._LayerCount = InLayers._LayerCount;
        OutPlates._CellToPlate.Init(FCk_GroundNav_Plate::kNoPlate, CellCount * InLayers._LayerCount);

        const auto Tolerance = static_cast<double>(InTunables.Get_PlaneFitToleranceUu());

        // Quantized normals mean two mathematically identical surfaces can differ in their last bits,
        // so a zero-degree cone still has to admit an exact match.
        const auto MinNormalDot =
            FMath::Cos(FMath::DegreesToRadians(InTunables.Get_NormalConeDegrees())) - UE_KINDA_SMALL_NUMBER;

        for (auto LayerIndex = 0; LayerIndex < InLayers._LayerCount; ++LayerIndex)
        {
            const auto PlaneOffset = LayerIndex * CellCount;

            const auto Get_IsMergeable = [&](int32 InX, int32 InY, const FSeedPlane& InSeed) -> bool
            {
                if (OutPlates._CellToPlate[PlaneOffset + (InY * SizeX) + InX] != FCk_GroundNav_Plate::kNoPlate)
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
                    if (OutPlates._CellToPlate[PlaneOffset + (Y * SizeX) + X] != FCk_GroundNav_Plate::kNoPlate)
                    { continue; }

                    auto SeedZ = 0.0f;
                    auto SeedNormal = FVector::UpVector;

                    if (NOT Get_CellSurface(InSpans, InLayers, X, Y, LayerIndex, SeedZ, SeedNormal))
                    { continue; }

                    auto Seed = FSeedPlane{};
                    Seed._Normal = SeedNormal;
                    Seed._OriginX = (static_cast<double>(X) + 0.5) * CellSize;
                    Seed._OriginY = (static_cast<double>(Y) + 0.5) * CellSize;
                    Seed._OriginZ = static_cast<double>(SeedZ);

                    auto MaxX = X;

                    while (MaxX + 1 < SizeX && Get_IsMergeable(MaxX + 1, Y, Seed))
                    { ++MaxX; }

                    auto MaxY = Y;

                    for (auto CandidateY = Y + 1; CandidateY < SizeY; ++CandidateY)
                    {
                        auto WholeRowJoins = true;

                        for (auto CandidateX = X; CandidateX <= MaxX && WholeRowJoins; ++CandidateX)
                        { WholeRowJoins = Get_IsMergeable(CandidateX, CandidateY, Seed); }

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
        Result.Set_ProbesSpent(OutPlates._Plates.Num());

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
