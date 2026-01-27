#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkDynamic_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Dynamic"))
class CKDYNAMIC_API UCk_Dynamic_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Dynamic_ProjectSettings_UE);

public:
    UCk_Dynamic_ProjectSettings_UE(const FObjectInitializer& ObjectInitializer);

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Dynamic ECS Handles",
              meta = (AllowPrivateAccess = true, RelativePath))
    FDirectoryPath _DynamicHandleRegistryDirectory;

public:
    CK_PROPERTY_GET(_DynamicHandleRegistryDirectory);

public:
    auto Get_DynamicHandleRegistryFilePath() const -> FString;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKDYNAMIC_API UCk_Utils_Dynamic_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Dynamic_Settings_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Dynamic|Settings")
    static FString
    Get_DynamicHandleRegistryFilePath();
};

// --------------------------------------------------------------------------------------------------------------------