#include "CkEditorStyle_Settings.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEditorStyle/Widgets/ClassPicker/CkEditorStyle_ClassPicker.h"

#include "DesktopPlatformModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDesktopPlatform.h"
#include "Kismet2/KismetEditorUtilities.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "EditorStyle_SettingsDetails"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    auto
        FEditorStyle_ProjectSettings_Details::
        MakeInstance()
        -> TSharedRef<IDetailCustomization>
    {
        return MakeShareable(new FEditorStyle_ProjectSettings_Details);
    }

    auto
        FEditorStyle_ProjectSettings_Details::
        CustomizeDetails(
            IDetailLayoutBuilder& DetailBuilder) -> void
    {
        TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
        DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);
        check(ObjectsBeingCustomized.Num() == 1);

        TWeakObjectPtr<UCk_EditorStyle_ProjectSettings_UE> Settings = Cast<UCk_EditorStyle_ProjectSettings_UE>(ObjectsBeingCustomized[0].Get());

        auto& AssetIconsCategory = DetailBuilder.EditCategory("Asset Icons");
        AssetIconsCategory.AddCustomRow(LOCTEXT("Add Custom Asset Icon", "Add Custom Asset Icon"))
            .ValueContent()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .Padding(5)
                .AutoWidth()
                [
                    SNew(SButton)
                    .Text(LOCTEXT("AddAssetIcon", "Add Custom Asset Icon"))
                    .ToolTipText(LOCTEXT("AddAssetIcon_Tooltip", "Associate a custom Icon to an Asset"))
                    .OnClicked_Lambda([this, Settings]()
                    {
                        const TSharedRef<widgets::SEditorStyle_AssetClassPicker> Dialog = SNew(widgets::SEditorStyle_AssetClassPicker);
                        Dialog->ConfigureProperties();
                        Settings->AssignCustomIconOrThumbnailToAsset(Dialog->Get_ClassPicked().Get(), ECk_AssetStyleType::AssetIcon);

                        return FReply::Handled();
                    })
                ]
            ];

        auto& AssetThumbnailsCategory = DetailBuilder.EditCategory("Asset Thumbnails");
        AssetThumbnailsCategory.AddCustomRow(LOCTEXT("Add Custom Asset Thumbnail", "Add Custom Asset Thumbnail"))
            .ValueContent()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .Padding(5)
                .AutoWidth()
                [
                    SNew(SButton)
                    .Text(LOCTEXT("AddAssetThumbnail", "Add Custom Asset Thumbnail"))
                    .ToolTipText(LOCTEXT("AddAssetThumbnail_Tooltip", "Associate a custom Thumbnail to an Asset"))
                    .OnClicked_Lambda([this, Settings]()
                    {
                        const TSharedRef<widgets::SEditorStyle_AssetClassPicker> Dialog = SNew(widgets::SEditorStyle_AssetClassPicker);
                        Dialog->ConfigureProperties();
                        Settings->AssignCustomIconOrThumbnailToAsset(Dialog->Get_ClassPicked().Get(), ECk_AssetStyleType::AssetThumbnail);

                        return FReply::Handled();
                    })
                ]
            ];
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EditorStyle_ProjectSettings_UE::
    AssignCustomIconOrThumbnailToAsset(
        UClass* InAssetClass,
        ECk_AssetStyleType InAssetStyle)
    -> void
{
    if (ck::Is_NOT_Valid(InAssetClass))
    { return; }

    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

    const auto Title = FString{ TEXT("Select a PNG or SVG file") };
    const auto FileTypes = FString{ TEXT("PNG files (*.png)|*.png|SVG files (*.svg)|*.svg") };
    const auto DefaultPath = UCk_Utils_EditorStyle_UE::Get_StyleInstance()->GetContentRootDir();
    const auto DefaultFile = FString{ TEXT("") };

    TArray<FString> OutFileNames;
    const auto& DialogResult = DesktopPlatform->OpenFileDialog
    (
        nullptr,
        Title,
        DefaultPath,
        DefaultFile,
        FileTypes,
        EFileDialogFlags::None,
        OutFileNames
    );

    if (NOT DialogResult)
    { return; }

    const auto& FullFileName = OutFileNames[0];
    const auto& BaseFileName = FPaths::GetBaseFilename(FullFileName);
    const auto& FilePath = FPaths::GetPath(FullFileName);

    CK_ENSURE_IF_NOT(FilePath.Equals(DefaultPath),
        TEXT("Cannot assign a custom [{}] to the Asset [{}] because the selected icon file [{}] is NOT located in the supported path [{}]"),
        InAssetStyle,
        InAssetClass,
        FullFileName,
        DefaultPath)
    { return; }

    CK_ENSURE_IF_NOT(FilePath.Equals(DefaultPath),
        TEXT("Cannot assign a custom [{}] to the Asset [{}] because the selected icon file [{}] is NOT located in the supported path [{}]"),
        InAssetStyle,
        InAssetClass,
        FullFileName,
        DefaultPath)
    { return; }

    const auto& FileExtension = FPaths::GetExtension(FullFileName);
    const auto& IsSvgFile = FileExtension.Equals(TEXT("svg"));
    const auto& IsPngFile = FileExtension.Equals(TEXT("png"));

    CK_ENSURE_IF_NOT(IsSvgFile || IsPngFile,
        TEXT("Cannot assign a custom [{}] to the Asset [{}] because the selected icon file [{}] is NOT a PNG or SVG"),
        InAssetStyle,
        InAssetClass,
        FullFileName,
        DefaultPath)
    { return; }

    const auto BrushType = IsSvgFile ? ECk_CustomIconBrushType::SVG : ECk_CustomIconBrushType::PNG;
    const auto AssetSoftClassPtr = TSoftClassPtr(InAssetClass);
    const auto& AssetClassName = InAssetClass->GetFName();

    switch (InAssetStyle)
    {
        case ECk_AssetStyleType::AssetThumbnail:
        {
            _CustomAssetThumbnails.Add
            (
                AssetSoftClassPtr,
                FCk_CustomAssetStyle_Params{ECk_AssetStyleType::AssetThumbnail, AssetClassName}
                    .Set_IconFileName(*BaseFileName)
                    .Set_IconBrushType(BrushType)
                    .Set_IconSize(ECk_IconSize::IconSize_64x64)
            );

            break;
        }
        case ECk_AssetStyleType::AssetIcon:
        {
            _CustomAssetIcons.Add
            (
                AssetSoftClassPtr,
                FCk_CustomAssetStyle_Params{ECk_AssetStyleType::AssetIcon, AssetClassName}
                    .Set_IconFileName(*BaseFileName)
                    .Set_IconBrushType(BrushType)
                    .Set_IconSize(ECk_IconSize::IconSize_16x16)
            );

            break;
        }
        case ECk_AssetStyleType::Other:
        {
            return;
        }
        default:
        {
            CK_INVALID_ENUM(InAssetStyle);
            return;
        }
    }

    this->TryUpdateDefaultConfigFile();

    DoUpdateCustomIconsAndThumbnail();
}

auto
    UCk_EditorStyle_ProjectSettings_UE::
    PostInitProperties()
    -> void
{
    Super::PostInitProperties();

    DoUpdateCustomIconsAndThumbnail();
}

#if WITH_EDITOR
auto
    UCk_EditorStyle_ProjectSettings_UE::
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    DoUpdateCustomIconsAndThumbnail();
}
#endif

auto
    UCk_EditorStyle_ProjectSettings_UE::
    DoUpdateCustomIconsAndThumbnail() const
    -> void
{
    for (const auto& Kvp : _CustomAssetIcons)
    {
        const auto& CustomAssetStyle = Kvp.Value;
        UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(CustomAssetStyle);
    }

    for (const auto& Kvp : _CustomAssetThumbnails)
    {
        const auto& CustomAssetStyle = Kvp.Value;
        UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(CustomAssetStyle);
    }
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
