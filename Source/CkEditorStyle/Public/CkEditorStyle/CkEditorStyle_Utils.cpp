#include "CkEditorStyle_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Styling/SlateStyleRegistry.h>

#define IMAGE_PLUGIN_BRUSH(RelativePath, ...) FSlateImageBrush(_StyleInstance->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define IMAGE_PLUGIN_BRUSH_SVG(RelativePath, ...) FSlateVectorImageBrush(_StyleInstance->RootToContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)

// --------------------------------------------------------------------------------------------------------------------

TSharedPtr<FSlateStyleSet> UCk_Utils_EditorStyle_UE::_StyleInstance = nullptr;

FName UCk_Utils_EditorStyle_UE::_StyleSetName = FName(TEXT("CkFondation_EditorStyle"));

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EditorStyle_UE::
    DoRegister_SlateStyle()
    -> void
{
    if (ck::IsValid(_StyleInstance))
    { return; }

    _StyleInstance = MakeShareable(new FSlateStyleSet(_StyleSetName));
    _StyleInstance->SetContentRoot(UCk_Utils_IO_UE::Get_PluginDir(TEXT("CkFoundation")) / TEXT("/Resources/Editor"));

    FSlateStyleRegistry::RegisterSlateStyle(*_StyleInstance);
}

auto
    UCk_Utils_EditorStyle_UE::
    DoUnregister_SlateStyle()
    -> void
{
    FSlateStyleRegistry::UnRegisterSlateStyle(*_StyleInstance);

    CK_TRIGGER_ENSURE_IF(NOT _StyleInstance.IsUnique(), TEXT("{} SlateStyle instance is NOT Unique!"), _StyleSetName);

    _StyleInstance.Reset();
}

auto
    UCk_Utils_EditorStyle_UE::
    DoGet_IconSize(
        ECk_IconSize InSize)
    -> const FVector2D&
{
    switch (InSize)
    {
        case ECk_IconSize::IconSize_16x16:
        {
            static FVector2D IconSize_16x16(16.0f, 16.0f);
            return IconSize_16x16;
        }
        case ECk_IconSize::IconSize_32x16:
        {
            static FVector2D IconSize_32x16(32.0f, 16.0f);
            return IconSize_32x16;
        }
        case ECk_IconSize::IconSize_48x16:
        {
            static FVector2D IconSize_48x16(48.0f, 16.0f);
            return IconSize_48x16;
        }
        case ECk_IconSize::IconSize_20x20:
        {
            static FVector2D IconSize_20x20(20.0f, 20.0f);
            return IconSize_20x20;
        }
        case ECk_IconSize::IconSize_32x32:
        {
            static FVector2D IconSize_32x32(32.0f, 32.0f);
            return IconSize_32x32;
        }
        case ECk_IconSize::IconSize_40x40:
        {
            static FVector2D IconSize_40x40(40.0f, 40.0f);
            return IconSize_40x40;
        }
        case ECk_IconSize::IconSize_64x64:
        {
            static FVector2D IconSize_64x64(64.0f, 64.0f);
            return IconSize_64x64;
        }
        default:
        {
            CK_INVALID_ENUM(InSize);
            static FVector2D InvalidIconSize_Fallback(16.0f, 16.0f);
            return InvalidIconSize_Fallback;
        }
    }
}

auto
    UCk_Utils_EditorStyle_UE::
    DoReloadTextures()
    -> void
{
    if (NOT FSlateApplication::IsInitialized())
    { return; }

    FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

auto
    UCk_Utils_EditorStyle_UE::
    Get()
    -> const ISlateStyle&
{
    return *_StyleInstance;
}

auto
    UCk_Utils_EditorStyle_UE::
    Register_CustomSlateStyle(
        const FCk_CustomSlateStyle_Params& InParams)
    -> void
{
    if (ck::Is_NOT_Valid(_StyleInstance))
    {
        DoRegister_SlateStyle();
    }

    switch (const auto& IconBrushType = InParams.Get_IconBrushType())
    {
        case ECk_IconBrushType::PNG:
        {
            _StyleInstance->Set
            (
                InParams.Get_StyleName(),
                new IMAGE_PLUGIN_BRUSH(InParams.Get_IconAssetName().ToString(), DoGet_IconSize(InParams.Get_IconSize()))
            );

            break;
        }
        case ECk_IconBrushType::SVG:
        {
            _StyleInstance->Set
            (
                InParams.Get_StyleName(),
                new IMAGE_PLUGIN_BRUSH_SVG(InParams.Get_IconAssetName().ToString(), DoGet_IconSize(InParams.Get_IconSize()))
            );

            break;
        }
        default:
        {
            CK_INVALID_ENUM(IconBrushType);
            return;
        }
    }

    DoReloadTextures();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_CustomSlateStyle_Params::
    Get_StyleName() const
    -> FName
{
    switch (_AssetStyleType)
    {
        case ECk_AssetStyleType::AssetThumbnail:
        {
            // For Asset Thumbnail the name needs to follow the format: 'ClassThumbnail.MyClassNameHere' without the 'U' prefix
            return *ck::Format_UE(TEXT("ClassThumbnail.{}"), _StyleName);
        }
        case ECk_AssetStyleType::AssetIcon:
        {
            // For Asset Icon the name needs to follow the format: 'ClassIcon.MyClassNameHere' without the 'U' prefix
            return *ck::Format_UE(TEXT("ClassIcon.{}"), _StyleName);
        }
        case ECk_AssetStyleType::Other:
        {
            return _StyleName;
        }
        default:
        {
            CK_INVALID_ENUM(_AssetStyleType);
            return {};
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
