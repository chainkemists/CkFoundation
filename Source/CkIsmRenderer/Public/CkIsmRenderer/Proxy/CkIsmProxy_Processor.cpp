#include "CkIsmProxy_Processor.h"

#include "CkIsmProxy_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkIsmRenderer/CkIsmSubsystem.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_ism_proxy_processor
{
    auto
        FindRendererIsmComp(
            const UWorld* InWorld,
            const UCk_IsmRenderer_Data* InRendererData)
        -> TWeakObjectPtr<UInstancedStaticMeshComponent>
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InWorld),
            TEXT("Trying to find ISM Renderer Component from an INVALID World"))
        { return {}; }

        const auto& IsmRendererSubsystem = InWorld->GetSubsystem<UCk_IsmRenderer_Subsystem_UE>(InWorld);

        CK_ENSURE_IF_NOT(ck::IsValid(IsmRendererSubsystem),
            TEXT("Could NOT find the IsmRender_Subsystem for the World [{}]"), InWorld)
        { return {}; }

        return IsmRendererSubsystem->FindOrCache_IsmComponent(InRendererData);
    }

    auto
        ApplyCustomPrimitiveDataToComponent(
            UInstancedStaticMeshComponent* InComponent,
            const FCk_CustomPrimitiveData& InData)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InComponent),
            TEXT("Trying to apply custom primitive data to an INVALID ISM component"))
        { return; }

        const auto& DataValue = InData.Get_Value();
        const auto& DataIndex = InData.Get_CustomDataIndex();

        switch (DataValue.Get_Type())
        {
            case ECk_CustomPrimitiveData_Type::Float:
            {
                InComponent->SetCustomPrimitiveDataFloat(DataIndex, DataValue.Get_Float());
                break;
            }
            case ECk_CustomPrimitiveData_Type::Vector2:
            {
                InComponent->SetCustomPrimitiveDataVector2(DataIndex, DataValue.Get_Vector2());
                break;
            }
            case ECk_CustomPrimitiveData_Type::Vector3:
            {
                InComponent->SetCustomPrimitiveDataVector3(DataIndex, DataValue.Get_Vector3());
                break;
            }
            case ECk_CustomPrimitiveData_Type::Vector4:
            {
                InComponent->SetCustomPrimitiveDataVector4(DataIndex, DataValue.Get_Vector4());
                break;
            }
            case ECk_CustomPrimitiveData_Type::LinearColor:
            {
                const auto& Color = DataValue.Get_LinearColor();
                InComponent->SetCustomPrimitiveDataVector4(DataIndex, FVector4(Color.R, Color.G, Color.B, Color.A));
                break;
            }
            default:
            {
                CK_INVALID_ENUM(DataValue.Get_Type());
                break;
            }
        }
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
        if (UCk_Utils_Net_UE::Get_EntityNetMode(_TransientEntity) == ECk_Net_NetModeType::Host)
        { return; }

        TProcessor::DoTick(InDeltaT);
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
        using namespace ck_ism_proxy_processor;

        const auto& RendererData = InParams.Get_IsmRenderer().Get();
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);

        const auto& IsmComp = FindRendererIsmComp(World, RendererData);

        if (ck::Is_NOT_Valid(IsmComp))
        { return; }

        InHandle.Remove<MarkedDirtyBy>();

        const auto& NumCustomDataFloats = IsmComp->NumCustomDataFloats;
        InCurrent._CustomInstanceDataValues.Init(0, NumCustomDataFloats);

        for (const auto& CustomInstanceDataDefaults = InParams.Get_CustomInstanceDataDefaults();
            const auto& Override : CustomInstanceDataDefaults)
        {
            const auto& DataIndex = Override.Get_CustomDataIndex();
            const auto& Value = Override.Get_Value();
            const auto& FloatArray = Value.ConvertToFloatArray();

            CK_ENSURE_IF_NOT(NOT FloatArray.IsEmpty(),
                TEXT("Custom Instance Data Override at index [{}] has no float values"), DataIndex)
            { continue; }

            for (auto FloatIndex = 0; FloatIndex < FloatArray.Num(); ++FloatIndex)
            {
                const auto& TargetIndex = DataIndex + FloatIndex;

                CK_ENSURE_IF_NOT(InCurrent.Get_CustomInstanceDataValues().IsValidIndex(TargetIndex),
                    TEXT("ISM Proxy [{}] tried to set custom instance data at index [{}], but the ISM component only has [{}] custom data floats"),
                    InHandle,
                    TargetIndex,
                    NumCustomDataFloats)
                { continue; }

                InCurrent._CustomInstanceDataValues[TargetIndex] = FloatArray[FloatIndex];
            }
        }

        UCk_Utils_IsmProxy_UE::Request_NeedsInstanceAdded(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_AddInstance::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        _World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_IsmProxy_AddInstance::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FFragment_Transform& InCurrentTransform) const
        -> void
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
            TEXT("Ism Proxy Entity [{}] does NOT have the required Transform feature. Unable to render the IsmProxy"), InHandle)
        { return; }

        using namespace ck_ism_proxy_processor;

        const auto& RendererData = InParams.Get_IsmRenderer().Get();
        const auto& IsmComp = FindRendererIsmComp(_World.Get(), RendererData);

        if (ck::Is_NOT_Valid(IsmComp))
        { return; }

        const auto& Get_TransformWithLocalOffset = [&](const FTransform& InTransform) -> FTransform
        {
            const auto& CombinedLocation = InTransform.GetLocation() + InParams.Get_LocalLocationOffset();
            const auto& CombinedRotation = InParams.Get_LocalRotationOffset().Quaternion() * InTransform.GetRotation();

            CK_ENSURE_IF_NOT(NOT UCk_Utils_Vector3_UE::Get_IsAnyAxisNearlyZero(InParams.Get_ScaleMultiplier()),
                TEXT("IsmProxy Scale Multiplier has one or more axis nearly equal to 0. Setting it to 1 in non-shipping build"), InParams.Get_ScaleMultiplier())
            { return FTransform{ CombinedRotation.Rotator(), CombinedLocation, FVector::OneVector }; }

            const auto& CombinedScale = InTransform.GetScale3D() * InParams.Get_ScaleMultiplier();

            return FTransform{ CombinedRotation.Rotator(), CombinedLocation, CombinedScale };
        };

        constexpr auto TransformAsWorldSpace = true;
        const auto& CurrentTransformWithLocalOffset = Get_TransformWithLocalOffset(InCurrentTransform.Get_Transform());

        const auto& InstanceIndex = IsmComp->AddInstanceById(CurrentTransformWithLocalOffset, TransformAsWorldSpace);
        InCurrent._IsmInstanceIndex = InstanceIndex;

        IsmComp->SetCustomDataById(InstanceIndex, InCurrent.Get_CustomInstanceDataValues());

        if (RendererData->Get_Mobility() == ECk_Mobility::Movable)
        {
            InHandle.AddOrGet<ck::FTag_IsmProxy_Movable>();

            if (InHandle.Has<ck::FFragment_Transform_Previous>())
            {
                const auto& PreviousTransform = InHandle.Get<ck::FFragment_Transform_Previous>();
                const auto& PreviousTransformWithLocalOffset = Get_TransformWithLocalOffset(PreviousTransform.Get_Transform());

                IsmComp->SetPreviousTransformById(InstanceIndex, PreviousTransformWithLocalOffset, TransformAsWorldSpace);
            }

            // Movable ISM instances with the Recreate Policy are re-added again every tick
            if (RendererData->Get_UpdatePolicy() == ECk_Ism_InstanceUpdatePolicy::Recreate)
            { return; }
        }

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_TransformInstance::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);

        _Isms.Reset();
        TProcessor::DoTick(InDeltaT);

        for (const auto Ism : _Isms)
        {
            Ism->MarkRenderStateDirty();
        }
    }

    auto
        FProcessor_IsmProxy_TransformInstance::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_IsmProxy_Current& InCurrent,
            const FFragment_Transform& InTransform)
        -> void
    {
        using namespace ck_ism_proxy_processor;

        const auto& RendererData = InParams.Get_IsmRenderer().Get();
        const auto& IsmComp = FindRendererIsmComp(_World.Get(), RendererData);
        const auto InstanceId = InCurrent.Get_IsmInstanceIndex();

        if (NOT IsmComp->IsValidId(InstanceId))
        { return; }

        _Isms.Add(IsmComp.Get());

        const auto& Get_TransformWithLocalOffset = [&](const FTransform& Transform) -> FTransform
        {
            const auto& CombinedLocation = Transform.GetLocation() + InParams.Get_LocalLocationOffset();
            const auto& CombinedRotation = InParams.Get_LocalRotationOffset().Quaternion() * Transform.GetRotation();
            const auto& CombinedScale = Transform.GetScale3D() * InParams.Get_ScaleMultiplier();

            return FTransform{ CombinedRotation.Rotator(), CombinedLocation, CombinedScale };
        };

        constexpr auto TransformAsWorldSpace = false;

        // TODO: this does not seem to do anything and just costs us CPU cycles. We thought it might help with TSR dithering
        // See more explanation in IsmProxyRenderer_Processor. NOTE that you have to enable 'SetHasPerInstancePrevTransform'
        // to `true` (which should probably be a Renderer Params field)
        //if (InHandle.Has<ck::FFragment_Transform_Previous>())
        //{
        //    const auto& PreviousTransform = InHandle.Get<ck::FFragment_Transform_Previous>();
        //    const auto& PreviousTransformWithLocalOffset = PreviousTransform.Get_Transform();// Get_TransformWithLocalOffset(PreviousTransform.Get_Transform());

        //    IsmComp->SetPreviousTransformById(InstanceId, PreviousTransformWithLocalOffset, TransformAsWorldSpace);
        //}

        IsmComp->UpdateInstanceTransformById(InstanceId,
            Get_TransformWithLocalOffset(InTransform.Get_Transform()), TransformAsWorldSpace);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_EnsureStaticNotMoved_DEBUG::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            const FFragment_IsmProxy_Current& InCurrent)
        -> void
    {
        const auto& Mobility = InParams.Get_IsmRenderer()->Get_Mobility();
        CK_TRIGGER_ENSURE(TEXT("ISM Proxy [{}] with Mobility [{}] had its Transform changed.\n"
                "If this ISM Proxy is meant to move its Mobility shouldn't be [{}]"),
            InHandle,
            Mobility,
            Mobility);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_EndPlay::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        _World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);

        if (UCk_Utils_Net_UE::Get_EntityNetMode(_TransientEntity) == ECk_Net_NetModeType::Host)
        { return; }

        TProcessor::DoTick(InDeltaT);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent) const
        -> void
    {
        using namespace ck_ism_proxy_processor;

        const auto& RendererData = InParams.Get_IsmRenderer().Get();
        const auto& IsmComp = FindRendererIsmComp(_World.Get(), RendererData);

        if (ck::Is_NOT_Valid(IsmComp))
        { return; }

        if (IsmComp->IsValidId(InCurrent.Get_IsmInstanceIndex()))
        {
            IsmComp->RemoveInstanceById(InCurrent.Get_IsmInstanceIndex());
            InCurrent._IsmInstanceIndex = FPrimitiveInstanceId{INDEX_NONE};
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(_TransientEntity);

        TProcessor::DoTick(InDeltaT);
    }

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
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }), policy::DontResetContainer{});
        });
    }

    auto
        FProcessor_IsmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_SetCustomInstanceData& InRequest) const
        -> void
    {
        const auto& NewCustomData = InRequest.Get_CustomData();

        if (NewCustomData.IsEmpty())
        { return; }

        const auto& CurrentCustomData = InCurrent.Get_CustomInstanceDataValues();

        CK_ENSURE_IF_NOT(CurrentCustomData.Num() == NewCustomData.Num(),
            TEXT("Trying to set [{}] number of custom instance data on Ism Proxy [{}], but it was setup to contain AT MOST [{}] elements\n"
                 "Setting the custom instance data in its entirety must have the exact same number of elements, otherwise use SetCustomInstanceDataValue"),
            NewCustomData.Num(),
            InHandle,
            CurrentCustomData.Num())
        { return; }

        InCurrent._CustomInstanceDataValues = NewCustomData;

        // TEMP: only movable ISM instances are updated again (every tick)
        if (const auto& Mobility = UCk_Utils_IsmProxy_UE::Get_Mobility(InHandle);
            Mobility == ECk_Mobility::Movable)
        {
            using namespace ck_ism_proxy_processor;

            const auto& RendererData = InParams.Get_IsmRenderer().Get();
            const auto& IsmComp = FindRendererIsmComp(_World.Get(), RendererData);

            if (ck::Is_NOT_Valid(IsmComp))
            { return; }

            if (IsmComp->IsValidId(InCurrent.Get_IsmInstanceIndex()))
            {
                IsmComp->SetCustomDataById(InCurrent.Get_IsmInstanceIndex(), NewCustomData);
            }
        }
    }

    auto
        FProcessor_IsmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_SetCustomInstanceDataValue& InRequest) const
        -> void
    {
        const auto& CurrentCustomInstanceData = InCurrent.Get_CustomInstanceDataValues();
        const auto& NewCustomDataIndex = InRequest.Get_CustomDataIndex();
        const auto& NewCustomDataValue = InRequest.Get_CustomDataValue();

        CK_ENSURE_IF_NOT(CurrentCustomInstanceData.IsValidIndex(NewCustomDataIndex),
            TEXT("Trying to set custom data value [{}] at index [{}] on Ism Proxy [{}], but it was setup to contain AT MOST [{}] elements"),
            NewCustomDataValue,
            NewCustomDataIndex,
                InHandle,
                CurrentCustomInstanceData.Num())
        { return; }

        InCurrent._CustomInstanceDataValues[NewCustomDataIndex] = NewCustomDataValue;

        // TEMP: only movable ISM instances are updated again (every tick)
        if (const auto& Mobility = UCk_Utils_IsmProxy_UE::Get_Mobility(InHandle);
            Mobility == ECk_Mobility::Movable)
        {
            using namespace ck_ism_proxy_processor;

            const auto& RendererData = InParams.Get_IsmRenderer().Get();
            const auto& IsmComp = FindRendererIsmComp(_World.Get(), RendererData);

            if (ck::Is_NOT_Valid(IsmComp))
            { return; }

            if (IsmComp->IsValidId(InCurrent.Get_IsmInstanceIndex()))
            {
                IsmComp->SetCustomDataValueById(InCurrent.Get_IsmInstanceIndex(), NewCustomDataIndex, NewCustomDataValue);
            }
        }
    }

    auto
        FProcessor_IsmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_SetCustomPrimitiveData& InRequest) const
        -> void
    {
        using namespace ck_ism_proxy_processor;

        const auto& RendererData = InParams.Get_IsmRenderer().Get();
        const auto& IsmComp = FindRendererIsmComp(_World.Get(), RendererData);

        if (ck::Is_NOT_Valid(IsmComp))
        { return; }

        ApplyCustomPrimitiveDataToComponent(IsmComp.Get(), InRequest.Get_Data());
    }

    auto
        FProcessor_IsmProxy_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_IsmProxy_Params& InParams,
            FFragment_IsmProxy_Current& InCurrent,
            const FCk_Request_IsmProxy_EnableDisable& InRequest) const
            -> void
    {
        using namespace ck_ism_proxy_processor;

        switch(InRequest.Get_EnableDisable())
        {
            case ECk_EnableDisable::Enable:
            {
                if (InHandle.Try_Remove<FTag_IsmProxy_Disabled>())
                {
                    InHandle.AddOrGet<ck::FTag_IsmProxy_NeedsInstanceAdded>();
                }

                break;
            }
            case ECk_EnableDisable::Disable:
            {
                InHandle.AddOrGet<FTag_IsmProxy_Disabled>();
                InHandle.Try_Remove<ck::FTag_IsmProxy_NeedsInstanceAdded>();

                const auto& RendererData = InParams.Get_IsmRenderer().Get();

                if (const auto& IsmComp = FindRendererIsmComp(_World.Get(), RendererData); 
                    IsmComp->IsValidId(InCurrent.Get_IsmInstanceIndex()))
                {
                    IsmComp->RemoveInstanceById(InCurrent.Get_IsmInstanceIndex());
                    InCurrent._IsmInstanceIndex = FPrimitiveInstanceId{ INDEX_NONE };
                }
                
                break;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------