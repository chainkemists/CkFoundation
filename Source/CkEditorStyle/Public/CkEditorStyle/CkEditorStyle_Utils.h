#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include <CoreMinimal.h>
#include <Styling/SlateStyle.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkEditorStyle_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM()
enum class ECk_IconSize : uint8
{
    IconSize_16x16,
    IconSize_32x16,
    IconSize_48x16,
    IconSize_20x20,
    IconSize_32x32,
    IconSize_40x40,
    IconSize_64x64,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_IconSize);

// --------------------------------------------------------------------------------------------------------------------

UENUM()
enum class ECk_IconBrushType : uint8
{
    PNG,
    SVG
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_IconBrushType);

// --------------------------------------------------------------------------------------------------------------------

UENUM()
enum class ECk_AssetStyleType : uint8
{
    AssetThumbnail,
    AssetIcon,
    Other
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AssetStyleType);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKEDITORSTYLE_API FCk_CustomSlateStyle_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_CustomSlateStyle_Params);

private:
    UPROPERTY()
    ECk_AssetStyleType _AssetStyleType = ECk_AssetStyleType::Other;

    UPROPERTY()
    ECk_IconSize _IconSize = ECk_IconSize::IconSize_16x16;

    UPROPERTY()
    ECk_IconBrushType _IconBrushType = ECk_IconBrushType::PNG;

    // StyleName for AssetStyle of type AssetThumbnail or AssetIcon mus tbe the name of the class without the 'U' prefix
    UPROPERTY()
    FName _StyleName = NAME_None;

    // Asset name on disk without the extension
    UPROPERTY()
    FName _IconAssetName = NAME_None;

public:
    auto Get_StyleName() const -> FName;

public:
    CK_PROPERTY(_IconSize);
    CK_PROPERTY(_IconAssetName);
    CK_PROPERTY(_IconBrushType);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_CustomSlateStyle_Params, _AssetStyleType, _StyleName);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKEDITORSTYLE_API UCk_Utils_EditorStyle_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EditorStyle_UE);

public:
    friend class FCkEditorStyleModule;

public:
    static auto
    Get() -> const ISlateStyle&;

    static auto
    Register_CustomSlateStyle(
        const FCk_CustomSlateStyle_Params& InParams) -> void;

private:
    static auto
    DoReloadTextures() -> void;

    static auto
    DoRegister_SlateStyle() -> void;

    static auto
    DoUnregister_SlateStyle() -> void;

    static auto
    DoGet_IconSize(
        ECk_IconSize InSize) -> const FVector2D&;

public:
    static TSharedPtr<class FSlateStyleSet> _StyleInstance;
    static FName                            _StyleSetName;
};

// --------------------------------------------------------------------------------------------------------------------
