// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI_LayerConfigAsset.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Layer/CkUI_LayerStack.h"

#if WITH_EDITOR
#include <Misc/DataValidation.h>
#endif

// --------------------------------------------------------------------------------------------------------------------
// Access
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerConfigAsset_UE::
    Get_LayerConfig(
        FGameplayTag InLayerTag) const
    -> TOptional<FCk_UI_LayerConfig>
{
    return ck::algo::FindIf(_LayerConfigs, [&InLayerTag](const FCk_UI_LayerConfig& InConfig)
    {
        return InConfig.Get_LayerTag() == InLayerTag;
    });
}

// --------------------------------------------------------------------------------------------------------------------
// UPrimaryDataAsset Overrides
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_LayerConfigAsset_UE::
    GetPrimaryAssetId() const
    -> FPrimaryAssetId
{
    return FPrimaryAssetId(TEXT("CkUI_LayerConfig"), GetFName());
}

#if WITH_EDITOR
auto
    UCk_UI_LayerConfigAsset_UE::
    IsDataValid(
        FDataValidationContext& Context) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(Context);

    TSet<FGameplayTag> SeenLayerTags;

    for (auto ConfigIndex = 0; ConfigIndex < _LayerConfigs.Num(); ++ConfigIndex)
    {
        const auto& Config = _LayerConfigs[ConfigIndex];
        const auto ConfigDisplayName = ck::Format_UE(TEXT("Layer Config [{}]"), ConfigIndex);

        if (ck::Is_NOT_Valid(Config.Get_LayerTag()))
        {
            Context.AddError(FText::FromString(
                ck::Format_UE(TEXT("{}: Layer tag is invalid or empty"), ConfigDisplayName)));
            Result = EDataValidationResult::Invalid;
            continue;
        }

        const auto& LayerTag = Config.Get_LayerTag();

        if (SeenLayerTags.Contains(LayerTag))
        {
            Context.AddError(FText::FromString(
                ck::Format_UE(TEXT("{}: Duplicate layer tag [{}]"), ConfigDisplayName, LayerTag.ToString())));
            Result = EDataValidationResult::Invalid;
        }
        else
        {
            SeenLayerTags.Add(LayerTag);
        }

        const auto& LayerWidgetClass = Config.Get_LayerWidgetClass();

        if (ck::IsValid(LayerWidgetClass))
        {
            if (NOT LayerWidgetClass->IsChildOf(UCk_UI_LayerStack_UE::StaticClass()))
            {
                Context.AddError(FText::FromString(
                    ck::Format_UE(TEXT("{}: Layer widget class [{}] must derive from UCk_UI_LayerStack_UE"),
                        ConfigDisplayName, LayerWidgetClass->GetName())));
                Result = EDataValidationResult::Invalid;
            }
        }

        for (auto WidgetIndex = 0; WidgetIndex < Config.Get_StartingWidgetClasses().Num(); ++WidgetIndex)
        {
            const auto& StartingWidgetClass = Config.Get_StartingWidgetClasses()[WidgetIndex];

            if (StartingWidgetClass.IsNull())
            {
                Context.AddError(FText::FromString(
                    ck::Format_UE(TEXT("{}: Starting widget [{}] is null"),
                        ConfigDisplayName, WidgetIndex)));
                Result = EDataValidationResult::Invalid;
            }
        }
    }

    return Result;
}
#endif

// --------------------------------------------------------------------------------------------------------------------