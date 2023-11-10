#include "CkPlayerState.h"

#include "CkCore/Time/CkTime_Utils.h"
#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

const FCk_Time ACk_PlayerState_UE::_PingFrequency = FCk_Time::OneSecond;

// --------------------------------------------------------------------------------------------------------------------

const FCk_Time FCk_PlayerState_PingRange::_UpdateFrequency = FCk_Time{ 5.0f };

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

    const auto& UpdateTime = _LastUpdateTime + _UpdateFrequency;

    if (InCurrentTime < UpdateTime)
    { return; }

    _LastUpdateTime = InCurrentTime.Get_Time();

    _MinPing = _NextMinPing;
    _MaxPing = _NextMaxPing;

    _NextMinPing = FCk_Time::OneSecond;
    _NextMaxPing = FCk_Time::Zero;

#if CK_BUILD_TEST
    const auto PingHistoryEntry = FCk_PlayerState_PingRange_History_Entry{}.Set_MinPing(_MinPing).Set_MaxPing(_MaxPing);

    _PingHistory.Request_AddNewEntry(PingHistoryEntry);
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_PlayerState_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    if (NOT HasAuthority())
    { return; }

    if (ck::Is_NOT_Valid(Owner))
    { return; }
    
    const auto& PlayerController = Cast<APlayerController>(Owner);
    if (ck::Is_NOT_Valid(PlayerController))
    { return; }

    const auto& GameInstance = GetGameInstance();
    if (ck::Is_NOT_Valid(GameInstance))
    { return; }

    constexpr bool ShouldTimerLoop = true;
    GameInstance->GetTimerManager().SetTimer
    (
        _PingTimerHandle,
        this,
        &ThisType::DoRequest_Ping,
        _PingFrequency.Get_Seconds(),
        ShouldTimerLoop
    );
}

auto
    ACk_PlayerState_UE::
    EndPlay(
        EEndPlayReason::Type InEndPlayReason)
    -> void
{
    const auto& GameInstance = GetGameInstance();
    if (ck::IsValid(GameInstance))
    {
        GameInstance->GetTimerManager().ClearTimer(_PingTimerHandle);
    }

    Super::EndPlay(InEndPlayReason);
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
        FCk_Utils_Time_GetWorldTime_Params
        {
            this, GetWorld(),
            ECk_Time_WorldTimeType::RealTime
        }
    );

    _PingRange.Request_HandleNewPing(FCk_Time{InPing}, CurrentTime.Get_WorldTime());
}

auto
    ACk_PlayerState_UE::
    Server_Request_Pong_Implementation()
    -> void
{
    const auto& CurrentTime      = FApp::GetCurrentTime();
    const auto& TimePassedInSecs = CurrentTime - static_cast<double>(_PingSentTime.Get_Seconds());

    _CurrentPingInMs  = FMath::RoundToInt(static_cast<float>(TimePassedInSecs) * 1000.0f);
    _PingSentTime = FCk_Time::Zero;
}

auto
    ACk_PlayerState_UE::
    Server_Request_Pong_Validate()
    -> bool
{
    return true;
}

auto
    ACk_PlayerState_UE::
    Client_Request_Ping_Implementation()
    -> void
{
    Server_Request_Pong();
}

auto
    ACk_PlayerState_UE::
    DoRequest_Ping()
    -> void
{
    if (_PingSentTime != FCk_Time::Zero)
    { return; }

    Client_Request_Ping();

    const auto& CurrentTime = FApp::GetCurrentTime();

    _PingSentTime = FCk_Time{static_cast<float>(CurrentTime)};
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

auto
    ACk_PlayerState_UE::
    GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME_CONDITION(ACk_PlayerState_UE, _CurrentPingInMs, COND_None)
}

// --------------------------------------------------------------------------------------------------------------------
