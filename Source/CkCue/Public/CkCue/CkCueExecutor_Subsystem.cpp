// CkCueExecutor_Subsystem.cpp
//
// UCk_CueExecutor_Subsystem_Base_UE implementation:
//   - Initialize / Deinitialize
//   - Request_ExecuteCue* methods (transient, local, replicated)
//   - Pending cue queue and timeout handling
//   - ActorRelay integration

#include "CkCueSubsystem_Base.h"

#include "CkCore/Debug/CkDebug_Utils.h"

#include "CkCue/CkCue_Fragment.h"
#include "CkCue/CkCue_Log.h"
#include "CkCue/Settings/CkCue_Settings.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

/*─────────────────────────────────────────────────────────────────────────────┐
│                         CUE EXECUTOR SUBSYSTEM BASE                          │
└─────────────────────────────────────────────────────────────────────────────*/

namespace ck_cue_subsystem_base
{
    auto Get_CueSubsystemFromClass(TSubclassOf<UCk_CueSubsystem_Base_UE> InCueSubsystemClass) -> UCk_CueSubsystem_Base_UE*;
    auto ExecuteCueEntityScript(FCk_Handle InOwnerEntity, const FGameplayTag& InCueName, TSubclassOf<UCk_CueBase_EntityScript> InCueClass, const FInstancedStruct& InSpawnParams) -> FCk_Handle_PendingEntityScript;
}

/*─────────────────────────────────────────────────────────────────────────────┐
│                       ACTOR RELAY CONFIG OVERRIDE                            │
└─────────────────────────────────────────────────────────────────────────────*/

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Get_ActorClass() const
    -> TSubclassOf<ACk_ActorRelay_UE>
{
    return ACk_CueRelay_UE::StaticClass();
}

/*─────────────────────────────────────────────────────────────────────────────┐
│                          INITIALIZE / DEINITIALIZE                           │
└─────────────────────────────────────────────────────────────────────────────*/

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Deinitialize()
    -> void
{
    if (_PendingCueTimeoutTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_PendingCueTimeoutTickerHandle);
        _PendingCueTimeoutTickerHandle.Reset();
    }

    Super::Deinitialize();
}

/*─────────────────────────────────────────────────────────────────────────────┐
│                            CHANNEL ACQUISITION                               │
└─────────────────────────────────────────────────────────────────────────────*/

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    DoAcquireCueRelay_ForClient()
    -> ACk_CueRelay_UE*
{
    auto LocalPC = GetWorld()->GetFirstPlayerController();

    if (ck::Is_NOT_Valid(LocalPC))
    {
        ck::cue::Warning(TEXT("Failed to acquire CueRelay: Local PlayerController is invalid"));
        return {};
    }

    auto LocalPlayerState = LocalPC->PlayerState;

    if (ck::Is_NOT_Valid(LocalPlayerState))
    {
        ck::cue::Warning(TEXT("Failed to acquire CueRelay: Local PlayerState is invalid"));
        return {};
    }

    auto ChannelResult = Request_AcquireChannel_ForPlayer(LocalPlayerState);

    if (NOT ChannelResult.Get_ChannelActor().IsValid())
    {
        ck::cue::Warning(TEXT("Failed to acquire CueRelay: Channel result is invalid"));
        return {};
    }

    return Cast<ACk_CueRelay_UE>(ChannelResult.Get_ChannelActor().Get());
}

/*─────────────────────────────────────────────────────────────────────────────┐
│                          CUE EXECUTION METHODS                               │
└─────────────────────────────────────────────────────────────────────────────*/

namespace ck_cue_subsystem_base
{
    auto DoExecuteLocal(
        UCk_CueExecutor_Subsystem_Base_UE* InExecutor,
        const FCk_Handle& InOwnerEntity,
        const FGameplayTag& InCueName,
        const FInstancedStruct& InSpawnParams)
    -> FCk_Handle_PendingEntityScript
    {
        const auto& CueSubsystemClass = InExecutor->Get_CueSubsystemClass();
        auto CueSubsystem = Get_CueSubsystemFromClass(CueSubsystemClass);
        CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
            TEXT("CueSubsystem is invalid for local cue execution of [{}]"), InCueName)
        { return {}; }

