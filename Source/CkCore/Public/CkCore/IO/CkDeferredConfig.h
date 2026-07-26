#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Engine/DataAsset.h>

#include "CkDeferredConfig.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Base for UDataAsset configs referencing assets that are unsafe to load during Angelscript
// __InitDefaults (UTypedElementRegistry is not ready yet): hold them as TSoftObjectPtr, override
// ResolveAssets(), and the base resolves on FCoreDelegates::OnFEngineLoopInitComplete.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType)
class CKCORE_API UCk_DeferredConfig_UE : public UDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DeferredConfig_UE);

    virtual void PostInitProperties() override;

    // Override in a subclass to turn its soft refs into hard refs (System::LoadAsset_Blocking).
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|Config",
              DisplayName = "[Ck] Resolve Deferred Assets")
    void
    ResolveAssets();

    // Idempotent, and already runs automatically after engine init — call it only as a safety net.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Config",
              DisplayName = "[Ck] Ensure Config Resolved")
    void
    EnsureResolved();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Config",
              DisplayName = "[Ck] Are Assets Resolved")
    bool
    AreAssetsResolved() const;

    // Called by OnFEngineLoopInitComplete to resolve all pending configs.
    static void ResolveAllPending();

private:
    UPROPERTY(Transient)
    bool _IsResolved = false;

    static TArray<TWeakObjectPtr<UCk_DeferredConfig_UE>> _PendingConfigs;
};

// --------------------------------------------------------------------------------------------------------------------
