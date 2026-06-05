#include "CkCameraLayer_EntityScript.h"

#include "CkCamera/CkCamera_Log.h"
#include "CkCamera/Camera/CameraLayer/CkCameraLayer_Fragment.h"
#include "CkCamera/Camera/CameraLayer/CkCameraLayer_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CameraLayer_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto ParentFlow = Super::Construct(InHandle, InSpawnParams);

    if (ck::TUtils_CameraLayer_OwningCamera::Has(InHandle))
    {
        _OwningCamera = ck::TUtils_CameraLayer_OwningCamera::Get_StoredEntity(InHandle);
    }

    return ParentFlow;
}

auto
    UCk_CameraLayer_EntityScript::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    auto Self = ck::StaticCast<FCk_Handle_CameraLayer>(Get_AssociatedEntity());
    if (ck::Is_NOT_Valid(Self))
    { return; }

    EnterLayer(Self);
}

auto
    UCk_CameraLayer_EntityScript::
    EndPlay()
    -> void
{
    // Fallback for cascade-destroyed layers whose processor-driven Exit never runs. Deduped via
    // FTag_CameraLayer_Active inside ExitLayer — a no-op if the processor already handled it.
    if (auto Self = ck::StaticCast<FCk_Handle_CameraLayer>(Get_AssociatedEntity());
        ck::IsValid(Self))
    {
        ExitLayer(Self);
    }

    Super::EndPlay();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CameraLayer_EntityScript::
    Get_EffectiveReplication() const
    -> ECk_Replication
{
    return ECk_Replication::DoesNotReplicate;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CameraLayer_EntityScript::
    EnterLayer(
        FCk_Handle_CameraLayer InHandle)
    -> void
{
    ck::camera::VeryVerbose(TEXT("[Camera] EnterLayer [{}] on entity [{}]"), GetClass(), InHandle);

    InHandle.AddOrGet<ck::FTag_CameraLayer_Active>();
    DoEnter(InHandle);
}

auto
    UCk_CameraLayer_EntityScript::
    ExitLayer(
        FCk_Handle_CameraLayer InHandle)
    -> void
{
    if (NOT InHandle.Has<ck::FTag_CameraLayer_Active>())
    { return; }

    ck::camera::VeryVerbose(TEXT("[Camera] ExitLayer [{}] on entity [{}]"), GetClass(), InHandle);

    InHandle.Try_Remove<ck::FTag_CameraLayer_Active>();
    DoExit(InHandle);

    // Remove the attribute modifiers this layer acquired — they live under the tuner attributes (not this layer
    // entity), so they would otherwise outlive the layer. Robust across every destruction path (prune / cascade).
    UCk_Utils_CameraLayer_UE::Remove_AllAcquiredModifiers(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CameraLayer_EntityScript::
    Tick(
        FCk_Handle_CameraLayer InHandle,
        FCk_Time InDeltaT)
    -> void
{
    DoTick(InHandle, InDeltaT);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CameraLayer_EntityScript::
    Blend(
        FCk_Handle_CameraLayer InHandle,
        float InAlpha)
    -> void
{
    DoBlend(InHandle, InAlpha);
}

auto
    UCk_CameraLayer_EntityScript::
    FullyBlendedIn(
        FCk_Handle_CameraLayer InHandle)
    -> void
{
    ck::camera::VeryVerbose(TEXT("[Camera] FullyBlendedIn [{}] on entity [{}]"), GetClass(), InHandle);
    DoFullyBlendedIn(InHandle);
}

auto
    UCk_CameraLayer_EntityScript::
    FullyBlendedOut(
        FCk_Handle_CameraLayer InHandle)
    -> void
{
    ck::camera::VeryVerbose(TEXT("[Camera] FullyBlendedOut [{}] on entity [{}]"), GetClass(), InHandle);
    DoFullyBlendedOut(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CameraLayer_EntityScript::
    Get_LayerTag() const
    -> FGameplayTag
{
    return UCk_Utils_Object_UE::Get_TagFromClassName(
        GetClass(), ck::Format_UE(TEXT("Auto-generated camera layer tag for {}"), GetClass()));
}

auto
    UCk_CameraLayer_EntityScript::
    Get_OwningCamera() const
    -> FCk_Handle_Camera
{
    return _OwningCamera;
}

// --------------------------------------------------------------------------------------------------------------------
