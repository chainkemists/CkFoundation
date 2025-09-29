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

    UPROPERTY(EditAnywhere, Category = "Effect", meta = (AllowPrivateAccess))
    bool ShowEffectsInDesigner = true;

protected:
    auto SynchronizeProperties() -> void override;

#if WITH_EDITOR
    auto PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent) -> void override;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
