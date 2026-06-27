#include "CkPlayerState.h"

#include "CkCore/BuildId/CkBuildId.h"
#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "GameFramework/PlayerController.h"
#include "Misc/App.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

#if NOT CK_BUILD_TEST_OR_SHIPPING
auto
    FCk_PlayerState_PingRange_History::
    Request_AddNewEntry(
        const EntryType& InEntry)
    -> void
{
    _Entries.Emplace(InEntry);

    if (_Entries.Num() <= _MaxHistorySize)
    { return; }

    _Entries.RemoveAt(0);
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PlayerState_PingRange::
    Request_HandleNewPing(
        const FCk_Time& InPing,
        const FCk_Time_Unreal& InCurrentTime)
    -> void
{
    _NextMaxPing = FMath::Max(InPing, _NextMaxPing);
    _NextMinPing = FMath::Min(InPing, _NextMinPing);

    const auto& UpdateTime = _LastUpdateTime + Get_UpdateFrequency();

    if (InCurrentTime < UpdateTime)
    { return; }

    _LastUpdateTime = InCurrentTime.Get_Time();

    _MinPing = _NextMinPing;
    _MaxPing = _NextMaxPing;

    _NextMinPing = FCk_Time::OneSecond();
    _NextMaxPing = FCk_Time::ZeroSecond();

#if NOT CK_BUILD_TEST_OR_SHIPPING
    const auto PingHistoryEntry = FCk_PlayerState_PingRange_History_Entry{}.Set_MinPing(_MinPing).Set_MaxPing(_MaxPing);

    _PingHistory.Request_AddNewEntry(PingHistoryEntry);
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PlayerState_PingRange::
    Get_UpdateFrequency()
    -> FCk_Time
{
    static auto UpdateFrequency = FCk_Time{5.0f};
    return UpdateFrequency;
}

auto
    ACk_PlayerState_UE::
    UpdatePing(
        float InPing)
    -> void
{
    Super::UpdatePing(InPing);

    const auto& CurrentTime = UCk_Utils_Time_UE::Get_WorldTime
    (
        FCk_Utils_Time_GetWorldTime_Params{GetWorld()}.Set_TimeType(ECk_Time_WorldTimeType::RealTime)
    );

    _PingRange.Request_HandleNewPing(FCk_Time{InPing}, CurrentTime.Get_WorldTime());
}

auto
    ACk_PlayerState_UE::
    PostActorCreated()
    -> void
{
    TRACE_BOOKMARK(TEXT("Player State Created"));
    ck::core::Log(TEXT("Player State [{}] Created"), this);
    Super::PostActorCreated();
}

auto
    ACk_PlayerState_UE::
    BeginPlay()
    -> void
{
    TRACE_BOOKMARK(TEXT("Player State Begin Play"));
    ck::core::Log(TEXT("Player State [{}] Begin Play"), this);
    Super::BeginPlay();

    // Covers the authority/host + standalone case (owner is set synchronously). The remote-client case
    // is handled by ClientInitialize once the owning controller link replicates. Both are idempotent.
    DoTryReportBuildId();
}

auto
    ACk_PlayerState_UE::
    ClientInitialize(
        AController* InController)
    -> void
{
    Super::ClientInitialize(InController);

    // Fired on the owning client once this PlayerState is associated with its controller — the
    // earliest point at which a Server RPC on this (now-owned) PlayerState is guaranteed to route.
    DoTryReportBuildId();
}

auto
    ACk_PlayerState_UE::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _ClientBuildId, Params);
}

auto
    ACk_PlayerState_UE::
    DoTryReportBuildId()
    -> void
{
    if (_HasReportedBuildId)
    { return; }

    // Only the locally-controlled PlayerState reports. Other players' PlayerStates also exist on this
    // machine as simulated proxies; a Server RPC from those would not route (not owned by this connection).
    const auto* OwningController = GetOwningController();
    if (ck::Is_NOT_Valid(OwningController) || NOT OwningController->IsLocalController())
    { return; }

    _HasReportedBuildId = true;

    const auto LocalBuildId = ck::Get_BuildId();

    if (HasAuthority())
    {
        // Listen-host local player (or standalone) — set directly; no RPC needed.
        _ClientBuildId = LocalBuildId;
        MARK_PROPERTY_DIRTY_FROM_NAME(ACk_PlayerState_UE, _ClientBuildId, this);
    }
    else
    {
        Server_ReportBuildId(LocalBuildId);
    }
}

bool
    ACk_PlayerState_UE::
    Server_ReportBuildId_Validate(
        const FString& InClientBuildId)
{
    return true;
}

void
    ACk_PlayerState_UE::
    Server_ReportBuildId_Implementation(
        const FString& InClientBuildId)
{
    _ClientBuildId = InClientBuildId;
    MARK_PROPERTY_DIRTY_FROM_NAME(ACk_PlayerState_UE, _ClientBuildId, this);

    // Server-authoritative comparison — the surface that works on a headless dedicated server (which
    // renders no watermark). Event-driven (once per client report), so it never spams the log.
    const auto ServerBuildId = ck::Get_BuildId();
    if (InClientBuildId != ServerBuildId)
    {
        ck::core::Warning(TEXT("[Version] Client [{}] connected with build [{}] != server build [{}] - VERSION MISMATCH"),
            GetPlayerName(), InClientBuildId, ServerBuildId);
    }
    else
    {
        ck::core::Log(TEXT("[Version] Client [{}] connected with matching build [{}]"),
            GetPlayerName(), InClientBuildId);
    }
}

auto
    ACk_PlayerState_UE::
    Get_ClientBuildId() const
    -> FString
{
    return _ClientBuildId;
}

auto
    ACk_PlayerState_UE::
    Get_MinPing() const
    -> FCk_Time
{
    return _PingRange.Get_MinPing();
}

auto
    ACk_PlayerState_UE::
    Get_MaxPing() const
    -> FCk_Time
{
    return _PingRange.Get_MaxPing();
}

#if NOT CK_BUILD_TEST_OR_SHIPPING
auto
    ACk_PlayerState_UE::
    Get_PingRangeHistoryEntries() const
    -> TArray<FCk_PlayerState_PingRange_History_Entry>
{
    return _PingRange.Get_PingHistory().Get_Entries();
}
#endif

// --------------------------------------------------------------------------------------------------------------------