        const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
        return ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
    }
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Request_ExecuteCue_Transient(
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliability,
        ECk_Cue_MulticastPolicy InMulticastPolicy)
    -> FCk_Handle_PendingEntityScript
{
    if (InMulticastPolicy == ECk_Cue_MulticastPolicy::LocalOnly)
    {
        FCk_Handle InvalidHandle{};
        return ck_cue_subsystem_base::DoExecuteLocal(this, InvalidHandle, InCueName, InSpawnParams);
    }

    if (Get_ChannelCount_Active() == 0)
    {
        ck::cue::Warning(TEXT("No CueRelay actors available yet. Caching transient cue [{}] for later execution"), InCueName);

        FCk_Handle InvalidHandle{};
        _PendingCues.Emplace(InvalidHandle, InCueName, InSpawnParams, InReliability, InMulticastPolicy);

        if (NOT _PendingCueTimeoutTickerHandle.IsValid())
        {
            _PendingCueTimeoutTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateUObject(this, &UCk_CueExecutor_Subsystem_Base_UE::DoCheckPendingCueTimeout)
            );
        }

        return {};
    }

    auto ChannelResult = Request_AcquireAnyChannel();

    if (NOT ChannelResult.Get_ChannelActor().IsValid())
    {
        ck::cue::Warning(TEXT("Failed to acquire channel for transient cue [{}]"), InCueName);
        return {};
    }

    auto CueRelayEntity = ChannelResult.Get_ChannelEntity();
    return Request_ExecuteCue(CueRelayEntity, InCueName, InSpawnParams, InReliability, InMulticastPolicy);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Request_ExecuteCue(
        const FCk_Handle& InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliability,
        ECk_Cue_MulticastPolicy InMulticastPolicy)
    -> FCk_Handle_PendingEntityScript
{
    if (InMulticastPolicy == ECk_Cue_MulticastPolicy::LocalOnly)
    {
        return ck_cue_subsystem_base::DoExecuteLocal(this, InOwnerEntity, InCueName, InSpawnParams);
    }

    if (ck::Is_NOT_Valid(InOwnerEntity))
    {
        ck::cue::Verbose(TEXT("OwnerEntity is invalid when trying to execute Cue [{}]. Deferring to ExecuteCueEntityScript for policy check."), InCueName);
        return {};
    }

    if (Get_ChannelCount_Active() == 0)
    {
        ck::cue::Warning(TEXT("No CueRelay actors available yet. Caching cue [{}] for later execution"), InCueName);

        _PendingCues.Emplace(InOwnerEntity, InCueName, InSpawnParams, InReliability, InMulticastPolicy);

        if (NOT _PendingCueTimeoutTickerHandle.IsValid())
        {
            _PendingCueTimeoutTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
                FTickerDelegate::CreateUObject(this, &UCk_CueExecutor_Subsystem_Base_UE::DoCheckPendingCueTimeout)
            );
        }

        return {};
    }

    if (GetWorld()->IsNetMode(NM_Standalone))
    {
        if (InMulticastPolicy == ECk_Cue_MulticastPolicy::OtherClientsOnly)
        {
            ck::cue::Verbose(TEXT("OtherClientsOnly cue [{}] skipped in standalone net mode"), InCueName);
            return {};
        }
        return ck_cue_subsystem_base::DoExecuteLocal(this, InOwnerEntity, InCueName, InSpawnParams);
    }

    const auto IsReliable = InReliability == ECk_Cue_ReliabilityPolicy::Reliable;

    if (GetWorld()->IsNetMode(NM_Client))
    {
        auto CueRelay = DoAcquireCueRelay_ForClient();

        if (ck::Is_NOT_Valid(CueRelay))
        { return {}; }

        switch (InMulticastPolicy)
        {
            case ECk_Cue_MulticastPolicy::ServerOnly:
            {
                if (IsReliable) { CueRelay->Server_RequestExecuteCue_ServerOnly_Reliable(InOwnerEntity, InCueName, InSpawnParams); }
                else            { CueRelay->Server_RequestExecuteCue_ServerOnly(InOwnerEntity, InCueName, InSpawnParams); }
                break;
            }
            case ECk_Cue_MulticastPolicy::ServerAndSelf:
            {
                ck_cue_subsystem_base::DoExecuteLocal(this, InOwnerEntity, InCueName, InSpawnParams);
                if (IsReliable) { CueRelay->Server_RequestExecuteCue_ServerOnly_Reliable(InOwnerEntity, InCueName, InSpawnParams); }
                else            { CueRelay->Server_RequestExecuteCue_ServerOnly(InOwnerEntity, InCueName, InSpawnParams); }
                break;
            }
            case ECk_Cue_MulticastPolicy::ServerAndOtherClients:
            {
                constexpr auto SkipServer = false;
                if (IsReliable) { CueRelay->Server_RequestExecuteCue_ExcludingSender_Reliable(InOwnerEntity, InCueName, InSpawnParams, SkipServer); }
                else            { CueRelay->Server_RequestExecuteCue_ExcludingSender(InOwnerEntity, InCueName, InSpawnParams, SkipServer); }
                break;
            }
            case ECk_Cue_MulticastPolicy::OtherClientsOnly:
            {
                constexpr auto SkipServer = true;
                if (IsReliable) { CueRelay->Server_RequestExecuteCue_ExcludingSender_Reliable(InOwnerEntity, InCueName, InSpawnParams, SkipServer); }
                else            { CueRelay->Server_RequestExecuteCue_ExcludingSender(InOwnerEntity, InCueName, InSpawnParams, SkipServer); }
                break;
            }
            case ECk_Cue_MulticastPolicy::ServerAndAllClients:
            default:
            {
                if (IsReliable) { CueRelay->Server_RequestExecuteCue_Reliable(InOwnerEntity, InCueName, InSpawnParams); }
                else            { CueRelay->Server_RequestExecuteCue(InOwnerEntity, InCueName, InSpawnParams); }
                break;
            }
        }
        return {};
    }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer))
    {
        if (InMulticastPolicy == ECk_Cue_MulticastPolicy::ServerOnly ||
            InMulticastPolicy == ECk_Cue_MulticastPolicy::ServerAndSelf)
        {
            ck::cue::Verbose(TEXT("Executing server-only cue [{}] on entity [{}]"), InCueName, InOwnerEntity);
            return ck_cue_subsystem_base::DoExecuteLocal(this, InOwnerEntity, InCueName, InSpawnParams);
        }

        auto ChannelResult = Request_AcquireAnyChannel();
        auto CueRelay = Cast<ACk_CueRelay_UE>(ChannelResult.Get_ChannelActor().Get());

        CK_ENSURE_IF_NOT(ck::IsValid(CueRelay),
            TEXT("Failed to acquire CueRelay on server for cue [{}]"), InCueName)
        { return {}; }

        auto OriginatingPlayerState = static_cast<APlayerState*>(nullptr);

        switch (InMulticastPolicy)
        {
            case ECk_Cue_MulticastPolicy::ServerAndOtherClients:
            {
                constexpr auto SkipServer = false;
                if (IsReliable) { CueRelay->Multicast_ExecuteCue_ExcludingSender_Reliable(InOwnerEntity, InCueName, InSpawnParams, OriginatingPlayerState, SkipServer); }
                else            { CueRelay->Multicast_ExecuteCue_ExcludingSender(InOwnerEntity, InCueName, InSpawnParams, OriginatingPlayerState, SkipServer); }
                break;
            }
            case ECk_Cue_MulticastPolicy::OtherClientsOnly:
            {
                constexpr auto SkipServer = true;
                if (IsReliable) { CueRelay->Multicast_ExecuteCue_ExcludingSender_Reliable(InOwnerEntity, InCueName, InSpawnParams, OriginatingPlayerState, SkipServer); }
                else            { CueRelay->Multicast_ExecuteCue_ExcludingSender(InOwnerEntity, InCueName, InSpawnParams, OriginatingPlayerState, SkipServer); }
                break;
            }
            case ECk_Cue_MulticastPolicy::ServerAndAllClients:
            default:
            {
                if (IsReliable) { CueRelay->Multicast_ExecuteCue_Reliable(InOwnerEntity, InCueName, InSpawnParams); }
                else            { CueRelay->Multicast_ExecuteCue(InOwnerEntity, InCueName, InSpawnParams); }
                break;
            }
        }
    }

    return {};
}

