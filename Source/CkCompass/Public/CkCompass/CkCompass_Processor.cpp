#include "CkCompass_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkCamera/Camera/CkCamera_Utils.h"

#include "CkCompass/CkCompass_Log.h"
#include "CkCompass/CkCompass_Utils.h"

#include "CkEcs/Request/CkRequest_Completion.h"
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
    struct FCk_Compass_PoiFeed
    {
        float Distance = 0.0f;
        FCk_Handle DisplayDefinition;
    };
}

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Compass_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Compass_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Compass_CancelPendingRequests);
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

        const auto ProjectImmediately = FCk_Time{TNumericLimits<double>::Max()};
        InCurrent._TimeSinceUpdate = ProjectImmediately;
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
            // Every DoHandleRequest overload below is void and has no rejection path, so reaching the
            // line after the call IS the success condition.
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InCompassEntity, Result);

            DoHandleRequest(InCompassEntity, InCurrent, InParams, InRequest);

            if (InRequest.Get_IsRequestHandleValid())
            {
                InRequest.GetAndDestroyRequestHandle();
            }

            Result = ECk_Request_OperationResult::Succeeded;
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

        const auto ProjectImmediately = FCk_Time{TNumericLimits<double>::Max()};
        InCurrent._TimeSinceUpdate = ProjectImmediately;
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

        const auto ProjectImmediately = FCk_Time{TNumericLimits<double>::Max()};
        InCurrent._TimeSinceUpdate = ProjectImmediately;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Compass_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCompassEntity,
            const FFragment_Compass_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InCompassEntity, InRequestsComp.Get_Requests());
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

        TArray<TOptional<ck_compass_processor::FCk_Compass_PoiFeed>> FeedSlots;
        FeedSlots.SetNum(PoiEntities.Num());

        constexpr auto MinPoisForParallel = 64;
        const auto ForceSingleThread = PoiEntities.Num() < MinPoisForParallel;

        ParallelFor(PoiEntities.Num(),
        [&](const int32 InIndex)
        {
            const auto PoiGenericHandle = InCompassEntity.Get_ValidHandle(PoiEntities[InIndex].Get_ID());

            // Absence-safe: Has is false before the tag store exists — the disable add is deferred one pump
            if (UCk_Utils_EntityTag_UE::Has_UsingGameplayTag(PoiGenericHandle, Tag_Poi_DisabledName))
            { return; }

            if (NOT FilterIsEmpty && NOT CategoryFilter.Matches(UCk_Utils_EntityTag_UE::Get_AllTagsAsContainer(PoiGenericHandle)))
            { return; }

            const auto PoiTransform = UCk_Utils_Transform_UE::Cast(PoiGenericHandle);

            if (ck::Is_NOT_Valid(PoiTransform))
            { return; }

            const auto PoiLocation = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(PoiTransform).GetLocation();

            const auto Distance = static_cast<float>(FVector::Dist(InObserverLocation, PoiLocation));

            // Recorded before the culls below can bail: a culled POI must keep receiving its distance feed,
            // or it could never re-evaluate back into range
            FeedSlots[InIndex].Emplace(ck_compass_processor::FCk_Compass_PoiFeed{Distance});

            if (PoiGenericHandle.Has<ck::FTag_VisibleRange_Hidden>())
            { return; }

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

            const auto PoiHandle = UCk_Utils_Poi_UE::CastChecked(PoiGenericHandle);

            const auto DisplayDefinition = UCk_Utils_PoiDisplayDefinition_UE::TryGet_PoiDisplayDefinition_ByConsumer(
                PoiHandle, Tag_PoiConsumer_Compass);
            const auto HasDisplayDefinition = ck::IsValid(DisplayDefinition);

            FeedSlots[InIndex].GetValue().DisplayDefinition = DisplayDefinition;

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

        // Update_Distance mutates ECS state — calling thread only, never inside the ParallelFor above
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
        for (const auto& Entry : InCurrent._Entries)
        {
            UUtils_Signal_OnCompassEntryDisappeared::Broadcast(InCompassEntity, MakePayload(InCompassEntity, Entry.Get_Poi()));
        }

        InCurrent._Entries.Reset();
        InCurrent._ScratchEntries.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------