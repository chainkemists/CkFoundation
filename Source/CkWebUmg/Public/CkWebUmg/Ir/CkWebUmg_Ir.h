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
    TOptional<float> AngleDeg;
    TArray<FCkWebUmg_IrGradientStop> Stops;
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
};

// --------------------------------------------------------------------------------------------------------------------

struct CKWEBUMG_API FCkWebUmg_IrNode
{
    FString Id;
    FString Tag;
    FString Asset; // for <img>: id into FCkWebUmg_IrDocument::AssetSourcesById
    FCkWebUmg_IrBox Box;
    FCkWebUmg_IrLayout Layout;
    FCkWebUmg_IrPaint Paint;
    TOptional<FCkWebUmg_IrText> Text;
    TArray<TSharedPtr<FCkWebUmg_IrNode>> Children;
};

// --------------------------------------------------------------------------------------------------------------------

struct CKWEBUMG_API FCkWebUmg_IrDocument
{
    int32 Schema = 0;
    FIntPoint Viewport = FIntPoint::ZeroValue;
    float Dpr = 1.0f;
    FString Browser;
    TMap<FString, FString> AssetSourcesById; // asset id -> src path relative to the page
    TSharedPtr<FCkWebUmg_IrNode> Root;
};
