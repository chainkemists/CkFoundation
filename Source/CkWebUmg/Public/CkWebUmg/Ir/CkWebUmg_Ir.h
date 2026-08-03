#pragma once

#include "CoreMinimal.h"

// ====================================================================================================================
// In-memory form of a *.ckui.json document (schema v1 — Tools/ckwebumg-extract/SCHEMA.md).
// Every value is computed, resolved, and absolute; keyword fields keep the extractor's strings and
// are mapped to engine/Yoga enums at the consumer (the reflected asset form arrives with Gate 4).
// Plain aggregates by design — this is loader output, not a reflected or encapsulated surface.
// ====================================================================================================================

struct CKWEBUMG_API FCkWebUmg_IrRect
{
    float X = 0.0f;
    float Y = 0.0f;
    float W = 0.0f;
    float H = 0.0f;
};

// --------------------------------------------------------------------------------------------------------------------

struct CKWEBUMG_API FCkWebUmg_IrBox
{
    FCkWebUmg_IrRect Content;
    FCkWebUmg_IrRect Padding;
    FCkWebUmg_IrRect Border;
    FCkWebUmg_IrRect Margin;
};

// --------------------------------------------------------------------------------------------------------------------

struct CKWEBUMG_API FCkWebUmg_IrInset
{
    FString Top;
    FString Right;
    FString Bottom;
    FString Left;
    TArray<FString> Authored;
};

// --------------------------------------------------------------------------------------------------------------------

struct CKWEBUMG_API FCkWebUmg_IrLayout
{
    FString Display;
    FString Direction;
    FString Justify;
    FString Align;
    FString AlignSelf;
    FString AlignContent;
    FString Wrap;
    FString Position;
    FString Basis;
    FString BoxSizing;
    FString OverflowX;
    FString OverflowY;
    FVector2f Gap = FVector2f::ZeroVector;
    float Grow = 0.0f;
    float Shrink = 0.0f;
    int32 ZIndex = 0;
    int32 Order = 0;
    TOptional<FCkWebUmg_IrInset> Inset;
    TArray<FString> SizingAuthored; // which of width/height/min-*/max-*/basis the author declared
    TOptional<float> MinSize[2];    // min-width, min-height (absolute px; unset = none)
    TOptional<float> MaxSize[2];    // max-width, max-height
};

// --------------------------------------------------------------------------------------------------------------------

struct CKWEBUMG_API FCkWebUmg_IrGradientStop
{
    FColor Color = FColor::Black;
    TOptional<float> PosPct;
};

struct CKWEBUMG_API FCkWebUmg_IrGradient
{
    FString GradientType; // linear | radial | conic | unparsed
    TOptional<float> AngleDeg;         // linear only; CSS convention (0 = to top, 90 = to right)
    TOptional<FVector2f> RadialCenter; // radial only; absolute px within the painted box
    TOptional<FVector2f> RadialRadius; // radial only; rx, ry px (unset = unparseable prelude)
    TArray<FCkWebUmg_IrGradientStop> Stops;
};

// --------------------------------------------------------------------------------------------------------------------

struct CKWEBUMG_API FCkWebUmg_IrShadowLayer
{
    FColor Color = FColor::Black;
    FVector2f Offset = FVector2f::ZeroVector;
    float Blur = 0.0f;
    float Spread = 0.0f;
    bool Inset = false;
};

// --------------------------------------------------------------------------------------------------------------------

struct CKWEBUMG_API FCkWebUmg_IrTransform
{
    // CSS matrix order [a, b, c, d, tx, ty]; empty when the computed transform was 3D (the
    // extractor keeps the verbatim string; consumers diagnose, never guess).
    TArray<float> Matrix;
    FVector2f Origin = FVector2f::ZeroVector; // absolute px within the node
};

// --------------------------------------------------------------------------------------------------------------------

