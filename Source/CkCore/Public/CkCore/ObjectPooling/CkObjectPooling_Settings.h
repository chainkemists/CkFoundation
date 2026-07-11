#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/ObjectPooling/CkObjectPooling_Params.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include <Templates/SubclassOf.h>

#include "CkObjectPooling_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_ObjectPooling_SettingsEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ObjectPooling_SettingsEntry);

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    TSoftClassPtr<UObject> _ObjectClass;

    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_ObjectPooling_PoolParams _PoolParams;

public:
    CK_PROPERTY_GET(_ObjectClass);
    CK_PROPERTY_GET(_PoolParams);
};

// --------------------------------------------------------------------------------------------------------------------

// Per-class pool configuration. An entry for a class OVERRIDES the acquire-site params when that
// class's pool is created — it exists to let a project tune pools without touching call sites or
// assets. Settings are read ONCE when the pool is created — they are not live-reactive
UCLASS(meta = (DisplayName = "Object Pooling"))
class CKCORE_API UCk_ObjectPooling_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ObjectPooling_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Object Pools",
        meta = (AllowPrivateAccess = true, TitleProperty = "_ObjectClass",
            ToolTip = "Per-class pool configuration override, applied when the class's pool is created"))
    TArray<FCk_ObjectPooling_SettingsEntry> _ObjectPools;

public:
    CK_PROPERTY_GET(_ObjectPools);

public:
    static auto
    TryGet_PoolEntry(
        const TSubclassOf<UObject>& InObjectClass) -> TOptional<FCk_ObjectPooling_SettingsEntry>;
};

// --------------------------------------------------------------------------------------------------------------------
