#include "CkAnimatableRetainerBox_Widget.h"

#include "CkUI/Types/CkUI_Types.h"

#include "Slate/SRetainerWidget.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AnimatableRetainerBox::
    SynchronizeProperties()
    -> void
{
    Super::SynchronizeProperties();

    if (auto* BrushMaterial = Cast<UMaterialInterface>(Brush.GetResourceObject());
        ck::IsValid(BrushMaterial))
    {
        if (GetEffectMaterial() != BrushMaterial)
        {
            SetEffectMaterial(BrushMaterial);
        }
    }
    else if (ck::Is_NOT_Valid(Brush.GetResourceObject()) && ck::IsValid(GetEffectMaterial()))
    {
        SetEffectMaterial(nullptr);
    }

    if (ck::IsValid(MyRetainerWidget))
    {
        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        MyRetainerWidget->SetRetainedRendering(IsDesignTime() ? bShowEffectsPreview : IsRetainRendering());
        PRAGMA_ENABLE_DEPRECATION_WARNINGS
    }
}

#if WITH_EDITOR
auto
    UCk_AnimatableRetainerBox::
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    if (PropertyChangedEvent.Property)
    {
        const FName PropertyName = PropertyChangedEvent.Property->GetFName();

        Brush.SetResourceObject(GetEffectMaterial());

        if (ck::IsValid(MyRetainerWidget))
        {
            SynchronizeProperties();
        }

        if (PropertyName == GET_MEMBER_NAME_CHECKED(UCk_AnimatableRetainerBox, Brush))
        {
            if (UMaterialInterface* BrushMaterial = Cast<UMaterialInterface>(Brush.GetResourceObject()))
            {
                SetEffectMaterial(BrushMaterial);
            }
        }
    }

    // Last on purpose: our SynchronizeProperties must run before the parent's handling.
    Super::PostEditChangeProperty(PropertyChangedEvent);
}

auto
    UCk_AnimatableRetainerBox::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

// --------------------------------------------------------------------------------------------------------------------
