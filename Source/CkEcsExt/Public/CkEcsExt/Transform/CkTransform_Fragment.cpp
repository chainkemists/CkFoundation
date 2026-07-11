#include "CkTransform_Fragment.h"

#include "CkTransform_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h"          // M2b position-restore: snapshot registration
#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"
#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_Transform_MeshSocket::
        FFragment_Transform_MeshSocket(
            const UMeshComponent* InComponent,
            FName InSocket)
        : _Component(InComponent)
        , _Socket(InSocket)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handlers for Transform

static struct FTransformRepHandlerRegistrar
{
    FTransformRepHandlerRegistrar()
    {
        // Transform is composed during construction proper (pre-link), so it exists by dispatch
        // time in every healthy flow — the NotReady branches below are violation detectors (loud
        // via the dispatcher's timeout ensure), not an expected wait. An unset Old means first
        // application: snap directly (an interpolation offset would glide the entity in from origin).

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Location::StaticStruct(); },
            {
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                {
                    auto HandleTransform = UCk_Utils_Transform_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(HandleTransform))
                    { return ECk_RepFragment_ApplyResult::NotReady; }

                    const auto& Location = New.Get<FCk_RepData_Location>().Value;

                    if (Old.IsSet() && UCk_Utils_TransformInterpolation_UE::Has(HandleTransform))
                    {
                        auto HandleTransformInterpolation = UCk_Utils_TransformInterpolation_UE::CastChecked(Entity);
                        const auto CurrentLoc = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(HandleTransform);
                        UCk_Utils_TransformInterpolation_UE::Request_SetInterpolationGoal_LocationOffset(
                            HandleTransformInterpolation, Location - CurrentLoc);
                        return ECk_RepFragment_ApplyResult::Applied;
                    }

                    UCk_Utils_Transform_UE::Request_SetLocation(HandleTransform, FCk_Request_Transform_SetLocation{Location});
                    return ECk_RepFragment_ApplyResult::Applied;
                }
            });

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Rotation::StaticStruct(); },
            {
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                {
                    auto HandleTransform = UCk_Utils_Transform_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(HandleTransform))
                    { return ECk_RepFragment_ApplyResult::NotReady; }

                    const auto& Rotation = New.Get<FCk_RepData_Rotation>().Value;

                    if (Old.IsSet() && UCk_Utils_TransformInterpolation_UE::Has(HandleTransform))
                    {
                        auto HandleTransformInterpolation = UCk_Utils_TransformInterpolation_UE::CastChecked(Entity);
                        const auto CurrentRot = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(HandleTransform);
                        UCk_Utils_TransformInterpolation_UE::Request_SetInterpolationGoal_RotationOffset(
                            HandleTransformInterpolation, Rotation.Rotator() - CurrentRot);
                        return ECk_RepFragment_ApplyResult::Applied;
                    }

                    UCk_Utils_Transform_UE::Request_SetRotation(HandleTransform, FCk_Request_Transform_SetRotation{Rotation.Rotator()});
                    return ECk_RepFragment_ApplyResult::Applied;
                }
            });

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Scale::StaticStruct(); },
            {
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                {
                    auto HandleTransform = UCk_Utils_Transform_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(HandleTransform))
                    { return ECk_RepFragment_ApplyResult::NotReady; }

                    UCk_Utils_Transform_UE::Request_SetScale(HandleTransform,
                        FCk_Request_Transform_SetScale{New.Get<FCk_RepData_Scale>().Value}
                            .Set_LocalWorld(ECk_LocalWorld::World));
                    return ECk_RepFragment_ApplyResult::Applied;
                }
            });
    }
} GTransformRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
// M2b position-restore: persist the world transform so a respawned bridged entity comes back at its saved position.
// Value-only (no handle refs) — out-of-line body because FFragment_Transform is not CKECSEXT_API-exported and lives
// in ck::. Only _Transform persists; _ComponentsModified is transient dirty-tracking, recomputed after restore.

auto
    ck::FFragment_Transform::
    SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/)
    -> void
{
    InAr << _Transform;
}

using FSnap_Transform = ck::FFragment_Transform;
CK_REGISTER_SNAPSHOTABLE(FSnap_Transform);

// Persist Previous too — SyncFromActor's query requires it, so a restored bridged entity missing it would never
// sync from its re-bridged actor (freezing anything pinned to the entity transform). Value-only, same as Transform.

auto
    ck::FFragment_Transform_Previous::
    SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& /*InCtx*/)
    -> void
{
    InAr << _Transform;
}

using FSnap_Transform_Previous = ck::FFragment_Transform_Previous;
CK_REGISTER_SNAPSHOTABLE(FSnap_Transform_Previous);

// --------------------------------------------------------------------------------------------------------------------