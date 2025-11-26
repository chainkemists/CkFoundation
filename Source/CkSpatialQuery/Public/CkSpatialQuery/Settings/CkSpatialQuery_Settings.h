#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/Public/CkSettings/UserSettings/CkUserSettings.h"

#include <CoreMinimal.h>

#include "CkSpatialQuery_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "SpatialQuery"))
class CKSPATIALQUERY_API UCk_SpatialQuery_UserSettings : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

    CK_GENERATED_BODY(UCk_SpatialQuery_UserSettings);

private:
    // Master control for all trace preview cvars
    // 1 = Force enable all trace previews, 0 = Use individual cvar settings (default), -1 = Force disable all trace previews
    // CVar: ck.SpatialQuery.PreviewAllTraces
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable="ck.SpatialQuery.PreviewAllTraces", ClampMin = -1, ClampMax = 1))
    int32 _PreviewAllTraces = 0;

    // Draw the debug information of all existing Probe Fragments
    // CVar: ck.SpatialQuery.PreviewAllProbes
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable="ck.SpatialQuery.PreviewAllProbes"))
    bool _DebugPreviewAllProbes = false;

    // Draw the debug information of all existing Probe Fragments
    // CVar: ck.SpatialQuery.PreviewAllLineTraces
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable="ck.SpatialQuery.PreviewAllLineTraces"))
    bool _DebugPreviewAllLineTraces = false;

    // Draw the debug information of all existing Probe Fragment using Jolt
    // WARNING: At the moment, this is VERY slow. Only use if trying to figure out issue with Jolt physics and Probe Fragments.
    // CVar: ck.SpatialQuery.PreviewAllProbesUsingJolt
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable="ck.SpatialQuery.PreviewAllProbesUsingJolt"))
    bool _DebugPreviewAllProbesUsingJolt = false;

    // Duration in seconds for line trace debug display (0.0 = single frame)
    // CVar: ck.SpatialQuery.ProbeLineTraceDebugDuration
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable="ck.SpatialQuery.ProbeLineTraceDebugDuration"))
    float _ProbeLineTraceDebugDuration = 0.0f;

    // Enable debug preview for server line traces
    // CVar: ck.SpatialQuery.DebugPreviewServerLineTraces
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable="ck.SpatialQuery.DebugPreviewServerLineTraces"))
    bool _DebugPreviewServerLineTraces = true;

    // Enable debug preview for client line traces
    // CVar: ck.SpatialQuery.DebugPreviewClientLineTraces
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable="ck.SpatialQuery.DebugPreviewClientLineTraces"))
    bool _DebugPreviewClientLineTraces = true;

    // Line thickness for probe line trace debug drawing
    // CVar: ck.SpatialQuery.ProbeLineTraceDebugThickness
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable="ck.SpatialQuery.ProbeLineTraceDebugThickness"))
    float _ProbeLineTraceDebugThickness = 0.5f;

public:
    CK_PROPERTY_GET(_PreviewAllTraces);
    CK_PROPERTY_GET(_DebugPreviewAllProbes);
    CK_PROPERTY_GET(_DebugPreviewAllLineTraces);
    CK_PROPERTY_GET(_DebugPreviewAllProbesUsingJolt);
    CK_PROPERTY_GET(_ProbeLineTraceDebugDuration);
    CK_PROPERTY_GET(_DebugPreviewServerLineTraces);
    CK_PROPERTY_GET(_DebugPreviewClientLineTraces);
    CK_PROPERTY_GET(_ProbeLineTraceDebugThickness);
};

// --------------------------------------------------------------------------------------------------------------------

class CKSPATIALQUERY_API UCk_Utils_SpatialQuery_Settings
{
public:
    static auto
    Get_PreviewAllTraces() -> int32;

    static auto
    Get_DebugPreviewAllProbes() -> bool;

    static auto
    Get_DebugPreviewAllLineTraces() -> bool;

    static auto
    Get_DebugPreviewAllProbesUsingJolt() -> bool;

    static auto
    Get_ProbeLineTraceDebugDuration() -> float;

    static auto
    Get_DebugPreviewServerLineTraces() -> bool;

    static auto
    Get_DebugPreviewClientLineTraces() -> bool;

    static auto
    Get_ProbeLineTraceDebugThickness() -> float;
};

// --------------------------------------------------------------------------------------------------------------------
