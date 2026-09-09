#include "CkGroundNav_FieldTransform.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkGroundNav/Bake/CkGroundNav_Boundary.h"
#include "CkGroundNav/Bake/CkGroundNav_Fingerprint.h"
#include "CkGroundNav/Field/CkGroundNav_FieldLinks.h"

namespace ck::groundnav
{
namespace field_transform_private
{
auto NormalizeYaw(const int32 Yaw, int32 &Turns) -> bool
{
    if (Yaw % 90 != 0)
        return false;
    Turns = ((Yaw / 90) % 4 + 4) % 4;
    return true;
}
auto RotateCoord(const FIntPoint &P, const int32 X, const int32 Y, const int32 T) -> FIntPoint
{
    switch (T)
    {
    case 0:
        return P;
    case 1:
        return {Y - 1 - P.Y, P.X};
    case 2:
        return {X - 1 - P.X, Y - 1 - P.Y};
    default:
        return {P.Y, X - 1 - P.X};
    }
}
auto RotateDirection(const int32 D, const int32 T) -> int32
{
    return (D + T) % kDirectionCount;
}
auto IsFinite(const FVector &V) -> bool
{
    return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y) && FMath::IsFinite(V.Z);
}
auto Remap(const FGameplayTag &Tag, const TMap<FGameplayTag, FGameplayTag> &Map, FGameplayTag &Out) -> bool
{
    if (!Tag.IsValid())
    {
        Out = {};
        return true;
    }
    const auto *Found = Map.Find(Tag);
    if (Found == nullptr || !Found->IsValid())
        return false;
    Out = *Found;
    return true;
}
auto RemapContainer(const FGameplayTagContainer &Tags, const TMap<FGameplayTag, FGameplayTag> &Map,
                    FGameplayTagContainer &Out) -> bool
{
    TArray<FGameplayTag> Values;
    Tags.GetGameplayTagArray(Values);
    Out.Reset();
    for (const auto &Tag : Values)
    {
        FGameplayTag Mapped;
        if (!Remap(Tag, Map, Mapped))
            return false;
        Out.AddTag(Mapped);
    }
    return true;
}
auto Rebase(const int32 Id, const int32 Offset, int32 &Out) -> bool
{
    const int64 Value = int64(Id) + int64(Offset);
    if (Id == INDEX_NONE || Value <= 0 || Value > MAX_int32)
        return false;
    Out = int32(Value);
    return true;
}

auto IsValidTile(const FCk_GroundNav_Tile &Tile, const FCk_GroundNav_FieldParams &Params) -> bool
{
    if (!Tile.Get_IsBuilt())
        return true;
    const double CellSize = Params._Config.Get_CellSizeUu();
    const double Span = Params.Get_TileSpanUu();
    const int32 ExpectedSize = FMath::RoundToInt32(Span / CellSize);
    const FVector ExpectedOrigin{Params._OriginXY.X + Tile._Coord._X * Span,
                                 Params._OriginXY.Y + Tile._Coord._Y * Span, Params._MinZUu};
    const int64 Count = int64(Tile._SizeX) * Tile._SizeY * Tile._LayerCount;
    if (Tile._SizeX != ExpectedSize || Tile._SizeY != ExpectedSize || Tile._LayerCount <= 0 || Count <= 0 ||
        Count > MAX_int32 || !IsFinite(Tile._Origin) || !Tile._Origin.Equals(ExpectedOrigin) ||
        !FMath::IsFinite(Tile._CellSizeUu) ||
        !FMath::IsNearlyEqual(Tile._CellSizeUu, Params._Config.Get_CellSizeUu()) || Tile._SurfaceZ.Num() != Count ||
        Tile._Clearance._SizeX != Tile._SizeX || Tile._Clearance._SizeY != Tile._SizeY ||
        Tile._Clearance._LayerCount != Tile._LayerCount ||
        !FMath::IsNearlyEqual(Tile._Clearance._CellSizeUu, Tile._CellSizeUu) ||
        Tile._Clearance._Cells.Num() != Count || !FMath::IsNearlyEqual(Tile._MaxClearanceUu, Params._MaxClearanceUu) ||
        Tile._Plates._SizeX != Tile._SizeX || Tile._Plates._SizeY != Tile._SizeY ||
        Tile._Plates._LayerCount != Tile._LayerCount || Tile._Plates._CellToPlate.Num() != Count)
        return false;
    for (const float V : Tile._SurfaceZ)
        if (!FMath::IsFinite(V) && V != FCk_GroundNav_Tile::kNoSurfaceZ)
            return false;
    for (const float V : Tile._Clearance._Cells)
        if (!FMath::IsFinite(V) || V < 0)
            return false;
    for (const auto &Plate : Tile._Plates._Plates)
        if (Plate._LayerIndex < 0 || Plate._LayerIndex >= Tile._LayerCount || Plate._MinX < 0 || Plate._MinY < 0 ||
            Plate._MaxX < Plate._MinX || Plate._MaxY < Plate._MinY || Plate._MaxX >= Tile._SizeX ||
            Plate._MaxY >= Tile._SizeY ||
            (Plate._AreaPolicyIndex != INDEX_NONE && !Tile._Plates._AreaPolicies.IsValidIndex(Plate._AreaPolicyIndex)))
            return false;
    for (const int32 Index : Tile._Plates._CellToPlate)
        if (Index != FCk_GroundNav_Plate::kNoPlate && !Tile._Plates._Plates.IsValidIndex(Index))
            return false;
    for (const auto &Portal : Tile._Portals._Portals)
        if (!Tile._Plates._Plates.IsValidIndex(Portal._PlateA) || !Tile._Plates._Plates.IsValidIndex(Portal._PlateB) ||
            Portal._Direction < 0 || Portal._Direction > 1 || Portal._FromMin.X < 0 || Portal._FromMin.Y < 0 ||
            Portal._FromMax.X < Portal._FromMin.X || Portal._FromMax.Y < Portal._FromMin.Y ||
            Portal._FromMax.X >= Tile._SizeX || Portal._FromMax.Y >= Tile._SizeY ||
            (Portal._Direction == 0 && Portal._FromMax.X >= Tile._SizeX - 1) ||
            (Portal._Direction == 1 && Portal._FromMax.Y >= Tile._SizeY - 1))
            return false;
    for (const auto &Stub : Tile._SeamStubs)
    {
        const int32 Limit = (Stub._Direction == 0 || Stub._Direction == 2) ? Tile._SizeY : Tile._SizeX;
        if (Stub._Direction < 0 || Stub._Direction >= 4 || Stub._AlongIndex < 0 || Stub._AlongIndex >= Limit ||
            !Tile._Plates._Plates.IsValidIndex(Stub._PlateIndex))
            return false;
    }
    return true;
}
auto IsValidField(const FCk_GroundNav_Field &Field) -> bool
{
    const double Span = Field._Params.Get_TileSpanUu();
    if (!Field._Params.Get_IsValid() || !FMath::IsFinite(Field._Params._OriginXY.X) ||
        !FMath::IsFinite(Field._Params._OriginXY.Y) || !FMath::IsFinite(Field._Params._MinZUu) ||
        !FMath::IsFinite(Field._Params._MaxZUu) || !FMath::IsFinite(Field._Params._MaxClearanceUu) ||
        !FMath::IsFinite(Span) || Span <= 0.0 || Field._Tiles.Num() != Field._Params.Get_TileCount())
        return false;
    for (int32 I = 0; I < Field._Tiles.Num(); ++I)
        if (Field._Tiles[I]._Coord != Get_TileCoord(Field._Params._Divisions, I) ||
            !IsValidTile(Field._Tiles[I], Field._Params))
            return false;
    for (const auto &M : Field._Params._MarkupRecords)
        if (M.Get_Id() <= 0 || !IsFinite(M.Get_WorldTransform().GetLocation()) ||
            !M.Get_WorldTransform().GetRotation().IsNormalized())
            return false;
    for (const auto &L : Field._Params._Links)
        if (L.Get_Id() <= 0 || !IsFinite(L.Get_Start()) || !IsFinite(L.Get_End()))
            return false;
    return true;
}

auto TransformPoint(const FVector &Point, const FCk_GroundNav_FieldParams &Source,
                    const FCk_GroundNav_FieldParams &Host, const FCk_GroundNav_FieldInstanceTransform &Desc,
                    const int32 Turns) -> FVector
{
    const double Span = Host.Get_TileSpanUu(), X = Source._OriginXY.X, Y = Source._OriginXY.Y;
    const FVector Local = Point - FVector{X, Y, 0};
    const FVector Base{Host._OriginXY.X + Desc._DestinationTileAnchor._X * Span,
                       Host._OriginXY.Y + Desc._DestinationTileAnchor._Y * Span, 0};
    if (Turns == 0)
        return Base + Local;
    if (Turns == 1)
        return Base + FVector{Source._Divisions.Y * Span - Local.Y, Local.X, Local.Z};
    if (Turns == 2)
        return Base + FVector{Source._Divisions.X * Span - Local.X, Source._Divisions.Y * Span - Local.Y, Local.Z};
    return Base + FVector{Local.Y, Source._Divisions.X * Span - Local.X, Local.Z};
}

auto RotateTile(FCk_GroundNav_Tile &Tile, const int32 Turns, const FVector &NewOrigin,
                const TMap<FGameplayTag, FGameplayTag> &Map) -> bool
{
    const int32 OldX = Tile._SizeX, OldY = Tile._SizeY, NewX = (Turns % 2) ? OldY : OldX,
                NewY = (Turns % 2) ? OldX : OldY, Plane = OldX * OldY;
    auto RotateFloats = [&](TArray<float> &Values)
    {
        TArray<float> Result;
        Result.SetNum(Values.Num());
        for (int32 L = 0; L < Tile._LayerCount; ++L)
            for (int32 Y = 0; Y < OldY; ++Y)
                for (int32 X = 0; X < OldX; ++X)
                {
                    const auto P = RotateCoord({X, Y}, OldX, OldY, Turns);
                    Result[L * NewX * NewY + P.Y * NewX + P.X] = Values[L * Plane + Y * OldX + X];
                }
        Values = MoveTemp(Result);
    };
    RotateFloats(Tile._SurfaceZ);
    RotateFloats(Tile._Clearance._Cells);
    Tile._Clearance._SizeX = NewX;
    Tile._Clearance._SizeY = NewY;
    auto &Plates = Tile._Plates;
    TArray<int32> Cells;
    Cells.SetNum(Plates._CellToPlate.Num());
    for (int32 L = 0; L < Tile._LayerCount; ++L)
        for (int32 Y = 0; Y < OldY; ++Y)
            for (int32 X = 0; X < OldX; ++X)
            {
                const auto P = RotateCoord({X, Y}, OldX, OldY, Turns);
                Cells[L * NewX * NewY + P.Y * NewX + P.X] = Plates._CellToPlate[L * Plane + Y * OldX + X];
            }
    Plates._CellToPlate = MoveTemp(Cells);
    Plates._SizeX = NewX;
    Plates._SizeY = NewY;
    for (auto &Plate : Plates._Plates)
    {
        const auto A = RotateCoord({Plate._MinX, Plate._MinY}, OldX, OldY, Turns),
                   B = RotateCoord({Plate._MaxX, Plate._MaxY}, OldX, OldY, Turns);
        Plate._MinX = FMath::Min(A.X, B.X);
        Plate._MaxX = FMath::Max(A.X, B.X);
        Plate._MinY = FMath::Min(A.Y, B.Y);
        Plate._MaxY = FMath::Max(A.Y, B.Y);
    }
    for (auto &Policy : Plates._AreaPolicies)
    {
        FGameplayTagContainer NewPolicy;
        if (!RemapContainer(Policy, Map, NewPolicy))
            return false;
        Policy = MoveTemp(NewPolicy);
    }
    for (auto &Portal : Tile._Portals._Portals)
    {
        const int32 OldDirection = Portal._Direction, RotatedDirection = RotateDirection(OldDirection, Turns);
        FIntPoint A = Portal._FromMin, B = Portal._FromMax;
        if (RotatedDirection >= 2)
        {
            const auto Offset = Get_DirectionOffset(OldDirection);
            A += Offset;
            B += Offset;
            Swap(Portal._PlateA, Portal._PlateB);
        }
        A = RotateCoord(A, OldX, OldY, Turns);
        B = RotateCoord(B, OldX, OldY, Turns);
        const int32 Direction = RotatedDirection >= 2 ? RotatedDirection - 2 : RotatedDirection;
        const bool Reverse = Direction == 0 ? A.Y > B.Y : A.X > B.X;
        Portal._Direction = Direction;
        Portal._FromMin = {FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y)};
        Portal._FromMax = {FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y)};
        if (Reverse)
            Swap(Portal._MinEndZUu, Portal._MaxEndZUu);
    }
    for (auto &Stub : Tile._SeamStubs)
    {
        FIntPoint Cell;
        if (Stub._Direction == 0)
            Cell = {OldX - 1, Stub._AlongIndex};
        else if (Stub._Direction == 1)
            Cell = {Stub._AlongIndex, OldY - 1};
        else if (Stub._Direction == 2)
            Cell = {0, Stub._AlongIndex};
        else
            Cell = {Stub._AlongIndex, 0};
        Cell = RotateCoord(Cell, OldX, OldY, Turns);
        Stub._Direction = RotateDirection(Stub._Direction, Turns);
        Stub._AlongIndex = (Stub._Direction == 0 || Stub._Direction == 2) ? Cell.Y : Cell.X;
    }
    Tile._SizeX = NewX;
    Tile._SizeY = NewY;
    Tile._Origin = NewOrigin;
    Tile._Portals._PlateToPortals.Reset();
    Tile._Portals._PlateToPortals.SetNum(Plates._Plates.Num());
    for (int32 I = 0; I < Tile._Portals._Portals.Num(); ++I)
    {
        const auto &P = Tile._Portals._Portals[I];
        if (!Plates._Plates.IsValidIndex(P._PlateA) || !Plates._Plates.IsValidIndex(P._PlateB))
            return false;
        Tile._Portals._PlateToPortals[P._PlateA].Add(I);
        Tile._Portals._PlateToPortals[P._PlateB].Add(I);
    }
    FCk_GroundNav_BoundaryLattice Lattice;
    Lattice._Origin = Tile._Origin;
    Lattice._CellSizeUu = Tile._CellSizeUu;
    Lattice._SizeX = NewX;
    Lattice._SizeY = NewY;
    Lattice._LayerCount = Tile._LayerCount;
    Lattice._SurfaceZ = &Tile._SurfaceZ;
    int32 Probes = 0;
    DoDerive_Boundary(Lattice, Tile._Plates, Tile._Portals, Tile._Boundary, Probes);
    return true;
}
} // namespace field_transform_private

