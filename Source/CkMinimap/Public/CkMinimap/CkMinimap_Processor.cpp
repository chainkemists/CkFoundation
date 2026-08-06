#include "CkMinimap_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkCamera/Camera/CkCamera_Utils.h"

#include "CkEcs/Request/CkRequest_Completion.h"
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
    struct FCk_Minimap_PoiFeed
    {
        float Distance = 0.0f;
        FCk_Handle DisplayDefinition;
    };
}

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Minimap_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Minimap_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Minimap_CancelPendingRequests);
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

        // Direct-attach default only — Create points its child's observer at the lifetime owner via a request
        if (ck::Is_NOT_Valid(InCurrent._Observer))
        {
            InCurrent._Observer = InMinimapEntity;
        }

        InCurrent._ViewExtent = InParams.Get_ViewExtent();

        // Clamped for EVERY projection mode, ensured for only one. The stored value must
        // stay positive regardless (a later Request_SetRotationMode/mode flip would divide
        // by it), but see below for why FixedBounds does not complain about it.
        if (InCurrent._ViewExtent <= 0.0f)
        {
            InCurrent._ViewExtent = 1.0f;
        }

        if (InParams.Get_ProjectionMode() == ECk_Minimap_ProjectionMode::FixedBounds)
        {
            // FixedBounds projects through Get_BoundsToFrame and NEVER reads ViewExtent, so
            // ensuring on it here was a false positive: it forced every world-map author to
            // invent a meaningless positive number to silence a check on a value the mode
            // ignores. What FixedBounds actually requires is valid bounds.
            CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_FixedBounds()),
                TEXT("Minimap [{}] uses FixedBounds projection but its FixedBounds half-extents are not > 0. "
                     "Every entry will project to the frame center until the minimap is re-Added with valid bounds"),
                InMinimapEntity)
            {}
        }
        else
        {
            // ObserverCentric: ViewExtent IS the zoom, so a non-positive one is a real
            // authoring error and the 1cm fallback above is a visible degradation.
            CK_ENSURE_IF_NOT(InParams.Get_ViewExtent() > 0.0f,
                TEXT("Minimap [{}] ViewExtent [{}] must be > 0 — falling back to 1cm"),
                InMinimapEntity, InParams.Get_ViewExtent())
            {}
        }

        InCurrent._RotationMode = InParams.Get_RotationMode();

        const auto ProjectImmediately = FCk_Time{TNumericLimits<double>::Max()};
        InCurrent._TimeSinceUpdate = ProjectImmediately;
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
            using T = std::decay_t<decltype(InRequest)>;

            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InMinimapEntity, Result);

            // SetViewExtent has a genuine ensure-gated failure path; every other DoHandleRequest
            // overload is void with no rejection, so reaching the line after the call IS the
            // success condition.
            if constexpr (std::is_same_v<T, FCk_Request_Minimap_SetViewExtent>)
            {
                if (DoHandleRequest(InMinimapEntity, InCurrent, InParams, InRequest))
                { Result = ECk_Request_OperationResult::Succeeded; }
            }
            else
            {
                DoHandleRequest(InMinimapEntity, InCurrent, InParams, InRequest);
                Result = ECk_Request_OperationResult::Succeeded;
            }

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
        -> bool
    {
        minimap::VeryVerbose(TEXT("Handling SetViewExtent Request for Minimap with Entity [{}]"), InMinimapEntity);

        CK_ENSURE_IF_NOT(InRequest.Get_ViewExtent() > 0.0f,
            TEXT("Minimap [{}] SetViewExtent [{}] must be > 0 — request rejected"),
            InMinimapEntity, InRequest.Get_ViewExtent())
        { return false; }

        InCurrent._ViewExtent = InRequest.Get_ViewExtent();

        const auto ProjectImmediately = FCk_Time{TNumericLimits<double>::Max()};
        InCurrent._TimeSinceUpdate = ProjectImmediately;

        return true;
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

        const auto ProjectImmediately = FCk_Time{TNumericLimits<double>::Max()};
        InCurrent._TimeSinceUpdate = ProjectImmediately;
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

        const auto ProjectImmediately = FCk_Time{TNumericLimits<double>::Max()};
        InCurrent._TimeSinceUpdate = ProjectImmediately;
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

        const auto ProjectImmediately = FCk_Time{TNumericLimits<double>::Max()};
        InCurrent._TimeSinceUpdate = ProjectImmediately;
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

        const auto ProjectImmediately = FCk_Time{TNumericLimits<double>::Max()};
        InCurrent._TimeSinceUpdate = ProjectImmediately;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Minimap_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMinimapEntity,
            const FFragment_Minimap_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InMinimapEntity, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Minimap_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMinimapEntity,
            const FFragment_Minimap_Params& InParams,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Scratch& InScratch) const
        -> void
    {
        // Observers die (pawn destroyed, possession changed) as part of normal play — degrade silently and
        // wait for a Request_SetObserver. FixedBounds minimaps degrade the same way (no observer-less world map)
        if (ck::Is_NOT_Valid(InCurrent._Observer))
        {
            DoClearAllEntries(InMinimapEntity, InCurrent, InScratch);
            return;
        }

        const auto& Observer = InCurrent._Observer;

        auto ObserverTransform = UCk_Utils_Transform_UE::Cast(Observer);

        CK_ENSURE_IF_NOT(ck::IsValid(ObserverTransform),
            TEXT("Minimap [{}] Observer [{}] has no Transform feature. The Minimap cannot project POIs without an observer position"),
            InMinimapEntity, Observer)
        {
            DoClearAllEntries(InMinimapEntity, InCurrent, InScratch);
            return;
        }

        InCurrent._TimeSinceUpdate += InDeltaT;

        const auto UpdateInterval = InParams.Get_UpdateInterval();

        if (UpdateInterval > FCk_Time::ZeroSecond() && InCurrent._TimeSinceUpdate < UpdateInterval)
        { return; }

        InCurrent._TimeSinceUpdate = FCk_Time::ZeroSecond();

        InCurrent._ViewOrigin = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(ObserverTransform);
        InCurrent._ViewYawDegrees = FRotator::ClampAxis(DoResolveViewYaw(Observer, InCurrent));

        {
            SCOPE_CYCLE_COUNTER(STAT_CkMinimap_Projection);
            DoProjectPois(InMinimapEntity, InParams, InCurrent, InScratch);
        }

        {
            SCOPE_CYCLE_COUNTER(STAT_CkMinimap_DiffSignals);
            DoDiffAndPublishEntries(InMinimapEntity, InCurrent, InScratch);
        }
    }

    auto
        FProcessor_Minimap_Update::
        DoResolveViewYaw(
            const FCk_Handle& InObserver,
            const FFragment_Minimap_Current& InCurrent)
        -> float
    {
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
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Scratch& InScratch)
        -> void
    {
        auto& Scratch = InScratch._Entries;
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

        // The four pending-kill excludes matter: fragments survive until destruction Finalize (~2 ticks after
        // Destroy) — without them, dying POIs would linger on the minimap. Initiate-frame POIs are
        // deliberately still projected (same policy as CK_IGNORE_PENDING_KILL).
        auto& PoiEntities = InScratch._PoiEntities;
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

        auto& Slots = InScratch._ParallelSlots;
        Slots.Reset();
        Slots.SetNum(PoiEntities.Num());

        TArray<TOptional<ck_minimap_processor::FCk_Minimap_PoiFeed>> FeedSlots;
        FeedSlots.SetNum(PoiEntities.Num());

        constexpr auto MinPoisForParallel = 64;
        const auto ForceSingleThread = PoiEntities.Num() < MinPoisForParallel;

        ParallelFor(PoiEntities.Num(),
        [&](const int32 InIndex)
        {
            const auto PoiGenericHandle = InMinimapEntity.Get_ValidHandle(PoiEntities[InIndex].Get_ID());

            if (UCk_Utils_EntityTag_UE::Has_UsingGameplayTag(PoiGenericHandle, Tag_Poi_DisabledName))
            { return; }

            if (NOT FilterIsEmpty && NOT CategoryFilter.Matches(UCk_Utils_EntityTag_UE::Get_AllTagsAsContainer(PoiGenericHandle)))
            { return; }

            const auto PoiTransformHandle = UCk_Utils_Transform_UE::Cast(PoiGenericHandle);

            if (ck::Is_NOT_Valid(PoiTransformHandle))
            { return; }

            const auto PoiWorldTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(PoiTransformHandle);
            const auto PoiLocation = PoiWorldTransform.GetLocation();

            const auto Distance = static_cast<float>(FVector::Dist(ViewOrigin, PoiLocation));

            // Recorded BEFORE the culls below: a culled POI must keep feeding its VisibleRange, or it could
            // never re-evaluate back to visible
            FeedSlots[InIndex].Emplace(ck_minimap_processor::FCk_Minimap_PoiFeed{Distance});

            // A worker skip, deliberately NOT a view exclude — see the distance record above
            if (PoiGenericHandle.Has<ck::FTag_VisibleRange_Hidden>())
            { return; }

            constexpr auto UnlimitedRange = 0.0f;
            auto MaxVisibleRange = UnlimitedRange;

            if (UCk_Utils_VisibleRange_UE::Has(PoiGenericHandle))
            {
                MaxVisibleRange = UCk_Utils_VisibleRange_UE::Get_MaxRange(UCk_Utils_VisibleRange_UE::Cast(PoiGenericHandle));
            }

            if (MaxVisibleRange > UnlimitedRange && Distance > MaxVisibleRange)
            { return; }

            if (ck::IsValid(FogOfWar) && NOT UCk_Utils_FogOfWar_UE::Get_IsLocationExplored(FogOfWar, PoiLocation))
            { return; }

            const auto FramePos = ProjectionMode == ECk_Minimap_ProjectionMode::FixedBounds
                ? UCk_Utils_Minimap_UE::Get_BoundsToFrame(PoiLocation, FixedBounds)
                : UCk_Utils_Minimap_UE::Get_WorldToFrame(PoiLocation, ViewOrigin, ViewYawDegrees, ViewExtent, RotationMode);

            const auto IsOutsideFrame = NOT UCk_Utils_Minimap_UE::Get_IsInsideFrame(FramePos, FrameShape);

            const auto PoiHandle = UCk_Utils_Poi_UE::CastChecked(PoiGenericHandle);

            const auto DisplayDefinition = UCk_Utils_PoiDisplayDefinition_UE::TryGet_PoiDisplayDefinition_ByConsumer(
                PoiHandle, Tag_PoiConsumer_Minimap);
            const auto HasDisplayDefinition = ck::IsValid(DisplayDefinition);

            FeedSlots[InIndex].GetValue().DisplayDefinition = DisplayDefinition;

            // Per-consumer restriction: hidden state on THIS consumer's DisplayDefinition child culls only
            // this projector's entry. Direct-attach DDs alias the base entity, where both checks are harmless
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

        // Distance feed — calling thread ONLY: the workers above may not mutate ECS state. The
        // DisplayDefinition != BaseHandle guard avoids double-feeding a direct-attach DD, which IS the base
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
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Scratch& InScratch)
        -> void
    {
        const auto& NewEntries = InScratch._Entries;
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

        Swap(InCurrent._Entries, InScratch._Entries);
    }

    auto
        FProcessor_Minimap_Update::
        DoClearAllEntries(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Scratch& InScratch)
        -> void
    {
        for (const auto& Entry : InCurrent._Entries)
        {
            UUtils_Signal_OnMinimapEntryDisappeared::Broadcast(InMinimapEntity, MakePayload(InMinimapEntity, Entry.Get_Poi()));
        }

        InCurrent._Entries.Reset();
        InScratch._Entries.Reset();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Minimap_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMinimapEntity,
            const FFragment_Minimap_Params& InParams,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Scratch& InScratch)
        -> void
    {
        for (const auto& Entry : InCurrent._Entries)
        {
            UUtils_Signal_OnMinimapEntryDisappeared::Broadcast(InMinimapEntity, MakePayload(InMinimapEntity, Entry.Get_Poi()));
        }

        InCurrent._Entries.Reset();
        InScratch._Entries.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
