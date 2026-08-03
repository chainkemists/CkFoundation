#include "CkVoiceChatControlRelay_Actor.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"
#include "CkVoiceChat/VoiceListener/CkVoiceListener_Fragment.h"

#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_VoiceChatControlRelay_UE::
    Server_SetMutedTalkers_Implementation(
        const TArray<FCk_Handle>& InMutedTalkers)
    -> void
{
    auto* Sender = DoResolve_OwningPlayerState();

    if (ck::Is_NOT_Valid(Sender))
    {
        ck::voice_chat::Warning(TEXT("Server_SetMutedTalkers: could not resolve the sending player from the channel owner chain - dropping"));
        return;
    }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(GetWorld());

    if (ck::Is_NOT_Valid(TransientEntity))
    {
        ck::voice_chat::Warning(TEXT("Server_SetMutedTalkers: no transient entity for this world - dropping"));
        return;
    }

    TransientEntity.AddOrGet<ck::FFragment_VoiceChat_ControlInbox>().Get_Updates().Emplace(
        ck::FCk_VoiceChat_MuteSetUpdate{MakeWeakObjectPtr(Sender), InMutedTalkers});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_VoiceChatControlRelay_UE::
    DoResolve_OwningPlayerState() const
    -> APlayerState*
{
    for (auto* Outer = GetOwner(); Outer != nullptr; Outer = Outer->GetOwner())
    {
        if (const auto* OwningController = ::Cast<APlayerController>(Outer))
        { return OwningController->PlayerState; }

        if (auto* OwningPlayerState = ::Cast<APlayerState>(Outer))
        { return OwningPlayerState; }
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
