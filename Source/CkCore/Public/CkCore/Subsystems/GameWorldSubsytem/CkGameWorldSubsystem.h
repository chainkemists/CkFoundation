#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkGameWorldSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/*
 * This particular Subsystem should be derived from IFF we need a WorldSubsystem that is only created when we are
 * in-game. There are several other worlds created in the Editor that are not in PIE
 *
 */
UCLASS(BlueprintType)
class CKCORE_API UCk_Game_WorldSubsystem_Base_UE : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Game_WorldSubsystem_Base_UE);

public:
    auto Deinitialize() -> void override;
    auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    auto PostInitialize() -> void override;
    auto ShouldCreateSubsystem(UObject* Outer) const -> bool override;

protected:
    auto DoesSupportWorldType(const EWorldType::Type WorldType) const -> bool override;
    auto Get_AreAllWorldSubsystemsInitialized() const -> bool;

public:
    UFUNCTION(BlueprintImplementableEvent)
    void OnInitialize();

    UFUNCTION(BlueprintImplementableEvent)
    void OnAllWorldSubsystemsInitialized();

    UFUNCTION(BlueprintImplementableEvent)
    void OnDeinitialize();

private:
    bool _AllWorldSubsystemsInitialized = false;
};

// --------------------------------------------------------------------------------------------------------------------

/*
 * This particular Subsystem should be derived from IFF we need a TickableWorldSubsystem that is only created when we are
 * in-game. There are several other worlds created in the Editor that are not in PIE
 *
 */
UCLASS(BlueprintType)
class CKCORE_API UCk_Game_TickableWorldSubsystem_Base_UE : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Game_TickableWorldSubsystem_Base_UE);

public:
    auto Deinitialize() -> void override;
    auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    auto PostInitialize() -> void override;
    auto ShouldCreateSubsystem(UObject* Outer) const -> bool override;
    auto GetStatId() const -> TStatId override;

protected:
    auto DoesSupportWorldType(const EWorldType::Type WorldType) const -> bool override;
    auto Get_AreAllWorldSubsystemsInitialized() const -> bool;

public:
    UFUNCTION(BlueprintImplementableEvent)
    void OnInitialize();

    UFUNCTION(BlueprintImplementableEvent)
    void OnAllWorldSubsystemsInitialized();

    UFUNCTION(BlueprintImplementableEvent)
    void OnDeinitialize();

private:
    bool _AllWorldSubsystemsInitialized = false;
};

// --------------------------------------------------------------------------------------------------------------------
