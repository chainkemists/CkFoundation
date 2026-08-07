#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkUsf_OutlineSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkUsf_OutlinePreset;
class UMaterialInstanceDynamic;
class UTexture2D;
class UPostProcessComponent;
class UPrimitiveComponent;

// --------------------------------------------------------------------------------------------------------------------

// Per-world manager for CkUsf solid-color outlines: owns the SolidOutline post-process blendable on the
// local view, refcounted Custom-Stencil allocation (one value per ACTIVE preset, within a configurable
// range), and the params LUT. Project requirement: r.CustomDepth=3 (Custom Depth-Stencil WITH stencil).
UCLASS(NotBlueprintable, BlueprintType, DisplayName = "CkSubsystem_Usf_Outline")
class CKUSF_API UCkUsf_OutlineSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkUsf_OutlineSubsystem);

public:
    auto ShouldCreateSubsystem(UObject* InOuter) const -> bool override;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Get Outline Subsystem",
              meta = (WorldContext = "InWorldContextObject"))
    static UCkUsf_OutlineSubsystem*
    Get_OutlineSubsystem(
        const UObject* InWorldContextObject);

    // Enable an outline on every primitive component of InActor using InPreset.
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Apply Outline To Actor")
    void
    Apply_Outline_To_Actor(
        AActor* InActor,
        UCkUsf_OutlinePreset* InPreset);

    // Enable an outline on a single primitive component using InPreset.
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Apply Outline To Component")
    void
    Apply_Outline_To_Component(
        UPrimitiveComponent* InComponent,
        UCkUsf_OutlinePreset* InPreset);

    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Remove Outline From Actor")
    void
    Remove_Outline_From_Actor(
        AActor* InActor);

    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Remove Outline From Component")
    void
    Remove_Outline_From_Component(
        UPrimitiveComponent* InComponent);

    // Global outline thickness (pixels) applied to all presets, scaled per-preset by _ThicknessScale.
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Set Global Outline Thickness")
    void
    Set_GlobalOutlineThickness(
        float InThickness);

    // ---- Stencil allocation (refcounted; shared with the other renderer modules) ----

    // Returns the Custom-Stencil value assigned to InPreset (allocating + registering its LUT row on first
    // use), and increments its refcount. Returns 0 if allocation failed (e.g. range exhausted).
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Get Or Allocate Stencil For Preset")
    uint8
    Get_OrAllocate_StencilFor(
        UCkUsf_OutlinePreset* InPreset);

    // Decrements InPreset's refcount; frees its stencil value + LUT row when it reaches zero.
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Release Stencil For Preset")
    void
    Release_StencilFor(
        UCkUsf_OutlinePreset* InPreset);

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Get Outline Stencil Min")
    uint8 Get_StencilMin() const { return _StencilMin; }

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Get Outline Stencil Max")
    uint8 Get_StencilMax() const { return _StencilMax; }

private:
    auto DoEnsure_ViewEffect() -> bool;
    auto DoWrite_PresetRow(int32 InSlot, const UCkUsf_OutlinePreset* InPreset) -> void;
    auto DoUpload_Lut() -> void;
    // Also releases each reaped component's stencil refcount.
    auto DoReap_DeadComponents() -> void;

private:
    static constexpr int32 kLutWidth = 16;   // max active presets (must match SolidOutline.ush CKUSF_OUTLINE_LUT_W)
    static constexpr int32 kLutHeight = 2;    // rows per preset column   (must match CKUSF_OUTLINE_LUT_H)
    static constexpr int32 kLutRow_Outline = 0; // row order must match SolidOutline.ush
    static constexpr int32 kLutRow_Fill = 1;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> _OutlineMID;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> _ParamsTex;

    UPROPERTY(Transient)
    TObjectPtr<UPostProcessComponent> _ViewPP;

    UPROPERTY(Transient)
    TObjectPtr<AActor> _ViewActor;

    float _GlobalThickness = 5.0f;
    uint8 _StencilMin = 240;
    uint8 _StencilMax = 255;

    struct FStencilSlot { uint8 Value = 0; int32 RefCount = 0; };
    TMap<TWeakObjectPtr<UCkUsf_OutlinePreset>, FStencilSlot> _ActivePresets;

    // The VALUE is recorded alongside the preset because the undo has to be able to tell "this component
    // still carries what I wrote" from "someone else has taken the byte over since". Without it the undo
    // is unconditional and blanks whichever feature claimed the component next — the cel pattern and the
    // effect mask both guard their undos this way, and all three must agree or the last one to write
    // loses its silhouette when an unrelated feature is removed.
    struct FAppliedOutline { TWeakObjectPtr<UCkUsf_OutlinePreset> Preset; int32 StencilValue = 0; };
    TMap<TWeakObjectPtr<UPrimitiveComponent>, FAppliedOutline> _AppliedComponents;

    TArray<FFloat16Color> _LutData;
};
