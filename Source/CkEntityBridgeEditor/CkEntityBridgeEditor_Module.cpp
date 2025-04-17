#include "CkEntityBridgeEditor_Module.h"

#include "CkEditorStyle/CkEditorStyle_Utils.h"

#include "CkEntityBridge/CkEntityBridge_ConstructionScript.h"

#define LOCTEXT_NAMESPACE "FCkEntityBridgeEditorModule"

auto
    FCkEntityBridgeEditorModule::
    StartupModule()
    -> void
{
    // EntityBridge AC
    const auto& EntityBridgeAC_Name = UCk_EntityBridge_ActorComponent_UE::StaticClass()->GetFName();

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetIcon, EntityBridgeAC_Name}
        .Set_IconFileName(TEXT("Icon_EntityBridge_16px"))
        .Set_IconBrushType(ECk_CustomIconBrushType::PNG)
        .Set_IconSize(ECk_IconSize::IconSize_16x16));

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetThumbnail, EntityBridgeAC_Name}
        .Set_IconFileName(TEXT("Icon_EntityBridge_64px"))
        .Set_IconBrushType(ECk_CustomIconBrushType::PNG)
        .Set_IconSize(ECk_IconSize::IconSize_64x64));
}

auto
    FCkEntityBridgeEditorModule::
    ShutdownModule()
    -> void
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkEntityBridgeEditorModule, CkEntityBridgeEditor)
