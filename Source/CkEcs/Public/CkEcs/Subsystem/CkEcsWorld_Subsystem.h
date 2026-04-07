#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Scheduler/CkProcessorScheduler.h"

#include <Subsystems/WorldSubsystem.h>
#include <GameFramework/Info.h>
#include <GameplayTags.h>

#include "CkEcsWorld_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ecs_WorldStatCollection_Policy : uint8
{
    DoNotCollect,
    CollectOnLocalClientOnly,
    CollectOnServerOnly,
    CollectOnLocalClientAndServer
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ecs_WorldStatCollection_Policy);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, NotBlueprintType)
class CKECS_API ACk_EcsWorld_Actor_UE final : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_EcsWorld_Actor_UE);

public:
    friend class UCk_EcsWorld_Subsystem_UE;
    friend class UCk_EcsWorld_Stats_Subsystem_UE;

public:
    ACk_EcsWorld_Actor_UE();

protected:
    auto
    Tick(
        float DeltaSeconds) -> void override;

public:
    auto
    Initialize(
        ck::FProcessorScheduler&& InScheduler,
        const FCk_Registry& InRegistry,
        ETickingGroup InTickGroup) -> void;

private:
    TOptional<ck::FProcessorScheduler> _Scheduler;
    const FCk_Registry* _Registry = nullptr;

    TStatId _TickStatId;
    FString _TickStatName;
    ETickingGroup _UnrealTickingGroup = TG_PrePhysics;

    FGameplayTag _EcsWorldTickingGroup;
    ECk_Ecs_WorldStatCollection_Policy _StatCollectionPolicy = ECk_Ecs_WorldStatCollection_Policy::DoNotCollect;
    FName _EcsWorldDisplayName;

public:
    CK_PROPERTY_GET(_UnrealTickingGroup);
    CK_PROPERTY_GET(_TickStatName);
    CK_PROPERTY_GET(_EcsWorldTickingGroup);
    CK_PROPERTY_GET(_StatCollectionPolicy);
    CK_PROPERTY_GET(_EcsWorldDisplayName);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_EcsWorld")
class CKECS_API UCk_EcsWorld_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsWorld_Subsystem_UE);

public:
    friend class UCk_EcsWorld_Stats_Subsystem_UE;

public:
    auto
    Initialize(
        FSubsystemCollectionBase& Collection) -> void override;
    auto
    Deinitialize() -> void override;

    auto
    OnWorldBeginPlay(
        UWorld& InWorld) -> void override;

private:
    auto DoBuildGraphAndSpawnActors(
        UWorld& InWorld) -> void;

private:
    UPROPERTY(BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = true))
    FCk_Handle _TransientEntity;

private:
    TMap<TEnumAsByte<ETickingGroup>, TStrongObjectPtr<ACk_EcsWorld_Actor_UE>> _WorldActors;

private:
    FCk_Registry _Registry;

public:
    CK_PROPERTY_GET(_TransientEntity);
    CK_PROPERTY_GET(_Registry);
    CK_PROPERTY_GET_NON_CONST(_Registry);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_EcsWorld_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UCk_EcsWorld_Subsystem_UE;

public:
    static auto
    Get_TransientEntity(
        const UWorld* InWorld) -> FCk_Handle;

    UFUNCTION(BlueprintPure, BlueprintInternalUseOnly, meta = (WorldContext = "InWorldContextObject"))
    static FCk_Handle
    Get_TransientEntity_FromContextObject(
        const UObject* InWorldContextObject);

public:
    template <typename T_SubsystemClass>
    [[nodiscard]]
    static auto
    Get_WorldSubsystem(
        const FCk_Handle& InAnyHandle) -> T_SubsystemClass*
    ;
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_SubsystemClass>
auto
    UCk_Utils_EcsWorld_Subsystem_UE::
    Get_WorldSubsystem(
        const FCk_Handle& InAnyHandle)
    -> T_SubsystemClass*
{
    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyHandle);

    CK_ENSURE_IF_NOT(ck::IsValid(World),
        TEXT("Unable to a valid World [{}] from Handle [{}]"), World, InAnyHandle)
    { return nullptr; }

    return World->GetSubsystem<T_SubsystemClass>();
}

// --------------------------------------------------------------------------------------------------------------------
