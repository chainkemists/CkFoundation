#include "CkGroundNav_BuildInvoker.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace buildinvoker_private
    {
        struct FTileBoundsXY
        {
        public:
            double _MinX = 0.0;
            double _MinY = 0.0;
            double _MaxX = 0.0;
            double _MaxY = 0.0;
        };

        auto Get_IsFinite(const FVector& InValue) -> bool
        {
            return FMath::IsFinite(InValue.X) && FMath::IsFinite(InValue.Y) && FMath::IsFinite(InValue.Z);
        }

        auto Get_IsFinite(const FVector2D& InValue) -> bool
        {
            return FMath::IsFinite(InValue.X) && FMath::IsFinite(InValue.Y);
        }

        auto Get_IsFinite(const FBox& InValue) -> bool
        {
            return Get_IsFinite(InValue.Min) && Get_IsFinite(InValue.Max);
        }

        auto Get_IsValid(const FCk_GroundNav_BuildInvokerPoint& InInvoker) -> bool
        {
            return Get_IsFinite(InInvoker._Location) &&
                   FMath::IsFinite(InInvoker._InnerRadiusUu) &&
                   FMath::IsFinite(InInvoker._OuterRadiusUu) &&
                   InInvoker._InnerRadiusUu >= 0.0f &&
                   InInvoker._InnerRadiusUu <= InInvoker._OuterRadiusUu;
        }

        auto Get_IsValid(const FCk_GroundNav_BuildInvokerBox& InInvoker) -> bool
        {
            const auto& Bounds = InInvoker._InnerBounds;

            return Bounds.IsValid != 0 && Get_IsFinite(Bounds) &&
                   Bounds.Min.X <= Bounds.Max.X && Bounds.Min.Y <= Bounds.Max.Y && Bounds.Min.Z <= Bounds.Max.Z &&
                   FMath::IsFinite(InInvoker._OuterPaddingUu) && InInvoker._OuterPaddingUu >= 0.0f &&
                   FMath::IsFinite(Bounds.Min.X - InInvoker._OuterPaddingUu) &&
                   FMath::IsFinite(Bounds.Min.Y - InInvoker._OuterPaddingUu) &&
                   FMath::IsFinite(Bounds.Max.X + InInvoker._OuterPaddingUu) &&
                   FMath::IsFinite(Bounds.Max.Y + InInvoker._OuterPaddingUu);
        }

        auto Get_FieldCanDescribeTiles(const FCk_GroundNav_FieldParams& InParams) -> bool
        {
            const auto ParamsAreValid = InParams.Get_IsValid();
            const auto OriginIsFinite = Get_IsFinite(InParams._OriginXY);
            const auto HeightRangeIsFinite =
                FMath::IsFinite(InParams._MinZUu) && FMath::IsFinite(InParams._MaxZUu);

            if (NOT ParamsAreValid || NOT OriginIsFinite || NOT HeightRangeIsFinite)
            { return false; }

            const auto SpanUu = InParams.Get_TileSpanUu();
            const auto TileCount = static_cast<int64>(InParams._Divisions.X) * static_cast<int64>(InParams._Divisions.Y);
            const auto MaxX = InParams._OriginXY.X + (SpanUu * static_cast<double>(InParams._Divisions.X));
            const auto MaxY = InParams._OriginXY.Y + (SpanUu * static_cast<double>(InParams._Divisions.Y));

            return FMath::IsFinite(SpanUu) && SpanUu > 0.0 &&
                   TileCount > 0 && TileCount <= MAX_int32 &&
                   FMath::IsFinite(MaxX) && FMath::IsFinite(MaxY);
        }

        auto Get_TileBounds(
            const FCk_GroundNav_FieldParams& InParams,
            int32                            InTileIndex) -> FTileBoundsXY
        {
            const auto Coord = Get_TileCoord(InParams._Divisions, InTileIndex);
            const auto SpanUu = InParams.Get_TileSpanUu();
            const auto MinX = InParams._OriginXY.X + (SpanUu * static_cast<double>(Coord._X));
            const auto MinY = InParams._OriginXY.Y + (SpanUu * static_cast<double>(Coord._Y));

            return FTileBoundsXY{MinX, MinY, MinX + SpanUu, MinY + SpanUu};
        }

        auto Get_IsWithinClosedRadius(
            const FTileBoundsXY& InTileBounds,
            const FVector&       InPoint,
            float                InRadiusUu) -> bool
        {
            const auto DeltaX = FMath::Max(
                FMath::Max(InTileBounds._MinX - InPoint.X, 0.0), InPoint.X - InTileBounds._MaxX);
            const auto DeltaY = FMath::Max(
                FMath::Max(InTileBounds._MinY - InPoint.Y, 0.0), InPoint.Y - InTileBounds._MaxY);
            const auto Radius = static_cast<double>(InRadiusUu);

            return (DeltaX * DeltaX) + (DeltaY * DeltaY) <= Radius * Radius;
        }

        auto Get_IntersectsClosed(
            const FTileBoundsXY& InTileBounds,
            const FBox&          InBounds,
            float                InPaddingUu) -> bool
        {
            const auto Padding = static_cast<double>(InPaddingUu);

            return InTileBounds._MinX <= InBounds.Max.X + Padding &&
                   InTileBounds._MaxX >= InBounds.Min.X - Padding &&
                   InTileBounds._MinY <= InBounds.Max.Y + Padding &&
                   InTileBounds._MaxY >= InBounds.Min.Y - Padding;
        }

        auto Get_SortedIndices(const TSet<int32>& InIndices) -> TArray<int32>
        {
            auto Result = InIndices.Array();
            Result.Sort();
            return Result;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Request_ComputeBuildInvokerSelection(
            const FCk_GroundNav_FieldParams&                 InFieldParams,
            TConstArrayView<FCk_GroundNav_BuildInvokerPoint> InPointInvokers,
            TConstArrayView<FCk_GroundNav_BuildInvokerBox>   InBoxInvokers,
            TConstArrayView<int32>                           InCurrentlyBuiltTileIndices,
            FCk_GroundNav_BuildInvokerSelection&             OutSelection)
        -> bool
    {
        auto InputIsValid = buildinvoker_private::Get_FieldCanDescribeTiles(InFieldParams);

        const auto TileCount64 =
            static_cast<int64>(InFieldParams._Divisions.X) * static_cast<int64>(InFieldParams._Divisions.Y);
        const auto TileCount = InputIsValid ? static_cast<int32>(TileCount64) : 0;

        for (const auto TileIndex : InCurrentlyBuiltTileIndices)
        { InputIsValid = InputIsValid && TileIndex >= 0 && TileIndex < TileCount; }

        CK_ENSURE_IF_NOT(InputIsValid,
            TEXT("GroundNav build invoker selection requires a valid lattice and built tile indices"))
        {}

        if (NOT InputIsValid)
        { return false; }

        // One malformed entity must not freeze the owner that all of its healthy peers share. The
        // lattice and the currently-built indices are load-bearing input and still refuse the whole
        // request; descriptors are independently optional scope contributors.
        auto ValidPointInvokers = TArray<const FCk_GroundNav_BuildInvokerPoint*>{};
        ValidPointInvokers.Reserve(InPointInvokers.Num());
        for (const auto& Invoker : InPointInvokers)
        {
            const auto InvokerIsValid = buildinvoker_private::Get_IsValid(Invoker);
            CK_ENSURE_IF_NOT(InvokerIsValid, TEXT("GroundNav build invoker ignores an invalid point descriptor"))
            {}
            if (InvokerIsValid)
            { ValidPointInvokers.Emplace(&Invoker); }
        }

        auto ValidBoxInvokers = TArray<const FCk_GroundNav_BuildInvokerBox*>{};
        ValidBoxInvokers.Reserve(InBoxInvokers.Num());
        for (const auto& Invoker : InBoxInvokers)
        {
            const auto InvokerIsValid = buildinvoker_private::Get_IsValid(Invoker);
            CK_ENSURE_IF_NOT(InvokerIsValid, TEXT("GroundNav build invoker ignores an invalid box descriptor"))
            {}
            if (InvokerIsValid)
            { ValidBoxInvokers.Emplace(&Invoker); }
        }

        auto Generate = TSet<int32>{};
        auto Retain = TSet<int32>{};
        auto CurrentlyBuilt = TSet<int32>{};

        for (const auto TileIndex : InCurrentlyBuiltTileIndices)
        { CurrentlyBuilt.Add(TileIndex); }

        for (auto TileIndex = 0; TileIndex < TileCount; ++TileIndex)
        {
            const auto TileBounds = buildinvoker_private::Get_TileBounds(InFieldParams, TileIndex);
            auto Generates = false;
            auto Retains = false;

            for (const auto* Invoker : ValidPointInvokers)
            {
                Generates = Generates || buildinvoker_private::Get_IsWithinClosedRadius(
                    TileBounds, Invoker->_Location, Invoker->_InnerRadiusUu);
                Retains = Retains || buildinvoker_private::Get_IsWithinClosedRadius(
                    TileBounds, Invoker->_Location, Invoker->_OuterRadiusUu);
            }

            for (const auto* Invoker : ValidBoxInvokers)
            {
                Generates = Generates || buildinvoker_private::Get_IntersectsClosed(
                    TileBounds, Invoker->_InnerBounds, 0.0f);
                Retains = Retains || buildinvoker_private::Get_IntersectsClosed(
                    TileBounds, Invoker->_InnerBounds, Invoker->_OuterPaddingUu);
            }

            if (Generates)
            { Generate.Add(TileIndex); }

            if (Retains)
            { Retain.Add(TileIndex); }
        }

        auto Desired = Generate;

        for (const auto TileIndex : CurrentlyBuilt)
        {
            if (Retain.Contains(TileIndex))
            { Desired.Add(TileIndex); }
        }

        auto Result = FCk_GroundNav_BuildInvokerSelection{};
        Result._GenerateTileIndices = buildinvoker_private::Get_SortedIndices(Generate);
        Result._RetainTileIndices = buildinvoker_private::Get_SortedIndices(Retain);

        for (const auto TileIndex : Desired)
        {
            if (NOT CurrentlyBuilt.Contains(TileIndex))
            { Result._BuildTileIndices.Add(TileIndex); }
        }

        for (const auto TileIndex : CurrentlyBuilt)
        {
            if (NOT Desired.Contains(TileIndex))
            { Result._PurgeTileIndices.Add(TileIndex); }
        }

        Result._BuildTileIndices.Sort();
        Result._PurgeTileIndices.Sort();
        OutSelection = MoveTemp(Result);
        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------
