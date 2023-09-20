#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Engine/DeveloperSettingsBackedByCVars.h>

#include "CkOverlapBody_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "OverlapBody"))
class CKOVERLAPBODY_API UCk_OverlapBody_UserSettings_UE : public UDeveloperSettingsBackedByCVars
{
    GENERATED_BODY()

public:
    explicit UCk_OverlapBody_UserSettings_UE(const FObjectInitializer& ObjectInitializer);

private:
    // Should draw the debug information of all existing Sensor Fragments
    // CVar: ck.OverlapBody.ShouldPreviewAllSensors
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable="ck.OverlapBody.ShouldPreviewAllSensors"))
    bool _DebugPreviewAllSensors = false;

    // Should draw the debug information of all existing Marker Fragments
    // CVar: ck.OverlapBody.ShouldPreviewAllMarkers
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable="ck.OverlapBody.ShouldPreviewAllMarkers"))
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
