#pragma once

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"

#include <Subsystems/GameInstanceSubsystem.h>

#include "CkEnsure_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_Ensure")
class CKCORE_API UCk_Ensure_Subsystem_UE : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ensure_Subsystem_UE);

public:
    virtual auto Deinitialize() -> void override;
    virtual auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;

public:
    static auto Get_Instance() -> UCk_Ensure_Subsystem_UE*;

public:
    auto
    Get_IsEnsureIgnored(
        FName InFile,
        int32 InLine) const -> bool;

    auto
    Get_IsEnsureIgnored_WithCallstack(
        const FString& InCallstack) const -> bool;

    auto
    Get_AllIgnoredEnsures() const -> TArray<FCk_Ensure_IgnoredEntry>;

    auto
    Get_EnsureCount() const -> int32;

    auto
    Get_UniqueEnsureCount() const -> int32;

public:
    auto
    BindTo_OnEnsureIgnored(
        const FCk_Delegate_OnEnsureIgnored& InDelegate) -> void;

    auto
    UnbindFrom_OnEnsureIgnored(
        const FCk_Delegate_OnEnsureIgnored& InDelegate) -> void;

    auto
    BindTo_OnEnsureCountChanged(
        const FCk_Delegate_OnEnsureCountChanged& InDelegate) -> void;

    auto
    UnbindFrom_OnEnsureCountChanged(
        const FCk_Delegate_OnEnsureCountChanged& InDelegate) -> void;

public:
    auto
    Request_ClearAllIgnoredEnsures() -> void;

    auto
    Request_IncrementEnsureCountAtFileAndLine(
        FName InFile,
        int32 InLine) -> void;

    auto
    Request_IncrementEnsureCountWithCallstack(
        const FString& InCallstack) -> void;

    auto
    Request_IgnoreEnsureAtFileAndLineWithMessage(
        FName InFile,
        const FText& InMessage,
        int32 InLine) -> void;

    auto
    Request_IgnoreEnsureAtFileAndLine(
        FName InFile,
        int32 InLine) -> void;

    auto
    Request_IgnoreEnsure_WithCallstack(
        const FString& InCallstack) -> void;

    auto
    Request_IgnoreAllEnsures() -> void;

private:
    bool                                       _IgnoreAllEnsure = false;
    int32                                      _NumberOfEnsuresTriggered = 0;
    int32                                      _NumberOfUniqueEnsuresTriggered = 0;
    TMap<FName, TSet<FCk_Ensure_Entry>>        _UniqueTriggeredEnsures;
    TSet<FString>                              _UniqueTriggeredEnsures_BP;
    TMap<FName, TSet<FCk_Ensure_IgnoredEntry>> _IgnoredEnsures;
    TSet<FString>                              _IgnoredEnsures_BP;

private:
    UPROPERTY(Transient)
    FCk_Delegate_OnEnsureIgnored_MC      _OnIgnoredEnsure_MC;

    UPROPERTY(Transient)
    FCk_Delegate_OnEnsureCountChanged_MC _OnEnsureCountChanged_MC;

private:
    static TWeakObjectPtr<ThisType> _Instance;
};

// --------------------------------------------------------------------------------------------------------------------