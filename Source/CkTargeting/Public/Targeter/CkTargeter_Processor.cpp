#include "CkTargeter_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/TransientEntity/CkTransientEntity_Utils.h"

#include "CkNet/CkNet_Utils.h"
#include "CkTargeting/CkTargeting_Log.h"
#include "Targetable/CkTargetable_Fragment.h"
#include "Targetable/CkTargetable_Utils.h"
#include "Targeter/CkTargeter_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Targeter_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Targeter_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Targeter_Params& InParams,
            FFragment_Targeter_Current& InCurrent) const
        -> void
    {
        if (const auto& CustomTargetFilter = InParams.Get_Params().Get_CustomTargetFilter().Get();
                ck::IsValid(CustomTargetFilter))
        {
            const auto& World = UCk_Utils_TransientEntity_UE::Get_World(InHandle);
            InCurrent._InstancedTargetFilter = TStrongObjectPtr(UCk_Utils_Object_UE::Request_CloneObject(World, CustomTargetFilter));

            const auto TargeterBasicInfo = FCk_Targeter_BasicInfo{InHandle, UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle)};
            InCurrent._InstancedTargetFilter->OnFilterCreated(TargeterBasicInfo);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Targeter_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Targeter_Params& InParams)
        -> void
    {
        UCk_Utils_Targeter_UE::Request_QueryTargets(InHandle, {}, {});
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Targeter_Cleanup::
        DoTick(
            FCk_Time InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Targeter_Cleanup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_Targeter_Current& InCurrent)
        -> void
    {
        targeting::VeryVerbose(TEXT("Resetting Cached Targets from Targeter [{}]"), InHandle);
        InCurrent._CachedTargets.Reset();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Targeter_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Targeter_Params& InParams,
            FFragment_Targeter_Current& InCurrent,
            const FFragment_Targeter_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_Targeter_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequestVariant) -> void
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequestVariant);
            }), policy::DontResetContainer{});
        });
    }

    auto
        FProcessor_Targeter_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Targeter_Params& InParams,
            FFragment_Targeter_Current& InCurrent,
            const FFragment_Targeter_Requests::QueryValidTargetsRequestType& InRequest) const
        -> void
    {
        const auto TargeterBasicInfo = FCk_Targeter_BasicInfo{InHandle, UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle)};

        if (InCurrent.Get_CachedTargets().IsSet())
        {
            UUtils_Signal_Targeter_OnTargetsQueried::Broadcast(InHandle, MakePayload(TargeterBasicInfo, *InCurrent.Get_CachedTargets()));
            return;
        }

        auto ValidTargets = TArray<FCk_Targetable_BasicInfo>{};

        InHandle.View<FFragment_Targetable_Params, FFragment_Targetable_Current, FTag_Targetable_IsReady, CK_IGNORE_PENDING_KILL>()
        .ForEach([&](FCk_Entity InEntity, const FFragment_Targetable_Params&, const FFragment_Targetable_Current&)
        {
            const auto Handle = ck::MakeHandle(InEntity, InHandle);
            const auto Targetable = UCk_Utils_Targetable_UE::Cast(Handle);

            if (UCk_Utils_Targeter_UE::Get_CanTarget(InHandle, Targetable) != ECk_Targetable_Status::CanTarget)
            { return; }

            ValidTargets.Add(FCk_Targetable_BasicInfo{Targetable, UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Targetable)});
        });

        const auto ValidTargetList  = FCk_Targeter_TargetList{ValidTargets};
        InCurrent._CachedTargets = ValidTargetList;

        if (NOT ValidTargets.IsEmpty())
        {
            if (const auto& TargetFilter = InCurrent.Get_InstancedTargetFilter().Get();
                ck::IsValid(TargetFilter))
            {
                const auto& FilteredTargets = TargetFilter->FilterTargets(TargeterBasicInfo, ValidTargetList);
                const auto& SortedTargets = FilteredTargets.Get_Targets().Num() <= 1
                                                ? FilteredTargets
                                                : TargetFilter->SortTargets(TargeterBasicInfo, FilteredTargets);

                InCurrent._CachedTargets = SortedTargets;
            }
        }

        UCk_Utils_Targeter_UE::Request_Cleanup(InHandle);

        UUtils_Signal_Targeter_OnTargetsQueried::Broadcast(InHandle, MakePayload(TargeterBasicInfo, *InCurrent._CachedTargets));
    }
}

// --------------------------------------------------------------------------------------------------------------------
