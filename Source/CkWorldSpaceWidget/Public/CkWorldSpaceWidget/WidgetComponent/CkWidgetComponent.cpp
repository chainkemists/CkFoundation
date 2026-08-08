#include "CkWidgetComponent.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Game/CkGame_Utils.h"

#include <Kismet/KismetMathLibrary.h>
#include <Camera/PlayerCameraManager.h>
#include <Kismet/GameplayStatics.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_WidgetComponent_UE::
    UCk_WidgetComponent_UE()
{
    bWantsInitializeComponent = true;

    this->SetGenerateOverlapEvents(false);
    bWindowFocusable = false;
    bDrawAtDesiredSize = true;

    BodyInstance.SetCollisionProfileName(TEXT("NoCollision"));
    CanCharacterStepUpOn = ECB_No;
    CastShadow      = false;
    bUseAsOccluder  = false;
    bReceivesDecals = false;
}

auto
    UCk_WidgetComponent_UE::
    TickComponent(
        float InDeltaTime,
        ELevelTick InTickType,
        FActorComponentTickFunction* InThisTickFunction)
    -> void
{
    Super::TickComponent(InDeltaTime, InTickType, InThisTickFunction);

    if (bHiddenInGame || _CurrentWidgetSpacePolicy != ECk_WidgetComponent_WidgetSpacePolicy::AlwaysFaceCamera)
    { return; }

    const auto* PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (ck::Is_NOT_Valid(PlayerCameraManager))
    { return; }

    const auto& CameraQuat = PlayerCameraManager->GetCameraRotation().Quaternion();

    const auto& ForwardVec = CameraQuat.GetForwardVector() * -1.0f;
    const auto& RightVec = CameraQuat.GetRightVector();
    const auto& UpVec = CameraQuat.GetUpVector() * -1.0f;

    const auto& NewRot = UKismetMathLibrary::MakeRotationFromAxes(ForwardVec, RightVec, UpVec);

    SetWorldRotation(NewRot);
}

auto
    UCk_WidgetComponent_UE::
    InitializeComponent()
    -> void
{
    Super::InitializeComponent();

    Request_SetWidgetSpacePolicy(Get_Params().Get_WidgetSpacePolicy());
}

auto
    UCk_WidgetComponent_UE::
    InitWidget()
    -> void
{
    Super::InitWidget();

    //Enable tick for clients only
    PrimaryComponentTick.SetTickFunctionEnable(GetWorld()->GetNetMode() != NM_DedicatedServer && GetWorld()->GetNetMode() != NM_ListenServer);

    if (UCk_Utils_Game_UE::Get_IsInGame(this))
    {
        OnPostInitWidget();
    }
}

auto
    UCk_WidgetComponent_UE::
    Request_SetWidgetAndBind(
        UCk_UserWidget_UE* InNewWidget)
    -> void
{
    SetWidget(InNewWidget);

    OnPostInitWidget();
}

auto
    UCk_WidgetComponent_UE::
    Request_SetWidgetSpacePolicy(
        ECk_WidgetComponent_WidgetSpacePolicy InNewPolicy)
    -> void
{
    if (_CurrentWidgetSpacePolicy == InNewPolicy)
    { return; }

    _CurrentWidgetSpacePolicy = InNewPolicy;

    switch (_CurrentWidgetSpacePolicy)
    {
        case ECk_WidgetComponent_WidgetSpacePolicy::Default:
        {
            break;
        }
        case ECk_WidgetComponent_WidgetSpacePolicy::SwitchToScreenSpaceAtRuntime:
        {
            Space = EWidgetSpace::Screen;
            break;
        }
        case ECk_WidgetComponent_WidgetSpacePolicy::AlwaysFaceCamera:
        {
            if (Space == EWidgetSpace::Screen)
            {
                _Params = ParamsType{ ECk_WidgetComponent_WidgetSpacePolicy::Default };
                break;
            }

            auto NewScale = GetRelativeTransform().GetScale3D();

            NewScale.Y *= -1.0f;
            NewScale.Z *= -1.0f;

            SetRelativeScale3D(NewScale);

            break;
        }
        default:
        {
            CK_INVALID_ENUM(_CurrentWidgetSpacePolicy);
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

