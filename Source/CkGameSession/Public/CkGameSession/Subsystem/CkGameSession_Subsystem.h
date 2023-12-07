#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Registry/CkRegistry.h"

#include "CkSignal/CkSignal_Macros.h"

#include <GameFramework/GameModeBase.h>
#include <Subsystems/GameInstanceSubsystem.h>

#include "CkGameSession_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_OnLoginEvent,
    APlayerController*,
    InPlayerController,
    const TArray<APlayerController*>&,
    InAllPlayerControllers);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_OnLoginEvent_MC,
    APlayerController*,
    InPlayerController,
    const TArray<APlayerController*>&,
    InAllPlayerControllers);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_OnLogoutEvent,
    APlayerController*,
    InPlayerController,
    const TArray<APlayerController*>&,
    InAllPlayerControllers);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_OnLogoutEvent_MC,
    APlayerController*,
    InPlayerController,
    const TArray<APlayerController*>&,
    InAllPlayerControllers);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKGAMESESSION_API,
        OnLoginEvent,
        FCk_Delegate_OnLoginEvent_MC,
        TWeakObjectPtr<APlayerController>,
        TArray<TWeakObjectPtr<APlayerController>>);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKGAMESESSION_API,
        OnLogoutEvent,
        FCk_Delegate_OnLogoutEvent_MC,
        TWeakObjectPtr<APlayerController>,
        TArray<TWeakObjectPtr<APlayerController>>);
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, BlueprintType, DisplayName="CkSubsystem_GameSession")
class CKGAMESESSION_API UCk_GameSession_Subsystem_UE : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSession_Subsystem_UE);

public:
    auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    auto Deinitialize() -> void override;
    auto ShouldCreateSubsystem(UObject* InOuter) const -> bool override;

private:
    UFUNCTION()
    void
    OnLoginEvent(
        AGameModeBase* InGameModeBase,
        APlayerController* InPlayerController);

    UFUNCTION()
    void
    OnLogoutEvent(
        AGameModeBase* InGameModeBase,
        AController* InPlayerController);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSession|Subsystem")
    void
    BindTo_OnLoginEvent(
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_OnLoginEvent& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSession|Subsystem")
    void
    BindTo_OnLogoutEvent(
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_OnLogoutEvent& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSession|Subsystem")
    void
    Unbind_OnLoginEvent(
        const FCk_Delegate_OnLoginEvent& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSession|Subsystem")
    void
    Unbind_OnLogoutEvent(
        const FCk_Delegate_OnLogoutEvent& InDelegate);

private:
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<APlayerController>>  _AllPlayerControllers;

private:
    FCk_Registry _InternalRegistry;
    FCk_Handle _SignalHandle;

    FDelegateHandle _PostLoginDelegateHandle;
    FDelegateHandle _PostLogoutDelegateHandle;

public:
    CK_PROPERTY_GET(_SignalHandle);
};

// --------------------------------------------------------------------------------------------------------------------