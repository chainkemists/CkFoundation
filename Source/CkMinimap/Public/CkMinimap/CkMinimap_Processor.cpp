#include "CkMinimap_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkCamera/Camera/CkCamera_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkEntityTag/CkEntityTag_Utils.h"

#include "CkMinimap/CkFogOfWar_Utils.h"
#include "CkMinimap/CkMinimap_Log.h"
#include "CkMinimap/CkMinimap_Utils.h"

#include "CkPoi/CkPoi_Utils.h"

#include "CkPoiDisplayDefinition/CkPoiDisplayDefinition_Utils.h"

#include "CkVisibleRange/CkVisibleRange_Utils.h"

#include "Async/ParallelFor.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("CkMinimap"), STATGROUP_CkMinimap, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("Minimap::Projection"),  STAT_CkMinimap_Projection,  STATGROUP_CkMinimap);
DECLARE_CYCLE_STAT(TEXT("Minimap::DiffSignals"), STAT_CkMinimap_DiffSignals, STATGROUP_CkMinimap);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_minimap_processor
{
    // Per-index distance-feed record written by a worker into its own slot, consumed post-parallel on the
    // calling thread. Distance is always set once computed (before any range/fog cull early-return); the
    // DisplayDefinition is the resolved consumer child (invalid when this POI has none for this consumer).
    struct FCk_Minimap_PoiFeed
    {
        float Distance = 0.0f;
        FCk_Handle DisplayDefinition;
    };
}

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Minimap_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Minimap_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Minimap_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_Minimap_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Minimap_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMinimapEntity,
            const FFragment_Minimap_Params& InParams,
            FFragment_Minimap_Current& InCurrent)
        -> void
    {
        InMinimapEntity.Remove<MarkedDirtyBy>();

        // Direct-attach default: the minimap entity IS the observer (mirrors the compass). A standalone
        // minimap composed via Create points its observer at the lifetime owner through a SetObserver request.
        if (ck::Is_NOT_Valid(InCurrent._Observer))
        {
            InCurrent._Observer = InMinimapEntity;
        }

        InCurrent._ViewExtent = InParams.Get_ViewExtent();

        CK_ENSURE_IF_NOT(InParams.Get_ViewExtent() > 0.0f,
            TEXT("Minimap [{}] ViewExtent [{}] must be > 0 — falling back to 1cm"),
            InMinimapEntity, InParams.Get_ViewExtent())
        {
            InCurrent._ViewExtent = 1.0f;
        }

        if (InParams.Get_ProjectionMode() == ECk_Minimap_ProjectionMode::FixedBounds)
        {
            CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_FixedBounds()),
                TEXT("Minimap [{}] uses FixedBounds projection but its FixedBounds half-extents are not > 0. "
                     "Every entry will project to the frame center until the minimap is re-Added with valid bounds"),
                InMinimapEntity)
            {}
        }

        InCurrent._RotationMode = InParams.Get_RotationMode();

        // Force the first projection pass to run immediately regardless of the update interval
        InCurrent._TimeSinceUpdate = FCk_Time{TNumericLimits<double>::Max()};
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Minimap_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Params& InParams,
            FFragment_Minimap_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InMinimapEntity, InCurrent, InParams, InRequest);

            if (InRequest.Get_IsRequestHandleValid())
            {
                InRequest.GetAndDestroyRequestHandle();
            }
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        {
            InMinimapEntity.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_Minimap_HandleRequests::
        DoHandleRequest(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Params& InParams,
            const FCk_Request_Minimap_SetViewExtent& InRequest)
        -> void
    {
        minimap::VeryVerbose(TEXT("Handling SetViewExtent Request for Minimap with Entity [{}]"), InMinimapEntity);

        CK_ENSURE_IF_NOT(InRequest.Get_ViewExtent() > 0.0f,
            TEXT("Minimap [{}] SetViewExtent [{}] must be > 0 — request rejected"),
            InMinimapEntity, InRequest.Get_ViewExtent())
        { return; }

        InCurrent._ViewExtent = InRequest.Get_ViewExtent();

        // Zoom changes must reflect immediately, not at the next throttled interval
        InCurrent._TimeSinceUpdate = FCk_Time{TNumericLimits<double>::Max()};
    }

    auto
        FProcessor_Minimap_HandleRequests::
        DoHandleRequest(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Params& InParams,
            const FCk_Request_Minimap_SetCategoryFilter& InRequest)
        -> void
    {
        minimap::VeryVerbose(TEXT("Handling SetCategoryFilter Request for Minimap with Entity [{}]"), InMinimapEntity);

        InParams.Set_CategoryFilter(InRequest.Get_CategoryFilter());

        InCurrent._TimeSinceUpdate = FCk_Time{TNumericLimits<double>::Max()};
    }

    auto
        FProcessor_Minimap_HandleRequests::
        DoHandleRequest(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Params& InParams,
            const FCk_Request_Minimap_SetObserver& InRequest)
        -> void
    {
        minimap::VeryVerbose(TEXT("Handling SetObserver Request for Minimap with Entity [{}]"), InMinimapEntity);

        InCurrent._Observer = ck::IsValid(InRequest.Get_Observer())
            ? InRequest.Get_Observer()
            : static_cast<const FCk_Handle&>(InMinimapEntity);

        InCurrent._TimeSinceUpdate = FCk_Time{TNumericLimits<double>::Max()};
    }

    auto
        FProcessor_Minimap_HandleRequests::
        DoHandleRequest(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Params& InParams,
            const FCk_Request_Minimap_SetRotationMode& InRequest)
        -> void
    {
        minimap::VeryVerbose(TEXT("Handling SetRotationMode Request for Minimap with Entity [{}]"), InMinimapEntity);

        InCurrent._RotationMode = InRequest.Get_RotationMode();

        InCurrent._TimeSinceUpdate = FCk_Time{TNumericLimits<double>::Max()};
    }

    auto
        FProcessor_Minimap_HandleRequests::
        DoHandleRequest(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Params& InParams,
            const FCk_Request_Minimap_SetFogOfWar& InRequest)
        -> void
    {
        minimap::VeryVerbose(TEXT("Handling SetFogOfWar Request for Minimap with Entity [{}]"), InMinimapEntity);

        InCurrent._FogOfWar = InRequest.Get_FogOfWar();

        InCurrent._TimeSinceUpdate = FCk_Time{TNumericLimits<double>::Max()};
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Minimap_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMinimapEntity,
            const FFragment_Minimap_Params& InParams,
            FFragment_Minimap_Current& InCurrent) const
        -> void
    {
        // Observers die (pawn destroyed, possession changed) as part of normal play — degrade silently,
        // drain the entries so bound UIs empty their pools, and wait for a Request_SetObserver.
        // FixedBounds minimaps degrade the same way (recorded decision: no observer-less world map)
        if (ck::Is_NOT_Valid(InCurrent._Observer))
        {
            DoClearAllEntries(InMinimapEntity, InCurrent);
            return;
        }

        const auto& Observer = InCurrent._Observer;

        auto ObserverTransform = UCk_Utils_Transform_UE::Cast(Observer);

        CK_ENSURE_IF_NOT(ck::IsValid(ObserverTransform),
            TEXT("Minimap [{}] Observer [{}] has no Transform feature. The Minimap cannot project POIs without an observer position"),
            InMinimapEntity, Observer)
        {
            DoClearAllEntries(InMinimapEntity, InCurrent);
            return;
        }

        InCurrent._TimeSinceUpdate += InDeltaT;

        const auto UpdateInterval = InParams.Get_UpdateInterval();

        // UNLIKE the compass there is no unthrottled channel — view origin/yaw and every entry position
        // go stale together between updates (delivery contract point; default interval is 0)
        if (UpdateInterval > FCk_Time::ZeroSecond() && InCurrent._TimeSinceUpdate < UpdateInterval)
        { return; }

        InCurrent._TimeSinceUpdate = FCk_Time::ZeroSecond();

        InCurrent._ViewOrigin = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(ObserverTransform);
        InCurrent._ViewYawDegrees = FRotator::ClampAxis(DoResolveViewYaw(Observer, InCurrent));

        {
            SCOPE_CYCLE_COUNTER(STAT_CkMinimap_Projection);
            DoProjectPois(InMinimapEntity, InParams, InCurrent);
        }

        {
            SCOPE_CYCLE_COUNTER(STAT_CkMinimap_DiffSignals);
            DoDiffAndPublishEntries(InMinimapEntity, InCurrent);
        }
    }

    auto
        FProcessor_Minimap_Update::
        DoResolveViewYaw(
            const FCk_Handle& InObserver,
            const FFragment_Minimap_Current& InCurrent)
        -> float
    {
        // Same resolution order as the compass' Auto heading: composed camera POV wins, transform yaw is
        // the fallback (the observer is known transform-valid — the caller gated on it)
        if (const auto ObserverCamera = UCk_Utils_Camera_UE::Cast(InObserver);
            ck::IsValid(ObserverCamera))
        {
            return UCk_Utils_Camera_UE::Get_ViewRotation(ObserverCamera).Yaw;
        }

        if (const auto ObserverTransform = UCk_Utils_Transform_UE::Cast(InObserver);
            ck::IsValid(ObserverTransform))
        {
            return UCk_Utils_Transform_UE::Get_EntityCurrentRotation(ObserverTransform).Yaw;
        }

        return InCurrent._ViewYawDegrees;
    }

    auto
        FProcessor_Minimap_Update::
        DoProjectPois(
            HandleType InMinimapEntity,
            const FFragment_Minimap_Params& InParams,
            FFragment_Minimap_Current& InCurrent)
        -> void
    {
        auto& Scratch = InCurrent._ScratchEntries;
        Scratch.Reset();

        const auto& CategoryFilter = InParams.Get_CategoryFilter();
        const auto FilterIsEmpty = CategoryFilter.IsEmpty();
        const auto ProjectionMode = InParams.Get_ProjectionMode();
        const auto FrameShape = InParams.Get_FrameShape();
        const auto& FixedBounds = InParams.Get_FixedBounds();
        const auto& ViewOrigin = InCurrent._ViewOrigin;
        const auto ViewYawDegrees = InCurrent._ViewYawDegrees;
        const auto ViewExtent = InCurrent._ViewExtent;
        const auto RotationMode = InCurrent._RotationMode;
        const auto& FogOfWar = InCurrent._FogOfWar;

        // Manual view over every POI in this world's registry, keyed on the FTag_Poi identity. The four
        // pending-kill excludes matter: fragments survive until destruction Finalize (~2 ticks after
        // Destroy) — without them, dying POIs would linger on the minimap. Initiate-frame POIs are
        // deliberately still projected (same policy as CK_IGNORE_PENDING_KILL).
        //
        // Base-entity FTag_VisibleRange_Hidden is consumed as a WORKER skip (below, after the distance
        // record), deliberately NOT a view exclude — an excluded hidden POI would stop receiving the
        // distance feed and could never re-evaluate back to visible. The inline max-range cull further
        // down still runs for blip-free same-frame membership. The EntityTag disable convention stays a
        // per-worker skip too.
        //
        // The POI set is GATHERED first, then projected data-parallel: the per-POI body is pure registry
        // READS (EntityTag/VisibleRange/DisplayDefinition state, own transforms, fog grid) whose writers all
        // ran in earlier groups, and each worker writes only its own index slot. Signals/sort/diff and every
        // Update_Distance feed stay on the calling thread.
        auto& PoiEntities = InCurrent._ScratchPoiEntities;
        PoiEntities.Reset();

        InMinimapEntity.View<
            ck::FTag_Poi,
            ck::TExclude<ck::FTag_DestroyEntity_EndPlay>,
            ck::TExclude<ck::FTag_DestroyEntity_Teardown>,
            ck::TExclude<ck::FTag_DestroyEntity_Await>,
            ck::TExclude<ck::FTag_DestroyEntity_Finalize>>().ForEach(
        [&](const auto InPoiEntity) -> void
        {
            PoiEntities.Add(InPoiEntity);
        });

        auto& Slots = InCurrent._ScratchParallelSlots;
        Slots.Reset();
        Slots.SetNum(PoiEntities.Num());

        // Per-index feed scratch, mirroring Slots: a worker records its POI's distance (+ resolved consumer
        // DisplayDefinition) into its own slot; the calling-thread loop after the ParallelFor drives every
        // Update_Distance. Kept a local (not a fragment member) — the record type is a filename-namespace
        // struct, and the feed set is consumed and discarded within this one pass.
        TArray<TOptional<ck_minimap_processor::FCk_Minimap_PoiFeed>> FeedSlots;
        FeedSlots.SetNum(PoiEntities.Num());

        // Below this the fan-out overhead exceeds the projection math — same body runs inline
        constexpr auto MinPoisForParallel = 64;
        const auto ForceSingleThread = PoiEntities.Num() < MinPoisForParallel;

        ParallelFor(PoiEntities.Num(),
        [&](const int32 InIndex)
        {
            const auto PoiGenericHandle = InMinimapEntity.Get_ValidHandle(PoiEntities[InIndex].Get_ID());

            // Disabled POIs are excluded via the EntityTag convention tag (absence-safe: Has returns false
            // when the store isn't present yet — the disable add is deferred one pump).
            if (UCk_Utils_EntityTag_UE::Has_UsingGameplayTag(PoiGenericHandle, Tag_Poi_DisabledName))
            { return; }

            if (NOT FilterIsEmpty && NOT CategoryFilter.Matches(UCk_Utils_EntityTag_UE::Get_AllTagsAsContainer(PoiGenericHandle)))
            { return; }

            // POI position = the POI entity's own Transform location (direct-attach: the POI entity carries
            // the Transform — see CkPoi's composition contract)
            const auto PoiTransformHandle = UCk_Utils_Transform_UE::Cast(PoiGenericHandle);

            if (ck::Is_NOT_Valid(PoiTransformHandle))
            { return; }

            const auto PoiWorldTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(PoiTransformHandle);
            const auto PoiLocation = PoiWorldTransform.GetLocation();

            const auto Distance = static_cast<float>(FVector::Dist(ViewOrigin, PoiLocation));

            // Record the distance for the post-parallel feed BEFORE any subsequent early-return (the max-range
            // and fog culls below can bail). A culled entry still feeds its base VR its distance — fine; its
            // DisplayDefinition stays unresolved (invalid), so only the base VR is fed for it.
            FeedSlots[InIndex].Emplace(ck_minimap_processor::FCk_Minimap_PoiFeed{Distance});

            // Base-entity hidden state (explicit Request_SetVisibility, or its own range vote once fed) is a
            // WORKER skip, deliberately NOT a view exclude: a view-excluded hidden POI would stop receiving
            // the distance feed above and could never re-evaluate back to visible when the observer returns
            // into range. Skips here, keeps feeding.
            if (PoiGenericHandle.Has<ck::FTag_VisibleRange_Hidden>())
            { return; }

            // MaxVisibleRange CONFIG now lives in CkVisibleRange (composed onto the POI). Absent -> 0 = unlimited
            // (no cull). Same single-boundary cull as before, fed from the new home.
            auto MaxVisibleRange = 0.0f;

            if (UCk_Utils_VisibleRange_UE::Has(PoiGenericHandle))
            {
                MaxVisibleRange = UCk_Utils_VisibleRange_UE::Get_MaxRange(UCk_Utils_VisibleRange_UE::Cast(PoiGenericHandle));
            }

            if (MaxVisibleRange > 0.0f && Distance > MaxVisibleRange)
            { return; }

            if (ck::IsValid(FogOfWar) && NOT UCk_Utils_FogOfWar_UE::Get_IsLocationExplored(FogOfWar, PoiLocation))
            { return; }

            const auto FramePos = ProjectionMode == ECk_Minimap_ProjectionMode::FixedBounds
                ? UCk_Utils_Minimap_UE::Get_BoundsToFrame(PoiLocation, FixedBounds)
                : UCk_Utils_Minimap_UE::Get_WorldToFrame(PoiLocation, ViewOrigin, ViewYawDegrees, ViewExtent, RotationMode);

            const auto IsOutsideFrame = NOT UCk_Utils_Minimap_UE::Get_IsInsideFrame(FramePos, FrameShape);

            // Presentation (priority/offscreen) resolves per-consumer via CkPoiDisplayDefinition. No
            // definition -> the old field defaults (Hide / 0), so category-only POIs keep prior behavior.
            const auto DisplayDefinition = UCk_Utils_PoiDisplayDefinition_UE::TryGet_PoiDisplayDefinition_ByConsumer(
                PoiGenericHandle, Tag_PoiConsumer_Minimap);
            const auto HasDisplayDefinition = ck::IsValid(DisplayDefinition);

            // Record the resolved consumer DisplayDefinition for the post-parallel feed (invalid = none).
            FeedSlots[InIndex].GetValue().DisplayDefinition = DisplayDefinition;

            // Per-consumer restriction: a VisibleRange composed on THIS consumer's DisplayDefinition child
            // (own hidden state, or ParentHidden cascaded from the base) culls only this projector's entry —
            // the other projectors are unaffected. Pure Has reads; one-frame latency accepted (new
            // capability, no existing assertion depends on its timing). Direct-attach DDs alias the base
            // entity, where both checks are harmless: base Hidden is already view-excluded, and ParentHidden
            // only ever lands on record children.
            if (HasDisplayDefinition
                && (DisplayDefinition.Has<ck::FTag_VisibleRange_Hidden>()
                    || DisplayDefinition.Has<ck::FTag_PoiDisplayDefinition_ParentHidden>()))
            { return; }

            const auto OffscreenPolicy = HasDisplayDefinition
                ? UCk_Utils_PoiDisplayDefinition_UE::Get_OffscreenPolicy(DisplayDefinition)
                : ECk_Poi_OffscreenPolicy::Hide;
            const auto Priority = HasDisplayDefinition
                ? UCk_Utils_PoiDisplayDefinition_UE::Get_Priority(DisplayDefinition)
                : 0;

            if (IsOutsideFrame && OffscreenPolicy == ECk_Poi_OffscreenPolicy::Hide)
            { return; }

            const auto EdgeState = IsOutsideFrame
                ? ECk_Minimap_EntryEdgeState::ClampedToEdge
                : ECk_Minimap_EntryEdgeState::InsideFrame;

            const auto PoiHandle = ck::StaticCast<FCk_Handle_Poi>(PoiGenericHandle);

            Slots[InIndex].Emplace(FCk_Minimap_Entry
            {
                PoiHandle,
                UCk_Utils_Poi_UE::Get_CategoryTags(PoiHandle).First(),
                IsOutsideFrame ? UCk_Utils_Minimap_UE::Get_ClampToFrame(FramePos, FrameShape) : FramePos,
                EdgeState,
                static_cast<float>(PoiWorldTransform.Rotator().Yaw),
                Distance,
                static_cast<float>(PoiLocation.Z - ViewOrigin.Z),
                Priority
            });
        }, ForceSingleThread);

        // Distance feed — calling thread only. Worker purity holds: workers only WROTE their own FeedSlots
        // index (same contract as Slots); every Update_Distance runs here, post-parallel. Feed the base
        // entity's VisibleRange (if composed) and the resolved consumer DisplayDefinition's VisibleRange (if
        // composed and not aliasing the base — a direct-attach DD IS the base entity, already fed above; the
        // guard avoids a double-feed). Update_Distance is a plain setter; the VisibleRange processor
        // evaluates on its own cadence, so this is what turns the view-exclude above into real state.
        for (auto FeedIndex = 0; FeedIndex < FeedSlots.Num(); ++FeedIndex)
        {
            const auto& FeedSlot = FeedSlots[FeedIndex];

            if (NOT FeedSlot.IsSet())
            { continue; }

            const auto FeedDistance = FeedSlot->Distance;
            const auto BaseHandle = InMinimapEntity.Get_ValidHandle(PoiEntities[FeedIndex].Get_ID());

            if (UCk_Utils_VisibleRange_UE::Has(BaseHandle))
            {
                auto BaseVisibleRange = UCk_Utils_VisibleRange_UE::Cast(BaseHandle);
                UCk_Utils_VisibleRange_UE::Update_Distance(BaseVisibleRange, FeedDistance);
            }

            const auto& DisplayDefinition = FeedSlot->DisplayDefinition;

            if (ck::IsValid(DisplayDefinition)
                && DisplayDefinition != BaseHandle
                && UCk_Utils_VisibleRange_UE::Has(DisplayDefinition))
            {
                auto DisplayDefinitionVisibleRange = UCk_Utils_VisibleRange_UE::Cast(DisplayDefinition);
                UCk_Utils_VisibleRange_UE::Update_Distance(DisplayDefinitionVisibleRange, FeedDistance);
            }
        }

        for (auto& Slot : Slots)
        {
            if (Slot.IsSet())
            { Scratch.Add(MoveTemp(*Slot)); }
        }

        Scratch.Sort([](const FCk_Minimap_Entry& InA, const FCk_Minimap_Entry& InB) -> bool
        {
            if (InA.Get_Priority() != InB.Get_Priority())
            { return InA.Get_Priority() > InB.Get_Priority(); }

            return InA.Get_Distance() < InB.Get_Distance();
        });

        if (const auto MaxEntries = InParams.Get_MaxEntries();
            Scratch.Num() > MaxEntries)
        {
            Scratch.SetNum(MaxEntries, EAllowShrinking::No);
        }
    }

    auto
        FProcessor_Minimap_Update::
        DoDiffAndPublishEntries(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent)
        -> void
    {
        const auto& NewEntries = InCurrent._ScratchEntries;
        const auto& OldEntries = InCurrent._Entries;

        for (const auto& NewEntry : NewEntries)
        {
            const auto WasAlreadyPresent = OldEntries.ContainsByPredicate(
            [&](const FCk_Minimap_Entry& InOldEntry)
            {
                return InOldEntry.Get_Poi() == NewEntry.Get_Poi();
            });

            if (NOT WasAlreadyPresent)
            {
                minimap::VeryVerbose(TEXT("Minimap [{}] Entry Appeared for Poi [{}]"), InMinimapEntity, NewEntry.Get_Poi());

                UUtils_Signal_OnMinimapEntryAppeared::Broadcast(InMinimapEntity, MakePayload(InMinimapEntity, NewEntry));
            }
        }

        for (const auto& OldEntry : OldEntries)
        {
            const auto IsStillPresent = NewEntries.ContainsByPredicate(
            [&](const FCk_Minimap_Entry& InNewEntry)
            {
                return InNewEntry.Get_Poi() == OldEntry.Get_Poi();
            });

            if (NOT IsStillPresent)
            {
                minimap::VeryVerbose(TEXT("Minimap [{}] Entry Disappeared for Poi [{}]"), InMinimapEntity, OldEntry.Get_Poi());

                UUtils_Signal_OnMinimapEntryDisappeared::Broadcast(InMinimapEntity, MakePayload(InMinimapEntity, OldEntry.Get_Poi()));
            }
        }

        Swap(InCurrent._Entries, InCurrent._ScratchEntries);
    }

    auto
        FProcessor_Minimap_Update::
        DoClearAllEntries(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent)
        -> void
    {
        for (const auto& Entry : InCurrent._Entries)
        {
            UUtils_Signal_OnMinimapEntryDisappeared::Broadcast(InMinimapEntity, MakePayload(InMinimapEntity, Entry.Get_Poi()));
        }

        InCurrent._Entries.Reset();
        InCurrent._ScratchEntries.Reset();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Minimap_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMinimapEntity,
            const FFragment_Minimap_Params& InParams,
            FFragment_Minimap_Current& InCurrent)
        -> void
    {
        // Drain the pooled UI deterministically when the minimap entity dies — every remaining entry gets
        // exactly one Disappeared broadcast (the Update processor is excluded from the EndPlay window)
        for (const auto& Entry : InCurrent._Entries)
        {
            UUtils_Signal_OnMinimapEntryDisappeared::Broadcast(InMinimapEntity, MakePayload(InMinimapEntity, Entry.Get_Poi()));
        }

        InCurrent._Entries.Reset();
        InCurrent._ScratchEntries.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
