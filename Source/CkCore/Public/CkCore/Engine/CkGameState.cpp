#include "CkGameState.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_GameState_UE::
    ACk_GameState_UE()
{
    if (ck::IsValid(GetWorld()))
    {
        PrimaryActorTick.bCanEverTick = GetWorld()->IsNetMode(NM_DedicatedServer);
    }
}

auto
    ACk_GameState_UE::
    Tick(
        float InDeltaSeconds)
    -> void
{
    Super::Tick(InDeltaSeconds);

    const auto& FrameTime = InDeltaSeconds * 1000.0f;

    if (FrameTime <= 0.0f)
    { return; }

    const auto& FPS = 1000.0f / FrameTime;

    _ServerFPS = FPS;
    MARK_PROPERTY_DIRTY_FROM_NAME(ACk_GameState_UE, _ServerFPS, this);
}

auto
    ACk_GameState_UE::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _ServerFPS, Params);
}

auto
    ACk_GameState_UE::
    Get_ServerFPS() const
    -> float
{
    return _ServerFPS;
}

auto
    ACk_GameState_UE::
    AddPlayerState(
        APlayerState* InPlayerState)
    -> void
{
    Super::AddPlayerState(InPlayerState);

    OnPlayerAdded(InPlayerState);
}

auto
    ACk_GameState_UE::
    RemovePlayerState(
        APlayerState* InPlayerState)
    -> void
{
    Super::RemovePlayerState(InPlayerState);

    OnPlayerRemoved(InPlayerState);
}

// --------------------------------------------------------------------------------------------------------------------
