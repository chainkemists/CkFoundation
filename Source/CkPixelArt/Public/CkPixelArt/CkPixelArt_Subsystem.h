#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkPixelArt/CkPixelArt_Params.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkPixelArt_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkPixelArt_Preset;
class UMaterialInstanceDynamic;
class UPostProcessComponent;

// --------------------------------------------------------------------------------------------------------------------

// Per-world owner of the pixel-art configuration: the one thing game code, a preset, or a project default talks
// to in order to render pixelated.
//
// It never touches the scene view extension. The only channel is CkPixelArtRender's per-world state registry —
// which is what keeps the render module consumable by a project that does not use this one, and keeps a
// PostConfigInit module from needing to know about a Default-phase module that may not exist.
//
// Enabling is GATED, not forced. Several engine settings quietly make a pixel-art image impossible (a temporal
// anti-aliasing method takes over the upscale slot entirely; dynamic resolution fights the screen percentage the
// renderer drives). `Request_SetEnabled` refuses in that state and records a report naming each setting, its
// current value, and the console command that fixes it. `Request_Apply_RecommendedCVars` is the explicit,
// opt-in escape — the renderer never silently moves a project's rendering settings out from under it.
UCLASS(NotBlueprintable, BlueprintType, DisplayName = "CkSubsystem_PixelArt")
class CKPIXELART_API UCkPixelArt_Subsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkPixelArt_Subsystem);

public:
    auto ShouldCreateSubsystem(UObject* InOuter) const -> bool override;
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;
    auto OnWorldBeginPlay(UWorld& InWorld) -> void override;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|PixelArt",
              DisplayName = "[Ck][PixelArt] Get Pixel Art Subsystem",
              meta = (WorldContext = "InWorldContextObject"))
    static UCkPixelArt_Subsystem*
    Get_PixelArtSubsystem(
        const UObject* InWorldContextObject);

    /**
     * Turns the renderer on or off for this world.
     *
     * Enabling with any precondition unsatisfied does NOT enable: it ensures with the full report, leaves the
     * renderer off, and leaves Get_PreconditionReport populated. There is no half-enabled state — a caller can
     * read Get_IsEnabled and get the truth.
     *
     * Disabling always succeeds, clears the world's renderer configuration, and restores any console variables
     * Request_Apply_RecommendedCVars moved.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|PixelArt",
              DisplayName = "[Ck][PixelArt] Request Set Enabled")
    void
    Request_SetEnabled(
        ECk_EnableDisable InEnabled);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PixelArt",
              DisplayName = "[Ck][PixelArt] Get Is Enabled")
    ECk_EnableDisable
    Get_IsEnabled() const;

    // Replaces the whole settings value with the preset's. A null preset is rejected loudly and changes nothing
    // at all — no partial application.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|PixelArt",
              DisplayName = "[Ck][PixelArt] Apply Preset")
    void
    Apply_Preset(
        const UCkPixelArt_Preset* InPreset);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|PixelArt",
              DisplayName = "[Ck][PixelArt] Request Set Settings")
    void
    Request_SetSettings(
        const FCk_PixelArt_Params& InParams);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PixelArt",
              DisplayName = "[Ck][PixelArt] Get Settings")
    FCk_PixelArt_Params
    Get_Settings() const;

    // Back to the project default preset if there is one, otherwise to the params struct's own defaults.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|PixelArt",
              DisplayName = "[Ck][PixelArt] Request Reset To Defaults")
    void
    Request_ResetToDefaults();

    // Empty when the renderer can run.
    //
    // Recomputed every call, never cached — including right after a refusal. A cached refusal would keep
    // reporting the problem after the caller had fixed it, which is exactly when someone is looking at this and
    // trusting it.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|PixelArt",
              DisplayName = "[Ck][PixelArt] Get Precondition Report")
    FCk_PixelArt_PreconditionReport
    Get_PreconditionReport() const;

    /**
     * Moves the console variables the preconditions require, remembering their prior values so disabling puts
     * them back.
     *
     * This is the ONLY thing in the module that writes a project's rendering settings, and it exists precisely
     * so that the enable path never does it silently. Every change logs at Display level with old -> new.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|PixelArt",
              DisplayName = "[Ck][PixelArt] Request Apply Recommended CVars")
    void
    Request_Apply_RecommendedCVars();

private:
    auto DoGet_PreconditionReport() const -> FCk_PixelArt_PreconditionReport;
    auto DoSync_RenderConfig() -> void;

    auto DoEnsure_LookEffect() -> bool;
    auto DoSync_LookEffect() -> void;
    auto DoWrite_ChangedLookParams(const FCk_PixelArt_LookParams& InLook) -> void;

    auto DoRestore_RecommendedCVars() -> void;

    auto DoResolve_ProjectDefaultPreset() const -> UCkPixelArt_Preset*;
    auto DoApply_ProjectDefault() -> void;
    auto DoApply_ProjectDefault_Now() -> void;
    auto DoOn_CVarChanged() -> void;

private:
    // The look's blendable rides a hidden transient actor carrying an unbound post-process component, per the
    // Stylize subsystems. Lazily created: a world that never enables the look never loads the master, never
    // builds a MID and never spawns an actor.
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> _LookMID;

    UPROPERTY(Transient)
    TObjectPtr<UPostProcessComponent> _LookPP;

    UPROPERTY(Transient)
    TObjectPtr<AActor> _LookActor;

    // What the MID currently holds, so a change writes only the parameters that moved. Unset until the first
    // write, which is therefore a full write.
    TOptional<FCk_PixelArt_LookParams> _WrittenLook;

    // The missing-master warning is worth exactly one line per world, not one per settings change.
    bool _WarnedMissingMaster = false;

    FCk_PixelArt_Params _Settings;

    ECk_EnableDisable _Enabled = ECk_EnableDisable::Disable;

    // Console values Request_Apply_RecommendedCVars moved, keyed by CVar name. Present only while this subsystem
    // is the one holding them changed — disabling restores and clears, so a second apply cannot stack.
    TMap<FString, float> _CVarPriors;

    FDelegateHandle _CVarChangedHandle;

    // Set by any explicit Request_SetSettings / Apply_Preset / Request_SetEnabled. The project default is a
    // DEFAULT, so it must never overwrite a value game code already chose.
    bool _SettingsExplicitlySet = false;
};
