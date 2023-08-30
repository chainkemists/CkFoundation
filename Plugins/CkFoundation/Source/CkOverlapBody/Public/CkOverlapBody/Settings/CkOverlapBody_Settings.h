#pragma once

#include <CoreMinimal.h>
#include <Engine/DeveloperSettings.h>

#include "CkMacros/CkMacros.h"
#include "CkOverlapBody_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "OverlapBody"))
class CKOVERLAPBODY_API UCk_OverlapBody_UserSettings_UE : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    explicit UCk_OverlapBody_UserSettings_UE(const FObjectInitializer& ObjectInitializer);

private:
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging", meta = (AllowPrivateAccess = true))
    bool _DebugPreviewAllSensors = false;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging", meta = (AllowPrivateAccess = true))
    bool _DebugPreviewAllMarkers = false;

public:
    CK_PROPERTY_GET(_DebugPreviewAllSensors);
    CK_PROPERTY_GET(_DebugPreviewAllMarkers);
};

// --------------------------------------------------------------------------------------------------------------------

class CKOVERLAPBODY_API UCk_Utils_OverlapBody_UserSettings_UE
{
public:
    static auto Get_DebugPreviewAllSensors() -> bool;
    static auto Get_DebugPreviewAllMarkers() -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
