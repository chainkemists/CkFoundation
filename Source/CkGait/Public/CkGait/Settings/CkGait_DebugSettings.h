#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkSettings/UserSettings/CkUserSettings.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CkGait_DebugSettings.generated.h"

UCLASS(meta = (DisplayName = "Gait Debug"))
class CKGAIT_API UCk_Gait_DebugSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Gait_DebugSettings_UE);
    virtual void PostInitProperties() override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    UPROPERTY(Config, EditAnywhere, Category = "Visualization", meta = (AllowPrivateAccess = true))
    bool _DrawBobs = false;

public:
    CK_PROPERTY(_DrawBobs);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKGAIT_API UCk_Utils_Gait_DebugSettings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Gait_DebugSettings_UE);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Gait|DebugSettings")
    static bool
    Get_DrawBobs();
};
