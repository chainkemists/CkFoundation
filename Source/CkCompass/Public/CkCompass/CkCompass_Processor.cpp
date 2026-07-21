#include "CkCompass_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkCamera/Camera/CkCamera_Utils.h"

#include "CkCompass/CkCompass_Log.h"
#include "CkCompass/CkCompass_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPoi/CkPoi_Fragment.h"

#include "Async/ParallelFor.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("CkCompass"), STATGROUP_CkCompass, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("Compass::Heading"),     STAT_CkCompass_Heading,     STATGROUP_CkCompass);
DECLARE_CYCLE_STAT(TEXT("Compass::Projection"),  STAT_CkCompass_Projection,  STATGROUP_CkCompass);
DECLARE_CYCLE_STAT(TEXT("Compass::DiffSignals"), STAT_CkCompass_DiffSignals, STATGROUP_CkCompass);

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

        // Manual view over every POI in this world's registry. The four pending-kill excludes matter:
        // fragments survive until destruction Finalize (~2 ticks after Destroy) — without them, dying
        // POIs would linger on the compass. Initiate-frame POIs are deliberately still projected
        // (same policy as CK_IGNORE_PENDING_KILL).
        //
        // The POI set is GATHERED first, then projected data-parallel: the per-POI body is pure registry
        // READS (poi fragments, own transforms) whose writers all ran in earlier groups, and each worker
        // writes only its own index slot. Signals/sort/diff stay on the calling thread.
        auto& PoiEntities = InCurrent._ScratchPoiEntities;
        PoiEntities.Reset();

        InCompassEntity.View<
            ck::FFragment_Poi_Params,
            ck::FFragment_Poi_Current,
            ck::TExclude<ck::FTag_DestroyEntity_EndPlay>,
            ck::TExclude<ck::FTag_DestroyEntity_Teardown>,
            ck::TExclude<ck::FTag_DestroyEntity_Await>,
            ck::TExclude<ck::FTag_DestroyEntity_Finalize>>().ForEach(
        [&](const auto InPoiEntity, const ck::FFragment_Poi_Params&, const ck::FFragment_Poi_Current&) -> void
        {
            PoiEntities.Add(InPoiEntity);
        });

        auto& Slots = InCurrent._ScratchParallelSlots;
        Slots.Reset();
        Slots.SetNum(PoiEntities.Num());

        // Below this the fan-out overhead exceeds the projection math — same body runs inline
        constexpr auto MinPoisForParallel = 64;
        const auto ForceSingleThread = PoiEntities.Num() < MinPoisForParallel;

        ParallelFor(PoiEntities.Num(),
        [&](const int32 InIndex)
        {
            const auto PoiGenericHandle = InCompassEntity.Get_ValidHandle(PoiEntities[InIndex].Get_ID());

            const auto& PoiParams = PoiGenericHandle.Get<ck::FFragment_Poi_Params>();
            const auto& PoiCurrent = PoiGenericHandle.Get<ck::FFragment_Poi_Current>();

            if (PoiCurrent.Get_EnableDisable() == ECk_EnableDisable::Disable)
            { return; }

            if (NOT FilterIsEmpty && NOT CategoryFilter.Matches(FGameplayTagContainer{PoiParams.Get_Category()}))
            { return; }

            // POI position = the POI entity's own Transform + its relative offset (direct-attach: the POI
            // entity carries the Transform — see CkPoi's composition contract)
            const auto PoiTransform = UCk_Utils_Transform_UE::Cast(PoiGenericHandle);

            if (ck::Is_NOT_Valid(PoiTransform))
            { return; }

            const auto PoiLocation = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(PoiTransform).TransformPosition(PoiParams.Get_RelativeLocation());

            const auto Distance = static_cast<float>(FVector::Dist(InObserverLocation, PoiLocation));
            const auto MaxVisibleRange = PoiParams.Get_MaxVisibleRange();

            if (MaxVisibleRange > 0.0f && Distance > MaxVisibleRange)
            { return; }

            const auto WorldYawToPoi = UCk_Utils_Vector3_UE::Get_HeadingAngleBetweenLocations(InObserverLocation, PoiLocation);
            const auto SignedBearing = FMath::FindDeltaAngleDegrees(HeadingDegrees, WorldYawToPoi);
            const auto IsOutsideArc = UCk_Utils_Compass_UE::Get_IsOutsideArc(SignedBearing, ArcDegrees);

            if (IsOutsideArc && PoiParams.Get_OffscreenPolicy() == ECk_Poi_OffscreenPolicy::Hide)
            { return; }

            const auto ArcState = IsOutsideArc
                ? ECk_Compass_EntryArcState::ClampedToEdge
                : ECk_Compass_EntryArcState::InsideArc;

            Slots[InIndex].Emplace(FCk_Compass_Entry
            {
                ck::StaticCast<FCk_Handle_Poi>(PoiGenericHandle),
                PoiParams.Get_Category(),
                SignedBearing,
                UCk_Utils_Compass_UE::Get_NormalizedArcOffset(SignedBearing, ArcDegrees),
                ArcState,
                Distance,
                static_cast<float>(PoiLocation.Z - InObserverLocation.Z),
                PoiParams.Get_Priority()
            });
        }, ForceSingleThread);

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