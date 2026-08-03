#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"

#include "CkWebUmg_PageAsset.generated.h"

// ====================================================================================================================
// Reflected asset form of a *.ckui.json document (Gate 4, DECISION 2: DataAsset-runtime primary).
// A faithful projection of the loader IR (CkWebUmg_Ir.h) into UPROPERTY-reflected structs: the node
// tree is FLATTENED into an array with child indices (UHT forbids recursive USTRUCTs); optionality
// is explicit Has-flags. Generated assets are READ-ONLY by contract (DECISION 3): every property is
// VisibleAnywhere, and regeneration overwrites wholesale — hand-authoring belongs in consumers.
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKWEBUMG_API FCk_WebUmg_BoxData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WebUmg_BoxData);

private:
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FVector4f _Content = FVector4f::Zero(); // x, y, w, h

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FVector4f _Padding = FVector4f::Zero();

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FVector4f _Border = FVector4f::Zero();

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FVector4f _Margin = FVector4f::Zero();

public:
    CK_PROPERTY(_Content);
    CK_PROPERTY(_Padding);
    CK_PROPERTY(_Border);
    CK_PROPERTY(_Margin);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWEBUMG_API FCk_WebUmg_LayoutData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WebUmg_LayoutData);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Display;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Direction;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Justify;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Align;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _AlignSelf;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _AlignContent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Wrap;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Position;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Basis;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _BoxSizing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _OverflowX;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _OverflowY;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FVector2f _Gap = FVector2f::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _Grow = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _Shrink = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _ZIndex = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _Order = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasInset = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _InsetTop;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _InsetRight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _InsetBottom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _InsetLeft;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FString> _InsetAuthored;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FString> _SizingAuthored;

    // min-width, min-height, max-width, max-height; negative = not constrained (CSS values are >= 0)
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FVector4f _MinMaxSize = FVector4f(-1.0f, -1.0f, -1.0f, -1.0f);

public:
    CK_PROPERTY(_Display);
    CK_PROPERTY(_Direction);
    CK_PROPERTY(_Justify);
    CK_PROPERTY(_Align);
    CK_PROPERTY(_AlignSelf);
    CK_PROPERTY(_AlignContent);
    CK_PROPERTY(_Wrap);
    CK_PROPERTY(_Position);
    CK_PROPERTY(_Basis);
    CK_PROPERTY(_BoxSizing);
    CK_PROPERTY(_OverflowX);
    CK_PROPERTY(_OverflowY);
    CK_PROPERTY(_Gap);
    CK_PROPERTY(_Grow);
    CK_PROPERTY(_Shrink);
    CK_PROPERTY(_ZIndex);
    CK_PROPERTY(_Order);
    CK_PROPERTY(_HasInset);
    CK_PROPERTY(_InsetTop);
    CK_PROPERTY(_InsetRight);
    CK_PROPERTY(_InsetBottom);
    CK_PROPERTY(_InsetLeft);
    CK_PROPERTY(_InsetAuthored);
    CK_PROPERTY(_SizingAuthored);
    CK_PROPERTY(_MinMaxSize);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWEBUMG_API FCk_WebUmg_GradientStopData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WebUmg_GradientStopData);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FColor _Color = FColor::Black;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _PosPct = -1.0f; // negative = unpositioned (CSS resolves at paint)

public:
    CK_PROPERTY(_Color);
    CK_PROPERTY(_PosPct);
};

USTRUCT(BlueprintType)
struct CKWEBUMG_API FCk_WebUmg_GradientData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WebUmg_GradientData);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _GradientType;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasAngle = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _AngleDeg = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasRadialGeometry = false;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FVector2f _RadialCenter = FVector2f::ZeroVector;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FVector2f _RadialRadius = FVector2f::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FCk_WebUmg_GradientStopData> _Stops;

public:
    CK_PROPERTY(_GradientType);
    CK_PROPERTY(_HasAngle);
    CK_PROPERTY(_AngleDeg);
    CK_PROPERTY(_HasRadialGeometry);
    CK_PROPERTY(_RadialCenter);
    CK_PROPERTY(_RadialRadius);
    CK_PROPERTY(_Stops);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWEBUMG_API FCk_WebUmg_ShadowLayerData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WebUmg_ShadowLayerData);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FColor _Color = FColor::Black;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FVector2f _Offset = FVector2f::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _Blur = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _Spread = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _Inset = false;

public:
    CK_PROPERTY(_Color);
    CK_PROPERTY(_Offset);
    CK_PROPERTY(_Blur);
    CK_PROPERTY(_Spread);
    CK_PROPERTY(_Inset);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWEBUMG_API FCk_WebUmg_PaintData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WebUmg_PaintData);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasBackgroundColor = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FColor _BackgroundColor = FColor::Transparent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasGradient = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_WebUmg_GradientData _Gradient;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _BackgroundImageAsset;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FVector4f _BorderRadius = FVector4f::Zero(); // tl, tr, br, bl

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FVector4f _BorderWidth = FVector4f::Zero(); // t, r, b, l

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasBorderColor = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FColor _BorderColor = FColor::Transparent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FColor> _BorderColors; // t, r, b, l or empty

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FCk_WebUmg_ShadowLayerData> _ShadowLayers;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasUntypedShadow = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasTransform = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<float> _TransformMatrix; // [a,b,c,d,tx,ty] or empty (untypeable 3D)

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FVector2f _TransformOrigin = FVector2f::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _Opacity = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Visibility;

