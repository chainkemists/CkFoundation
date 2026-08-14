#include "CkParametricImage_Widget.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkGraphics/CkGraphics_Utils.h"
#include "CkUI/Types/CkUI_Types.h"

#include <Materials/MaterialInterface.h>
#include <Materials/MaterialInstanceDynamic.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ParametricImageWidget_UE::
    Request_SetSourceMaterial(
        UMaterialInterface* InSourceMaterial)
    -> void
{
    _SourceMaterial = InSourceMaterial;

    // The brush must hold the non-dynamic source: it is what the UMG animator discovers material tracks
    // from, and what GetDynamicMaterial builds the per-widget MID on top of.
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
    const auto PropertyName = ck::IsValid(PropertyChangedEvent.Property)
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCk_ParametricImageWidget_UE, _SourceMaterial))
    {
        SetBrushResourceObject(_SourceMaterial);

        constexpr auto PreserveOverrides = false;
        DiscoverParameters(PreserveOverrides);
    }

    // Before the parent on purpose, so the designer preview reflects the edit immediately.
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

    // A throwaway instance is the only way to read parameter defaults through the public FName API —
    // the alternative, FHashedMaterialParameterInfo, lives in an engine-private header.
    auto* Defaults = UMaterialInstanceDynamic::Create(_SourceMaterial, GetTransientPackage());

    const auto AppendParameter = [&](FCk_Material_Parameter InDiscovered) -> void
    {
        if (InPreserveOverrides)
        {
            // Only overridden rows carry their value forward — the rest refresh to the material default.
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
    // Animated parameters are re-applied by the material system AFTER this runs, so writing our
    // non-animated overrides onto the same MID the animator drives layers cleanly rather than fighting.
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
