#include "CkCueRelay_Actor.h"

#include "CkCueSubsystem_Base.h"

#include "CkCue/CkCue_Log.h"

#include "CkCore/Debug/CkDebug_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_cue_subsystem_base
{
    auto Get_CueSubsystemFromClass(TSubclassOf<UCk_CueSubsystem_Base_UE> InCueSubsystemClass) -> UCk_CueSubsystem_Base_UE*;
    auto ExecuteCueEntityScript(FCk_Handle InOwnerEntity, const FGameplayTag& InCueName, TSubclassOf<UCk_CueBase_EntityScript> InCueClass, const FInstancedStruct& InSpawnParams) -> FCk_Handle_PendingEntityScript;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_CueRelay_UE::
    DoGetCueExecutorSubsystem() const
    -> UCk_CueExecutor_Subsystem_Base_UE*
{
    return Cast<UCk_CueExecutor_Subsystem_Base_UE>(Get_GroupSubsystem().Get());
}

/*─────────────────────────────────────────────────────────────────────────────┐
│                              SERVER RPCs                                     │
└─────────────────────────────────────────────────────────────────────────────*/

auto
    ACk_CueRelay_UE::
    Server_RequestExecuteCue_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    Multicast_ExecuteCue(InOwnerEntity, InCueName, InSpawnParams);
}

auto
    ACk_CueRelay_UE::
    Server_RequestExecuteCue_Reliable_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    Multicast_ExecuteCue_Reliable(InOwnerEntity, InCueName, InSpawnParams);
}

auto
    ACk_CueRelay_UE::
    Server_RequestExecuteCue_ServerOnly_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    auto CueExecutorSubsystem = DoGetCueExecutorSubsystem();

    CK_ENSURE_IF_NOT(ck::IsValid(CueExecutorSubsystem),
        TEXT("CueExecutor subsystem is invalid when executing server-only cue [{}]"), InCueName)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) &&
        CueExecutorSubsystem->Get_DedicatedServerPolicy() == ECk_Cue_DedicatedServerPolicy::CosmeticOnly)
    { return; }

    const auto& CueSubsystemClass = CueExecutorSubsystem->Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);

    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid when executing server-only cue [{}]"), InCueName)
    { return; }

    ck::cue::Verbose(TEXT("Executing server-only cue [{}] on entity [{}]"), InCueName, InOwnerEntity);
    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    ACk_CueRelay_UE::
    Server_RequestExecuteCue_ServerOnly_Reliable_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    auto CueExecutorSubsystem = DoGetCueExecutorSubsystem();

    CK_ENSURE_IF_NOT(ck::IsValid(CueExecutorSubsystem),
        TEXT("CueExecutor subsystem is invalid when executing reliable server-only cue [{}]"), InCueName)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) &&
        CueExecutorSubsystem->Get_DedicatedServerPolicy() == ECk_Cue_DedicatedServerPolicy::CosmeticOnly)
    { return; }

    const auto& CueSubsystemClass = CueExecutorSubsystem->Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);

    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid when executing reliable server-only cue [{}]"), InCueName)
    { return; }

    ck::cue::Verbose(TEXT("Executing reliable server-only cue [{}] on entity [{}]"), InCueName, InOwnerEntity);
    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    ACk_CueRelay_UE::
    Server_RequestExecuteCue_ExcludingSender_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    auto OriginatingPlayerState = Cast<APlayerState>(GetOwner());
    Multicast_ExecuteCue_ExcludingSender(InOwnerEntity, InCueName, InSpawnParams, OriginatingPlayerState);
}

auto
    ACk_CueRelay_UE::
    Server_RequestExecuteCue_ExcludingSender_Reliable_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    auto OriginatingPlayerState = Cast<APlayerState>(GetOwner());
    Multicast_ExecuteCue_ExcludingSender_Reliable(InOwnerEntity, InCueName, InSpawnParams, OriginatingPlayerState);
}

