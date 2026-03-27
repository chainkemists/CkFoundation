#include "CkInventoryEditor_Module.h"

#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/Item/CkItem_Validator.h"
#include "CkInventory/ItemTrait/CkItemTrait.h"

#include "CkEditorStyle/CkEditorStyle_Utils.h"

#define LOCTEXT_NAMESPACE "FCkInventoryEditorModule"

// ----------------------------------------------------------------------------------------------------------------

auto
    FCkInventoryEditorModule::
    StartupModule()
    -> void
{
    // ---- Item Definition Icon ----

    const auto& ItemDefinition_Name = UCk_InventoryItem_Definition::StaticClass()->GetFName();

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetIcon, ItemDefinition_Name}
        .Set_IconFileName(TEXT("Icon_ItemDefinition"))
        .Set_IconBrushType(ECk_CustomIconBrushType::SVG)
        .Set_IconSize(ECk_IconSize::IconSize_16x16));

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetThumbnail, ItemDefinition_Name}
        .Set_IconFileName(TEXT("Icon_ItemDefinition"))
        .Set_IconBrushType(ECk_CustomIconBrushType::SVG)
        .Set_IconSize(ECk_IconSize::IconSize_64x64));

    // ---- Item Trait Icon ----

    const auto& ItemTrait_Name = UCk_ItemTrait::StaticClass()->GetFName();

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetIcon, ItemTrait_Name}
        .Set_IconFileName(TEXT("Icon_ItemTrait"))
        .Set_IconBrushType(ECk_CustomIconBrushType::SVG)
        .Set_IconSize(ECk_IconSize::IconSize_16x16));

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetThumbnail, ItemTrait_Name}
        .Set_IconFileName(TEXT("Icon_ItemTrait"))
        .Set_IconBrushType(ECk_CustomIconBrushType::SVG)
        .Set_IconSize(ECk_IconSize::IconSize_64x64));

    // ---- Item Validator Icon ----

    const auto& ItemValidator_Name = UCk_ItemValidator::StaticClass()->GetFName();

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetIcon, ItemValidator_Name}
        .Set_IconFileName(TEXT("Icon_ItemValidator"))
        .Set_IconBrushType(ECk_CustomIconBrushType::SVG)
        .Set_IconSize(ECk_IconSize::IconSize_16x16));

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetThumbnail, ItemValidator_Name}
        .Set_IconFileName(TEXT("Icon_ItemValidator"))
        .Set_IconBrushType(ECk_CustomIconBrushType::SVG)
        .Set_IconSize(ECk_IconSize::IconSize_64x64));
}

auto
    FCkInventoryEditorModule::
    ShutdownModule()
    -> void
{
}

// ----------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkInventoryEditorModule, CkInventoryEditor)
