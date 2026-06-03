#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/UserSettings/CkUserSettings.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkCrowd_DebugSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------
//
// Per-user debug visualization toggles for CkCrowd. Persisted to EditorPerProjectUserSettings.ini
// (via Config UPROPERTY) and shown in Editor Preferences → Crowd Debug.
//
// Each toggle is mirrored to a console variable (declared in the .cpp at file scope so the static
// initializer runs at DLL load — before any CDO construction). Sync is bidirectional:
//
//   - Editor Preferences edit  → PostEditChangeProperty pushes the new value to the CVar.
//   - Console `ck.Crowd.X 1`   → CVar callback writes the UPROPERTY back to the CDO + SaveConfig.
//
// CVar names match the historical declarations so existing console muscle memory keeps working
// (`ck.Crowd.Debug`, `ck.Crowd.DrawBreadcrumbs`, `ck.Crowd.DrawPlannedPaths`, plus the new
// `ck.Crowd.Debug.AgentBody`). The CrowdDebugger toolbar checkboxes resolve the same CVars via
// IConsoleManager::FindConsoleVariable, so flipping a checkbox flows through the CVar callback
// into the settings — checkbox state and persisted state stay aligned.
//
// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Crowd Debug"))
class CKCROWD_API UCk_Crowd_DebugSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Crowd_DebugSettings_UE);

    // PostInitProperties pushes the persisted UPROPERTY values into the static CVars on
    // CDO construction (load-time hydration). Without this, a freshly-launched editor would
    // keep the CVar default and ignore the previous session's saved value.
    virtual void PostInitProperties() override;

#if WITH_EDITOR
    // PostEditChangeProperty fires when the user edits a value in Editor Preferences. We push
    // the new value into the CVar so processors that read via the CVar (and the debugger
    // checkboxes that resolve through IConsoleManager) see the change immediately.
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    UPROPERTY(Config, EditAnywhere, Category = "Visualization",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "Draw a body capsule + forward-facing cone for every crowd agent. Color comes from the agent's debug color (per-agent override or hash-derived stable fallback). PathPending agents tint yellow; Asleep agents desaturate."))
    bool _DrawAgentBody = false;

    UPROPERTY(Config, EditAnywhere, Category = "Visualization",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "Draw separation diagnostics for every awake crowd agent: yellow separation-radius circle on the floor, cyan lines to each neighbor in the steering cache, orange arrow showing the active separation force."))
    bool _DrawSeparation = false;

    UPROPERTY(Config, EditAnywhere, Category = "Visualization",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "Draw a breadcrumb trail (the agent's actually-traversed path) for every agent that has the recorder feature. The selected agent is always drawn regardless of this toggle."))
    bool _DrawBreadcrumbs = false;

    UPROPERTY(Config, EditAnywhere, Category = "Visualization",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "Draw the planned-path waypoints for every agent that has a path result. The selected agent is always drawn regardless of this toggle."))
    bool _DrawPlannedPaths = false;

    UPROPERTY(Config, EditAnywhere, Category = "Visualization",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "Draw a navmesh-projection marker (green/red circle on the floor) under every crowd agent each tick. Off by default — at scale this is the most expensive Crowd debug viz because it runs a synchronous ProjectPointToNavigation per agent every frame."))
    bool _DrawNavProjection = false;

    UPROPERTY(Config, EditAnywhere, Category = "Visualization",
              meta = (AllowPrivateAccess = true,
                      ToolTip = "Draw the orbit-diagnosis rings for every crowd agent: arrival ring (green) + predicted-orbit ring (red, = MaxSpeed/MaxTurnRate) at the goal, the turn-radius circle (blue, = speed/MaxTurnRate) tangent to the agent, and the velocity vector (yellow). The selected agent is always drawn regardless of this toggle."))
    bool _DrawAgentRings = false;

public:
    CK_PROPERTY(_DrawAgentBody);
    CK_PROPERTY(_DrawSeparation);
    CK_PROPERTY(_DrawBreadcrumbs);
    CK_PROPERTY(_DrawPlannedPaths);
    CK_PROPERTY(_DrawNavProjection);
    CK_PROPERTY(_DrawAgentRings);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCROWD_API UCk_Utils_Crowd_DebugSettings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Crowd_DebugSettings_UE);

public:
    // Read paths — used by the per-tick draw processors. Reads the UPROPERTY (which is kept
    // in sync with the CVar via the callback in the .cpp).
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|DebugSettings")
    static bool
    Get_DrawAgentBody();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|DebugSettings")
    static bool
    Get_DrawSeparation();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|DebugSettings")
    static bool
    Get_DrawBreadcrumbs();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|DebugSettings")
    static bool
    Get_DrawPlannedPaths();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|DebugSettings")
    static bool
    Get_DrawNavProjection();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|DebugSettings")
    static bool
    Get_DrawAgentRings();
};

// --------------------------------------------------------------------------------------------------------------------
