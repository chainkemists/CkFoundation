#include "CkVfx_Utils.h"

#include "Vfx/CkVfx_Fragment.h"

#include "CkFx/CkFx_Log.h"

#include <NiagaraFunctionLibrary.h>
#include <NiagaraComponent.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Vfx_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Vfx_ParamsData& InParams)
    -> FCk_Handle_Vfx
{
     auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
     {
        UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_Name());

        InNewEntity.Add<ck::FFragment_Vfx_Params>(InParams);
        InNewEntity.Add<ck::FFragment_Vfx_Current>();
    });

    auto NewVfxEntity = Cast(NewEntity);

    RecordOfVfx_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfVfx_Utils::Request_Connect(InHandle, NewVfxEntity);

    return NewVfxEntity;
}

auto
    UCk_Utils_Vfx_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleVfx_ParamsData& InParams)
    -> TArray<FCk_Handle_Vfx>
{
    return ck::algo::Transform<TArray<FCk_Handle_Vfx>>(InParams.Get_VfxParams(),
    [&](const FCk_Fragment_Vfx_ParamsData& InVfxParams)
    {
        return Add(InHandle, InVfxParams);
    });
}

auto
    UCk_Utils_Vfx_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return RecordOfVfx_Utils::Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Vfx_UE, FCk_Handle_Vfx, ck::FFragment_Vfx_Current, ck::FFragment_Vfx_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Vfx_UE::
    TryGet_Vfx(
        const FCk_Handle& InVfxOwnerEntity,
        FGameplayTag InVfxName)
    -> FCk_Handle_Vfx
{
    return RecordOfVfx_Utils::Get_ValidEntry_If(InVfxOwnerEntity, ck::algo::MatchesGameplayLabelExact{InVfxName});
}

auto
    UCk_Utils_Vfx_UE::
    Get_ParticleSystem(
        const FCk_Handle_Vfx& InVfxHandle)
    -> UNiagaraSystem*
{
    return InVfxHandle.Get<ck::FFragment_Vfx_Params>().Get_Params().Get_ParticleSystem();
}

auto
    UCk_Utils_Vfx_UE::
    Get_AttachmentSettings(
        const FCk_Handle_Vfx& InVfxHandle)
    -> FCk_Vfx_AttachmentSettings
{
    return InVfxHandle.Get<ck::FFragment_Vfx_Params>().Get_Params().Get_AttachmentSettings();
}

