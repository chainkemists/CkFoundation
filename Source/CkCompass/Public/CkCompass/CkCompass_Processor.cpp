#include "CkCompass_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkCamera/Camera/CkCamera_Utils.h"

#include "CkCompass/CkCompass_Log.h"
#include "CkCompass/CkCompass_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkEntityTag/CkEntityTag_Utils.h"

#include "CkPoi/CkPoi_Utils.h"

#include "CkPoiDisplayDefinition/CkPoiDisplayDefinition_Utils.h"

#include "CkVisibleRange/CkVisibleRange_Utils.h"

#include "Async/ParallelFor.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("CkCompass"), STATGROUP_CkCompass, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("Compass::Heading"),     STAT_CkCompass_Heading,     STATGROUP_CkCompass);
DECLARE_CYCLE_STAT(TEXT("Compass::Projection"),  STAT_CkCompass_Projection,  STATGROUP_CkCompass);
DECLARE_CYCLE_STAT(TEXT("Compass::DiffSignals"), STAT_CkCompass_DiffSignals, STATGROUP_CkCompass);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_compass_processor
{
    // Per-index distance-feed record written by a worker into its own slot, consumed post-parallel on the
    // calling thread. Distance is always set once computed (before any range cull early-return); the
    // DisplayDefinition is the resolved consumer child (invalid when this POI has none for this consumer).
    struct FCk_Compass_PoiFeed
    {
        float Distance = 0.0f;
        FCk_Handle DisplayDefinition;
    };
}

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Compass_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Compass_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Compass_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_Compass_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Compass_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCompassEntity,
            const FFragment_Compass_Params& InParams,
            FFragment_Compass_Current& InCurrent)
        -> void
    {
        InCompassEntity.Remove<MarkedDirtyBy>();

        if (ck::Is_NOT_Valid(InCurrent._Observer))
        {
            InCurrent._Observer = InCompassEntity;
        }

        // Force the first projection pass to run immediately regardless of the update interval
        InCurrent._TimeSinceUpdate = FCk_Time{TNumericLimits<double>::Max()};
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Compass_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCompassEntity,
            FFragment_Compass_Current& InCurrent,
            FFragment_Compass_Params& InParams,
            FFragment_Compass_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InCompassEntity, InCurrent, InParams, InRequest);

            if (InRequest.Get_IsRequestHandleValid())
            {
                InRequest.GetAndDestroyRequestHandle();
            }
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        {
            InCompassEntity.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_Compass_HandleRequests::
        DoHandleRequest(
            HandleType InCompassEntity,
            FFragment_Compass_Current& InCurrent,
            FFragment_Compass_Params& InParams,
            const FCk_Request_Compass_SetCategoryFilter& InRequest)
        -> void
    {
        compass::VeryVerbose(TEXT("Handling SetCategoryFilter Request for Compass with Entity [{}]"), InCompassEntity);

        InParams.Set_CategoryFilter(InRequest.Get_CategoryFilter());

        // Filter changes must reflect immediately, not at the next throttled interval
        InCurrent._TimeSinceUpdate = FCk_Time{TNumericLimits<double>::Max()};
    }

    auto
        FProcessor_Compass_HandleRequests::
        DoHandleRequest(
            HandleType InCompassEntity,
            FFragment_Compass_Current& InCurrent,
            FFragment_Compass_Params& InParams,
            const FCk_Request_Compass_SetManualHeading& InRequest)
        -> void
    {
        compass::VeryVerbose(TEXT("Handling SetManualHeading Request for Compass with Entity [{}]"), InCompassEntity);

        InCurrent._ManualHeadingDegrees = InRequest.Get_HeadingDegrees();
    }

    auto
        FProcessor_Compass_HandleRequests::
        DoHandleRequest(
            HandleType InCompassEntity,
            FFragment_Compass_Current& InCurrent,
            FFragment_Compass_Params& InParams,
            const FCk_Request_Compass_SetObserver& InRequest)
        -> void
    {
        compass::VeryVerbose(TEXT("Handling SetObserver Request for Compass with Entity [{}]"), InCompassEntity);

        InCurrent._Observer = ck::IsValid(InRequest.Get_Observer())
            ? InRequest.Get_Observer()
            : static_cast<const FCk_Handle&>(InCompassEntity);

        InCurrent._TimeSinceUpdate = FCk_Time{TNumericLimits<double>::Max()};
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Compass_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCompassEntity,
            const FFragment_Compass_Params& InParams,
            FFragment_Compass_Current& InCurrent) const
        -> void
    {
        // Observers die (pawn destroyed, possession changed) as part of normal play — degrade silently,
        // drain the entries so bound UIs empty their pools, and wait for a Request_SetObserver
        if (ck::Is_NOT_Valid(InCurrent._Observer))
        {
            DoClearAllEntries(InCompassEntity, InCurrent);
            return;
        }

        const auto& Observer = InCurrent._Observer;

        auto ObserverTransform = UCk_Utils_Transform_UE::Cast(Observer);

        CK_ENSURE_IF_NOT(ck::IsValid(ObserverTransform),
            TEXT("Compass [{}] Observer [{}] has no Transform feature. The Compass cannot project POIs without an observer position"),
            InCompassEntity, Observer)
        {
            DoClearAllEntries(InCompassEntity, InCurrent);
            return;
        }

        // Heading refreshes EVERY frame — only the O(POIs) projection below is throttled by the update
        // interval (a throttled heading makes the compass ribbon visibly stutter during camera pans)
        {
            SCOPE_CYCLE_COUNTER(STAT_CkCompass_Heading);
            InCurrent._HeadingDegrees = FRotator::ClampAxis(DoResolveHeading(InCompassEntity, InParams, InCurrent, Observer));
        }

        InCurrent._TimeSinceUpdate += InDeltaT;

        const auto UpdateInterval = InParams.Get_UpdateInterval();

        if (UpdateInterval > FCk_Time::ZeroSecond() && InCurrent._TimeSinceUpdate < UpdateInterval)
        { return; }

        InCurrent._TimeSinceUpdate = FCk_Time::ZeroSecond();

        {
            SCOPE_CYCLE_COUNTER(STAT_CkCompass_Projection);
            DoProjectPois(InCompassEntity, InParams, InCurrent, UCk_Utils_Transform_UE::Get_EntityCurrentLocation(ObserverTransform));
        }

        {
            SCOPE_CYCLE_COUNTER(STAT_CkCompass_DiffSignals);
            DoDiffAndPublishEntries(InCompassEntity, InCurrent);
        }
    }

    auto
        FProcessor_Compass_Update::
        DoResolveHeading(
            HandleType InCompassEntity,
            const FFragment_Compass_Params& InParams,
            const FFragment_Compass_Current& InCurrent,
            const FCk_Handle& InObserver)
        -> float
    {
        switch (const auto HeadingSource = InParams.Get_HeadingSource())
        {
            case ECk_Compass_HeadingSource::Manual:
            {
                return InCurrent._ManualHeadingDegrees;
            }
            case ECk_Compass_HeadingSource::CameraView:
            case ECk_Compass_HeadingSource::Auto:
            {
                if (const auto ObserverCamera = UCk_Utils_Camera_UE::Cast(InObserver);
                    ck::IsValid(ObserverCamera))
                {
                    return UCk_Utils_Camera_UE::Get_ViewRotation(ObserverCamera).Yaw;
                }

                if (HeadingSource == ECk_Compass_HeadingSource::CameraView)
                {
                    CK_TRIGGER_ENSURE(
                        TEXT("Compass [{}] HeadingSource is CameraView but Observer [{}] has no Camera feature. Falling back to the observer's Transform yaw"),
                        InCompassEntity, InObserver);
                }

                // fallthrough to EntityTransform resolution
                [[fallthrough]];
            }
            case ECk_Compass_HeadingSource::EntityTransform:
            {
                if (const auto ObserverTransform = UCk_Utils_Transform_UE::Cast(InObserver);
                    ck::IsValid(ObserverTransform))
                {
                    return UCk_Utils_Transform_UE::Get_EntityCurrentRotation(ObserverTransform).Yaw;
                }

                return InCurrent._HeadingDegrees;
            }
            default:
            {
                CK_INVALID_ENUM(HeadingSource);
                return InCurrent._HeadingDegrees;
            }
        }
    }

    auto
        FProcessor_Compass_Update::
        DoProjectPois(
            HandleType InCompassEntity,
            const FFragment_Compass_Params& InParams,
            FFragment_Compass_Current& InCurrent,
            const FVector& InObserverLocation)
        -> void
    {
        auto& Scratch = InCurrent._ScratchEntries;
        Scratch.Reset();

        const auto& CategoryFilter = InParams.Get_CategoryFilter();
        const auto FilterIsEmpty = CategoryFilter.IsEmpty();
        const auto HeadingDegrees = InCurrent._HeadingDegrees;
        const auto ArcDegrees = InParams.Get_ArcDegrees();

        // Manual view over every POI in this world's registry, keyed on the FTag_Poi identity. The four
        // pending-kill excludes matter: fragments survive until destruction Finalize (~2 ticks after
        // Destroy) — without them, dying POIs would linger on the compass. Initiate-frame POIs are
        // deliberately still projected (same policy as CK_IGNORE_PENDING_KILL).
        //
        // Base-entity FTag_VisibleRange_Hidden is consumed as a WORKER skip (below, after the distance
        // record), deliberately NOT a view exclude — an excluded hidden POI would stop receiving the
        // distance feed and could never re-evaluate back to visible. The inline cull/fade math further
        // down still runs for blip-free same-frame membership. The EntityTag disable convention stays a
        // per-worker skip too.
        //
        // The POI set is GATHERED first, then projected data-parallel: the per-POI body is pure registry
        // READS (EntityTag/VisibleRange/DisplayDefinition state, own transforms) whose writers all ran in
        // earlier groups, and each worker writes only its own index slot. Signals/sort/diff and every
        // Update_Distance feed stay on the calling thread.
        auto& PoiEntities = InCurrent._ScratchPoiEntities;
        PoiEntities.Reset();

        InCompassEntity.View<
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
        TArray<TOptional<ck_compass_processor::FCk_Compass_PoiFeed>> FeedSlots;
        FeedSlots.SetNum(PoiEntities.Num());

        // Below this the fan-out overhead exceeds the projection math — same body runs inline
        constexpr auto MinPoisForParallel = 64;
        const auto ForceSingleThread = PoiEntities.Num() < MinPoisForParallel;

        ParallelFor(PoiEntities.Num(),
        [&](const int32 InIndex)
        {
            const auto PoiGenericHandle = InCompassEntity.Get_ValidHandle(PoiEntities[InIndex].Get_ID());

            // Disabled POIs are excluded via the EntityTag convention tag (absence-safe: Has returns false
            // when the store isn't present yet — the disable add is deferred one pump).
            if (UCk_Utils_EntityTag_UE::Has_UsingGameplayTag(PoiGenericHandle, Tag_Poi_DisabledName))
            { return; }

            if (NOT FilterIsEmpty && NOT CategoryFilter.Matches(UCk_Utils_EntityTag_UE::Get_AllTagsAsContainer(PoiGenericHandle)))
            { return; }

            // POI position = the POI entity's own Transform location (direct-attach: the POI entity carries
            // the Transform — see CkPoi's composition contract)
            const auto PoiTransform = UCk_Utils_Transform_UE::Cast(PoiGenericHandle);

            if (ck::Is_NOT_Valid(PoiTransform))
            { return; }

            const auto PoiLocation = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(PoiTransform).GetLocation();

            const auto Distance = static_cast<float>(FVector::Dist(InObserverLocation, PoiLocation));

            // Record the distance for the post-parallel feed BEFORE any subsequent early-return (the range
            // cull below can bail). A base-range-culled entry still feeds its base VR its distance — fine;
            // its DisplayDefinition stays unresolved (invalid), so only the base VR is fed for it.
            FeedSlots[InIndex].Emplace(ck_compass_processor::FCk_Compass_PoiFeed{Distance});

            // Base-entity hidden state (explicit Request_SetVisibility, or its own range vote once fed) is a
            // WORKER skip, deliberately NOT a view exclude: a view-excluded hidden POI would stop receiving
            // the distance feed above and could never re-evaluate back to visible when the observer returns
            // into range. Skips here, keeps feeding.
            if (PoiGenericHandle.Has<ck::FTag_VisibleRange_Hidden>())
            { return; }

            // Range/fade CONFIG now lives in CkVisibleRange (composed onto the POI). Absent -> unlimited
            // (Min/Max/Band 0 = no cull, alpha 1); the inline cull/fade math below is unchanged.
            auto MinVisibleRange = 0.0f;
            auto MaxVisibleRange = 0.0f;
            auto RangeFadeBandCm = 0.0f;

            if (UCk_Utils_VisibleRange_UE::Has(PoiGenericHandle))
            {
                const auto PoiVisibleRange = UCk_Utils_VisibleRange_UE::Cast(PoiGenericHandle);
                MinVisibleRange = UCk_Utils_VisibleRange_UE::Get_MinRange(PoiVisibleRange);
                MaxVisibleRange = UCk_Utils_VisibleRange_UE::Get_MaxRange(PoiVisibleRange);
                RangeFadeBandCm = UCk_Utils_VisibleRange_UE::Get_FadeBandCm(PoiVisibleRange);
            }

            if (MaxVisibleRange > 0.0f && Distance > MaxVisibleRange)
            { return; }

            if (MinVisibleRange > 0.0f && Distance < MinVisibleRange)
            { return; }

            // Presentation (priority/offscreen) resolves per-consumer via CkPoiDisplayDefinition. No
            // definition -> the old field defaults (Hide / 0), so category-only POIs keep prior behavior.
            const auto DisplayDefinition = UCk_Utils_PoiDisplayDefinition_UE::TryGet_PoiDisplayDefinition_ByConsumer(
                PoiGenericHandle, Tag_PoiConsumer_Compass);
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

            const auto WorldYawToPoi = UCk_Utils_Vector3_UE::Get_HeadingAngleBetweenLocations(InObserverLocation, PoiLocation);
            const auto SignedBearing = FMath::FindDeltaAngleDegrees(HeadingDegrees, WorldYawToPoi);
            const auto IsOutsideArc = UCk_Utils_Compass_UE::Get_IsOutsideArc(SignedBearing, ArcDegrees);

            if (IsOutsideArc && OffscreenPolicy == ECk_Poi_OffscreenPolicy::Hide)
            { return; }

            const auto ArcState = IsOutsideArc
                ? ECk_Compass_EntryArcState::ClampedToEdge
                : ECk_Compass_EntryArcState::InsideArc;

            const auto FadeAlpha = UCk_Utils_Compass_UE::Get_RangeFadeAlpha(
                Distance, MinVisibleRange, MaxVisibleRange, RangeFadeBandCm);

            const auto PoiHandle = ck::StaticCast<FCk_Handle_Poi>(PoiGenericHandle);

            Slots[InIndex].Emplace(FCk_Compass_Entry
            {
                PoiHandle,
                UCk_Utils_Poi_UE::Get_CategoryTags(PoiHandle).First(),
                SignedBearing,
                UCk_Utils_Compass_UE::Get_NormalizedArcOffset(SignedBearing, ArcDegrees),
                ArcState,
                Distance,
                static_cast<float>(PoiLocation.Z - InObserverLocation.Z),
                Priority,
                FadeAlpha
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
            const auto BaseHandle = InCompassEntity.Get_ValidHandle(PoiEntities[FeedIndex].Get_ID());

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

        Scratch.Sort([](const FCk_Compass_Entry& InA, const FCk_Compass_Entry& InB) -> bool
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
        FProcessor_Compass_Update::
        DoDiffAndPublishEntries(
            HandleType InCompassEntity,
            FFragment_Compass_Current& InCurrent)
        -> void
    {
        const auto& NewEntries = InCurrent._ScratchEntries;
        const auto& OldEntries = InCurrent._Entries;

        for (const auto& NewEntry : NewEntries)
        {
            const auto WasAlreadyPresent = OldEntries.ContainsByPredicate(
            [&](const FCk_Compass_Entry& InOldEntry)
            {
                return InOldEntry.Get_Poi() == NewEntry.Get_Poi();
            });

            if (NOT WasAlreadyPresent)
            {
                compass::VeryVerbose(TEXT("Compass [{}] Entry Appeared for Poi [{}]"), InCompassEntity, NewEntry.Get_Poi());

                UUtils_Signal_OnCompassEntryAppeared::Broadcast(InCompassEntity, MakePayload(InCompassEntity, NewEntry));
            }
        }

        for (const auto& OldEntry : OldEntries)
        {
            const auto IsStillPresent = NewEntries.ContainsByPredicate(
            [&](const FCk_Compass_Entry& InNewEntry)
            {
                return InNewEntry.Get_Poi() == OldEntry.Get_Poi();
            });

            if (NOT IsStillPresent)
            {
                compass::VeryVerbose(TEXT("Compass [{}] Entry Disappeared for Poi [{}]"), InCompassEntity, OldEntry.Get_Poi());

                UUtils_Signal_OnCompassEntryDisappeared::Broadcast(InCompassEntity, MakePayload(InCompassEntity, OldEntry.Get_Poi()));
            }
        }

        Swap(InCurrent._Entries, InCurrent._ScratchEntries);
    }

    auto
        FProcessor_Compass_Update::
        DoClearAllEntries(
            HandleType InCompassEntity,
            FFragment_Compass_Current& InCurrent)
        -> void
    {
        for (const auto& Entry : InCurrent._Entries)
        {
            UUtils_Signal_OnCompassEntryDisappeared::Broadcast(InCompassEntity, MakePayload(InCompassEntity, Entry.Get_Poi()));
        }

        InCurrent._Entries.Reset();
        InCurrent._ScratchEntries.Reset();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Compass_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCompassEntity,
            const FFragment_Compass_Params& InParams,
            FFragment_Compass_Current& InCurrent)
        -> void
    {
        // Drain the pooled UI deterministically when the compass entity dies — every remaining entry gets
        // exactly one Disappeared broadcast (the Update processor is excluded from the EndPlay window)
        for (const auto& Entry : InCurrent._Entries)
        {
            UUtils_Signal_OnCompassEntryDisappeared::Broadcast(InCompassEntity, MakePayload(InCompassEntity, Entry.Get_Poi()));
        }

        InCurrent._Entries.Reset();
        InCurrent._ScratchEntries.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------