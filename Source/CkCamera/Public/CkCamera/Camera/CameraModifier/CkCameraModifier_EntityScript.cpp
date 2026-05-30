#include "CkCameraModifier_EntityScript.h"

#include "CkCamera/CkCamera_Log.h"
#include "CkCamera/Camera/CkCamera_Fragment.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CameraModifier_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto ParentFlow = Super::Construct(InHandle, InSpawnParams);

    if (ck::TUtils_CameraModifier_OwningCamera::Has(InHandle))
    {
        _OwningCamera = ck::TUtils_CameraModifier_OwningCamera::Get_StoredEntity(InHandle);
    }

    return ParentFlow;
}

auto
    UCk_CameraModifier_EntityScript::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    auto Self = ck::StaticCast<FCk_Handle_CameraModifier>(Get_AssociatedEntity());
    if (ck::Is_NOT_Valid(Self))
    { return; }

    EnterModifier(Self);
}

auto
    UCk_CameraModifier_EntityScript::
    EndPlay()
    -> void
{
    // Fallback for cascade-destroyed modifiers whose processor-driven Exit never runs. Deduped via
    // FTag_CameraModifier_Active inside ExitModifier — a no-op if the processor already handled it.
    auto Self = ck::StaticCast<FCk_Handle_CameraModifier>(Get_AssociatedEntity());
    if (ck::IsValid(Self))
    {
        ExitModifier(Self);
    }

    Super::EndPlay();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CameraModifier_EntityScript::
    Get_EffectiveReplication() const
    -> ECk_Replication
{
    return ECk_Replication::DoesNotReplicate;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CameraModifier_EntityScript::
    EnterModifier(
        FCk_Handle_CameraModifier InHandle)
    -> void
{
    ck::camera::VeryVerbose(TEXT("[Camera] EnterModifier [{}] on entity [{}]"), GetClass(), InHandle);

    InHandle.AddOrGet<ck::FTag_CameraModifier_Active>();
    DoEnter(InHandle);
}

auto
    UCk_CameraModifier_EntityScript::
    ExitModifier(
        FCk_Handle_CameraModifier InHandle)
    -> void
{
    if (NOT InHandle.Has<ck::FTag_CameraModifier_Active>())
    { return; }

    ck::camera::VeryVerbose(TEXT("[Camera] ExitModifier [{}] on entity [{}]"), GetClass(), InHandle);

    InHandle.Try_Remove<ck::FTag_CameraModifier_Active>();
    DoExit(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CameraModifier_EntityScript::
    ContributeToProfile(
        FCk_Handle_CameraModifier InHandle,
        FCk_CameraProfile& InOutProfile,
        float InBlendAlpha)
    -> void
{
    DoContributeToProfile(InHandle, InOutProfile, InBlendAlpha);
}

auto
    UCk_CameraModifier_EntityScript::
    Tick(
        FCk_Handle_CameraModifier InHandle,
        FCk_Time InDeltaT)
    -> void
{
    DoTick(InHandle, InDeltaT);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CameraModifier_EntityScript::
    Get_OwningCamera() const
    -> FCk_Handle_Camera
{
    return _OwningCamera;
}

// --------------------------------------------------------------------------------------------------------------------