auto
    UCk_Utils_Vfx_UE::
    ForEach_Vfx(
        FCk_Handle& InVfxOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Vfx>
{
    auto Vfx = TArray<FCk_Handle_Vfx>{};

    ForEach_Vfx(InVfxOwnerEntity, [&](FCk_Handle_Vfx InVfx)
    {
        Vfx.Emplace(InVfx);

        std::ignore = InDelegate.ExecuteIfBound(InVfx, InOptionalPayload);
    });

    return Vfx;
}

auto
    UCk_Utils_Vfx_UE::
    ForEach_Vfx(
        FCk_Handle& InVfxOwnerEntity,
        const TFunction<void(FCk_Handle_Vfx)>& InFunc)
    -> void
{
    RecordOfVfx_Utils::ForEach_ValidEntry(InVfxOwnerEntity, InFunc);
}

auto
    UCk_Utils_Vfx_UE::
    Request_PlayAttached(
        FCk_Handle_Vfx& InVfxHandle,
        const FCk_Request_Vfx_PlayAttached& InRequest)
    -> FCk_Handle_Vfx
{
    // TODO: Move this to processor

    const auto& ParticleSystem     = Get_ParticleSystem(InVfxHandle);
    const auto& AttachmentSettings = Get_AttachmentSettings(InVfxHandle);
    const auto& TransformRules     = AttachmentSettings.Get_TransformRules();

    const auto& SpawnedVfx = UNiagaraFunctionLibrary::SpawnSystemAttached
    (
        ParticleSystem,
        InRequest.Get_AttachComponent().Get(),
        AttachmentSettings.Get_SocketName(),
        InRequest.Get_RelativeTransformSettings().Get_Location(),
        InRequest.Get_RelativeTransformSettings().Get_Rotation(),
        EAttachLocation::Type::KeepRelativeOffset,
        true
    );

    CK_ENSURE_IF_NOT(ck::IsValid(SpawnedVfx), TEXT("Failed to create new VFX to Play Attached"))
    { return InVfxHandle; }

    DoSet_NiagaraInstanceParameter(SpawnedVfx, InRequest.Get_InstanceParameterSettings());

    SpawnedVfx->SetAbsolute
    (
        TransformRules.Get_LocationPolicy() == ECk_VFX_Location_Policy::UseAbsoluteLocation,
        TransformRules.Get_RotationPolicy() == ECk_VFX_Rotation_Policy::UseAbsoluteRotation,
        TransformRules.Get_ScalePolicy()    == ECk_VFX_Scale_Policy::UseAbsoluteScale
    );

    return InVfxHandle;
}

auto
    UCk_Utils_Vfx_UE::
    Request_PlayAtLocation(
        FCk_Handle_Vfx& InVfxHandle,
        const FCk_Request_Vfx_PlayAtLocation& InRequest)
    -> FCk_Handle_Vfx
{
    // TODO: Move this to processor

    const auto& ParticleSystem     = Get_ParticleSystem(InVfxHandle);
    const auto& AttachmentSettings = Get_AttachmentSettings(InVfxHandle);
    const auto& TransformRules     = AttachmentSettings.Get_TransformRules();

    const auto& SpawnedVfx = UNiagaraFunctionLibrary::SpawnSystemAtLocation
    (
        InRequest.Get_Outer().Get(),
        ParticleSystem,
        InRequest.Get_TransformSettings().Get_Location(),
        InRequest.Get_TransformSettings().Get_Rotation()
    );

    CK_ENSURE_IF_NOT(ck::IsValid(SpawnedVfx), TEXT("Failed to create new VFX to Play At Location"))
    { return InVfxHandle; }

    DoSet_NiagaraInstanceParameter(SpawnedVfx, InRequest.Get_InstanceParameterSettings());

    SpawnedVfx->SetAbsolute
    (
        TransformRules.Get_LocationPolicy() == ECk_VFX_Location_Policy::UseAbsoluteLocation,
        TransformRules.Get_RotationPolicy() == ECk_VFX_Rotation_Policy::UseAbsoluteRotation,
        TransformRules.Get_ScalePolicy()    == ECk_VFX_Scale_Policy::UseAbsoluteScale
    );

    return InVfxHandle;
}

auto
    UCk_Utils_Vfx_UE::
    DoSet_NiagaraInstanceParameter(
        UNiagaraComponent* InVfx,
        const FCk_Vfx_InstanceParameterSettings& InInstanceParameterSettings)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InVfx), TEXT("Cannot set the Niagara Instance Parameters because the Vfx is INVALID"))
    { return; }

    for (const auto& Param : InInstanceParameterSettings.Get_ParticleSysParams())
    {
        switch(Param.ParamType.GetValue())
        {
            case PSPT_None:
            { break; }
            case PSPT_Scalar:
            {
                InVfx->SetFloatParameter(Param.Name, Param.Scalar);
                break;
            }
            case PSPT_ScalarRand:
            {
                InVfx->SetFloatParameter(Param.Name, FMath::RandRange(Param.Scalar_Low, Param.Scalar));
                break;
            }
            case PSPT_Vector:
            {
                InVfx->SetVectorParameter(Param.Name, Param.Vector);
                break;
            }
            case PSPT_VectorUnitRand:
            case PSPT_VectorRand:
            {
                InVfx->SetVectorParameter
                (
                    Param.Name,
                    FVector
                    {
                        FMath::RandRange (Param.Vector_Low.X, Param.Vector.X),
                        FMath::RandRange (Param.Vector_Low.Y, Param.Vector.Y),
                        FMath::RandRange (Param.Vector_Low.Z, Param.Vector.Z),
                    }
                );
                break;
            }
            case PSPT_Color:
            {
                InVfx->SetColorParameter(Param.Name, Param.Color);
                break;
            }
            case PSPT_Actor:
            {
                InVfx->SetActorParameter(Param.Name, Param.Actor);
                break;
            }
            case PSPT_Material:
            {
                InVfx->SetMaterialByName(Param.Name, Param.Material);
                break;
            }
            case PSPT_MAX:
            [[fallthrough]];
            default:
            {
                break;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
