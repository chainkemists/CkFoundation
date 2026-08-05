#include "CkVoiceChatControlRelay_Subsystem.h"

#include "CkVoiceChat/Net/CkVoiceChatControlRelay_Actor.h"

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VoiceChatControlRelay_Subsystem_UE::
    Get_GroupTag() const
    -> FGameplayTag
{
    return UCk_Utils_GameplayTag_UE::ResolveGameplayTag(TEXT("ActorRelay.VoiceChatControl"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VoiceChatControlRelay_Subsystem_UE::
    Get_ActorClass() const
    -> TSubclassOf<ACk_ActorRelay_UE>
{
    return ACk_VoiceChatControlRelay_UE::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------
