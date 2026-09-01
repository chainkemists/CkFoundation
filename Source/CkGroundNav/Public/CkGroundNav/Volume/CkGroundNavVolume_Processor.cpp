#include "CkGroundNavVolume_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkGroundNav/CkGroundNav_Log.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_StartBuild);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_Build);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace ck_groundnav_volume_processor
    {
        auto Get_FieldParams(
            const FFragment_GroundNavVolume_Params& InParams) -> groundnav::FCk_GroundNav_FieldParams
        {
            auto FieldParams = groundnav::FCk_GroundNav_FieldParams{};

            const auto Bounds = InParams.Get_VolumeBounds();

            FieldParams._OriginXY = FVector2D{Bounds.Min.X, Bounds.Min.Y};
            FieldParams._MinZUu = static_cast<float>(Bounds.Min.Z);
            FieldParams._MaxZUu = static_cast<float>(Bounds.Max.Z);
            FieldParams._Config = InParams.Get_Config();
            FieldParams._Profile = InParams.Get_Profile();
            FieldParams._MergeTunables = InParams.Get_MergeTunables();
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

        auto Get_ParamsAreBakeable(const FFragment_GroundNavVolume_Params& InParams) -> bool
        {
            const auto Bounds = InParams.Get_VolumeBounds();

            return Bounds.IsValid != 0 &&
                   Bounds.GetSize().X > 0.0 && Bounds.GetSize().Y > 0.0 && Bounds.GetSize().Z > 0.0 &&
                   Get_FieldParams(InParams).Get_IsValid() &&
                   InParams.Get_ProbeBudgetPerTick() > 0;
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

        CK_ENSURE_IF_NOT(ParamsAreBakeable,
            TEXT("GroundNav Volume [{}] cannot be baked. Bounds [{}], cell size [{}]uu, tile size [{}]uu, "
                 "clearance ceiling [{}]uu, probe budget [{}]"),
            InVolumeEntity, InParams.Get_VolumeBounds(), InParams.Get_Config().Get_CellSizeUu(),
            InParams.Get_Config().Get_TileSizeUu(), InParams.Get_MaxClearanceUu(),
            InParams.Get_ProbeBudgetPerTick())
        {}

        // The volume stays inert rather than arming a build that would bake nonsense. A later
        // Request_Build on it fails loudly through the same validation.
        if (NOT ParamsAreBakeable)
        { return; }

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
        const auto BeginResult = groundnav::Request_BeginBuild(
            Get_FieldParams(InParams),
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

        InVolumeEntity.AddOrGet<FTag_GroundNavVolume_Built>();

        InBuildState._PendingRequest.TryFireCompletion(
            InVolumeEntity, ECk_Request_OperationResult::Succeeded);
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
}

// --------------------------------------------------------------------------------------------------------------------
