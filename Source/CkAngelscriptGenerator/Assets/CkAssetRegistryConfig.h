#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CkAssetRegistryConfig.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Category = "Ck|AssetRegistry")
class CKANGELSCRIPTGENERATOR_API UCkAssetRegistryConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset Registry",
              meta = (ContentDir))
    FString AssetDiscoveryRoot = TEXT("/Game");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset Registry")
    FString OutputFileName = TEXT("Assets.as");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset Registry")
    FString Namespace = TEXT("assets");

    /**
     * Class names, UNPREFIXED (e.g. "RecastNavMesh"), whose assets must never reach the generated
     * registry. Discovery is path-based, so a root inevitably sweeps up engine artifacts a level
     * happens to contain - and an emitted accessor is a name script can reach for. Empty by
     * default: a config only excludes a class it can say why it is excluding.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Asset Registry")
    TArray<FName> ExcludedAssetClasses;

public:
    auto
    GetDisplayName() const -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
