#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Engine/DataAsset.h>

#include "CkDeferredConfig.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Deferred Config Base Class
//
// Base class for UDataAsset configs that reference assets which are unsafe to
// load during Angelscript __InitDefaults (before UTypedElementRegistry is ready).
//
// Usage pattern:
//   1. Subclass this instead of UDataAsset
//   2. Use TSoftObjectPtr / TSoftClassPtr for properties that reference assets
//      whose packages contain UActorComponent exports (Niagara, Blueprints, etc.)
//   3. Add corresponding hard-reference properties for runtime access
//   4. Override ResolveAssets() to resolve soft refs → hard refs
//   5. In asset declarations, use assets:: (soft refs) instead of assets::load::
//
// The base class automatically calls ResolveAssets() after engine initialization
// completes (FCoreDelegates::OnFEngineLoopInitComplete).
//
// Example (Angelscript):
//
//   class UMyConfig : UCk_DeferredConfig_UE
//   {
//       // Soft reference (safe to set during asset init)
//       UPROPERTY(EditDefaultsOnly)
//       TSoftObjectPtr<UNiagaraSystem> MyVfx_Deferred;
//
//       // Hard reference (auto-resolved after engine init)
//       UPROPERTY(Transient)
//       UNiagaraSystem MyVfx;
//
//       UFUNCTION(BlueprintOverride)
//       void ResolveAssets()
//       {
//           MyVfx = System::LoadAsset_Blocking(MyVfx_Deferred);
//       }
//   }
//
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType)
class CKCORE_API UCk_DeferredConfig_UE : public UDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DeferredConfig_UE);

    virtual void PostInitProperties() override;

    // Resolve all soft references to hard references.
    // Override in subclass to call System::LoadAsset_Blocking etc.
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|Config",
              DisplayName = "[Ck] Resolve Deferred Assets")
    void
    ResolveAssets();

    // Manually trigger resolution (idempotent).
    // Called automatically after engine init, but can be called explicitly as a safety net.
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
