#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Components/RetainerBox.h>
#include <CoreMinimal.h>

#include "CkAnimatableRetainerBox_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUI_API UCk_AnimatableRetainerBox : public URetainerBox
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_AnimatableRetainerBox);

private:
    UPROPERTY(EditAnywhere, Category = "Appearance", meta = (AllowPrivateAccess))
    FSlateBrush Brush;

    // Runtime-available designer-preview toggle. NOTE: distinct name from the parent's
    // URetainerBox::bShowEffectsInDesigner ON PURPOSE — the parent's is editor-only
    // (WITH_EDITORONLY_DATA, unavailable in game builds), so we need our own. The previous
    // name "ShowEffectsInDesigner" collided with the parent's under AngelScript binding (the
    // binder strips the parent's `b`, so both produced Get/SetShowEffectsInDesigner ->
    // asALREADY_REGISTERED, failing the cook). A distinct name keeps both bindings unique.
    UPROPERTY(EditAnywhere, Category = "Effect", meta = (AllowPrivateAccess))
    bool bShowEffectsPreview = true;

protected:
    auto SynchronizeProperties() -> void override;

#if WITH_EDITOR
    auto PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent) -> void override;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
