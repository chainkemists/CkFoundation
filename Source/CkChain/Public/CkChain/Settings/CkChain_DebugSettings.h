#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkSettings/UserSettings/CkUserSettings.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CkChain_DebugSettings.generated.h"

UCLASS(meta = (DisplayName = "Chain Debug"))
class CKCHAIN_API UCk_Chain_DebugSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Chain_DebugSettings_UE);
    virtual void PostInitProperties() override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    UPROPERTY(Config, EditAnywhere, Category = "Visualization", meta = (AllowPrivateAccess = true))
    bool _DrawHistory = false;

    UPROPERTY(Config, EditAnywhere, Category = "Visualization", meta = (AllowPrivateAccess = true))
    bool _DrawLinkTargets = false;

public:
    CK_PROPERTY(_DrawHistory);
    CK_PROPERTY(_DrawLinkTargets);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCHAIN_API UCk_Utils_Chain_DebugSettings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Chain_DebugSettings_UE);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain|DebugSettings")
    static bool
    Get_DrawHistory();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain|DebugSettings")
    static bool
    Get_DrawLinkTargets();
};