public:
    CK_PROPERTY(_HasBackgroundColor);
    CK_PROPERTY(_BackgroundColor);
    CK_PROPERTY(_HasGradient);
    CK_PROPERTY(_Gradient);
    CK_PROPERTY(_BackgroundImageAsset);
    CK_PROPERTY(_BorderRadius);
    CK_PROPERTY(_BorderWidth);
    CK_PROPERTY(_HasBorderColor);
    CK_PROPERTY(_BorderColor);
    CK_PROPERTY(_BorderColors);
    CK_PROPERTY(_ShadowLayers);
    CK_PROPERTY(_HasUntypedShadow);
    CK_PROPERTY(_HasTransform);
    CK_PROPERTY(_TransformMatrix);
    CK_PROPERTY(_TransformOrigin);
    CK_PROPERTY(_Opacity);
    CK_PROPERTY(_Visibility);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWEBUMG_API FCk_WebUmg_TextData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WebUmg_TextData);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Content;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Family;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _SizePx = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _Weight = 400;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _LineHeightPx = -1.0f; // negative = 'normal'

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _LetterSpacingPx = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasColor = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FColor _Color = FColor::White;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Align;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _WhiteSpace;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _TransformCase;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    TArray<FVector4f> _LineBoxes; // x, y, w, h per rendered line

public:
    CK_PROPERTY(_Content);
    CK_PROPERTY(_Family);
    CK_PROPERTY(_SizePx);
    CK_PROPERTY(_Weight);
    CK_PROPERTY(_LineHeightPx);
    CK_PROPERTY(_LetterSpacingPx);
    CK_PROPERTY(_HasColor);
    CK_PROPERTY(_Color);
    CK_PROPERTY(_Align);
    CK_PROPERTY(_WhiteSpace);
    CK_PROPERTY(_TransformCase);
    CK_PROPERTY(_LineBoxes);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWEBUMG_API FCk_WebUmg_NodeData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WebUmg_NodeData);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Id;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Tag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _CkName; // data-ck-name; unique per asset (duplicate = hard import error, DECISION 3)

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _CkBind; // data-ck-bind, verbatim (semantics: Gate 5 real-consumer design)

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _CkSlot; // data-ck-slot, verbatim

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _AssetId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_WebUmg_BoxData _Box;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasBoxUntransformed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_WebUmg_BoxData _BoxUntransformed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_WebUmg_LayoutData _Layout;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_WebUmg_PaintData _Paint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasText = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_WebUmg_TextData _Text;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<int32> _ChildIndices;

public:
    CK_PROPERTY(_Id);
    CK_PROPERTY(_Tag);
    CK_PROPERTY(_CkName);
    CK_PROPERTY(_CkBind);
    CK_PROPERTY(_CkSlot);
    CK_PROPERTY(_AssetId);
    CK_PROPERTY(_Box);
    CK_PROPERTY(_HasBoxUntransformed);
    CK_PROPERTY(_BoxUntransformed);
    CK_PROPERTY(_Layout);
    CK_PROPERTY(_Paint);
    CK_PROPERTY(_HasText);
    CK_PROPERTY(_Text);
    CK_PROPERTY(_ChildIndices);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWEBUMG_API FCk_WebUmg_ReportEntryData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WebUmg_ReportEntryData);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _NodeId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Property;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Value;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Source;

public:
    CK_PROPERTY(_NodeId);
    CK_PROPERTY(_Property);
    CK_PROPERTY(_Value);
    CK_PROPERTY(_Source);
};

// --------------------------------------------------------------------------------------------------------------------

/// One imported web page. Node 0 is the root; the tree is child-index encoded.
/// Regeneration (DECISION 3) overwrites the whole node array + textures from the stamped source —
/// a source-hash match makes re-import a no-op (idempotence contract, verified by the round-trip test).
UCLASS(BlueprintType)
class CKWEBUMG_API UCk_WebUmg_PageAsset_UE : public UDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_WebUmg_PageAsset_UE);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _SchemaVersion = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FIntPoint _Viewport = FIntPoint::ZeroValue;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _Browser;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _PageTitle; // the mockup's <title> — display metadata; naming stays the file basename

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _SourceHash; // MD5 of the source ckui.json text — the regeneration stamp

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _SourceJsonPath; // where the bundle came from — reimport re-runs from here

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FString _SourceHtmlPath; // set when imported via the html toolchain; reimport re-extracts

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FCk_WebUmg_NodeData> _Nodes;

    // The conversion report: every author-set declaration the v1 surface dropped, with node id
    // and stylesheet provenance — the no-silent-drops contract, visible on the asset itself.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FCk_WebUmg_ReportEntryData> _ConversionReport;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TMap<FString, TObjectPtr<UTexture2D>> _Textures; // asset id -> imported texture

public:
    CK_PROPERTY(_SchemaVersion);
    CK_PROPERTY(_Viewport);
    CK_PROPERTY(_Browser);
    CK_PROPERTY(_PageTitle);
    CK_PROPERTY(_SourceHash);
    CK_PROPERTY(_SourceJsonPath);
    CK_PROPERTY(_SourceHtmlPath);
    CK_PROPERTY(_Nodes);
    CK_PROPERTY(_ConversionReport);
    CK_PROPERTY(_Textures);
};
