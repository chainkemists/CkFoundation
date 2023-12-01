#pragma once

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkLog_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Logging"))
class CKLOG_API UCk_Log_Settings_UE : public UCk_Engine_ProjectSettings_UE
{
    GENERATED_BODY()

protected:
    /**
    * Default logger to use if none was explicitly specified when calling the various log functions
    */
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Logger",
              meta = (AllowPrivateAccess = true))
    FName _DefaultLoggerName = TEXT("CkLogger");

public:
    auto Get_DefaultLoggerName() const -> FName;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKLOG_API UCk_Utils_Log_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Settings|Logger")
    static FName Get_DefaultLoggerName();
};
// --------------------------------------------------------------------------------------------------------------------
