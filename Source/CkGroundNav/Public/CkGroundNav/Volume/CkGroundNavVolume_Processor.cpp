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
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_HandleRepairRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_HandleMarkupRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_StartBuild);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_Build);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_StartRepair);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_Repair);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_MarkupCostDerive);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_CancelPendingRepairRequests);
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

        // A box that bounds nothing must not be unioned in: FBox::operator+ takes the union with a box
        // at the origin, which would drag every dirty region back to the world centre.
        auto Get_UnionedBounds(
            const FBox& InAccumulated,
            const FBox& InAdded) -> FBox
        {
            if (InAdded.IsValid == 0)
            { return InAccumulated; }

            return InAccumulated.IsValid != 0 ? InAccumulated + InAdded : InAdded;
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
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_Requests& InRequests) const
        -> void
    {
        ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InVolumeEntity, InParams, InBuildState, InRepairState, InRequest);
            }));
    }

    auto
        FProcessor_GroundNavVolume_HandleRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            FFragment_GroundNavVolume_RepairState& InRepairState,
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

        DoCancel_RepairInFlight(InVolumeEntity, InRepairState);

        InBuildState._Backend.Reset();
        InBuildState._PendingRequest = InRequest;

        InVolumeEntity.Try_Remove<FTag_GroundNavVolume_BuildInProgress>();
        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsBuild>();
    }

    auto
        FProcessor_GroundNavVolume_HandleRequests::
        DoCancel_RepairInFlight(
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_RepairState& InRepairState)
        -> void
    {
        if (NOT InVolumeEntity.Has<FTag_GroundNavVolume_RepairInProgress>())
        { return; }

        for (const auto& InFlightRequest : InRepairState._InFlightRequests)
        {
            InFlightRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);
        }

        InRepairState._InFlightRequests.Reset();
        InRepairState._Repair = {};
        InRepairState._Backend.Reset();

        // A build taking a repair off the volume is no evidence its region was failing, so the retry
        // budget is not spent by one.
        InRepairState._StaleRetryCount = 0;

        InVolumeEntity.Remove<FTag_GroundNavVolume_RepairInProgress>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_HandleRepairRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_RepairRequests& InRequests) const
        -> void
    {
        ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InVolumeEntity, InBuiltField, InRepairState, InRequest);
            }));
    }

    auto
        FProcessor_GroundNavVolume_HandleRepairRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            const FCk_Request_GroundNavVolume_Repair& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        const auto BuildWillAnswerIt = InVolumeEntity.Has<FTag_GroundNavVolume_NeedsBuild>() ||
                                       InVolumeEntity.Has<FTag_GroundNavVolume_BuildInProgress>();

        // A repair carries every tile OUTSIDE its dirty box over from a previous bake, so a volume with
        // nothing published has nothing to carry and no repair to make. A retry does not change that -
        // a build is what bakes that ground - which is what Failed means. Unless a build is already
        // coming: then the region rides it and completes when it publishes.
        if (NOT InBuiltField.Get_Field().IsValid() && NOT BuildWillAnswerIt)
        {
            groundnav::Verbose(
                TEXT("GroundNav Volume [{}] was handed a dirty region [{}] before it published a field - "
                     "dropping the region; a build is what bakes that ground"),
                InVolumeEntity, InRequest.Get_DirtyBounds());

            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed);
            return;
        }

        // Several bodies may dirty one volume in a frame, and one repair over their union costs less
        // than one repair each.
        InRepairState._PendingDirtyBounds =
            Get_UnionedBounds(InRepairState._PendingDirtyBounds, InRequest.Get_DirtyBounds());

        InRepairState._PendingRequests.Emplace(InRequest);

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsRepair>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_HandleMarkupRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_Markup& InMarkup,
            FFragment_GroundNavVolume_MarkupRequests& InRequests) const
        -> void
    {
        ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InVolumeEntity, InBuiltField, InRepairState, InMarkup, InRequest);
            }));
    }

    auto
        FProcessor_GroundNavVolume_HandleMarkupRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
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

        // The WHOLE previous record, because an update that MOVED one leaves the ground it used to
        // cover decided by nothing, and a copy is needed anyway: the entry is overwritten below.
        const auto PreviousRecord = EntryExists
            ? TOptional<FCk_GroundNav_MarkupRecord>{InMarkup._Entries[ExistingIndex].Get_Record()}
            : TOptional<FCk_GroundNav_MarkupRecord>{};

        const auto RecordId = PreviousRecord.IsSet()
            ? PreviousRecord->Get_Id()
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

        // The record's OLD footprint owes its old kind an answer whether the update moved it, retagged
        // it or changed nothing: ground the previous record was deciding is decided by the new one only
        // where the two overlap, and a footprint left behind is ground nothing else will ever revisit.
        if (PreviousRecord.IsSet())
        {
            DoMark_MarkupDirty(InVolumeEntity, InBuiltField, InRepairState, PreviousRecord->Get_Kind(),
                groundnav::Get_MarkupWorldBounds(*PreviousRecord));
        }

        DoMark_MarkupDirty(InVolumeEntity, InBuiltField, InRepairState, Kind,
            groundnav::Get_MarkupWorldBounds(Record));

        InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    auto
        FProcessor_GroundNavVolume_HandleMarkupRequests::
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_Markup& InMarkup,
            const FCk_Request_GroundNavVolume_ReleaseAreaMarkup& InRequest)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        auto MarkupEntity = InRequest.Get_MarkupEntity();

        // A release is not guarded on the entity being alive, because the ordinary release IS that
        // entity's own teardown: the neutral NavSurface EndPlay asks the provider to release, and the
        // volume drains that request a frame later, with the entity already dead. The entry is keyed on
        // the entity's IDENTITY - which FCk_Handle equality answers from the entity id and registry
        // alone, never from validity - so a dead handle still finds the entry stored beside its twin.
        // Refusing one here refuses the single moment a record must still be findable, and the ground
        // it covers is then decided by a record nothing will ever release.
        const auto ExistingIndex = Get_MarkupEntryIndex(InMarkup, MarkupEntity);

        // Releasing a record the volume does not hold leaves the caller's intent holding afterwards,
        // which is what Succeeded means.
        if (NOT InMarkup._Entries.IsValidIndex(ExistingIndex))
        {
            InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
            return;
        }

        // Copied before the entry goes: the ground the release changed is the ground the record covered,
        // and after RemoveAt there is nothing left to ask.
        const auto ReleasedRecord = InMarkup._Entries[ExistingIndex].Get_Record();

        InMarkup._Entries.RemoveAt(ExistingIndex);

        // Only where there is still an entity to drop it from: a dead one carries no fragments, and the
        // record just removed above was the only state that outlived it.
        if (ck::IsValid(MarkupEntity))
        { MarkupEntity.Try_Remove<FFragment_GroundNav_MarkupRef>(); }

        // Ground a released record was deciding is decided by nothing now, which is as much a change of
        // that record's kind as painting it was.
        DoMark_MarkupDirty(InVolumeEntity, InBuiltField, InRepairState, ReleasedRecord.Get_Kind(),
            groundnav::Get_MarkupWorldBounds(ReleasedRecord));

        InRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    auto
        FProcessor_GroundNavVolume_HandleMarkupRequests::
        DoMark_MarkupDirty(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            ECk_GroundNav_MarkupKind InKind,
            const FBox& InMarkupWorldBounds)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        if (InKind == ECk_GroundNav_MarkupKind::Cost)
        {
            InVolumeEntity.AddOrGet<FTag_GroundNavVolume_MarkupCostDirty>();
            return;
        }

        // A volume with nothing published has no field for a repair to carry its untouched tiles over
        // from. The record is already on the volume, so the first build bakes it in through
        // FCk_GroundNav_FieldParams::_MarkupRecords - the same wait the cost derive makes, and the
        // reason a paint before the first bake is never lost.
        if (NOT InBuiltField.Get_Field().IsValid())
        { return; }

        InRepairState._PendingDirtyBounds =
            Get_UnionedBounds(InRepairState._PendingDirtyBounds, InMarkupWorldBounds);

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsRepair>();
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
            FFragment_GroundNavVolume_RepairState& InRepairState,
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

        // A completed build re-baked every tile from live geometry, so every dirty region waiting on a
        // repair has already been answered - and answered better. Opening a repair for them now would
        // spend probes re-deriving ground this build just published, so their callers' intent holds and
        // they complete. _InFlightRequests is empty here by the drain's own rule - arming a build cancels
        // an open repair - and is drained anyway so a broken invariant strands nobody.
        for (const auto& ParkedRepairRequest : InRepairState._PendingRequests)
        { ParkedRepairRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded); }

        for (const auto& InFlightRepairRequest : InRepairState._InFlightRequests)
        { InFlightRepairRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Succeeded); }

        InRepairState._PendingRequests.Reset();
        InRepairState._InFlightRequests.Reset();
        InRepairState._PendingDirtyBounds = FBox{ForceInit};
        InRepairState._StaleRetryCount = 0;

        InVolumeEntity.Try_Remove<FTag_GroundNavVolume_NeedsRepair>();

        InBuildState._PendingRequest.TryFireCompletion(
            InVolumeEntity, ECk_Request_OperationResult::Succeeded);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_StartRepair::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState) const
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        InVolumeEntity.Remove<MarkedDirtyBy>();

        // Snapshotted and cleared in ONE step. The tile set a repair fixes at Begin is the set this box
        // selects and no other, so a box arriving from here on belongs to the next repair - which is
        // also why the requests that named this box move with it.
        const auto DirtyBounds = InRepairState._PendingDirtyBounds;

        InRepairState._PendingDirtyBounds = FBox{ForceInit};

        auto Parked = MoveTemp(InRepairState._PendingRequests);
        InRepairState._PendingRequests.Reset();

        const auto DoFailParked = [&]() -> void
        {
            for (const auto& ParkedRequest : Parked)
            { ParkedRequest.TryFireCompletion(InVolumeEntity, ECk_Request_OperationResult::Failed); }

            InRepairState._Repair = {};
            InRepairState._Backend.Reset();
            InRepairState._StaleRetryCount = 0;
        };

        const auto Published = InBuiltField.Get_Field();

        // Reachable without a defect: the drain parks a region behind a build when nothing is published
        // yet, and that build can fail. There is still no field to carry the untouched tiles over from,
        // and the next build - not a repair - is what bakes this ground.
        if (NOT Published.IsValid())
        {
            groundnav::Verbose(
                TEXT("GroundNav Volume [{}] holds a dirty region [{}] but has published no field to "
                     "repair - dropping the region; a build is what bakes that ground"),
                InVolumeEntity, DirtyBounds);

            DoFailParked();
            return;
        }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

        InRepairState._Backend = MakeUnique<groundnav::FCk_GroundNav_GeometryBackend_Jolt>(World);

        const auto BackendIsUsable = InRepairState._Backend->Get_IsValid();

        // Same reasoning as the build's, and worse here: a backend that cannot answer reports no
        // geometry, so a repair accepting one would re-bake the dirty tiles as open ground exactly where
        // something was reported to have changed.
        CK_ENSURE_IF_NOT(BackendIsUsable,
            TEXT("GroundNav Volume [{}] cannot start a repair: no physics world to read geometry from"),
            InVolumeEntity)
        {
            DoFailParked();
            return;
        }

        // The markup goes in HERE and not at the request, exactly as StartBuild does it, so a repair
        // bakes its tiles against what the volume currently holds. The records the SOURCE field's params
        // carry are the ones the last publish baked with - re-baking a dirty tile under those would
        // answer the very markup change that raised the region with the records it replaced. Only the
        // records are replaced: every other field of the source params places the lattice the untouched
        // tiles were produced on.
        const auto MarkupRecords =
            Get_MarkupRecordsOf(UCk_Utils_GroundNavVolume_UE::Get_MarkupRecords(InVolumeEntity));

        const auto BeginResult = groundnav::Request_BeginRepair(
            InRepairState._Repair, Published, DirtyBounds, InBuiltField.Get_Epoch().Get_Next(),
            MarkupRecords);

        CK_ENSURE_IF_NOT(BeginResult.Get_IsCompleted(),
            TEXT("GroundNav Volume [{}] could not begin a repair of [{}]: [{}]"),
            InVolumeEntity, DirtyBounds, BeginResult.Get_Status())
        {
            DoFailParked();
            return;
        }

        InRepairState._InFlightRequests = MoveTemp(Parked);

        // A box that reached no tile leaves the repair already whole. It is still handed to the slice
        // below rather than published from here, so there is exactly ONE place a repair publishes from.
        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_RepairInProgress>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavVolume_Repair::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_BuiltField& InBuiltField) const
        -> void
    {
        const auto BackendIsHeld = InRepairState._Backend.IsValid();

        CK_ENSURE_IF_NOT(BackendIsHeld,
            TEXT("GroundNav Volume [{}] is marked as repairing but holds no geometry backend"),
            InVolumeEntity)
        {
            DoEnd(InVolumeEntity, InRepairState, ECk_Request_OperationResult::Failed);
            return;
        }

        const auto SliceResult = groundnav::Request_AdvanceRepair(
            *InRepairState._Backend, InParams.Get_ProbeBudgetPerTick(), InRepairState._Repair);

        if (SliceResult.Get_Status() == ECk_GroundNav_BakeStatus::BudgetExhausted)
        { return; }

        // Read before anything spends the state that holds it.
        const auto SnapshotBounds = InRepairState._Repair.Get_DirtyBounds();

        if (NOT SliceResult.Get_IsCompleted())
        {
            groundnav::Display(TEXT("GroundNav Volume [{}] failed a local repair of [{}]: [{}]"),
                InVolumeEntity, SnapshotBounds, SliceResult.Get_Status());

            DoEnd(InVolumeEntity, InRepairState, ECk_Request_OperationResult::Failed);
            DoRetain_FailedRegion(InVolumeEntity, InRepairState, SnapshotBounds);

            return;
        }

        // A repair carried every tile outside its box across from the ONE field it opened against.
        // Publishing it over a field somebody swapped in meanwhile would silently undo that publish
        // everywhere the repair did not look, which is the one corruption this representation is
        // otherwise unable to express.
        const auto SourceIsStillPublished = InRepairState._Repair.Get_Source() == InBuiltField.Get_Field();

        if (NOT SourceIsStillPublished)
        {
            groundnav::Display(
                TEXT("GroundNav Volume [{}] lost a race on its local repair of [{}]: the field it opened "
                     "against was replaced while it was slicing, so the repaired field is dropped and the "
                     "region raised again"),
                InVolumeEntity, SnapshotBounds);

            DoEnd(InVolumeEntity, InRepairState, ECk_Request_OperationResult::Failed);
            DoRetain_FailedRegion(InVolumeEntity, InRepairState, SnapshotBounds);

            return;
        }

        auto Repaired = groundnav::Request_ReleaseRepairedField(InRepairState._Repair);

        const auto RepairProducedAField = Repaired.IsValid();

        CK_ENSURE_IF_NOT(RepairProducedAField,
            TEXT("GroundNav Volume [{}] completed a local repair of [{}] that yielded no field"),
            InVolumeEntity, SnapshotBounds)
        {
            DoEnd(InVolumeEntity, InRepairState, ECk_Request_OperationResult::Failed);
            DoRetain_FailedRegion(InVolumeEntity, InRepairState, SnapshotBounds);

            return;
        }

        InRepairState._StaleRetryCount = 0;

        // A box that reached no tile re-bakes nothing and moves no epoch. Swapping a byte-identical
        // field in would announce a rebuild that changed no ground, so the honest answer is the one the
        // cost derive gives when nothing it restamped moved: succeed, and publish nothing.
        if (Repaired->_Epoch.Get_IsNewerThan(InBuiltField.Get_Epoch()))
        {
            // The same swap the build publishes through: what is out stays out, whole, for whoever holds it.
            InBuiltField._Epoch = Repaired->_Epoch;
            InBuiltField._Field = MoveTemp(Repaired);

            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InVolumeEntity);

            groundnav::world_fields::Publish(World, InVolumeEntity, InBuiltField._Field);

            // Only the re-baked tiles carry the new epoch, so this names exactly the ground the repair
            // touched and nothing besides.
            nav_surface::Request_NotifySurfaceRebuilt(World,
                groundnav::Get_ChangedTileBounds(*InBuiltField._Field, InBuiltField._Epoch));
        }

        DoEnd(InVolumeEntity, InRepairState, ECk_Request_OperationResult::Succeeded);

        // Arrived while this repair was slicing, so it names ground this repair never re-read.
        if (InRepairState._PendingDirtyBounds.IsValid != 0)
        { InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsRepair>(); }
    }

    auto
        FProcessor_GroundNavVolume_Repair::
        DoEnd(
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            ECk_Request_OperationResult InResult)
        -> void
    {
        InVolumeEntity.Remove<FTag_GroundNavVolume_RepairInProgress>();

        InRepairState._Backend.Reset();
        InRepairState._Repair = {};

        for (const auto& InFlightRequest : InRepairState._InFlightRequests)
        { InFlightRequest.TryFireCompletion(InVolumeEntity, InResult); }

        InRepairState._InFlightRequests.Reset();
    }

    auto
        FProcessor_GroundNavVolume_Repair::
        DoRetain_FailedRegion(
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            const FBox& InFailedBounds)
        -> void
    {
        using namespace ck_groundnav_volume_processor;

        const auto RegionMayBeRetried = InRepairState._StaleRetryCount == 0;

        // Fail-closed with a BOUNDED escape. A region that fails twice running is failing for a reason
        // another attempt does not address, and one re-raised forever would open the same doomed repair
        // every tick for the life of the volume.
        CK_ENSURE_IF_NOT(RegionMayBeRetried,
            TEXT("GroundNav Volume [{}] failed a local repair of [{}] twice in a row - dropping the "
                 "region. The ground it covers keeps whatever the last successful bake published, which "
                 "is no longer trustworthy; a full build is what recovers it"),
            InVolumeEntity, InFailedBounds)
        {
            InRepairState._StaleRetryCount = 0;
            return;
        }

        ++InRepairState._StaleRetryCount;

        InRepairState._PendingDirtyBounds =
            Get_UnionedBounds(InRepairState._PendingDirtyBounds, InFailedBounds);

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_NeedsRepair>();
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
        FProcessor_GroundNavVolume_CancelPendingRepairRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            const FFragment_GroundNavVolume_RepairRequests& InRequests)
        -> void
    {
        // Three separate populations, and missing any one strands a caller. The QUEUE holds requests the
        // drain never reached, _PendingRequests those it parked against a repair that will now never
        // open, and _InFlightRequests the delegates that have been riding one mid-slice.
        request::FireCancelledForPending(InVolumeEntity, InRequests.Get_Requests());

        for (const auto& ParkedRequest : InRepairState._PendingRequests)
        {
            ParkedRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);
        }

        for (const auto& InFlightRequest : InRepairState._InFlightRequests)
        {
            InFlightRequest.TryFireCompletion(
                InVolumeEntity, ECk_Request_OperationResult::Failed_Cancelled);
        }

        InRepairState._PendingRequests.Reset();
        InRepairState._InFlightRequests.Reset();

        // Drops the pinned physics session with the entity rather than leaving it to fragment teardown.
        InRepairState._Backend.Reset();
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
