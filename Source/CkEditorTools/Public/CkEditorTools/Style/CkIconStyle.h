#pragma once

#include "CkEditorTools/Style/CkIcons_Generated.h"

#include <CoreMinimal.h>
#include <Styling/SlateStyle.h>

#include "CkIconStyle.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Icon_BrushSize : uint8
{
    Size_16x16,
    Size_24x24,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Icon_BrushSize);

// ====================================================================================================================
// The CkFoundation typed icon registry.
//
// Icons are referenced by the GENERATED ECk_Icon identifier (CkIcons_Generated.h) — never by a
// hand-written style-key string, and never by an MDI icon name. The assets are the vendored,
// recoloured Material Design Icons under Resources/Icons/Mdi/ (provenance: Mdi/NOTICE.md); the
// manifest at Resources/Icons/CkIcons_Manifest.json owns the semantic-name -> asset mapping, and
// Source/CkScripts/Generate-CkIcons.ps1 turns it into the enum + table this registry consumes.
//
// Every icon registers one FSlateVectorImageBrush PER SIZE — UE caches the rasterization per
// brush, so sharing one brush across sizes would produce a scaled blur instead of a crisp
// re-raster. All glyphs are monochrome white: tint at draw time via SImage.ColorAndOpacity.
//
// Registered from FCkEditorToolsModule startup/shutdown, in every process type — like the
// CkGameplayDebugger style sets (and unlike UCk_Utils_EditorStyle_UE, which is editor-only),
// because consumers ship in packaged Development/DebugGame builds.
// ====================================================================================================================

class CKEDITORTOOLS_API FCkIconStyle
{
public:
    static auto Initialize() -> void;
    static auto Shutdown() -> void;
    static auto Get() -> const ISlateStyle&;
    static auto GetStyleSetName() -> FName;

    /**
     * Resolves the brush for a typed icon at a registered size. Returns nullptr only if the
     * style was never initialized or the icon's vendored SVG was missing at registration —
     * both fire ensures at the point of fault, so callers may pass the result straight to
     * SImage (a null brush draws nothing).
     */
    static auto Get_Brush(ECk_Icon InIcon, ECk_Icon_BrushSize InSize) -> const FSlateBrush*;

    /**
     * The registered style key for a typed icon — for the rare consumer that needs an
     * FSlateIcon (tab spawners) instead of a brush pointer. Never hand-write these keys.
     */
    static auto Get_StyleKey(ECk_Icon InIcon, ECk_Icon_BrushSize InSize) -> FName;

    /** Reverse lookup by the generated semantic name (e.g. legacy archetype-descriptor basenames). */
    static auto TryGet_IconBySemanticName(FName InSemanticName) -> TOptional<ECk_Icon>;

    /**
     * The dynamic side-lane for GAME-supplied icons (archetype descriptors' IconSvgPath):
     * a generated enum is closed, so consumers outside CkFoundation register their SVGs here
     * by name. First-party code must never use this — it uses ECk_Icon.
     * Registration is idempotent per id; the SVG must be monochrome white like the corpus.
     */
    static auto Register_DynamicIcon(FName InId, const FString& InAbsoluteSvgPath) -> void;
    static auto Get_DynamicBrush(FName InId, ECk_Icon_BrushSize InSize) -> const FSlateBrush*;

private:
    static auto Create() -> TSharedRef<FSlateStyleSet>;
    static auto CreateIconBrushes(TSharedRef<FSlateStyleSet> InStyle) -> void;

    static TSharedPtr<FSlateStyleSet> _StyleInstance;
    static TArray<const FSlateBrush*> _Brushes_16x16;
    static TArray<const FSlateBrush*> _Brushes_24x24;
    static TArray<FName>              _StyleKeys_16x16;
    static TArray<FName>              _StyleKeys_24x24;
    static TMap<FName, ECk_Icon>      _IconBySemanticName;
    static TMap<FName, TPair<const FSlateBrush*, const FSlateBrush*>> _DynamicBrushes;
};

// --------------------------------------------------------------------------------------------------------------------
