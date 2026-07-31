#pragma once

#include <CoreMinimal.h>
#include <Tools/LegacyEdModeWidgetHelpers.h>

#include "CkPathNetwork_DesignerEdMode.generated.h"

class FPrimitiveDrawInterface;
class FSceneView;
class FViewport;
class UCk_PathNetworkDesigner_Session_UE;

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPATHNETWORKEDITOR_API UCk_PathNetworkDesigner_EdMode : public UBaseLegacyWidgetEdMode
{
    GENERATED_BODY()

public:
    static const FEditorModeID EM_CkPathNetworkDesignerModeId;

    UCk_PathNetworkDesigner_EdMode();

    auto
    Enter() -> void override;

    auto
    Exit() -> void override;

    auto
    CreateToolkit() -> void override;

    auto
    Render(
        const FSceneView* InView,
        FViewport* InViewport,
        FPrimitiveDrawInterface* InPDI) -> void override;

    auto
    Get_Session() const -> UCk_PathNetworkDesigner_Session_UE*;

private:
    auto
    GetOrCreate_Session() -> UCk_PathNetworkDesigner_Session_UE*;

    auto
    OnMapChanged(
        UWorld* InWorld,
        EMapChangeType InMapChangeType) -> void;

private:
    UPROPERTY(Transient)
    TObjectPtr<UCk_PathNetworkDesigner_Session_UE> _Session;

    FDelegateHandle _MapChangedHandle;
};

// --------------------------------------------------------------------------------------------------------------------
