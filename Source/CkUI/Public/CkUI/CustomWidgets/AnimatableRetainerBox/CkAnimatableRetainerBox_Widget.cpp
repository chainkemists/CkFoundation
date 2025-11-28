#include "CkAnimatableRetainerBox_Widget.h"

#include "Slate/SRetainerWidget.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AnimatableRetainerBox::
    SynchronizeProperties()
    -> void
{
    Super::SynchronizeProperties();

    // Sync the brush material to the effect material
    if (auto* BrushMaterial = Cast<UMaterialInterface>(Brush.GetResourceObject());
        ck::IsValid(BrushMaterial))
    {
        // Only update if different to avoid unnecessary invalidation
        if (GetEffectMaterial() != BrushMaterial)
        {
            SetEffectMaterial(BrushMaterial);
        }
    }
    else if (ck::Is_NOT_Valid(Brush.GetResourceObject()) && ck::IsValid(GetEffectMaterial()))
    {
        // Clear effect material if brush has no material
        SetEffectMaterial(nullptr);
    }

    // Handle designer preview
    if (ck::IsValid(MyRetainerWidget))
    {
        PRAGMA_DISABLE_DEPRECATION_WARNINGS
        MyRetainerWidget->SetRetainedRendering(IsDesignTime() ? ShowEffectsInDesigner : IsRetainRendering());
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

    // Called at the end because we want to handle SynchronizeProperties before our parent
    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

// --------------------------------------------------------------------------------------------------------------------
