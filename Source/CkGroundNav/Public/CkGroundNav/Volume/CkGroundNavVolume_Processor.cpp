#include "CkGroundNavVolume_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkGroundNav/Bake/CkGroundNav_MarkupMask.h"
#include "CkGroundNav/CkGroundNav_Log.h"
#include "CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h"
#include "CkGroundNav/Field/CkGroundNav_FieldMarkupCost.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Utils.h"

#include "CkNavigation/NavSurface/CkNavSurface_AreaPolicy.h"
#include "CkNavigation/NavSurface/CkNavSurface_Utils.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_HandleMarkupRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_StartBuild);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_Build);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_MarkupCostDerive);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_MarkupWalkabilityRebuild);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_CancelPendingMarkupRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace ck_groundnav_volume_processor
    {
        // The records the volume currently holds, enabled and disabled alike: the bake decides per
        // record which of them reach a tile and whether a disabled one applies, and a caller filtering
        // either question here would be a second answer to it.
        auto Get_MarkupRecordsOf(
            TConstArrayView<FCk_GroundNav_MarkupEntry> InEntries) -> TArray<FCk_GroundNav_MarkupRecord>
        {
            return algo::Transform<TArray<FCk_GroundNav_MarkupRecord>>(InEntries,
                [](const FCk_GroundNav_MarkupEntry& InEntry) -> FCk_GroundNav_MarkupRecord
                {
                    return InEntry.Get_Record();
                });
        }

        auto Get_FieldParams(
            const FFragment_GroundNavVolume_Params&   InParams,
            const TArray<FCk_GroundNav_MarkupRecord>& InMarkupRecords)
            -> groundnav::FCk_GroundNav_FieldParams
        {
            auto FieldParams = groundnav::FCk_GroundNav_FieldParams{};

            const auto Bounds = InParams.Get_VolumeBounds();

            FieldParams._OriginXY = FVector2D{Bounds.Min.X, Bounds.Min.Y};
            FieldParams._MinZUu = static_cast<float>(Bounds.Min.Z);
            FieldParams._MaxZUu = static_cast<float>(Bounds.Max.Z);
            FieldParams._Config = InParams.Get_Config();
            FieldParams._Profile = InParams.Get_Profile();
            FieldParams._MergeTunables = InParams.Get_MergeTunables();
            FieldParams._MarkupRecords = InMarkupRecords;
            FieldParams._MaxClearanceUu = InParams.Get_MaxClearanceUu();

            // Derived rather than authored beside the bounds: an origin and a division count that
            // disagreed about which ground the volume covers would each look reasonable on its own.
            const auto SpanUu = FieldParams.Get_TileSpanUu();

            if (SpanUu > 0.0)
            {
                const auto Size = Bounds.GetSize();

                FieldParams._Divisions = FIntPoint{
                    FMath::Max(1, FMath::CeilToInt32(Size.X / SpanUu)),
                    FMath::Max(1, FMath::CeilToInt32(Size.Y / SpanUu))};
            }

            return FieldParams;
        }

        // Markup is deliberately absent: what a volume is PAINTED with cannot make its bake settings
        // valid or invalid, and feeding the records in here would suggest it could.
        auto Get_ParamsAreBakeable(const FFragment_GroundNavVolume_Params& InParams) -> bool
        {
            const auto Bounds = InParams.Get_VolumeBounds();

            return Bounds.IsValid != 0 &&
                   Bounds.GetSize().X > 0.0 && Bounds.GetSize().Y > 0.0 && Bounds.GetSize().Z > 0.0 &&
                   Get_FieldParams(InParams, {}).Get_IsValid() &&
                   InParams.Get_ProbeBudgetPerTick() > 0;
        }

        auto Get_MarkupKind(
            ECk_NavSurface_AreaPolicyKind InPolicyKind) -> ECk_GroundNav_MarkupKind
        {
            return InPolicyKind == ECk_NavSurface_AreaPolicyKind::Walkability
                ? ECk_GroundNav_MarkupKind::Walkability
                : ECk_GroundNav_MarkupKind::Cost;
        }

        auto DoMark_MarkupDirty(
            FCk_Handle_GroundNavVolume InVolumeEntity,
            ECk_GroundNav_MarkupKind   InKind) -> void
        {
            if (InKind == ECk_GroundNav_MarkupKind::Walkability)
            { InVolumeEntity.AddOrGet<FTag_GroundNavVolume_MarkupWalkabilityDirty>(); }
            else
            { InVolumeEntity.AddOrGet<FTag_GroundNavVolume_MarkupCostDirty>(); }
        }

        auto Get_MarkupEntryIndex(
            const FFragment_GroundNavVolume_Markup& InMarkup,
            const FCk_Handle&                       InMarkupEntity) -> int32
        {
            return algo::FindIndex(InMarkup.Get_Entries(),
                [&](const FCk_GroundNav_MarkupEntry& InEntry) -> bool
                {
                    return InEntry.Get_MarkupEntity() == InMarkupEntity;
                });
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        InVolumeEntity.Remove<MarkedDirtyBy>();

        const auto ParamsAreBakeable = Get_ParamsAreBakeable(InParams);

        // The volume stays inert rather than arming a build that would bake nonsense. A later
        // Request_Build on it fails loudly through the same validation.
        CK_ENSURE_IF_NOT(ParamsAreBakeable,
            TEXT("GroundNav Volume [{}] cannot be baked. Bounds [{}], cell size [{}]uu, tile size [{}]uu, "
                 "clearance ceiling [{}]uu, probe budget [{}]"),
            InVolumeEntity, InParams.Get_VolumeBounds(), InParams.Get_Config().Get_CellSizeUu(),
            InParams.Get_Config().Get_TileSizeUu(), InParams.Get_MaxClearanceUu(),
            InParams.Get_ProbeBudgetPerTick())
        { return; }

        // Registered before anything is baked, with no field yet, so a caller that can only name a
        // WORLD — the NavSurface provider adapter — can find the volume to paint on it. A volume that
        // only entered the registry at its first publish could not be painted until then, and the
        // paint would be refused rather than deferred.
        groundnav::world_fields::Publish(
            UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity),
            InVolumeEntity,
            {});

        if (InParams.Get_AutoBuildOnSetup() == ECk_EnableDisable::Disable)
        { return; }

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsBuild>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            FFragment_GroundNavVolume_Requests& InRequests) const
        -> void
    {
        ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InVolumeEntity, InParams, InBuildState, InRequest);
            }));
    }

    auto
        FProcessor_GroundNavVolume_HandleRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            const FCk_Request_GroundNavVolume_Build& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        const auto ParamsAreBakeable = Get_ParamsAreBakeable(InParams);

        CK_ENSURE_IF_NOT(ParamsAreBakeable,
            TEXT("Cannot build GroundNav Volume [{}] - its params are not bakeable"), InVolumeEntity)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        const auto IsBuilding = InVolumeEntity.Has<FTag_GroundNavVolume_BuildInProgress>() ||
                                InVolumeEntity.Has<FTag_GroundNavVolume_NeedsBuild>();

        // A request arriving while a build is already running is an idempotent no-op: the running build
        // already satisfies the caller's intent, and Succeeded is what that means.
        if (IsBuilding && InRequest.Get_ForceRestart() == ECk_EnableDisable::Disable)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
            return;
        }

        // A restart strands whoever was waiting on the build it replaces, so that build completes as
        // cancelled rather than silently never reporting.
        InBuildState._PendingRequest.TryFireCompletion(
            InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);

        InBuildState._Backend.Reset();
        InBuildState._PendingRequest = InRequest;

        InVolumeEntity.Try_Remove<FTag_GroundNavVolume_BuildInProgress>();
        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsBuild>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_HandleMarkupRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_Markup& InMarkup,
            FFragment_GroundNavVolume_MarkupRequests& InRequests) const
        -> void
    {
        ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InVolumeEntity, InBuiltField, InMarkup, InRequest);
            }));
    }

    auto
        FProcessor_GroundNavVolume_HandleMarkupRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_Markup& InMarkup,
            const FCk_Request_GroundNavVolume_AreaMarkup& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        auto MarkupEntity = InRequest.Get_MarkupEntity();

        const auto MarkupEntityIsValid = ck::IsValid(MarkupEntity);

        CK_ENSURE_IF_NOT(MarkupEntityIsValid,
            TEXT("Cannot mark up GroundNav Volume [{}] - the request names an invalid markup Entity [{}], "
                 "which is the identity its record would be keyed on"),
            InVolumeEntity, MarkupEntity)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        const auto Policy = nav_surface::TryGet_AreaPolicy(InRequest.Get_AreaTag());

        const auto AreaTagIsRegistered = Policy.IsSet();

        CK_ENSURE_IF_NOT(AreaTagIsRegistered,
            TEXT("Cannot mark up GroundNav Volume [{}] with area tag [{}] - nothing published what that "
                 "tag MEANS, and a record carrying it would bake into ground nothing knows how to apply"),
            InVolumeEntity, InRequest.Get_AreaTag())
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        const auto ExistingIndex = Get_MarkupEntryIndex(InMarkup, MarkupEntity);
        const auto EntryExists = InMarkup._Entries.IsValidIndex(ExistingIndex);

        const auto PreviousKind = EntryExists
            ? TOptional<ECk_GroundNav_MarkupKind>{InMarkup._Entries[ExistingIndex].Get_Record().Get_Kind()}
            : TOptional<ECk_GroundNav_MarkupKind>{};

        const auto RecordId = EntryExists
            ? InMarkup._Entries[ExistingIndex].Get_Record().Get_Id()
            : InMarkup._NextId;

        const auto Kind = Get_MarkupKind(Policy->Get_Kind());

        // A Walkability record carries the identity multiplier by its own contract, so both kinds
        // multiply through one cost path downstream without a branch.
        const auto CostMultiplier = Kind == ECk_GroundNav_MarkupKind::Cost
            ? Policy->Get_CostMultiplier()
            : 1.0f;

        auto Record = FCk_GroundNav_MarkupRecord{
            RecordId, InRequest.Get_Shape(), InRequest.Get_WorldTransform(), Kind};

        Record.Set_AreaTag(InRequest.Get_AreaTag())
              .Set_Enable(InRequest.Get_Enable())
              .Set_CostMultiplier(CostMultiplier)
              .Set_RequestedAtEpoch(InBuiltField.Get_Epoch()._Value);

        const auto BoundsAreValid = groundnav::Get_MarkupWorldBounds(Record).IsValid != 0;

        CK_ENSURE_IF_NOT(BoundsAreValid,
            TEXT("Cannot mark up GroundNav Volume [{}] from markup Entity [{}] - its shape and transform "
                 "bound nothing. A degenerate volume and a volume that covers no ground are different "
                 "answers, and only the second is admissible."),
            InVolumeEntity, MarkupEntity)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        // A record whose bounds miss every tile, or land where nothing has baked, is still admitted: the
        // volume owns what was AUTHORED, and what that reaches is the bake's separate answer.
        if (EntryExists)
        {
            InMarkup._Entries[ExistingIndex]._Record = Record;
        }
        else
        {
            auto& NewEntry = InMarkup._Entries.AddDefaulted_GetRef();

            NewEntry._MarkupEntity = MarkupEntity;
            NewEntry._Record = Record;

            ++InMarkup._NextId;
        }

        auto& MarkupRef = MarkupEntity.AddOrGet<FFragment_GroundNav_MarkupRef>();

        MarkupRef._VolumeEntity = InVolumeEntity.ConvertToHandle();
        MarkupRef._RecordId = RecordId;

        // A record that changed KIND changed both: the ground its old kind was deciding is decided by
        // nothing now, and the ground its new kind decides was not decided before.
        if (PreviousKind.IsSet() && *PreviousKind != Kind)
        { DoMark_MarkupDirty(InVolumeEntity, *PreviousKind); }

        DoMark_MarkupDirty(InVolumeEntity, Kind);

        InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    auto
        FProcessor_GroundNavVolume_HandleMarkupRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_Markup& InMarkup,
            const FCk_Request_GroundNavVolume_ReleaseAreaMarkup& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        auto MarkupEntity = InRequest.Get_MarkupEntity();

        const auto MarkupEntityIsValid = ck::IsValid(MarkupEntity);

        CK_ENSURE_IF_NOT(MarkupEntityIsValid,
            TEXT("Cannot release markup on GroundNav Volume [{}] - the request names an invalid markup "
                 "Entity [{}], and there is nothing else a record is keyed on"),
            InVolumeEntity, MarkupEntity)
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        const auto ExistingIndex = Get_MarkupEntryIndex(InMarkup, MarkupEntity);

        // Releasing a record the volume does not hold leaves the caller's intent holding afterwards,
        // which is what Succeeded means.
        if (NOT InMarkup._Entries.IsValidIndex(ExistingIndex))
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
            return;
        }

        const auto ReleasedKind = InMarkup._Entries[ExistingIndex].Get_Record().Get_Kind();

        InMarkup._Entries.RemoveAt(ExistingIndex);

        MarkupEntity.Try_Remove<FFragment_GroundNav_MarkupRef>();

        // Ground a released record was deciding is decided by nothing now, which is as much a change of
        // that record's kind as painting it was.
        DoMark_MarkupDirty(InVolumeEntity, ReleasedKind);

        InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_StartBuild::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_BuildState& InBuildState) const
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        InVolumeEntity.Remove<MarkedDirtyBy>();

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

        InBuildState._Backend = MakeUnique<groundnav::FCk_GroundNav_GeometryBackend_Jolt>(World);

        const auto BackendIsUsable = InBuildState._Backend->Get_IsValid();

        CK_ENSURE_IF_NOT(BackendIsUsable,
            TEXT("GroundNav Volume [{}] cannot start a build: no physics world to read geometry from"),
            InVolumeEntity)
        {
            // Never a Built field with nothing in it. A backend that cannot answer means the world is
            // unknown, which is not the same as a world with no floor.
            InBuildState._Backend.Reset();

            InBuildState._PendingRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed);

            return;
        }

        // The epoch comes from what is PUBLISHED, not from the build state: beginning a build resets
        // that state, so reading the counter from it would restart at one on every rebuild and every
        // reader comparing epochs would conclude it was up to date.
        //
        // The markup goes in HERE and not at the request, so every build — a plain Request_Build as
        // much as one a markup change asked for — bakes against what the volume currently holds. A
        // rebuild that took no records would silently unpaint the world.
        const auto BeginResult = groundnav::Request_BeginBuild(
            Get_FieldParams(InParams,
                Get_MarkupRecordsOf(UCk_Utils_GroundNavVolume_UE::Get_MarkupRecords(InVolumeEntity))),
            InBuiltField.Get_Epoch().Get_Next(),
            InBuildState._Build);

        CK_ENSURE_IF_NOT(BeginResult.Get_IsCompleted(),
            TEXT("GroundNav Volume [{}] could not begin a build: [{}]"),
            InVolumeEntity, BeginResult.Get_Status())
        {
            InBuildState._Backend.Reset();

            InBuildState._PendingRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed);

            return;
        }

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_BuildInProgress>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_Build::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            FFragment_GroundNavVolume_BuiltField& InBuiltField) const
        -> void
    {
        const auto BackendIsHeld = InBuildState._Backend.IsValid();

        CK_ENSURE_IF_NOT(BackendIsHeld,
            TEXT("GroundNav Volume [{}] is marked as building but holds no geometry backend"),
            InVolumeEntity)
        {
            InVolumeEntity.Remove<FTag_GroundNavVolume_BuildInProgress>();

            InBuildState._PendingRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed);

            return;
        }

        const auto SliceResult = groundnav::Request_AdvanceBuild(
            *InBuildState._Backend, InParams.Get_ProbeBudgetPerTick(), InBuildState._Build);

        if (SliceResult.Get_Status() == ECk_GroundNav_BakeStatus::BudgetExhausted)
        { return; }

        InVolumeEntity.Remove<FTag_GroundNavVolume_BuildInProgress>();
        InBuildState._Backend.Reset();

        auto Completed = groundnav::Request_ReleaseCompletedField(InBuildState._Build);

        if (NOT SliceResult.Get_IsCompleted() || NOT Completed.IsValid())
        {
            // A failed REBUILD leaves the previously published field alone: stale ground is still
            // ground, and dropping it would strand every agent standing on it.
            groundnav::Error(TEXT("GroundNav Volume [{}] failed to build: [{}]"),
                InVolumeEntity, SliceResult.Get_Status());

            InBuildState._PendingRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed);

            return;
        }

        // Published by SWAPPING the pointer. Whoever is holding the previous field keeps reading it,
        // whole, for as long as they hold it.
        InBuiltField._Epoch = Completed->_Epoch;
        InBuiltField._Field = MoveTemp(Completed);

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

        // A published field nobody can find from a world answers nothing: this is what the NavSurface
        // provider adapter resolves against.
        groundnav::world_fields::Publish(World, InVolumeEntity, InBuiltField._Field);

        // An invalid box goes out AS IS when no tile built: bounds-unknown is the honest payload, where
        // substituting the volume's own bounds would name ground this publish never produced.
        nav_surface::Request_NotifySurfaceRebuilt(World,
            groundnav::Get_ChangedTileBounds(*InBuiltField._Field, InBuiltField._Epoch));

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_Built>();

        InBuildState._PendingRequest.TryFireCompletion(
            InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_MarkupCostDerive::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Markup& InMarkup,
            FFragment_GroundNavVolume_BuiltField& InBuiltField) const
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        InVolumeEntity.Remove<MarkedDirtyBy>();

        const auto Published = InBuiltField.Get_Field();

        // Nothing published is nothing to derive from, and nothing to repair either: the records live
        // on the volume, so the first build prices them through its own params.
        if (NOT Published.IsValid())
        { return; }

        const auto Records = Get_MarkupRecordsOf(InMarkup.Get_Entries());

        const auto Derived = groundnav::Get_FieldWithMarkupCost(
            *Published, Records, InBuiltField.Get_Epoch().Get_Next());

        const auto DeriveProducedAField = Derived.Value.Get_IsCompleted() && Derived.Key.IsValid();

        CK_ENSURE_IF_NOT(DeriveProducedAField,
            TEXT("GroundNav Volume [{}] could not derive a cost-only field from what it has published: [{}]"),
            InVolumeEntity, Derived.Value.Get_Status())
        { return; }

        // A restamp that lands on the labels already published moves no epoch, so there is nothing for
        // a reader to notice and nothing worth swapping a pointer for.
        if (NOT Derived.Key->_Epoch.Get_IsNewerThan(InBuiltField.Get_Epoch()))
        { return; }

        // The same swap the build publishes through: what is out stays out, whole, for whoever holds it.
        InBuiltField._Epoch = Derived.Key->_Epoch;
        InBuiltField._Field = Derived.Key;

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

        groundnav::world_fields::Publish(World, InVolumeEntity, InBuiltField._Field);

        // Past the no-change early-out above, so this notify is only ever raised for a publish that
        // moved something. An invalid box is reported as-is for the same reason the build reports one.
        nav_surface::Request_NotifySurfaceRebuilt(World,
            groundnav::Get_ChangedTileBounds(*InBuiltField._Field, InBuiltField._Epoch));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_MarkupWalkabilityRebuild::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity)
        -> void
    {
        InVolumeEntity.Remove<MarkedDirtyBy>();

        auto Volume = InVolumeEntity;

        UCk_Utils_GroundNavVolume_UE::Request_Build(Volume,
            FCk_Request_GroundNavVolume_Build{}.Set_ForceRestart(ECk_EnableDisable::Enable), {});
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            const FFragment_GroundNavVolume_Requests& InRequests)
        -> void
    {
        // Two separate populations, and missing either one strands a caller. The QUEUE holds requests
        // the drain never reached; _PendingRequest holds the delegate that has been riding the
        // multi-tick build and is the one a caller is most likely actually waiting on.
        request::FireCancelledForPending(InVolumeEntity, InRequests.Get_Requests());

        InBuildState._PendingRequest.TryFireCompletion(
            InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);

        // Drops the pinned physics session with the entity rather than leaving it to fragment teardown.
        InBuildState._Backend.Reset();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_CancelPendingMarkupRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_MarkupRequests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InVolumeEntity, InRequests.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
