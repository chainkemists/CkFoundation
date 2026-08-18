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
TArray<FName>              FCkIconStyle::_StyleKeys_16x16 = {};
TArray<FName>              FCkIconStyle::_StyleKeys_24x24 = {};
TMap<FName, ECk_Icon>      FCkIconStyle::_IconBySemanticName = {};
TMap<FName, TPair<const FSlateBrush*, const FSlateBrush*>> FCkIconStyle::_DynamicBrushes = {};

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
    _StyleKeys_16x16.Reset();
    _StyleKeys_24x24.Reset();
    _IconBySemanticName.Reset();
    _DynamicBrushes.Reset();
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
    if (InIcon == ECk_Icon::None)
    { return nullptr; }

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

auto
    FCkIconStyle::
    Get_StyleKey(
        ECk_Icon InIcon,
        ECk_Icon_BrushSize InSize)
    -> FName
{
    if (InIcon == ECk_Icon::None)
    { return NAME_None; }

    const auto Index = static_cast<int32>(InIcon);
    const auto IndexIsValid = _StyleKeys_16x16.IsValidIndex(Index);
    CK_ENSURE_IF_NOT(IndexIsValid, TEXT("Get_StyleKey([{}]) before Initialize, or outside the generated table"), InIcon)
    { return NAME_None; }

    switch (InSize)
    {
        case ECk_Icon_BrushSize::Size_16x16:
        {
            return _StyleKeys_16x16[Index];
        }
        case ECk_Icon_BrushSize::Size_24x24:
        {
            return _StyleKeys_24x24[Index];
        }
        default:
        {
            CK_INVALID_ENUM(InSize);
            return NAME_None;
        }
    }
}

auto
    FCkIconStyle::
    TryGet_IconBySemanticName(
        FName InSemanticName)
    -> TOptional<ECk_Icon>
{
    if (const auto* Found = _IconBySemanticName.Find(InSemanticName))
    { return *Found; }

    return {};
}

auto
    FCkIconStyle::
    Register_DynamicIcon(
        FName InId,
        const FString& InAbsoluteSvgPath)
    -> void
{
    const auto StyleIsInitialized = _StyleInstance.IsValid();
    CK_ENSURE_IF_NOT(StyleIsInitialized, TEXT("Register_DynamicIcon([{}]) called before Initialize"), InId)
    { return; }

    if (_DynamicBrushes.Contains(InId))
    { return; }

    const auto SvgExists = FPaths::FileExists(InAbsoluteSvgPath);
    CK_ENSURE_IF_NOT(SvgExists, TEXT("Dynamic icon [{}] SVG not found at [{}]"), InId, InAbsoluteSvgPath)
    { return; }

    const auto Register = [&](const TCHAR* InSizeSuffix, const FVector2D& InSizeVec) -> const FSlateBrush*
    {
        const auto Brush = new FSlateVectorImageBrush{InAbsoluteSvgPath, InSizeVec};
        _StyleInstance->Set(FName{*ck::Format_UE(TEXT("CkIcon.Dynamic.{}.{}"), InId, InSizeSuffix)}, Brush);
        return Brush;
    };

    _DynamicBrushes.Add(InId, {Register(TEXT("16"), FVector2D{16.0f, 16.0f}),
                               Register(TEXT("24"), FVector2D{24.0f, 24.0f})});
}

auto
    FCkIconStyle::
    Get_DynamicBrush(
        FName InId,
        ECk_Icon_BrushSize InSize)
    -> const FSlateBrush*
{
    if (InId.IsNone())
    { return nullptr; }

    if (const auto* Found = _DynamicBrushes.Find(InId))
    { return InSize == ECk_Icon_BrushSize::Size_24x24 ? Found->Value : Found->Key; }

    // A legacy descriptor may name a generated semantic (its SVG basename) instead of
    // registering — resolve it against the typed table so those keep working.
    if (const auto Icon = TryGet_IconBySemanticName(InId); Icon.IsSet())
    { return Get_Brush(*Icon, InSize); }

    return nullptr;
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

    // Slot 0 stays empty — it is ECk_Icon::None, the legitimate "no icon".
    _Brushes_16x16.Init(nullptr, Entries.Num() + 1);
    _Brushes_24x24.Init(nullptr, Entries.Num() + 1);
    _StyleKeys_16x16.Init(NAME_None, Entries.Num() + 1);
    _StyleKeys_24x24.Init(NAME_None, Entries.Num() + 1);
    _IconBySemanticName.Reserve(Entries.Num());

    for (const auto& Entry : Entries)
    {
        const auto Index = static_cast<int32>(Entry.Icon);
        const auto IndexIsValid = _Brushes_16x16.IsValidIndex(Index);
        CK_ENSURE_IF_NOT(IndexIsValid, TEXT("Generated icon table entry [{}] does not match ECk_Icon — regenerate CkIcons via Generate-CkIcons.ps1"), Entry.SemanticName)
        { continue; }

        _IconBySemanticName.Add(FName{Entry.SemanticName}, Entry.Icon);

        const auto SvgPath = InStyle->RootToContentDir(Entry.RelativePath, TEXT(".svg"));
        const auto SvgExists = FPaths::FileExists(SvgPath);
        CK_ENSURE_IF_NOT(SvgExists,
            TEXT("Vendored SVG missing for icon [{}] at [{}] — it will draw as nothing. Re-vendor via Import-MdiIcon.ps1 and regenerate"),
            Entry.SemanticName, SvgPath)
        { continue; }

        const auto Register = [&](const TCHAR* InSizeSuffix, const FVector2D& InSizeVec) -> TPair<const FSlateBrush*, FName>
        {
            const auto Key = FName{*ck::Format_UE(TEXT("CkIcon.{}.{}"), Entry.SemanticName, InSizeSuffix)};
            const auto Brush = new FSlateVectorImageBrush{SvgPath, InSizeVec};
            InStyle->Set(Key, Brush);
            return {Brush, Key};
        };

        const auto [Brush16, Key16] = Register(TEXT("16"), FVector2D{16.0f, 16.0f});
        const auto [Brush24, Key24] = Register(TEXT("24"), FVector2D{24.0f, 24.0f});
        _Brushes_16x16[Index] = Brush16;
        _Brushes_24x24[Index] = Brush24;
        _StyleKeys_16x16[Index] = Key16;
        _StyleKeys_24x24[Index] = Key24;
    }
}

// --------------------------------------------------------------------------------------------------------------------
