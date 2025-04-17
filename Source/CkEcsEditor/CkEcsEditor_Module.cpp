#include "CkEcsEditor_Module.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkEditorStyle/CkEditorStyle_Utils.h"

#define LOCTEXT_NAMESPACE "FCkEcsEditorModule"

auto
    FCkEcsEditorModule::
    StartupModule()
    -> void
{
    // EntityScript
    const auto& EntityScript_Name = UCk_EntityScript_UE::StaticClass()->GetFName();

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetIcon, EntityScript_Name}
        .Set_IconFileName(TEXT("Icon_EntityScript_16px"))
        .Set_IconBrushType(ECk_CustomIconBrushType::PNG)
        .Set_IconSize(ECk_IconSize::IconSize_16x16));

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetThumbnail, EntityScript_Name}
        .Set_IconFileName(TEXT("Icon_EntityScript_64px"))
        .Set_IconBrushType(ECk_CustomIconBrushType::PNG)
        .Set_IconSize(ECk_IconSize::IconSize_64x64));

    // EntityConstructionScript
    const auto& EntityConstructionScript_Name = UCk_Entity_ConstructionScript_PDA::StaticClass()->GetFName();

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetIcon, EntityConstructionScript_Name}
        .Set_IconFileName(TEXT("Icon_EntityConstructionScript_16px"))
        .Set_IconBrushType(ECk_CustomIconBrushType::PNG)
        .Set_IconSize(ECk_IconSize::IconSize_16x16));

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetThumbnail, EntityConstructionScript_Name}
        .Set_IconFileName(TEXT("Icon_EntityConstructionScript_64px"))
        .Set_IconBrushType(ECk_CustomIconBrushType::PNG)
        .Set_IconSize(ECk_IconSize::IconSize_64x64));
}

auto
    FCkEcsEditorModule::
    ShutdownModule()
    -> void
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkEcsEditorModule, CkEcsEditor)
