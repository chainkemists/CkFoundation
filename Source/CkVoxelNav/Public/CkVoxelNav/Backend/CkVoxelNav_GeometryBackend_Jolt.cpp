#include "CkVoxelNav_GeometryBackend_Jolt.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid_Defaults.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_VoxelNav_GeometryBackend_Jolt::
    FCk_VoxelNav_GeometryBackend_Jolt(
        const UObject* InWorldContextObject)
{
    const auto WorldContextIsValid = ck::IsValid(InWorldContextObject, ck::IsValid_Policy_NullptrOnly{});

    CK_ENSURE_IF_NOT(WorldContextIsValid,
        TEXT("Cannot resolve a VoxelNav geometry backend without a World Context Object"))
    {}

    if (NOT WorldContextIsValid)
    { return; }

    _Session = ck::jolt::FCk_Jolt_QuerySession{InWorldContextObject};
}

auto
    FCk_VoxelNav_GeometryBackend_Jolt::
    Get_IsValid() const
    -> bool
{
    return _Session.Get_IsValid();
}

auto
    FCk_VoxelNav_GeometryBackend_Jolt::
    Get_IsBoxOccupied(
        const FVector& InCenter,
        const FVector& InHalfExtents) const
    -> bool
{
    return _Session.Get_IsBoxOccupied(DoGet_Probe(InHalfExtents), InCenter);
}

auto
    FCk_VoxelNav_GeometryBackend_Jolt::
    Get_BodiesInBox(
        const FBox& InWorldBounds,
        TArray<FCk_VoxelNav_BodyId>& OutBodies) const
    -> void
{
    OutBodies.Reset();

    auto SessionBodyIds = TArray<uint64>{};
    _Session.Get_BodiesInAABox(InWorldBounds, SessionBodyIds);

    OutBodies.Reserve(SessionBodyIds.Num());

    for (const auto& SessionBodyId : SessionBodyIds)
    { OutBodies.Emplace(FCk_VoxelNav_BodyId{SessionBodyId}); }
}

auto
    FCk_VoxelNav_GeometryBackend_Jolt::
    Get_IsSegmentBlocked(
        const FVector& InFrom,
        const FVector& InTo) const
    -> bool
{
    return _Session.Get_IsSegmentBlocked(InFrom, InTo);
}

auto
    FCk_VoxelNav_GeometryBackend_Jolt::
    DoGet_Probe(
        const FVector& InHalfExtents) const
    -> const ck::jolt::FCk_Jolt_BoxProbe&
{
    // Exact equality on purpose: every extent is derived from the same expression on every visit, so
    // a bit-exact match is the common case, a miss costs one extra probe, and a tolerant match would
    // silently answer with a differently-sized box.
    const auto* ExistingProbe = _ProbesByHalfExtent.FindByPredicate(
        [&](const ck::jolt::FCk_Jolt_BoxProbe& InProbe) -> bool
        { return InProbe.Get_HalfExtents() == InHalfExtents; });

    if (ck::IsValid(ExistingProbe, ck::IsValid_Policy_NullptrOnly{}))
    { return *ExistingProbe; }

    return _ProbesByHalfExtent.Emplace_GetRef(InHalfExtents);
}

// --------------------------------------------------------------------------------------------------------------------
