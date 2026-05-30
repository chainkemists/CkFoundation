#include "CkCamera/Camera/CkCamera_Utils.h"

#include "CkCamera/CkCamera_Log.h"
#include "CkCamera/Camera/CkCamera_Component.h"
#include "CkCamera/Camera/CkCameraProfile_Utils.h"
#include "CkCamera/Camera/CameraModifier/CkCameraModifier_EntityScript.h"

#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

#include <GameFramework/Actor.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Camera_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Camera_ParamsData& InParams)
    -> FCk_Handle_Camera
{
    InHandle.Add<ck::FFragment_Camera_Params>(InParams);
    InHandle.AddOrGet<ck::FFragment_Camera_Current>();

    ck::FUtils_RecordOfCameraModifiers::AddIfMissing(InHandle);

    // The composed profile lives on its own entity (owned by the director). Modifiers contribute into it via
    // handle in DoContributeToProfile; ComposeProfile resolves it back into the director's read-cache each frame.
    auto ProfileEntity = UCk_Utils_CameraProfile_UE::Add(InHandle);
    InHandle.Get<ck::FFragment_Camera_Current>().Set_ProfileEntity(ProfileEntity);

    auto Director = Cast(InHandle);

    if (InParams.Get_OutputMode() == ECk_Camera_OutputMode::DriveCameraComponent)
    {
        UCameraComponent* OutputComponent = InParams.Get_OutputComponent();

        if (ck::Is_NOT_Valid(OutputComponent))
        {
            auto* OwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InHandle);

            if (ck::IsValid(OwningActor))
            {
                auto* NewComp = NewObject<UCk_CameraComponent>(OwningActor);

                if (auto* Root = OwningActor->GetRootComponent();
                    ck::IsValid(Root))
                {
                    NewComp->SetupAttachment(Root);
                }

                NewComp->RegisterComponent();
                OutputComponent = NewComp;
            }
            else
            {
                ck::camera::Warning(TEXT("Camera Add on entity [{}] requested DriveCameraComponent "
                    "but the entity has no owning actor and no component was supplied."), InHandle);
            }
        }

        // NOTE: ::Cast (global UE cast) — unqualified Cast would resolve to this class's own
        // typesafe-handle Cast (from CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE).
        if (auto* CkComp = ::Cast<UCk_CameraComponent>(OutputComponent);
            ck::IsValid(CkComp))
        {
            CkComp->Set_DirectorEntity(Director);
        }
    }

    return Director;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Camera_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Camera_UE::
    Get_ModifierCount(
        const FCk_Handle_Camera& InCamera)
    -> int32
{
    auto MutableCamera = InCamera;
    auto Count = int32{0};

    ck::FUtils_RecordOfCameraModifiers::ForEach_ValidEntry(MutableCamera,
    [&](FCk_Handle_CameraModifier)
    {
        ++Count;
    });

    return Count;
}

auto
    UCk_Utils_Camera_UE::
    Has_Modifier(
        const FCk_Handle_Camera& InCamera,
        TSubclassOf<UCk_CameraModifier_EntityScript> InModifierClass)
    -> bool
{
    auto MutableCamera = InCamera;
    auto Found = false;

    ck::FUtils_RecordOfCameraModifiers::ForEach_ValidEntry(MutableCamera,
    [&](FCk_Handle_CameraModifier InModifier)
    {
        if (Found)
        { return; }

        if (NOT InModifier.Has<ck::FFragment_CameraModifier_Params>())
        { return; }

        if (InModifier.Get<ck::FFragment_CameraModifier_Params>().Get_ModifierClass() == InModifierClass)
        { Found = true; }
    });

    return Found;
}

auto
    UCk_Utils_Camera_UE::
    Get_DominantModifierClass(
        const FCk_Handle_Camera& InCamera)
    -> TSubclassOf<UCk_CameraModifier_EntityScript>
{
    auto MutableCamera = InCamera;

    if (NOT MutableCamera.Has<ck::FFragment_Camera_Current>())
    { return nullptr; }

    return MutableCamera.Get<ck::FFragment_Camera_Current>().Get_DominantModifierClass();
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Camera_UE, FCk_Handle_Camera, ck::FFragment_Camera_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Camera_UE::
    Request_AddModifier(
        FCk_Handle_Camera& InCamera,
        const FCk_Request_Camera_AddModifier& InRequest)
    -> FCk_Handle_Camera
{
    InCamera.AddOrGet<ck::FFragment_Camera_Requests>()._Requests.Emplace(InRequest);
    return InCamera;
}

auto
    UCk_Utils_Camera_UE::
    Request_RemoveModifier(
        FCk_Handle_Camera& InCamera,
        const FCk_Request_Camera_RemoveModifier& InRequest)
    -> FCk_Handle_Camera
{
    InCamera.AddOrGet<ck::FFragment_Camera_Requests>()._Requests.Emplace(InRequest);
    return InCamera;
}

auto
    UCk_Utils_Camera_UE::
    Request_SetOrientationIntention(
        FCk_Handle_Camera& InCamera,
        FVector InOrientationIntention)
    -> FCk_Handle_Camera
{
    if (InCamera.Has<ck::FFragment_Camera_Current>())
    {
        InCamera.Get<ck::FFragment_Camera_Current>().Set_OrientationIntention(InOrientationIntention);
    }
    return InCamera;
}

auto
    UCk_Utils_Camera_UE::
    Get_ViewRotation(
        const FCk_Handle_Camera& InCamera)
    -> FRotator
{
    if (ck::Is_NOT_Valid(InCamera) || NOT InCamera.Has<ck::FFragment_Camera_Current>())
    { return FRotator::ZeroRotator; }

    return InCamera.Get<ck::FFragment_Camera_Current>().Get_ViewInfo().Rotation;
}

// --------------------------------------------------------------------------------------------------------------------
