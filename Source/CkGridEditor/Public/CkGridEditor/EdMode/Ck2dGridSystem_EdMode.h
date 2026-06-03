#pragma once

#include "CoreMinimal.h"

#include "Tools/LegacyEdModeWidgetHelpers.h"

#include "Ck2dGridSystem_EdMode.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ACk_EntitySpawner_UE;
class UCk_2dGridSystem_Spec;

class FEditorViewportClient;
class FPrimitiveDrawInterface;
class FSceneView;
class FViewport;
class HHitProxy;
struct FViewportClick;

// --------------------------------------------------------------------------------------------------------------------

// "Grid Paint" editor mode. Renders the AUTHORED state of the selected grid spawner's Spec as a
// wireframe overlay and (in a later task) lets the user paint cell state by clicking the viewport.
//
// Subclasses UBaseLegacyWidgetEdMode rather than UEdMode directly: the legacy-widget base exposes
// overridable Render/HandleClick/InputDelta (via ILegacyEdModeWidgetInterface +
// ILegacyEdModeViewportInterface), which is what a draw/click-based mode needs — plain UEdMode only
// forwards input to its InteractiveTools context.
//
// Registration is AUTO-DISCOVERY: UAssetEditorSubsystem::RegisterEditorModes() iterates every
// non-abstract UEdMode CDO on OnAllModuleLoadingPhasesComplete and registers it by its
// FEditorModeInfo::ID. No explicit RegisterMode() call is required; we only need a stable ID and a
// visible FEditorModeInfo assigned in the constructor.
UCLASS()
class CKGRIDEDITOR_API UCk_2dGridSystem_EdMode : public UBaseLegacyWidgetEdMode
{
    GENERATED_BODY()

public:
    // Stable identifier for this editor mode. Used by the toolbar and by auto-discovery.
    static const FEditorModeID EM_Ck2dGridSystemPaintModeId;

    UCk_2dGridSystem_EdMode();

    // UEdMode interface
    virtual void CreateToolkit() override;

    // ILegacyEdModeWidgetInterface (via UBaseLegacyWidgetEdMode)
    virtual void Render(const FSceneView* InView, FViewport* InViewport, FPrimitiveDrawInterface* InPDI) override;
    virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override;

    // ILegacyEdModeViewportInterface (via UBaseLegacyWidgetEdMode)
    virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* InHitProxy, const FViewportClick& InClick) override;
    virtual bool MouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;

private:
    // Resolved grid selection: the selected spawner whose _EntityScript is a grid script, plus the
    // Spec it hosts and the spawner's world transform. Invalid when the current selection is not a
    // grid spawner.
    struct FResolvedGridSelection
    {
        ACk_EntitySpawner_UE*       Spawner = nullptr;
        const UCk_2dGridSystem_Spec* Spec    = nullptr;
        FTransform                  GridTransform = FTransform::Identity;

        auto IsValid() const -> bool { return Spawner != nullptr && Spec != nullptr; }
    };

    // Walks the mode's selected actors and returns the first grid spawner selection (or an invalid
    // result if none is selected).
    auto Resolve_SelectedGridSpawner() const -> FResolvedGridSelection;
};

// --------------------------------------------------------------------------------------------------------------------
