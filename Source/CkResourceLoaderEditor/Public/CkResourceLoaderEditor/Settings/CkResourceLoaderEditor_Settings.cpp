#include "CkResourceLoaderEditor_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkResourceLoaderEditor/Subsystem/CkEditorAssetLoader_Subsystem.h"

#include <DetailCategoryBuilder.h>
#include <DetailLayoutBuilder.h>
#include <DetailWidgetRow.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "ResourceLoader_SettingsDetails"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    auto
        FResourceLoaderEditor_ProjectSettings_Details::
        MakeInstance()
        -> TSharedRef<IDetailCustomization>
    {
        return MakeShareable(new FResourceLoaderEditor_ProjectSettings_Details);
    }

    auto
        FResourceLoaderEditor_ProjectSettings_Details::
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
    UCk_ResourceLoaderEditor_ProjectSettings_UE::
    PostEditChangeProperty(
        FPropertyChangedEvent& InPropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(InPropertyChangedEvent);

    if (ck::Is_NOT_Valid(InPropertyChangedEvent.Property))
    { return; }

    if (InPropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UCk_ResourceLoaderEditor_ProjectSettings_UE, _ClassesToLoad))
    {
        UCk_Utils_EditorAssetLoader_SubSystem_UE::Request_RefreshLoadingAssets();
    }
}

auto
    UCk_Utils_ResourceLoaderEditor_Settings_UE::
    Get_ClassesToLoad()
    -> const TArray<FSoftClassPath>&
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_ResourceLoaderEditor_ProjectSettings_UE>()->Get_ClassesToLoad();
}

auto
    UCk_Utils_ResourceLoaderEditor_Settings_UE::
    Request_ClearAllLoadedObjects()
    -> void
{
    UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_ResourceLoaderEditor_ProjectSettings_UE>()->_LoadedObjects.Empty();
}

auto
    UCk_Utils_ResourceLoaderEditor_Settings_UE::
    Request_AddLoadedObject(
        const FSoftObjectPath& InLoadedObject)
    -> void
{
    UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_ResourceLoaderEditor_ProjectSettings_UE>()->_LoadedObjects.Emplace(InLoadedObject);
}

auto
    UCk_Utils_ResourceLoaderEditor_Settings_UE::
    Request_AddLoadedObjects(
        const TArray<FSoftObjectPath>& InLoadedObjects)
    -> void
{
    UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_ResourceLoaderEditor_ProjectSettings_UE>()->_LoadedObjects.Append(InLoadedObjects);
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
