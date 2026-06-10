#include "CkRenderTargetRelay_Subsystem.h"

#include "CkRenderTarget/Net/CkRenderTargetRelay_Actor.h"

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RenderTargetRelay_Subsystem_UE::
    Get_GroupTag() const
    -> FGameplayTag
{
    return UCk_Utils_GameplayTag_UE::ResolveGameplayTag(TEXT("ActorRelay.RenderTarget"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RenderTargetRelay_Subsystem_UE::
    Get_ActorClass() const
    -> TSubclassOf<ACk_ActorRelay_UE>
{
    return ACk_RenderTargetRelay_UE::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------