auto TryMerge_TransformedField(const FCk_GroundNav_Field &Host, const FCk_GroundNav_Field &Source,
                               const FCk_GroundNav_FieldInstanceTransform &Desc,
                               FCk_GroundNav_TransformedFieldInstance &Out) -> ECk_GroundNav_FieldMergeStatus
{
    using namespace field_transform_private;
    int32 Turns = 0;
    const bool Valid = Desc._DestinationVolumeId.Get_IsStreamingValid() && NormalizeYaw(Desc._YawDegrees, Turns) &&
                       IsValidField(Host) && IsValidField(Source);
    CK_ENSURE_IF_NOT(Valid, TEXT("GroundNav field transform has invalid descriptor or field payload")) {}
    if (!Valid)
        return ECk_GroundNav_FieldMergeStatus::InvalidInput;
    const FBox CompatibilityRegion{FVector{-1.0, -1.0, -1.0}, FVector{1.0, 1.0, 1.0}};
    const bool Compatible =
        Get_InputFingerprint(CompatibilityRegion, Host._Params._Config, Host._Params._Profile, {}, {},
                             Host._Params._MergeTunables, Host._Params._MaxClearanceUu) ==
            Get_InputFingerprint(CompatibilityRegion, Source._Params._Config, Source._Params._Profile, {}, {},
                                 Source._Params._MergeTunables, Source._Params._MaxClearanceUu) &&
        Host._Params._MinZUu == Source._Params._MinZUu && Host._Params._MaxZUu == Source._Params._MaxZUu;
    CK_ENSURE_IF_NOT(Compatible, TEXT("GroundNav field transform needs matching lattice settings")) {}
    if (!Compatible)
        return ECk_GroundNav_FieldMergeStatus::IncompatibleLattice;
    const FIntPoint RotatedFootprint = Turns % 2 == 0
                                         ? Source._Params._Divisions
                                         : FIntPoint{Source._Params._Divisions.Y, Source._Params._Divisions.X};
    const bool FootprintFits = Desc._DestinationTileAnchor._X >= 0 && Desc._DestinationTileAnchor._Y >= 0 &&
                               Desc._DestinationTileAnchor._X <= Host._Params._Divisions.X - RotatedFootprint.X &&
                               Desc._DestinationTileAnchor._Y <= Host._Params._Divisions.Y - RotatedFootprint.Y;
    if (!FootprintFits)
        return ECk_GroundNav_FieldMergeStatus::OutOfBounds;
    TSet<int32> MarkupIds, LinkIds;
    for (const auto &M : Host._Params._MarkupRecords)
    {
        if (MarkupIds.Contains(M.Get_Id()))
            return ECk_GroundNav_FieldMergeStatus::IdentityConflict;
        MarkupIds.Add(M.Get_Id());
    }
    for (const auto &L : Host._Params._Links)
    {
        if (LinkIds.Contains(L.Get_Id()))
            return ECk_GroundNav_FieldMergeStatus::IdentityConflict;
        LinkIds.Add(L.Get_Id());
    }
    TSet<FIntPoint> Coords;
    for (const auto &M : Source._Params._MarkupRecords)
    {
        int32 Id;
        FGameplayTag Tag;
        if (!Rebase(M.Get_Id(), Desc._MarkupIdOffset, Id) || MarkupIds.Contains(Id))
            return ECk_GroundNav_FieldMergeStatus::IdentityConflict;
        if (!Remap(M.Get_AreaTag(), Desc._TagRemap, Tag))
            return ECk_GroundNav_FieldMergeStatus::MissingTagRemap;
        MarkupIds.Add(Id);
    }
    for (const auto &L : Source._Params._Links)
    {
        int32 Id;
        FGameplayTag A, U;
        if (!Rebase(L.Get_Id(), Desc._LinkIdOffset, Id) || LinkIds.Contains(Id))
            return ECk_GroundNav_FieldMergeStatus::IdentityConflict;
        if (!Remap(L.Get_AreaTag(), Desc._TagRemap, A) || !Remap(L.Get_UserTypeTag(), Desc._TagRemap, U))
            return ECk_GroundNav_FieldMergeStatus::MissingTagRemap;
        LinkIds.Add(Id);
    }
    for (const auto &Tile : Source._Tiles)
    {
        if (!Tile.Get_IsBuilt())
            continue;
        auto P = RotateCoord({Tile._Coord._X, Tile._Coord._Y}, Source._Params._Divisions.X, Source._Params._Divisions.Y,
                             Turns) +
                 FIntPoint{Desc._DestinationTileAnchor._X, Desc._DestinationTileAnchor._Y};
        if (P.X < 0 || P.Y < 0 || P.X >= Host._Params._Divisions.X || P.Y >= Host._Params._Divisions.Y ||
            Coords.Contains(P))
            return ECk_GroundNav_FieldMergeStatus::OutOfBounds;
        const int32 I = Get_TileIndex(Host._Params._Divisions, {P.X, P.Y});
        if (!Host._Tiles.IsValidIndex(I) || Host._Tiles[I].Get_IsBuilt())
            return ECk_GroundNav_FieldMergeStatus::TileOverlap;
        Coords.Add(P);
        for (const auto &Policy : Tile._Plates._AreaPolicies)
        {
            FGameplayTagContainer Remapped;
            if (!RemapContainer(Policy, Desc._TagRemap, Remapped))
                return ECk_GroundNav_FieldMergeStatus::MissingTagRemap;
        }
    }
    FCk_GroundNav_Field Candidate = Host;
    const FQuat Q{FVector::UpVector, FMath::DegreesToRadians(float(Turns * 90))};
    for (const auto &M : Source._Params._MarkupRecords)
    {
        int32 Id;
        FGameplayTag Tag;
        if (!Rebase(M.Get_Id(), Desc._MarkupIdOffset, Id) || !Remap(M.Get_AreaTag(), Desc._TagRemap, Tag))
            return ECk_GroundNav_FieldMergeStatus::InvalidInput;
        FTransform T = M.Get_WorldTransform();
        T.SetLocation(TransformPoint(T.GetLocation(), Source._Params, Host._Params, Desc, Turns));
        T.SetRotation(Q * T.GetRotation());
        FCk_GroundNav_MarkupRecord C{Id, M.Get_Shape(), T, M.Get_Kind()};
        C.Set_AreaTag(Tag);
        C.Set_Enable(M.Get_Enable());
        C.Set_CostMultiplier(M.Get_CostMultiplier());
        C.Set_RequestedAtEpoch(M.Get_RequestedAtEpoch());
        Candidate._Params._MarkupRecords.Add(MoveTemp(C));
    }
    for (const auto &L : Source._Params._Links)
    {
        int32 Id;
        FGameplayTag A, U;
        if (!Rebase(L.Get_Id(), Desc._LinkIdOffset, Id) || !Remap(L.Get_AreaTag(), Desc._TagRemap, A) ||
            !Remap(L.Get_UserTypeTag(), Desc._TagRemap, U))
            return ECk_GroundNav_FieldMergeStatus::InvalidInput;
        FCk_GroundNav_LinkRecord C{Id, TransformPoint(L.Get_Start(), Source._Params, Host._Params, Desc, Turns),
                                   TransformPoint(L.Get_End(), Source._Params, Host._Params, Desc, Turns)};
        C.Set_AreaTag(A);
        C.Set_UserTypeTag(U);
        C.Set_Direction(L.Get_Direction());
        C.Set_CostMultiplierForward(L.Get_CostMultiplierForward());
        C.Set_CostMultiplierBackward(L.Get_CostMultiplierBackward());
        C.Set_ClearanceUu(L.Get_ClearanceUu());
        C.Set_Enable(L.Get_Enable());
        C.Set_ProjectionMode(L.Get_ProjectionMode());
        C.Set_ProjectionHorizontalExtentUu(L.Get_ProjectionHorizontalExtentUu());
        C.Set_ProjectionVerticalExtentUu(L.Get_ProjectionVerticalExtentUu());
        C.Set_RequestedAtEpoch(L.Get_RequestedAtEpoch());
        Candidate._Params._Links.Add(MoveTemp(C));
    }
    const double Span = Host._Params.Get_TileSpanUu();
    for (const auto &SourceTile : Source._Tiles)
    {
        if (!SourceTile.Get_IsBuilt())
            continue;
        auto P = RotateCoord({SourceTile._Coord._X, SourceTile._Coord._Y}, Source._Params._Divisions.X,
                             Source._Params._Divisions.Y, Turns) +
                 FIntPoint{Desc._DestinationTileAnchor._X, Desc._DestinationTileAnchor._Y};
        const int32 I = Get_TileIndex(Candidate._Params._Divisions, {P.X, P.Y});
        FCk_GroundNav_Tile Tile = SourceTile;
        Tile._Coord = {P.X, P.Y};
        if (!RotateTile(
                Tile, Turns,
                {Host._Params._OriginXY.X + P.X * Span, Host._Params._OriginXY.Y + P.Y * Span, SourceTile._Origin.Z},
                Desc._TagRemap))
            return ECk_GroundNav_FieldMergeStatus::InvalidInput;
        Candidate._Tiles[I] = MoveTemp(Tile);
    }
    Candidate._SeamPortals.Reset();
    Candidate._SeamAdjacencies.Reset();
    Candidate._TileEdgeBoundary.Reset();
    Candidate._ResolvedLinks.Reset();
    Candidate._TilePlateOffsets.Reset();
    Candidate._ReachabilityLabels.Reset();
    Candidate._ComponentIsOpen.Reset();
    Candidate._UnmatchedSeamStubCount = 0;
    Candidate._UnresolvedLinkCount = 0;
    DoDerive_SeamPortals(Candidate);
    DoResolve_Links(Candidate);
    DoLabel_Reachability(Candidate);
    Out = {Desc._DestinationVolumeId, MoveTemp(Candidate)};
    return ECk_GroundNav_FieldMergeStatus::Completed;
}
} // namespace ck::groundnav
