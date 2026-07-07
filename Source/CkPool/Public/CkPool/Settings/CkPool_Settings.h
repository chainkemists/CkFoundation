#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkPool/CkPool_Common.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include <Templates/SubclassOf.h>

#include "CkPool_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOOL_API FCk_EntityPool_SettingsEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityPool_SettingsEntry);

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    TSoftClassPtr<UCk_EntityScript_UE> _EntityScriptClass;

    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _PrewarmCount = 0;

    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _PrewarmBudgetPerTick = 1;

    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    ECk_Pool_CapacityPolicy _CapacityPolicy = ECk_Pool_CapacityPolicy::Unbounded;

    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = "1", EditCondition = "_CapacityPolicy == ECk_Pool_CapacityPolicy::Bounded"))
    int32 _MaxSize = 32;

    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    ECk_Pool_ExhaustionPolicy _ExhaustionPolicy = ECk_Pool_ExhaustionPolicy::Grow;

public:
    CK_PROPERTY_GET(_EntityScriptClass);
    CK_PROPERTY_GET(_PrewarmCount);
    CK_PROPERTY_GET(_PrewarmBudgetPerTick);
    CK_PROPERTY_GET(_CapacityPolicy);
    CK_PROPERTY_GET(_MaxSize);
    CK_PROPERTY_GET(_ExhaustionPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPOOL_API FCk_ObjectPool_SettingsEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ObjectPool_SettingsEntry);

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    TSoftClassPtr<UObject> _ObjectClass;

    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _PrewarmCount = 0;

    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _PrewarmBudgetPerTick = 1;

    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    ECk_Pool_CapacityPolicy _CapacityPolicy = ECk_Pool_CapacityPolicy::Unbounded;

    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, ClampMin = "1", EditCondition = "_CapacityPolicy == ECk_Pool_CapacityPolicy::Bounded"))
    int32 _MaxSize = 32;

    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    ECk_Pool_ExhaustionPolicy _ExhaustionPolicy = ECk_Pool_ExhaustionPolicy::Grow;

public:
    CK_PROPERTY_GET(_ObjectClass);
    CK_PROPERTY_GET(_PrewarmCount);
    CK_PROPERTY_GET(_PrewarmBudgetPerTick);
    CK_PROPERTY_GET(_CapacityPolicy);
    CK_PROPERTY_GET(_MaxSize);
    CK_PROPERTY_GET(_ExhaustionPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

// Per-class pool configuration for AUTO-CREATED pools (the bare Acquire path). Resolution order at pool
// creation: explicit Request_CreatePool params (settings never consulted) > settings entry for the class >
// built-in defaults. Settings are read ONCE when the pool is created — they are not live-reactive
UCLASS(meta = (DisplayName = "Pool"))
class CKPOOL_API UCk_Pool_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Pool_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Entity Pools",
        meta = (AllowPrivateAccess = true, TitleProperty = "_EntityScriptClass",
            ToolTip = "Configuration applied when Request_Acquire auto-creates a class's default EntityPool"))
    TArray<FCk_EntityPool_SettingsEntry> _EntityPools;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Object Pools",
        meta = (AllowPrivateAccess = true, TitleProperty = "_ObjectClass",
            ToolTip = "Configuration applied when Acquire auto-creates a class's ObjectPool"))
    TArray<FCk_ObjectPool_SettingsEntry> _ObjectPools;

public:
    CK_PROPERTY_GET(_EntityPools);
    CK_PROPERTY_GET(_ObjectPools);

public:
    static auto
    TryGet_EntityPoolEntry(
        const TSubclassOf<UCk_EntityScript_UE>& InEntityScriptClass) -> TOptional<FCk_EntityPool_SettingsEntry>;

    static auto
    TryGet_ObjectPoolEntry(
        const TSubclassOf<UObject>& InObjectClass) -> TOptional<FCk_ObjectPool_SettingsEntry>;
};

// --------------------------------------------------------------------------------------------------------------------
