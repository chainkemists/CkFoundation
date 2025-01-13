#include "CkPlayerState.h"

#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "GameFramework/PlayerController.h"
#include "Misc/App.h"
#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"

#include "CkCore/CkCoreLog.h"

// --------------------------------------------------------------------------------------------------------------------

#if CK_BUILD_TEST
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

#if CK_BUILD_TEST
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

#if CK_BUILD_TEST
auto
    ACk_PlayerState_UE::
    Get_PingRangeHistoryEntries() const
    -> TArray<FCk_PlayerState_PingRange_History_Entry>
{
    return _PingRange.Get_PingHistory().Get_Entries();
}
#endif

// --------------------------------------------------------------------------------------------------------------------
