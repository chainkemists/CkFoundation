#pragma once

#include "CoreMinimal.h"
#include "CkAssetRegistry_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKANGELSCRIPTGENERATOR_API UCk_Utils_AssetRegistry_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|AssetRegistry")
    static void
    GenerateAllAssetRegistries();
#endif
};

// --------------------------------------------------------------------------------------------------------------------