/*─────────────────────────────────────────────────────────────────────────────┐
│                          PENDING CUE MANAGEMENT                              │
└─────────────────────────────────────────────────────────────────────────────*/

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    DoProcessPendingCues()
    -> void
{
    if (_PendingCues.IsEmpty())
    { return; }

    if (Get_ChannelCount_Active() == 0)
    { return; }

    ck::cue::Log(TEXT("Processing [{}] pending cues"), _PendingCues.Num());

    auto PendingCuesToProcess = MoveTemp(_PendingCues);
    _PendingCues.Reset();

    if (_PendingCueTimeoutTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_PendingCueTimeoutTickerHandle);
        _PendingCueTimeoutTickerHandle.Reset();
    }

    for (const auto& PendingCue : PendingCuesToProcess)
    {
        ck::cue::Verbose(TEXT("Executing cached cue [{}]"), PendingCue.CueName);

        if (ck::Is_NOT_Valid(PendingCue.OwnerEntity))
        {
            Request_ExecuteCue_Transient(
                PendingCue.CueName,
                PendingCue.SpawnParams,
                PendingCue.Reliability,
                PendingCue.MulticastPolicy
            );
        }
        else
        {
            Request_ExecuteCue(
                PendingCue.OwnerEntity,
                PendingCue.CueName,
                PendingCue.SpawnParams,
                PendingCue.Reliability,
                PendingCue.MulticastPolicy
            );
        }
    }
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    DoCheckPendingCueTimeout(
        float InDeltaTime)
    -> bool
{
    DoProcessPendingCues();

    if (_PendingCues.IsEmpty())
    {
        _PendingCueTimeoutTickerHandle.Reset();
        return false;
    }

    const auto TimeoutSeconds = UCk_Utils_Cue_Settings_UE::Get_PendingCueTimeoutSeconds();
    const auto CurrentTime = FPlatformTime::Seconds();

    for (const auto& PendingCue : _PendingCues)
    {
        const auto ElapsedTime = CurrentTime - PendingCue.TimeRequested;

        if (ElapsedTime >= TimeoutSeconds)
        {
            CK_TRIGGER_ENSURE(TEXT("Pending cue [{}] has been waiting for [{}] seconds without channels becoming available. Timeout threshold: [{}]s"),
                PendingCue.CueName,
                ElapsedTime,
                TimeoutSeconds);

            return true;
        }
    }

    return true;
}
