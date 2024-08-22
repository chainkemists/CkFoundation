#include "CkSpectatorPawn.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_SpectatorPawn_UE::
    Request_ViewNextPlayer()
    -> void
{
    if (auto* PC = GetController<APlayerController>();
        ck::IsValid(PC))
    {
        PC->ServerViewNextPlayer();
    }
}

auto
    ACk_SpectatorPawn_UE::
    Request_ViewPreviousPlayer()
    -> void
{
    if (auto* PC = GetController<APlayerController>();
        ck::IsValid(PC))
    {
        PC->ServerViewPrevPlayer();
    }
}

// --------------------------------------------------------------------------------------------------------------------
