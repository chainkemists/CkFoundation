#include "CkParametricImage_Widget.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkGraphics/CkGraphics_Utils.h"
#include "CkUI/Types/CkUI_Types.h"

#include <Materials/MaterialInterface.h>
#include <Materials/MaterialInstanceDynamic.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ParametricImageWidget_UE::
    Set_SourceMaterial(
        UMaterialInterface* InSourceMaterial)
    -> void
{
    _SourceMaterial = InSourceMaterial;

    // Seed the (non-dynamic) source into the inherited brush so the UMG animator can discover material
    // tracks and GetDynamicMaterial has a parent to build the per-widget MID from.
    SetBrushResourceObject(_SourceMaterial);

    constexpr auto PreserveOverrides = true;
    DiscoverParameters(PreserveOverrides);
    SynchronizeProperties();
}

auto
    UCk_ParametricImageWidget_UE::
    Get_SourceMaterial() const
    -> UMaterialInterface*
{
    return _SourceMaterial;
}

auto
    UCk_ParametricImageWidget_UE::
    Update_FromMaterial()
    -> void
{
    SetBrushResourceObject(_SourceMaterial);

    constexpr auto PreserveOverrides = true;
    DiscoverParameters(PreserveOverrides);
    SynchronizeProperties();
}

auto
    UCk_ParametricImageWidget_UE::
    Reset_ToMaterialDefaults()
    -> void
{
    SetBrushResourceObject(_SourceMaterial);

    constexpr auto PreserveOverrides = false;
    DiscoverParameters(PreserveOverrides);
    SynchronizeProperties();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ParametricImageWidget_UE::
    SynchronizeProperties()
    -> void
{
    Super::SynchronizeProperties();

    EnsureBrushHasMaterial();
    ApplyParametersToDynamicMaterial();
}

#if WITH_EDITOR
auto
    UCk_ParametricImageWidget_UE::
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    const auto PropertyName = ck::IsValid(PropertyChangedEvent.Property, ck::IsValid_Policy_NullptrOnly{})
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCk_ParametricImageWidget_UE, _SourceMaterial))
    {
        // Material swapped — re-seed the brush and refresh the exposed parameter list from scratch.
        SetBrushResourceObject(_SourceMaterial);

        constexpr auto PreserveOverrides = false;
        DiscoverParameters(PreserveOverrides);
    }

    // Apply before our parent so the designer preview reflects the edit immediately.
    SynchronizeProperties();

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

auto
    UCk_ParametricImageWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ParametricImageWidget_UE::
    DiscoverParameters(
        bool InPreserveOverrides)
    -> void
{
    const auto PreviousParameters = _Parameters;
    _Parameters.Reset();

    if (ck::Is_NOT_Valid(_SourceMaterial))
    { return; }

    // A throwaway instance lets us read each parameter's default through the public FName API without
    // touching FHashedMaterialParameterInfo (which lives in an engine-private header).
    auto* Defaults = UMaterialInstanceDynamic::Create(_SourceMaterial, GetTransientPackage());

    const auto AppendParameter = [&](FCk_Material_Parameter InDiscovered) -> void
    {
        if (InPreserveOverrides)
        {
            // Only overridden rows carry their value forward; non-overridden rows refresh to the material
            // default we just captured (so a changed material default is picked up).
            if (const auto* Existing = PreviousParameters.FindByPredicate(
                    [&](const FCk_Material_Parameter& In)
                    {
                        return In.Get_ParameterName() == InDiscovered.Get_ParameterName()
                            && In.Get_Type() == InDiscovered.Get_Type();
                    });
                ck::IsValid(Existing, ck::IsValid_Policy_NullptrOnly{}) && Existing->Get_Override())
            {
                InDiscovered.Set_Override(true);
                InDiscovered.Set_ScalarValue(Existing->Get_ScalarValue());
                InDiscovered.Set_ColorValue(Existing->Get_ColorValue());
                InDiscovered.Set_TextureValue(Existing->Get_TextureValue());
            }
        }

        _Parameters.Add(InDiscovered);
    };

    // ---- Scalars ----
    {
        auto Infos = TArray<FMaterialParameterInfo>{};
        auto Ids   = TArray<FGuid>{};
        _SourceMaterial->GetAllScalarParameterInfo(Infos, Ids);

        for (const auto& Info : Infos)
        {
            const auto DefaultValue = ck::IsValid(Defaults) ? Defaults->K2_GetScalarParameterValue(Info.Name) : 0.0f;
            AppendParameter(FCk_Material_Parameter{Info.Name, DefaultValue});
        }
    }

    // ---- Colors (vector parameters) ----
    {
        auto Infos = TArray<FMaterialParameterInfo>{};
        auto Ids   = TArray<FGuid>{};
        _SourceMaterial->GetAllVectorParameterInfo(Infos, Ids);

        for (const auto& Info : Infos)
        {
            const auto DefaultValue = ck::IsValid(Defaults) ? Defaults->K2_GetVectorParameterValue(Info.Name) : FLinearColor::White;
            AppendParameter(FCk_Material_Parameter{Info.Name, DefaultValue});
        }
    }

    // ---- Textures ----
    {
        auto Infos = TArray<FMaterialParameterInfo>{};
        auto Ids   = TArray<FGuid>{};
        _SourceMaterial->GetAllTextureParameterInfo(Infos, Ids);

        for (const auto& Info : Infos)
        {
            auto* DefaultValue = ck::IsValid(Defaults) ? Defaults->K2_GetTextureParameterValue(Info.Name) : nullptr;
            AppendParameter(FCk_Material_Parameter{Info.Name, DefaultValue});
        }
    }
}

auto
    UCk_ParametricImageWidget_UE::
    EnsureBrushHasMaterial()
    -> void
{
    if (const auto* CurrentMaterial = Cast<UMaterialInterface>(GetBrush().GetResourceObject());
        ck::IsValid(CurrentMaterial))
    { return; }

    if (ck::Is_NOT_Valid(_SourceMaterial))
    { return; }

    SetBrushResourceObject(_SourceMaterial);
}

auto
    UCk_ParametricImageWidget_UE::
    ApplyParametersToDynamicMaterial()
    -> void
{
    // GetDynamicMaterial creates the per-widget MID from the brush material on first call and writes it
    // back into the brush; on later calls (and while the sequencer drives its own MID) it returns the
    // existing instance. Animated parameters are re-applied by the material system after this runs, so
    // our non-animated overrides and the animator's tracks layer cleanly.
    auto* DynamicMaterial = GetDynamicMaterial();
    if (ck::Is_NOT_Valid(DynamicMaterial))
    { return; }

    for (const auto& Parameter : _Parameters)
    {
        if (NOT Parameter.Get_Override())
        { continue; }

        UCk_Utils_Graphics_UE::Apply_MaterialParameter(DynamicMaterial, Parameter);
    }
}

// --------------------------------------------------------------------------------------------------------------------
