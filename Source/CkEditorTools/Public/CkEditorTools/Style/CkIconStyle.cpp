#include "CkIconStyle.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Brushes/SlateImageBrush.h>
#include <Misc/Paths.h>
#include <Styling/SlateStyleRegistry.h>

// --------------------------------------------------------------------------------------------------------------------

TSharedPtr<FSlateStyleSet> FCkIconStyle::_StyleInstance = nullptr;
TArray<const FSlateBrush*> FCkIconStyle::_Brushes_16x16 = {};
TArray<const FSlateBrush*> FCkIconStyle::_Brushes_24x24 = {};

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkIconStyle::
    Initialize()
    -> void
{
    if (_StyleInstance.IsValid())
    { return; }

    _StyleInstance = Create();
    FSlateStyleRegistry::RegisterSlateStyle(*_StyleInstance);
}

auto
    FCkIconStyle::
    Shutdown()
    -> void
{
    if (NOT _StyleInstance.IsValid())
    { return; }

    FSlateStyleRegistry::UnRegisterSlateStyle(*_StyleInstance);
    _StyleInstance.Reset();
    _Brushes_16x16.Reset();
    _Brushes_24x24.Reset();
}

auto
    FCkIconStyle::
    Get()
    -> const ISlateStyle&
{
    return *_StyleInstance;
}

auto
    FCkIconStyle::
    GetStyleSetName()
    -> FName
{
    static const FName StyleSetName(TEXT("CkIconStyle"));
    return StyleSetName;
}

auto
    FCkIconStyle::
    Get_Brush(
        ECk_Icon InIcon,
        ECk_Icon_BrushSize InSize)
    -> const FSlateBrush*
{
    const auto StyleIsInitialized = _StyleInstance.IsValid();
    CK_ENSURE_IF_NOT(StyleIsInitialized, TEXT("FCkIconStyle::Get_Brush([{}]) called before Initialize"), InIcon)
    { return nullptr; }

    const auto Index = static_cast<int32>(InIcon);
    const auto IndexIsValid = _Brushes_16x16.IsValidIndex(Index);
    CK_ENSURE_IF_NOT(IndexIsValid, TEXT("Icon [{}] is outside the generated table — regenerate CkIcons via Generate-CkIcons.ps1"), InIcon)
    { return nullptr; }

    switch (InSize)
    {
        case ECk_Icon_BrushSize::Size_16x16:
        {
            return _Brushes_16x16[Index];
        }
        case ECk_Icon_BrushSize::Size_24x24:
        {
            return _Brushes_24x24[Index];
        }
        default:
        {
            CK_INVALID_ENUM(InSize);
            return nullptr;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkIconStyle::
    Create()
    -> TSharedRef<FSlateStyleSet>
{
    auto Style = MakeShared<FSlateStyleSet>(GetStyleSetName());

    // Path-only lookup (folder name, not plugin name), matching CkDebuggerStyle — resolves
    // identically from any module and avoids a Projects-module dependency.
    Style->SetContentRoot(UCk_Utils_IO_UE::Get_PluginsDir(TEXT("CkFoundation")) / TEXT("Resources/Icons"));

    CreateIconBrushes(Style);

    return Style;
}

auto
    FCkIconStyle::
    CreateIconBrushes(
        TSharedRef<FSlateStyleSet> InStyle)
    -> void
{
    const auto Entries = ck::icons::Get_GeneratedEntries();

    _Brushes_16x16.Init(nullptr, Entries.Num());
    _Brushes_24x24.Init(nullptr, Entries.Num());

    for (const auto& Entry : Entries)
    {
        const auto Index = static_cast<int32>(Entry.Icon);
        const auto IndexIsValid = _Brushes_16x16.IsValidIndex(Index);
        CK_ENSURE_IF_NOT(IndexIsValid, TEXT("Generated icon table entry [{}] does not match ECk_Icon — regenerate CkIcons via Generate-CkIcons.ps1"), Entry.SemanticName)
        { continue; }

        const auto SvgPath = InStyle->RootToContentDir(Entry.RelativePath, TEXT(".svg"));
        const auto SvgExists = FPaths::FileExists(SvgPath);
        CK_ENSURE_IF_NOT(SvgExists,
            TEXT("Vendored SVG missing for icon [{}] at [{}] — it will draw as nothing. Re-vendor via Import-MdiIcon.ps1 and regenerate"),
            Entry.SemanticName, SvgPath)
        { continue; }

        const auto Register = [&](const TCHAR* InSizeSuffix, const FVector2D& InSizeVec) -> const FSlateBrush*
        {
            const auto Brush = new FSlateVectorImageBrush{SvgPath, InSizeVec};
            InStyle->Set(FName{*ck::Format_UE(TEXT("CkIcon.{}.{}"), Entry.SemanticName, InSizeSuffix)}, Brush);
            return Brush;
        };

        _Brushes_16x16[Index] = Register(TEXT("16"), FVector2D{16.0f, 16.0f});
        _Brushes_24x24[Index] = Register(TEXT("24"), FVector2D{24.0f, 24.0f});
    }
}

// --------------------------------------------------------------------------------------------------------------------
