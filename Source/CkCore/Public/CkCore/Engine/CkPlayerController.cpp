#include "CkPlayerController.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Build/CkBuild_Macros.h"

#include <GameFramework/PlayerState.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_PlayerController_UE::
    BeginSpectatingState()
    -> void
{
    if (ck::IsValid(PlayerCameraManager))
    {
        SetSpawnLocation(PlayerCameraManager->GetCameraLocation());
    }

    Super::BeginSpectatingState();
}

auto
    ACk_PlayerController_UE::
    OnRep_Pawn()
    -> void
{
    Super::OnRep_Pawn();

    if (IsInState(NAME_Spectating))
    {
        ServerViewNextPlayer();
    }
}

auto
    ACk_PlayerController_UE::
    AddCheats(
        bool InForce)
    -> void
{
#if CK_BUILD_RELEASE
    Super::AddCheats(InForce);
#else
    Super::AddCheats(InForce || _AddMultiplayerCheats);
#endif
}

auto
    ACk_PlayerController_UE::
    PostActorCreated()
    -> void
{
    TRACE_BOOKMARK(TEXT("Player Controller Created"));
    ck::core::Log(TEXT("Player Controller [{}] Created"), this);
    Super::PostActorCreated();
}

auto
    ACk_PlayerController_UE::
    ClientSetHUD_Implementation(
        TSubclassOf<AHUD> NewHUDClass)
    -> void
{
    TRACE_BOOKMARK(TEXT("Player Controller Client Set HUD"));
    ck::core::Log(TEXT("Player Controller [{}] Client Set HUD"), this);
    Super::ClientSetHUD_Implementation(NewHUDClass);
}

auto
    ACk_PlayerController_UE::
    BeginPlay()
    -> void
{
    TRACE_BOOKMARK(TEXT("Player Controller Begin Play"));
    ck::core::Log(TEXT("Player Controller [{}] Begin Play"), this);
    Super::BeginPlay();
}

auto
    ACk_PlayerController_UE::
    Request_SetControllerState(
        ECk_PlayerController_State InState)
    -> void
{
    if (NOT HasAuthority())
    { return; }

    switch (InState)
    {
        case ECk_PlayerController_State::Playing:
        {
            DoSetControllerState_Playing();
            break;
        }
        case ECk_PlayerController_State::Spectating:
        {
            DoSetControllerState_Spectating();
            break;
        }
        case ECk_PlayerController_State::Inactive:
        {
            DoSetControllerState_Inactive();
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InState);
            break;
        }
    }
}

auto
    ACk_PlayerController_UE::
    DoSetControllerState_Playing()
    -> void
{
    if (ck::IsValid(PlayerState))
    {
        PlayerState->SetIsSpectator(false);
    }

    ChangeState(NAME_Playing);
    bPlayerIsWaiting = false;

    ClientGotoState(NAME_Playing);
}

auto
    ACk_PlayerController_UE::
    DoSetControllerState_Spectating()
    -> void
{
    if (ck::IsValid(PlayerState))
    {
        PlayerState->SetIsSpectator(true);
    }

    ChangeState(NAME_Spectating);
    bPlayerIsWaiting = true;

    ClientGotoState(NAME_Spectating);

    if (_SpectatingPolicy == ECk_PlayerController_Spectating_Policy::AutoSpectate)
    {
        DoViewNextPlayer();
    }
}

auto
    ACk_PlayerController_UE::
    DoSetControllerState_Inactive()
    -> void
{
    if (ck::IsValid(PlayerState))
    {
        PlayerState->SetIsSpectator(false);
    }

    ChangeState(NAME_Inactive);
    ClientGotoState(NAME_Inactive);
}

auto
    ACk_PlayerController_UE::
    DoViewNextPlayer()
    -> void
{
    constexpr auto Forward = 1;

    if (auto* NextPlayerState = GetNextViewablePlayer(Forward);
        ck::IsValid(NextPlayerState))
    {
        SetViewTarget(NextPlayerState);
    }
}

auto
    ACk_PlayerController_UE::
    DoViewPreviousPlayer()
    -> void
{
    constexpr auto Backward = -1;

    if (auto* PreviousPlayerState = GetNextViewablePlayer(Backward);
        ck::IsValid(PreviousPlayerState))
    {
        SetViewTarget(PreviousPlayerState);
    }
}

// --------------------------------------------------------------------------------------------------------------------
