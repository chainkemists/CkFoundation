// CkCueExecutor_Subsystem.cpp
//
// UCk_CueExecutor_Subsystem_Base_UE implementation:
//   - Initialize / Deinitialize
//   - Request_ExecuteCue* methods (transient, local, replicated)
//   - Pending cue queue and timeout handling
//   - ActorRelay integration

#include "CkCueSubsystem_Base.h"

#include "CkActorRelay/CkActorRelay_Utils.h"

#include "CkCore/Debug/CkDebug_Utils.h"
#include "CkCore/Validation/CkUntracedStructSafety.h"

#include "CkCue/CkCue_Fragment.h"
#include "CkCue/CkCue_Log.h"
#include "CkCue/Settings/CkCue_Settings.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_cue_subsystem_base
{
    auto Get_CueSubsystemFromClass(TSubclassOf<UCk_CueSubsystem_Base_UE> InCueSubsystemClass) -> UCk_CueSubsystem_Base_UE*;
    auto ExecuteCueEntityScript(FCk_Handle InOwnerEntity, const FGameplayTag& InCueName, TSubclassOf<UCk_CueBase_EntityScript> InCueClass, const FInstancedStruct& InSpawnParams) -> FCk_Handle_PendingEntityScript;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Get_ActorClass() const
    -> TSubclassOf<ACk_ActorRelay_UE>
{
    return ACk_CueRelay_UE::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    DoAcquireCueRelay_ForClient_Async(
        TFunction<void(ACk_CueRelay_UE*)> InCallback)
    -> void
{
    auto LocalPC = GetWorld()->GetFirstPlayerController();

    if (ck::Is_NOT_Valid(LocalPC))
    {
        ck::cue::Warning(TEXT("Failed to acquire CueRelay: Local PlayerController is invalid"));
        return;
    }

    auto LocalPlayerState = LocalPC->PlayerState;

    if (ck::Is_NOT_Valid(LocalPlayerState))
    {
        ck::cue::Warning(TEXT("Failed to acquire CueRelay: Local PlayerState is invalid"));
        return;
    }

    auto Pending = Request_AcquireChannel_ForPlayer(LocalPlayerState);
    UCk_Utils_PendingActorRelay_UE::Promise_OnAcquired(Pending,
        [Callback = MoveTemp(InCallback)](FCk_ActorRelay_ChannelResult InResult)
        {
            auto CueRelay = Cast<ACk_CueRelay_UE>(InResult.Get_ChannelActor().Get());
            if (ck::IsValid(CueRelay))
            { Callback(CueRelay); }
        });
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_cue_subsystem_base
{
    auto ValidateRetainedSpawnParams(const FGameplayTag& InCueName, const FInstancedStruct& InSpawnParams) -> bool
    {
        if (NOT InSpawnParams.IsValid())
        { return true; }

        const auto* ScriptStruct = InSpawnParams.GetScriptStruct();
        const auto HasScriptStruct = ScriptStruct != nullptr;
        CK_ENSURE_IF_NOT(HasScriptStruct,
            TEXT("Cue [{}] rejected spawn params without a reflected struct type"), InCueName)
        { }
        if (NOT HasScriptStruct)
        { return false; }

        const auto Safety = ck::Analyze_UntracedStructSafety(ScriptStruct);
        const auto IsSpawnParamsSafe = Safety.IsGcIndependent();
        CK_ENSURE_IF_NOT(IsSpawnParamsSafe,
            TEXT("Cue [{}] rejected unsafe spawn params [{}]; [{}]: {}"),
            InCueName,
            ScriptStruct->GetName(),
            Safety.FailurePath,
            Safety.FailureReason)
        { }

        if (NOT IsSpawnParamsSafe)
        { return false; }

        return true;
    }

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
    if (NOT ck_cue_subsystem_base::ValidateRetainedSpawnParams(InCueName, InSpawnParams))
    { return {}; }

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

    auto Pending = Request_AcquireAnyChannel();
    UCk_Utils_PendingActorRelay_UE::Promise_OnAcquired(Pending,
        [this, InCueName, InSpawnParams, InReliability, InMulticastPolicy]
        (FCk_ActorRelay_ChannelResult InResult)
        {
            if (ck::Is_NOT_Valid(InResult))
            {
                ck::cue::Warning(TEXT("Channel acquired but result is invalid for transient cue [{}]"), InCueName);
                return;
            }

            Request_ExecuteCue(InResult.Get_ChannelEntity(),
                InCueName, InSpawnParams, InReliability, InMulticastPolicy);
        });

    return {};
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
    if (NOT ck_cue_subsystem_base::ValidateRetainedSpawnParams(InCueName, InSpawnParams))
    { return {}; }

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
        // On client: optionally execute locally on Self path, then defer the
        // RPC until a CueRelay channel is ready. The lambda captures the
        // dispatch payload so the relay can come up after the cue is queued.
        if (InMulticastPolicy == ECk_Cue_MulticastPolicy::ServerAndSelf)
        { ck_cue_subsystem_base::DoExecuteLocal(this, InOwnerEntity, InCueName, InSpawnParams); }

        DoAcquireCueRelay_ForClient_Async(
            [OwnerEntity = InOwnerEntity, CueName = InCueName, SpawnParams = InSpawnParams, IsReliable, InMulticastPolicy]
            (ACk_CueRelay_UE* CueRelay)
            {
                // The relay can come up well after the cue was queued — the owner
                // may have been destroyed in the interim. Don't ship a dead handle.
                if (ck::Is_NOT_Valid(OwnerEntity))
                { return; }

                switch (InMulticastPolicy)
                {
                    case ECk_Cue_MulticastPolicy::ServerOnly:
                    case ECk_Cue_MulticastPolicy::ServerAndSelf:
                    {
                        if (IsReliable) { CueRelay->Server_RequestExecuteCue_ServerOnly_Reliable(OwnerEntity, CueName, SpawnParams); }
                        else            { CueRelay->Server_RequestExecuteCue_ServerOnly(OwnerEntity, CueName, SpawnParams); }
                        break;
                    }
                    case ECk_Cue_MulticastPolicy::ServerAndOtherClients:
                    {
                        constexpr auto SkipServer = false;
                        if (IsReliable) { CueRelay->Server_RequestExecuteCue_ExcludingSender_Reliable(OwnerEntity, CueName, SpawnParams, SkipServer); }
                        else            { CueRelay->Server_RequestExecuteCue_ExcludingSender(OwnerEntity, CueName, SpawnParams, SkipServer); }
                        break;
                    }
                    case ECk_Cue_MulticastPolicy::OtherClientsOnly:
                    {
                        constexpr auto SkipServer = true;
                        if (IsReliable) { CueRelay->Server_RequestExecuteCue_ExcludingSender_Reliable(OwnerEntity, CueName, SpawnParams, SkipServer); }
                        else            { CueRelay->Server_RequestExecuteCue_ExcludingSender(OwnerEntity, CueName, SpawnParams, SkipServer); }
                        break;
                    }
                    case ECk_Cue_MulticastPolicy::ServerAndAllClients:
                    default:
                    {
                        if (IsReliable) { CueRelay->Server_RequestExecuteCue_Reliable(OwnerEntity, CueName, SpawnParams); }
                        else            { CueRelay->Server_RequestExecuteCue(OwnerEntity, CueName, SpawnParams); }
                        break;
                    }
                }
            });

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

        auto Pending = Request_AcquireAnyChannel();
        UCk_Utils_PendingActorRelay_UE::Promise_OnAcquired(Pending,
            [OwnerEntity = InOwnerEntity, CueName = InCueName, SpawnParams = InSpawnParams, IsReliable, InMulticastPolicy]
            (FCk_ActorRelay_ChannelResult InResult)
            {
                auto CueRelay = Cast<ACk_CueRelay_UE>(InResult.Get_ChannelActor().Get());
                if (ck::Is_NOT_Valid(CueRelay))
                { return; }

                // Same as the client path — the owner may have been destroyed while
                // waiting for a channel. Don't multicast a dead handle.
                if (ck::Is_NOT_Valid(OwnerEntity))
                { return; }

                auto OriginatingPlayerState = static_cast<APlayerState*>(nullptr);

                switch (InMulticastPolicy)
                {
                    case ECk_Cue_MulticastPolicy::ServerAndOtherClients:
                    {
                        constexpr auto SkipServer = false;
                        if (IsReliable) { CueRelay->Multicast_ExecuteCue_ExcludingSender_Reliable(OwnerEntity, CueName, SpawnParams, OriginatingPlayerState, SkipServer); }
                        else            { CueRelay->Multicast_ExecuteCue_ExcludingSender(OwnerEntity, CueName, SpawnParams, OriginatingPlayerState, SkipServer); }
                        break;
                    }
                    case ECk_Cue_MulticastPolicy::OtherClientsOnly:
                    {
                        constexpr auto SkipServer = true;
                        if (IsReliable) { CueRelay->Multicast_ExecuteCue_ExcludingSender_Reliable(OwnerEntity, CueName, SpawnParams, OriginatingPlayerState, SkipServer); }
                        else            { CueRelay->Multicast_ExecuteCue_ExcludingSender(OwnerEntity, CueName, SpawnParams, OriginatingPlayerState, SkipServer); }
                        break;
                    }
                    case ECk_Cue_MulticastPolicy::ServerAndAllClients:
                    default:
                    {
                        if (IsReliable) { CueRelay->Multicast_ExecuteCue_Reliable(OwnerEntity, CueName, SpawnParams); }
                        else            { CueRelay->Multicast_ExecuteCue(OwnerEntity, CueName, SpawnParams); }
                        break;
                    }
                }
            });
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------

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
