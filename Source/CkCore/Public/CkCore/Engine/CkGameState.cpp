#include "CkGameState.h"

// --------------------------------------------------------------------------------------------------------------------

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
