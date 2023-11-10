#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkCore/Build/CkBuild_Macros.h"
#include "Containers/RingBuffer.h"

#include <GameFramework/PlayerState.h>

#include "CkPlayerState.generated.h"

// --------------------------------------------------------------------------------------------------------------------

#if CK_BUILD_TEST

struct FCk_PlayerState_PingRange_History_Entry
{
    CK_GENERATED_BODY(FCk_PlayerState_PingRange_History_Entry);

private:
    FCk_Time _MinPing;
    FCk_Time _MaxPing;

public:
    CK_PROPERTY(_MinPing);
    CK_PROPERTY(_MaxPing);
};

// --------------------------------------------------------------------------------------------------------------------

struct FCk_PlayerState_PingRange_History
{
public:
    CK_GENERATED_BODY(FCk_PlayerState_PingRange_History);

public:
    using EntryType   = FCk_PlayerState_PingRange_History_Entry;
    using EntriesType = TArray<EntryType>;

public:
    auto Request_AddNewEntry(const EntryType& InEntry) -> void;

private:
    EntriesType _Entries;

    static constexpr int _MaxHistorySize = 5;

public:
    CK_PROPERTY_GET(_Entries);
};

#endif

// --------------------------------------------------------------------------------------------------------------------

struct FCk_PlayerState_PingRange
{
    CK_GENERATED_BODY(FCk_PlayerState_PingRange);

public:
    auto Request_HandleNewPing(
        const FCk_Time& InPing,
        const FCk_Time_Unreal& InCurrentTime) -> void;

private:
    FCk_Time _MinPing;
    FCk_Time _MaxPing;

    FCk_Time _NextMinPing = FCk_Time::OneSecond;
    FCk_Time _NextMaxPing = FCk_Time::Zero;

    FCk_Time _LastUpdateTime;

    static const FCk_Time _UpdateFrequency;

public:
    CK_PROPERTY_GET(_MaxPing);
    CK_PROPERTY_GET(_MinPing);

#if CK_BUILD_TEST
private:
    FCk_PlayerState_PingRange_History _PingHistory;

public:
    CK_PROPERTY_GET(_PingHistory);
#endif
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKCORE_API ACk_PlayerState_UE : public APlayerState
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_PlayerState_UE);

public:
    auto BeginPlay() -> void override;
    auto EndPlay(EEndPlayReason::Type InEndPlayReason) -> void override;
    auto UpdatePing(float InPing) -> void override;

    auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

public:
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_Request_Pong();

    UFUNCTION(Client, Reliable)
    void Client_Request_Ping();

private:
    void DoRequest_Ping();

public:
    auto Get_MinPing() const -> FCk_Time;
    auto Get_MaxPing() const -> FCk_Time;

#if CK_BUILD_TEST
    auto Get_PingRangeHistoryEntries() const -> TArray<FCk_PlayerState_PingRange_History_Entry>;
#endif

private:
    UPROPERTY(BlueprintReadOnly, Transient, Replicated, meta = (AllowPrivateAccess = true, Units = "ms"))
    int32 _CurrentPingInMs = 0;

private:
    FCk_PlayerState_PingRange _PingRange;

    FTimerHandle _PingTimerHandle;

    FCk_Time _PingSentTime = FCk_Time::Zero;

    static const FCk_Time _PingFrequency;

public:
    CK_PROPERTY_GET(_CurrentPingInMs);
    CK_PROPERTY_GET(_PingFrequency);
    CK_PROPERTY_GET(_PingSentTime);
};

// --------------------------------------------------------------------------------------------------------------------
