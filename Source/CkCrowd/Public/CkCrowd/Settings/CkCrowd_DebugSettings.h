#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/UserSettings/CkUserSettings.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkCrowd_DebugSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Per-user debug visualization toggles for CkCrowd: persisted to EditorPerProjectUserSettings.ini and
// shown in Editor Preferences -> Crowd Debug. Each toggle is mirrored BIDIRECTIONALLY to a console
// variable declared at file scope in the .cpp (Editor edit -> CVar; console set -> UPROPERTY + save).
// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Crowd Debug"))
class CKCROWD_API UCk_Crowd_DebugSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Crowd_DebugSettings_UE);

    // Hydrates the CVars from the persisted UPROPERTY values at CDO construction; without it a
    // freshly-launched editor keeps the CVar defaults and ignores the previous session's values.
    virtual void PostInitProperties() override;

#if WITH_EDITOR
    // Pushes an Editor Preferences edit into the CVar so CVar readers (draw processors, debugger
    // checkboxes resolving through IConsoleManager) see the change immediately.
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
    // Read paths for the per-tick draw processors — the UPROPERTY, kept in sync with the CVar.
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
