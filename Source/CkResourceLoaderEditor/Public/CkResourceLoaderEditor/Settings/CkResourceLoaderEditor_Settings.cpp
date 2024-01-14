#include "CkResourceLoaderEditor_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkResourceLoaderEditor/Subsystem/CkEditorAssetLoader_Subsystem.h"

#include <DetailCategoryBuilder.h>
#include <DetailLayoutBuilder.h>
#include <DetailWidgetRow.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "EditorStyle_SettingsDetails"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::details
{
    auto
        FEditorStyle_ResourceLoaderSettings_ProjectSettings_Details::
        MakeInstance()
        -> TSharedRef<IDetailCustomization>
    {
        return MakeShareable(new FEditorStyle_ResourceLoaderSettings_ProjectSettings_Details);
    }

    auto
        FEditorStyle_ResourceLoaderSettings_ProjectSettings_Details::
        CustomizeDetails(
            IDetailLayoutBuilder& DetailBuilder) -> void
    {
        auto& AssetIconsCategory = DetailBuilder.EditCategory("Asset Loading");
        AssetIconsCategory.AddCustomRow(LOCTEXT("Reload Resources", "Reload Resources"))
            .ValueContent()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .Padding(5)
                .AutoWidth()
                [
                    SNew(SButton)
                    .Text(LOCTEXT("ReloadResources", "Reload Resources"))
                    .ToolTipText(LOCTEXT("ReloadResources_Tooltip", "Force reload all resources with the ClassTypes and Paths specified"))
                    .OnClicked_Lambda([]()
                    {
                        UCk_Utils_EditorAssetLoader_SubSystem_UE::Request_RefreshLoadingAssets();

                        return FReply::Handled();
                    })
                ]
            ];
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResourceLoaderEditor_ProjectSettings_UE::
    Get_ClassesToLoad()
    -> const TArray<FSoftClassPath>&
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_ResourceLoaderEditor_ProjectSettings_UE>()->Get_ClassesToLoad();
}

auto
    UCk_Utils_ResourceLoaderEditor_ProjectSettings_UE::
    Request_AddLoadedObject(
        FSoftObjectPath InLoadedObject)
    -> void
{
    UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_ResourceLoaderEditor_ProjectSettings_UE>()->_LoadedObjects.Emplace(InLoadedObject);
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
