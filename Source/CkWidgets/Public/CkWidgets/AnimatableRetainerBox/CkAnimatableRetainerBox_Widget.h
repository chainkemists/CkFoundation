#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Components/RetainerBox.h>
#include <CoreMinimal.h>

#include "CkAnimatableRetainerBox_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKWIDGETS_API UCk_AnimatableRetainerBox : public URetainerBox
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_AnimatableRetainerBox);

private:
    UPROPERTY(EditAnywhere, Category = "Appearance", meta = (AllowPrivateAccess = true))
    FSlateBrush Brush;

    // Runtime-available designer-preview toggle; the parent's URetainerBox::bShowEffectsInDesigner is
    // editor-only. The name must NOT match the parent's — see CkUI/CLAUDE.md (AngelScript binder collision).
    UPROPERTY(EditAnywhere, Category = "Effect", meta = (AllowPrivateAccess = true))
    bool ShowEffectsPreview = true;

protected:
    auto SynchronizeProperties() -> void override;

#if WITH_EDITOR
    auto PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent) -> void override;

    auto GetPaletteCategory() -> const FText override;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
