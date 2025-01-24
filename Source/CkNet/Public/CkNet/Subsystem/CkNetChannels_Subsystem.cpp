#include "CkNetChannels_Subsystem.h"

#include "CkNet/CkNet_Utils.h"
#include "CkNet/Settings/CkNet_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_NetChannels_Subsystem_UE::
    OnWorldBeginPlay(
        UWorld& InWorld)
    -> void
{
    Super::OnWorldBeginPlay(InWorld);

    //for (auto Index = 0; Index < UCk_Utils_Net_Settings_UE::Get_NumberOfEcsReplicationChannels(); ++Index)
    //{
    //    const auto& ActorLabel = ck::Format_UE(TEXT("EcsReplicationChannelActor_#{}"), Index);

    //    UCk_Utils_Actor_UE::Request_SpawnActor
    //    (
    //        FCk_Utils_Actor_SpawnActor_Params{GetWorld(), UCk_Utils_Net_Settings_UE::Get_EcsReplicationChannel()}
    //        .Set_Label(ActorLabel)
    //        .Set_NetworkingType(ECk_Actor_NetworkingType::Replicated)
    //        .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel)
    //    );
    //}
}

auto
    UCk_NetChannels_Subsystem_UE::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    return Super::ShouldCreateSubsystem(InOuter) && NOT InOuter->GetWorld()->IsNetMode(NM_Client);
}

// --------------------------------------------------------------------------------------------------------------------