/*─────────────────────────────────────────────────────────────────────────────┐
│                            MULTICAST RPCs                                    │
└─────────────────────────────────────────────────────────────────────────────*/

auto
    ACk_CueRelay_UE::
    Multicast_ExecuteCue_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    auto CueExecutorSubsystem = DoGetCueExecutorSubsystem();

    CK_ENSURE_IF_NOT(ck::IsValid(CueExecutorSubsystem),
        TEXT("CueExecutor subsystem is invalid when executing multicast cue [{}]"), InCueName)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) &&
        CueExecutorSubsystem->Get_DedicatedServerPolicy() == ECk_Cue_DedicatedServerPolicy::CosmeticOnly)
    { return; }

    const auto& CueSubsystemClass = CueExecutorSubsystem->Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);

    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid when executing multicast cue [{}]"), InCueName)
    { return; }

    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    ACk_CueRelay_UE::
    Multicast_ExecuteCue_Reliable_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    auto CueExecutorSubsystem = DoGetCueExecutorSubsystem();

    CK_ENSURE_IF_NOT(ck::IsValid(CueExecutorSubsystem),
        TEXT("CueExecutor subsystem is invalid when executing reliable multicast cue [{}]"), InCueName)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) &&
        CueExecutorSubsystem->Get_DedicatedServerPolicy() == ECk_Cue_DedicatedServerPolicy::CosmeticOnly)
    { return; }

    const auto& CueSubsystemClass = CueExecutorSubsystem->Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);

    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid when executing reliable multicast cue [{}]"), InCueName)
    { return; }

    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    ACk_CueRelay_UE::
    Multicast_ExecuteCue_ExcludingSender_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams,
        APlayerState* InExcludedPlayerState)
    -> void
{
    auto CueExecutorSubsystem = DoGetCueExecutorSubsystem();

    CK_ENSURE_IF_NOT(ck::IsValid(CueExecutorSubsystem),
        TEXT("CueExecutor subsystem is invalid when executing multicast cue [{}]"), InCueName)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) &&
        CueExecutorSubsystem->Get_DedicatedServerPolicy() == ECk_Cue_DedicatedServerPolicy::CosmeticOnly)
    { return; }

    if (GetWorld()->IsNetMode(NM_Client))
    {
        auto LocalPC = GetWorld()->GetFirstPlayerController();
        if (ck::IsValid(LocalPC) && ck::IsValid(LocalPC->PlayerState) && LocalPC->PlayerState == InExcludedPlayerState)
        {
            ck::cue::Verbose(TEXT("Skipping cue [{}] on excluded client"), InCueName);
            return;
        }
    }

    const auto& CueSubsystemClass = CueExecutorSubsystem->Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);

    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid when executing multicast cue [{}]"), InCueName)
    { return; }

    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    ACk_CueRelay_UE::
    Multicast_ExecuteCue_ExcludingSender_Reliable_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams,
        APlayerState* InExcludedPlayerState)
    -> void
{
    auto CueExecutorSubsystem = DoGetCueExecutorSubsystem();

    CK_ENSURE_IF_NOT(ck::IsValid(CueExecutorSubsystem),
        TEXT("CueExecutor subsystem is invalid when executing reliable multicast cue [{}]"), InCueName)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) &&
        CueExecutorSubsystem->Get_DedicatedServerPolicy() == ECk_Cue_DedicatedServerPolicy::CosmeticOnly)
    { return; }

    if (GetWorld()->IsNetMode(NM_Client))
    {
        auto LocalPC = GetWorld()->GetFirstPlayerController();
        if (ck::IsValid(LocalPC) && ck::IsValid(LocalPC->PlayerState) && LocalPC->PlayerState == InExcludedPlayerState)
        {
            ck::cue::Verbose(TEXT("Skipping reliable cue [{}] on excluded client"), InCueName);
            return;
        }
    }

    const auto& CueSubsystemClass = CueExecutorSubsystem->Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);

    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid when executing reliable multicast cue [{}]"), InCueName)
    { return; }

    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

// --------------------------------------------------------------------------------------------------------------------
