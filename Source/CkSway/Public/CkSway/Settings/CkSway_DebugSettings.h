#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkSettings/UserSettings/CkUserSettings.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CkSway_DebugSettings.generated.h"

UCLASS(meta = (DisplayName = "Sway Debug"))
class CKSWAY_API UCk_Sway_DebugSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Sway_DebugSettings_UE);
    virtual void PostInitProperties() override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    UPROPERTY(Config, EditAnywhere, Category = "Visualization", meta = (AllowPrivateAccess = true))
    bool _DrawOffsets = false;

public:
    CK_PROPERTY(_DrawOffsets);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKSWAY_API UCk_Utils_Sway_DebugSettings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Sway_DebugSettings_UE);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Sway|DebugSettings")
    static bool
    Get_DrawOffsets();
};
