#pragma once

#include "CoreMinimal.h"
#include "CkAssetRegistrySubsystem.h"
#include "CkAssetRegistry_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKANGELSCRIPTGENERATOR_API UCk_Utils_AssetRegistry_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|AssetRegistry",
              DisplayName = "[Ck][AssetRegistry] Generate All Asset Registries")
    static void
    GenerateAllAssetRegistries();

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|AssetRegistry",
              DisplayName = "[Ck][AssetRegistry] Get Asset Registry Subsystem")
    static UCkAssetRegistrySubsystem*
    Get_AssetRegistrySubsystem();

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|AssetRegistry",
              DisplayName = "[Ck][AssetRegistry] Bind To Asset Registry Progress")
    static void
    BindTo_AssetRegistryProgress(
        const FOnAssetRegistryProgressDelegate& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|AssetRegistry",
              DisplayName = "[Ck][AssetRegistry] Unbind From Asset Registry Progress")
    static void
    UnbindFrom_AssetRegistryProgress(
        const FOnAssetRegistryProgressDelegate& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|AssetRegistry",
              DisplayName = "[Ck][AssetRegistry] Bind To Asset Registry Complete")
    static void
    BindTo_AssetRegistryComplete(
        const FOnAssetRegistryCompleteDelegate& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|AssetRegistry",
              DisplayName = "[Ck][AssetRegistry] Unbind From Asset Registry Complete")
    static void
    UnbindFrom_AssetRegistryComplete(
        const FOnAssetRegistryCompleteDelegate& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|AssetRegistry",
              DisplayName = "[Ck][AssetRegistry] Unbind All Asset Registry Delegates")
    static void
    UnbindAll_AssetRegistryDelegates();
#endif
};

// --------------------------------------------------------------------------------------------------------------------
