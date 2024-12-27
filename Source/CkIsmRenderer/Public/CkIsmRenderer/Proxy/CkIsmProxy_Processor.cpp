#include "CkIsmProxy_Processor.h"

#include "CkIsmProxy_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_ism_proxy_processor
{
    using IsmAcArray = std::array<TWeakObjectPtr<UInstancedStaticMeshComponent>,  static_cast<int32>(ECk_Mobility::Count)>;
    static TMap<FGameplayTag, IsmAcArray> Renderers;

    auto
        FindRendererIsmComp(
            const FGameplayTag& InRendererName,
            ECk_Mobility InMobility)
        -> TWeakObjectPtr<UInstancedStaticMeshComponent>
    {
        const auto& MaybeFound = Renderers.Find(InRendererName);

        CK_ENSURE_IF_NOT(ck::IsValid(MaybeFound, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("No Ism Renderer with Name [{}] found"), InRendererName)
        { return {}; }

        const auto& RendererToUse = *MaybeFound;
        const auto& IsmComp =  RendererToUse[static_cast<int32>(InMobility)];

        CK_ENSURE_IF_NOT(ck::IsValid(IsmComp),
            TEXT("The Renderer Ism Component with Name [{}] and Mobility [{}] is INVALID. Unable to Render the IsmProxy"),
            InRendererName,
            InMobility)
        { return {}; }

        return IsmComp;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_IsmProxy_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        using ck_ism_proxy_processor::Renderers;

        if (UCk_Utils_Net_UE::Get_EntityNetMode(_TransientEntity) != ECk_Net_NetModeType::Client)
        { return; }

        Renderers.Empty();

        _TransientEntity.View<
            FFragment_IsmRenderer_Params,
            FFragment_IsmRenderer_Current,
            TExclude<FTag_IsmRenderer_NeedsSetup>>().ForEach(
            [&](FCk_Entity InHandle, const FFragment_IsmRenderer_Params& InParams, const FFragment_IsmRenderer_Current& InCurrent)
            {
                using ck_ism_proxy_processor::IsmAcArray;

                const auto IsmArray = IsmAcArray{
                    InCurrent.Get_IsmComponent_Static(), InCurrent.Get_IsmComponent_Static(), InCurrent.Get_IsmComponent_Movable()};

                CK_ENSURE_IF_NOT(NOT Renderers.Contains(InParams.Get_Params().Get_RendererName()),
                    TEXT("Ism Renderer with Name [{}] already exists. Unable to add another one"),
                    InParams.Get_Params().Get_RendererName())
                { return; }

                Renderers.Add(InParams.Get_Params().Get_RendererName(), IsmArray);
            });

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_IsmProxy_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent) const
        -> void
    {
        const auto& Params = InParams.Get_Params();
        const auto& RendererName = Params.Get_RendererName();
        const auto& Mobility = Params.Get_Mobility();
        const auto& IsmComp = ck_ism_proxy_processor::FindRendererIsmComp(RendererName, Mobility);

        if (ck::Is_NOT_Valid(IsmComp))
        { return; }

        InCurrent._CustomDataValues.Init(0, IsmComp->NumCustomDataFloats);
        UCk_Utils_IsmProxy_UE::Request_NeedsInstanceAdded(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_AddInstance::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent) const
        -> void
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
            TEXT("Ism Proxy Entity [{}] does NOT have the required Transform feature. Unable to render the IsmProxy"), InHandle)
        { return; }

        const auto& Params = InParams.Get_Params();
        const auto& RendererName = Params.Get_RendererName();
        const auto& Mobility = Params.Get_Mobility();
        const auto& IsmComp = ck_ism_proxy_processor::FindRendererIsmComp(RendererName, Mobility);

        if (ck::Is_NOT_Valid(IsmComp))
        { return; }

        const auto& CurrentTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InHandle);
        const auto& CombinedTransform = [&]() -> FTransform
        {
            const auto& CombinedLocation = CurrentTransform.GetLocation() + Params.Get_LocalLocationOffset();
            const auto& CombinedRotation = Params.Get_LocalRotationOffset().Quaternion() * CurrentTransform.GetRotation();

            CK_ENSURE_IF_NOT(NOT UCk_Utils_Vector3_UE::Get_IsAnyAxisNearlyZero(Params.Get_ScaleMultiplier()),
                TEXT("IsmProxy Scale Multiplier has one or more axis nearly equal to 0. Setting it to 1 in non-shipping build"), Params.Get_ScaleMultiplier())
            { return FTransform{ CombinedRotation.Rotator(), CombinedLocation, FVector::OneVector }; }

            const auto& CombinedScale = CurrentTransform.GetScale3D() * Params.Get_ScaleMultiplier();

            return FTransform{ CombinedRotation.Rotator(), CombinedLocation, CombinedScale };
        }();

        constexpr auto TransformAsWorldSpace = true;
        const auto& InstanceIndex = IsmComp->AddInstanceById(CombinedTransform, TransformAsWorldSpace);
        InCurrent._IsmInstanceIndex = InstanceIndex;

        constexpr auto MarkRenderStateDirty = false;
        IsmComp->SetCustomDataById(InstanceIndex, InCurrent.Get_CustomDataValues());

        // Movable ISM instances are re-added again every tick
        if (Mobility != ECk_Mobility::Movable)
        {
            InHandle.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_IsmProxy_Teardown::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        TProcessor::DoTick(InDeltaT);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_IsmProxy_Current& InCurrent) const
        -> void
    {
        const auto& Params = InParams.Get_Params();

        const auto& RendererName = Params.Get_RendererName();
        const auto& Mobility = Params.Get_Mobility();
        const auto& IsmComp = ck_ism_proxy_processor::FindRendererIsmComp(RendererName, Mobility);

        if (ck::Is_NOT_Valid(IsmComp))
        { return; }

        if (IsmComp->IsValidId(InCurrent.Get_IsmInstanceIndex()))
        {
            IsmComp->RemoveInstanceById(InCurrent.Get_IsmInstanceIndex());
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FFragment_IsmProxy_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_IsmProxy_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequestVariant) -> void
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequestVariant);
            }), policy::DontResetContainer{});
        });
    }

    auto
        FProcessor_IsmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_SetCustomData& InRequest)
        -> void
    {
        const auto& NewCustomData = InRequest.Get_CustomData();

        if (NewCustomData.IsEmpty())
        { return; }

        const auto& CurrentCustomData = InCurrent.Get_CustomDataValues();

        CK_ENSURE_IF_NOT(CurrentCustomData.Num() == NewCustomData.Num(),
            TEXT("Trying to set [{}] number of custom data on Ism Proxy [{}], but it was setup to contain AT MOST [{}] elements\n"
                 "Setting the custom data in its entirety must have the exact same number of elements, otherwise use SetCustomDataValue"),
            NewCustomData.Num(),
            InHandle,
            CurrentCustomData.Num())
        { return; }

        InCurrent._CustomDataValues = NewCustomData;

        // TEMP: only movable ISM instances are updated again (every tick)
        if (const auto& Mobility = UCk_Utils_IsmProxy_UE::Get_Mobility(InHandle);
            Mobility == ECk_Mobility::Movable)
        {
            const auto& Params = InParams.Get_Params();
            const auto& RendererName = Params.Get_RendererName();
            const auto& IsmComp = ck_ism_proxy_processor::FindRendererIsmComp(RendererName, Mobility);

            if (ck::Is_NOT_Valid(IsmComp))
            { return; }

            constexpr auto MarkRenderStateDirty = true;
            IsmComp->SetCustomDataById(InCurrent.Get_IsmInstanceIndex(), NewCustomData);
        }
    }

    auto
        FProcessor_IsmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_SetCustomDataValue& InRequest)
        -> void
    {
        const auto& CurrentCustomData = InCurrent.Get_CustomDataValues();
        const auto& NewCustomDataIndex = InRequest.Get_CustomDataIndex();
        const auto& NewCustomDataValue = InRequest.Get_CustomDataValue();

        CK_ENSURE_IF_NOT(CurrentCustomData.IsValidIndex(NewCustomDataIndex),
            TEXT("Trying to set custom data value [{}] at index [{}] on Ism Proxy [{}], but it was setup to contain AT MOST [{}] elements"),
            NewCustomDataValue,
            NewCustomDataIndex,
            InHandle,
            CurrentCustomData.Num())
        { return; }

        InCurrent._CustomDataValues[NewCustomDataIndex] = NewCustomDataValue;

        // TEMP: only movable ISM instances are updated again (every tick)
        if (const auto& Mobility = UCk_Utils_IsmProxy_UE::Get_Mobility(InHandle);
            Mobility == ECk_Mobility::Movable)
        {
            const auto& Params = InParams.Get_Params();
            const auto& RendererName = Params.Get_RendererName();
            const auto& IsmComp = ck_ism_proxy_processor::FindRendererIsmComp(RendererName, Mobility);

            if (ck::Is_NOT_Valid(IsmComp))
            { return; }

            constexpr auto MarkRenderStateDirty = true;
            IsmComp->SetCustomDataValueById(InCurrent.Get_IsmInstanceIndex(), NewCustomDataIndex, NewCustomDataValue);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
