#include "CkAbilityEditor_Module.h"

#include "CkAbility/Ability/CkAbility_Script.h"

#include "CkEditorStyle/CkEditorStyle_Utils.h"

#define LOCTEXT_NAMESPACE "FCkAbilityEditorModule"

auto
    FCkAbilityEditorModule::
    StartupModule()
    -> void
{
    // AbilityScript
    const auto& AbilityScript_Name = UCk_Ability_Script_PDA::StaticClass()->GetFName();

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetIcon, AbilityScript_Name}
        .Set_IconFileName(TEXT("Icon_AbilityScript_16px"))
        .Set_IconBrushType(ECk_CustomIconBrushType::PNG)
        .Set_IconSize(ECk_IconSize::IconSize_16x16));

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetThumbnail, AbilityScript_Name}
        .Set_IconFileName(TEXT("Icon_AbilityScript_64px"))
        .Set_IconBrushType(ECk_CustomIconBrushType::PNG)
        .Set_IconSize(ECk_IconSize::IconSize_64x64));
}

auto
    FCkAbilityEditorModule::
    ShutdownModule()
    -> void
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkAbilityEditorModule, CkAbilityEditor)
