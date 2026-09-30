#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkSettings/UserSettings/CkUserSettings.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CkRotateTowards_DebugSettings.generated.h"

UCLASS(meta = (DisplayName = "Rotate Towards Debug"))
class CKROTATETOWARDS_API UCk_RotateTowards_DebugSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_RotateTowards_DebugSettings_UE);
    virtual void PostInitProperties() override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    UPROPERTY(Config, EditAnywhere, Category = "Visualization", meta = (AllowPrivateAccess = true))
    bool _DrawTargets = false;

public:
    CK_PROPERTY(_DrawTargets);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKROTATETOWARDS_API UCk_Utils_RotateTowards_DebugSettings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RotateTowards_DebugSettings_UE);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|RotateTowards|DebugSettings")
    static bool
    Get_DrawTargets();
};
