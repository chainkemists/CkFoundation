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
        FProcessor_IsmProxy_Static::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        using ck_ism_proxy_processor::Renderers;

        _TransientEntity.View<
            FFragment_IsmRenderer_Params,
            FFragment_IsmRenderer_Current,
            TExclude<FTag_IsmRenderer_NeedsSetup>>().ForEach(
            [&](FCk_Entity InHandle, const FFragment_IsmRenderer_Params& InParams, const FFragment_IsmRenderer_Current& InCurrent)
            {
                using ck_ism_proxy_processor::IsmAcArray;

                const auto IsmArray = IsmAcArray{
                    InCurrent.Get_IsmComponent_Static(), InCurrent.Get_IsmComponent_Static(), InCurrent.Get_IsmComponent_Movable()};

                Renderers.Add(InParams.Get_Params().Get_RendererName(), IsmArray);
            });

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_IsmProxy_Static::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent) const
        -> void
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
            TEXT("Ism Proxy Entity [{}] does NOT have the required Transform feature. Unable to Setup the IsmProxy"), InHandle)
        { return; }

        const auto& Params = InParams.Get_Params();
        const auto& RendererName = Params.Get_RendererName();
        const auto& Mobility = Params.Get_Mobility();
        const auto& IsmComp = ck_ism_proxy_processor::FindRendererIsmComp(RendererName, Mobility);

        if (ck::Is_NOT_Valid(IsmComp))
        { return; }

        InCurrent._CustomDataValues.Init(0, IsmComp->NumCustomDataFloats);

        const auto& RelativeTransform = [&]
        {
            const auto& ProxyRelativeTransform = Params.Get_RelativeTransform();
            const auto& ProxyRelativeScale = ProxyRelativeTransform.GetScale3D();

            CK_ENSURE_IF_NOT(NOT UCk_Utils_Vector3_UE::Get_IsAnyAxisNearlyZero(ProxyRelativeScale),
                TEXT("IsmProxy Relative Scale has one or more axis nearly equal to 0. Setting it to 1 in non-shipping build"), ProxyRelativeScale)
            { return FTransform{ ProxyRelativeTransform.GetRotation(), ProxyRelativeTransform.GetLocation(), FVector::OneVector }; }

            return ProxyRelativeTransform;
        }();

        const auto& CurrentTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InHandle);
        const auto& CombinedTransform = CurrentTransform * RelativeTransform;

        constexpr auto TransformAsWorldSpace = true;
        const auto& InstanceIndex = IsmComp->AddInstance(CombinedTransform, TransformAsWorldSpace);
        InCurrent._IsmInstanceIndex = InstanceIndex;

        constexpr auto MarkRenderStateDirty = false;
        IsmComp->SetCustomData(InstanceIndex, InCurrent.Get_CustomDataValues(), MarkRenderStateDirty);

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_Dynamic::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent) const
        -> void
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
            TEXT("Ism Proxy Entity [{}] does NOT have the required Transform feature. Unable to Setup the IsmProxy"), InHandle)
        { return; }

        const auto& Params = InParams.Get_Params();
        const auto& RendererName = Params.Get_RendererName();
        const auto& Mobility = Params.Get_Mobility();
        const auto& IsmComp = ck_ism_proxy_processor::FindRendererIsmComp(RendererName, Mobility);

        if (ck::Is_NOT_Valid(IsmComp))
        { return; }

        const auto& RelativeTransform = [&]
        {
            const auto& ProxyRelativeTransform = Params.Get_RelativeTransform();
            const auto& ProxyRelativeScale = ProxyRelativeTransform.GetScale3D();

            CK_ENSURE_IF_NOT(NOT UCk_Utils_Vector3_UE::Get_IsAnyAxisNearlyZero(ProxyRelativeScale),
                TEXT("IsmProxy Relative Scale has one or more axis nearly equal to 0. Setting it to 1 in non-shipping build"), ProxyRelativeScale)
            { return FTransform{ ProxyRelativeTransform.GetRotation(), ProxyRelativeTransform.GetLocation(), FVector::OneVector }; }

            return ProxyRelativeTransform;
        }();

        const auto& CurrentTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InHandle);
        const auto& CombinedTransform = CurrentTransform * RelativeTransform;

        constexpr auto TransformAsWorldSpace = true;
        const auto& InstanceIndex = IsmComp->AddInstance(CombinedTransform, TransformAsWorldSpace);
        InCurrent._IsmInstanceIndex = InstanceIndex;

        constexpr auto MarkRenderStateDirty = false;
        IsmComp->SetCustomData(InstanceIndex, InCurrent.Get_CustomDataValues(), MarkRenderStateDirty);
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
            IsmComp->SetCustomData(InCurrent.Get_IsmInstanceIndex(), NewCustomData, MarkRenderStateDirty);
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
            NewCustomDataIndex,
            NewCustomDataValue,
            InHandle)
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
            IsmComp->SetCustomDataValue(InCurrent.Get_IsmInstanceIndex(), NewCustomDataIndex, NewCustomDataValue, MarkRenderStateDirty);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
