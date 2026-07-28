#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkResourceLoader/CkResourceLoader_Fragment_Data.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkResourceLoader_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Resource Loader"))
class CKRESOURCELOADER_API UCk_ResourceLoader_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ResourceLoader_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Caching",
              meta = (AllowPrivateAccess = true, UIMin = 100, ClampMin = 100))
    int32 _MaxNumberOfCachedResourcesPerType = 100;

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Loading",
              meta = (AllowPrivateAccess = true))
    ECk_ResourceLoader_LoadingPolicy _DefaultLoadingPolicy = ECk_ResourceLoader_LoadingPolicy::Async;

    // Per-consumer loading-policy override — the per-project debug knob. Key = the ConsumerId a
    // processor passes to RequestLoad_RootedBatch (e.g. "AudioTrack.Setup"); flip that one consumer
    // to Synchronous in this project alone via Project Settings or the config ini.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Loading",
              meta = (AllowPrivateAccess = true))
    TMap<FName, ECk_ResourceLoader_LoadingPolicy> _PerConsumerLoadingPolicyOverrides;

public:
    CK_PROPERTY_GET(_MaxNumberOfCachedResourcesPerType);
    CK_PROPERTY_GET(_DefaultLoadingPolicy);
    CK_PROPERTY_GET(_PerConsumerLoadingPolicyOverrides);
};

// --------------------------------------------------------------------------------------------------------------------

class CKRESOURCELOADER_API UCk_Utils_ResourceLoader_Settings_UE
{
public:
    static auto Get_MaxNumberOfCachedResourcesPerType() -> int32;
    static auto Get_DefaultLoadingPolicy() -> ECk_ResourceLoader_LoadingPolicy;
    static auto Get_LoadingPolicyForConsumer(FName InConsumerId) -> ECk_ResourceLoader_LoadingPolicy;
};

// --------------------------------------------------------------------------------------------------------------------