struct CKWEBUMG_API FCkWebUmg_IrPaint
{
    TOptional<FColor> BackgroundColor;
    TOptional<FCkWebUmg_IrGradient> Gradient;
    TOptional<FString> BackgroundImageAsset;
    FVector4f BorderRadius = FVector4f::Zero(); // tl, tr, br, bl
    FVector4f BorderWidth = FVector4f::Zero();  // t, r, b, l
    TOptional<FColor> BorderColor;
    TArray<FColor> BorderColors; // t, r, b, l (empty when the page has no borders)
    TOptional<FCkWebUmg_IrTransform> Transform;
    TArray<FCkWebUmg_IrShadowLayer> ShadowLayers; // CSS order: first layer paints topmost
    bool HasUntypedShadow = false;                // boxShadow present but layers unparseable
    float Opacity = 1.0f;
    FString Visibility;
};

// --------------------------------------------------------------------------------------------------------------------

struct CKWEBUMG_API FCkWebUmg_IrText
{
    FString Content;
    FString Family;
    float SizePx = 0.0f;
    int32 Weight = 400;
    TOptional<float> LineHeightPx;
    float LetterSpacingPx = 0.0f;
    TOptional<FColor> Color;
    FString Align;
    FString WhiteSpace;
    FString TransformCase; // CSS text-transform keyword; content stores the AUTHORED text
    TArray<FCkWebUmg_IrRect> LineBoxes; // one per rendered line (Chromium Range.getClientRects)
};

// --------------------------------------------------------------------------------------------------------------------

// One extractor diagnostic: an author-set declaration outside the v1 surface (SCHEMA.md, four
// classes). Carried so the no-silent-drops contract survives into the emitted asset.
struct CKWEBUMG_API FCkWebUmg_IrUnsupported
{
    FString Property;
    FString Value;
    FString Source; // file:line or "computed"/"inline style="
};

// --------------------------------------------------------------------------------------------------------------------

struct CKWEBUMG_API FCkWebUmg_IrNode
{
    FString Id;
    FString Tag;
    FString CkName; // data-ck-name (empty = unnamed); uniqueness enforced at emission (D3)
    FString CkBind; // data-ck-bind — carried verbatim; binding SEMANTICS are designed against the
    FString CkSlot; // data-ck-slot    Gate-5 real consumer, not invented here (DATA_CK_SPEC.md)
    FString Asset;  // for <img>: id into FCkWebUmg_IrDocument::AssetSourcesById
    FCkWebUmg_IrBox Box;               // post-transform AABB when a transform applies (SCHEMA.md)
    TOptional<FCkWebUmg_IrBox> BoxUntransformed; // the rect layout reproduces; transform reapplies at paint
    FCkWebUmg_IrLayout Layout;
    FCkWebUmg_IrPaint Paint;
    TOptional<FCkWebUmg_IrText> Text;
    TArray<FCkWebUmg_IrUnsupported> Unsupported;
    TArray<TSharedPtr<FCkWebUmg_IrNode>> Children;

    // The box the layout runtime reproduces — untransformed geometry when a transform (own or
    // ancestral) moved this node; the transform itself reapplies at paint as a render transform.
    auto Get_LayoutBox() const -> const FCkWebUmg_IrBox&
    { return BoxUntransformed.IsSet() ? *BoxUntransformed : Box; }
};

// --------------------------------------------------------------------------------------------------------------------

struct CKWEBUMG_API FCkWebUmg_IrDiagnostic
{
    FString Kind; // duplicate-ck-name | unknown-ck-attribute
    FString Node;
    FString Detail;
};

// --------------------------------------------------------------------------------------------------------------------

struct CKWEBUMG_API FCkWebUmg_IrDocument
{
    int32 Schema = 0;
    FIntPoint Viewport = FIntPoint::ZeroValue;
    float Dpr = 1.0f;
    FString Browser;
    FString Title; // the page's <title> (empty when the mockup has none) — display metadata only;
                   // asset/package naming stays the collision-safe file basename
    TMap<FString, FString> AssetSourcesById; // asset id -> src path relative to the page
    TArray<FCkWebUmg_IrDiagnostic> Diagnostics; // page-level extractor diagnostics
    TSharedPtr<FCkWebUmg_IrNode> Root;
};
