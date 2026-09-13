#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkUsf/Outline/CkUsf_Outline_Fragment.h"
#include "CkUsf/Outline/CkUsf_Outline_ProjectSettings.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkUsf_OutlineSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkUsf_OutlinePreset;
class UPrimitiveComponent;
namespace ck::usf { class FOutlineRenderer; }

// --------------------------------------------------------------------------------------------------------------------

// Per-world solid outlines: owns the native view extension, refcounted Custom-Stencil allocation
// (one value per active preset), and CPU color rows copied into immutable render snapshots.
// Project requirement: r.CustomDepth=3 (Custom Depth-Stencil WITH stencil).
UCLASS(NotBlueprintable, BlueprintType, DisplayName = "CkSubsystem_Usf_Outline")
class CKUSF_API UCkUsf_OutlineSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkUsf_OutlineSubsystem);

public:
    auto ShouldCreateSubsystem(UObject* InOuter) const -> bool override;
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Get Outline Subsystem",
              meta = (WorldContext = "InWorldContextObject"))
    static UCkUsf_OutlineSubsystem*
    Get_OutlineSubsystem(
        const UObject* InWorldContextObject);

    // Register one resolved ECS entity as an owner of the physical component's outline. Multiple entities
    // may legitimately resolve to the same primitive (for example an actor entity and one of its managed
    // component dependents); the subsystem arbitrates those owners using the same deterministic ordering as
    // semantic outline claims and reveals the next owner when the winner clears.
    void
    Set_ResolvedOutline(
        UPrimitiveComponent* InComponent,
        const FCk_Handle& InRenderOwner,
        const ck::FFragment_Usf_OutlineResolved& InResolved);

    void
    Clear_ResolvedOutline(
        UPrimitiveComponent* InComponent,
        const FCk_Handle& InRenderOwner);

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Get Current Outline Preset")
    UCkUsf_OutlinePreset*
    Get_CurrentOutlinePreset(
        UPrimitiveComponent* InComponent) const;

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Get Outline Owner Count")
    int32
    Get_OutlineOwnerCount(
        UPrimitiveComponent* InComponent) const;

    // Invalid settings are rejected atomically; world-space centimeters are the default.
    UFUNCTION(BlueprintCallable, Category = "Ck|Usf|Outline",
              DisplayName = "[Ck][Usf] Try Set Outline Thickness Settings")
    bool TrySet_ThicknessSettings(const FCk_Usf_OutlineThicknessSettings& InSettings);

    UFUNCTION(BlueprintPure, Category = "Ck|Usf|Outline")
    FCk_Usf_OutlineThicknessSettings Get_ThicknessSettings() const { return _ThicknessSettings; }

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
    struct FResolvedOwner
    {
        FCk_Handle RenderOwner;
        ck::FFragment_Usf_OutlineResolved Resolved;
    };

    auto DoFind_WinningResolvedOwner(const TArray<FResolvedOwner>& InOwners) const -> const FResolvedOwner*;
    auto DoReconcile_ResolvedOutline(UPrimitiveComponent* InComponent) -> void;
    auto DoApply_PhysicalOutline(UPrimitiveComponent* InComponent, UCkUsf_OutlinePreset* InPreset) -> void;
    auto DoRemove_PhysicalOutline(UPrimitiveComponent* InComponent) -> void;
    auto DoEnsure_ViewEffect() -> bool;
    auto DoWrite_PresetRow(int32 InSlot, const UCkUsf_OutlinePreset* InPreset) -> void;
    auto DoUpload_Lut() -> void;
    // Also releases each reaped component's stencil refcount.
    auto DoReap_DeadComponents() -> void;

private:
    static constexpr int32 kLutWidth = 16;   // max active presets; matches FOutlineRenderState arrays
    static constexpr int32 kLutHeight = 2;   // CPU outline/fill rows (not a GPU texture)
    static constexpr int32 kLutRow_Outline = 0;
    static constexpr int32 kLutRow_Fill = 1;

    TSharedPtr<ck::usf::FOutlineRenderer, ESPMode::ThreadSafe> _Renderer;
    FCk_Usf_OutlineThicknessSettings _ThicknessSettings;
    bool _ThicknessSettingsAreValid = false;
    uint8 _StencilMin = 240;
    uint8 _StencilMax = 255;

    struct FStencilSlot { uint8 Value = 0; int32 RefCount = 0; };
    TMap<TWeakObjectPtr<UCkUsf_OutlinePreset>, FStencilSlot> _ActivePresets;

    // The VALUE is recorded alongside the preset because the undo has to be able to tell "this component
    // still carries what I wrote" from "someone else has taken the byte over since". Without it the undo
    // is unconditional and blanks whichever feature claimed the component next — the cel pattern and the
    // effect mask both guard their undos this way, and all three must agree or the last one to write
    // loses its silhouette when an unrelated feature is removed.
    struct FAppliedOutline
    {
        TWeakObjectPtr<UCkUsf_OutlinePreset> Preset;
        int32 StencilValue = 0;
        bool PreviousRenderCustomDepth = false;
        int32 PreviousStencilValue = 0;
    };
    TMap<TWeakObjectPtr<UPrimitiveComponent>, FAppliedOutline> _AppliedComponents;
    TMap<TWeakObjectPtr<UPrimitiveComponent>, TArray<FResolvedOwner>> _ResolvedOwners;

    TArray<FFloat16Color> _LutData;
};
