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

public:
    auto
    GetDisplayName() const -> FString;
};

// --------------------------------------------------------------------------------------------------------------------