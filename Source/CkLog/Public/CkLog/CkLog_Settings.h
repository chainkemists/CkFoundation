#pragma once

#include "CkMacros/CkMacros.h"

#include <Engine/DeveloperSettings.h>

#include "CkLog_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Ck Log"))
class CKLOG_API UCk_Log_Settings_UE : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Log_Settings_UE);

public:
    UCk_Log_Settings_UE();

protected:
    /**
    * Default logger to use if none was explicitly specified when calling the various log functions
    */
    UPROPERTY(EditAnywhere, Category = "Logger Settings")
    FString _DefaultLoggerName;

private:
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual FName GetCategoryName() const override;
    void SaveToIni();
#endif

public:
    CK_PROPERTY_GET(_DefaultLoggerName)
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKLOG_API UCk_Utils_Log_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Log_Settings_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Settings|Logger")
    static FString Get_DefaultLoggerName();
};
// --------------------------------------------------------------------------------------------------------------------
