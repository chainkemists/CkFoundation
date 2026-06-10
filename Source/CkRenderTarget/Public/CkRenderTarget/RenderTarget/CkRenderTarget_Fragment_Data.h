#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <Engine/Canvas.h>
#include <GameplayTags.h>
#include <NativeGameplayTags.h>

#include "CkRenderTarget_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

CKRENDERTARGET_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_RenderTarget_CategoryName);

// --------------------------------------------------------------------------------------------------------------------

class UTexture;
class UTextureRenderTarget2D;
class UMaterialInterface;
class UFont;
class APlayerState;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RenderTarget_TargetMode : uint8
{
    // The module creates and owns a transient UTextureRenderTarget2D from _Size (RGBA8)
    CreateManaged,
    // The caller supplies an existing render target asset/object (must be RTF_RGBA8 in v1)
    UseProvided
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RenderTarget_TargetMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RenderTarget_SyncMode : uint8
{
    // Replicate draw instructions only — cheapest, requires all draw inputs to exist on clients
    InstructionsOnly,
    // Replicate pixel payloads only (FullSync baseline + block deltas)
    Pixels,
    // Instructions for responsiveness + periodic pixel reconciliation
    Hybrid
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RenderTarget_SyncMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RenderTarget_PixelSyncPolicy : uint8
{
    // Pixel syncs happen only via explicit Request_SyncPixels
    Manual,
    // Pixel syncs are kicked automatically every _SyncInterval
    Interval
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RenderTarget_PixelSyncPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RenderTarget_ClientAuthoring : uint8
{
    // Clients may neither draw nor upload pixels — server is the sole author
    Disallowed,
    // Clients may draw (predicted locally, pushed to the server) and upload pixels
    Allowed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RenderTarget_ClientAuthoring);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RenderTarget_DrawCmdType : uint8
{
    Line,
    Texture,
    Material,
    Text,
    Box,
    Clear,
    Border,
    Triangles,
    Polygon
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RenderTarget_DrawCmdType);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RenderTarget_PixelPayloadKind : uint8
{
    // Whole-image baseline — replaces the receiver's staging buffer
    FullSync,
    // Changed 16x16 blocks only — patched onto an established baseline
    Delta
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_RenderTarget_PixelPayloadKind);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKRENDERTARGET_API FCk_Handle_RenderTarget : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_RenderTarget);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_RenderTarget);

// --------------------------------------------------------------------------------------------------------------------

// Wire/storage form of a single draw operation. Requests normalize into this flat struct so a
// batch replicates as one homogeneous array (no per-cmd FInstancedStruct overhead). Field usage
// per _Type:
//
//   Line      — _PositionA (start), _PositionB (end), _Thickness, _Color
//   Texture   — _Asset (UTexture), _PositionA, _Size, _CoordinatePosition/_CoordinateSize (UVs),
//               _Color, _Rotation
//   Material  — _Asset (UMaterialInterface), _PositionA, _Size, _Rotation
//   Text      — _Asset (UFont, optional), _Text, _PositionA, _Size (scale), _Color
//   Box       — _PositionA, _Size, _Thickness, _Color
//   Clear     — _Color
//   Border    — _Asset (border UTexture), _ExtraAssets[0..4] (background, left, right, top,
//               bottom UTextures), _PositionA, _Size, _CoordinatePosition/_CoordinateSize,
//               _Color, _Rotation
//   Triangles — _Asset (UTexture), _Triangles
//   Polygon   — _Asset (UTexture, optional), _PositionA, _Size (radius), _Sides, _Color
//
// _Asset replicates as an asset reference — the object must exist on clients. Runtime-created
// textures are unsupported in instructions (ensured against when the batch is published).
USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_RenderTarget_DrawCmd
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RenderTarget_DrawCmd);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_RenderTarget_DrawCmdType _Type = ECk_RenderTarget_DrawCmdType::Line;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _PositionA = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _PositionB = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Size = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _CoordinatePosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _CoordinateSize = FVector2D::UnitVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FLinearColor _Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _Rotation = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _Thickness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FString _Text;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UObject> _Asset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TArray<TObjectPtr<UObject>> _ExtraAssets;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TArray<FCanvasUVTri> _Triangles;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    int32 _Sides = 3;

public:
    CK_PROPERTY(_Type);
    CK_PROPERTY(_PositionA);
    CK_PROPERTY(_PositionB);
    CK_PROPERTY(_Size);
    CK_PROPERTY(_CoordinatePosition);
    CK_PROPERTY(_CoordinateSize);
    CK_PROPERTY(_Color);
    CK_PROPERTY(_Rotation);
    CK_PROPERTY(_Thickness);
    CK_PROPERTY(_Text);
    CK_PROPERTY(_Asset);
    CK_PROPERTY(_ExtraAssets);
    CK_PROPERTY(_Triangles);
    CK_PROPERTY(_Sides);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_Fragment_RenderTarget_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_RenderTarget_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Categories = "RenderTarget"))
    FGameplayTag _SyncName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_RenderTarget_TargetMode _TargetMode = ECk_RenderTarget_TargetMode::CreateManaged;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true,
                EditCondition = "_TargetMode == ECk_RenderTarget_TargetMode::UseProvided"))
    TObjectPtr<UTextureRenderTarget2D> _ProvidedTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true,
                EditCondition = "_TargetMode == ECk_RenderTarget_TargetMode::CreateManaged"))
    FIntPoint _Size = FIntPoint(512, 512);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_RenderTarget_SyncMode _SyncMode = ECk_RenderTarget_SyncMode::InstructionsOnly;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_RenderTarget_PixelSyncPolicy _PixelSyncPolicy = ECk_RenderTarget_PixelSyncPolicy::Manual;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true,
                EditCondition = "_PixelSyncPolicy == ECk_RenderTarget_PixelSyncPolicy::Interval"))
    FCk_Time _SyncInterval;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_RenderTarget_ClientAuthoring _ClientAuthoring = ECk_RenderTarget_ClientAuthoring::Disallowed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_Replication _Replication = ECk_Replication::Replicates;

public:
    CK_PROPERTY_GET(_SyncName);
    CK_PROPERTY(_TargetMode);
    CK_PROPERTY(_ProvidedTarget);
    CK_PROPERTY(_Size);
    CK_PROPERTY(_SyncMode);
    CK_PROPERTY(_PixelSyncPolicy);
    CK_PROPERTY(_SyncInterval);
    CK_PROPERTY(_ClientAuthoring);
    CK_PROPERTY(_Replication);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_RenderTarget_ParamsData, _SyncName);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_Request_RenderTarget_DrawLine : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RenderTarget_DrawLine);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RenderTarget_DrawLine);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Start = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _End = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _Thickness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FLinearColor _Color = FLinearColor::White;

public:
    CK_PROPERTY_GET(_Start);
    CK_PROPERTY_GET(_End);
    CK_PROPERTY(_Thickness);
    CK_PROPERTY(_Color);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RenderTarget_DrawLine, _Start, _End);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_Request_RenderTarget_DrawTexture : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RenderTarget_DrawTexture);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RenderTarget_DrawTexture);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture> _Texture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Position = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Size = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _CoordinatePosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _CoordinateSize = FVector2D::UnitVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FLinearColor _Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _Rotation = 0.0f;

public:
    CK_PROPERTY_GET(_Texture);
    CK_PROPERTY_GET(_Position);
    CK_PROPERTY_GET(_Size);
    CK_PROPERTY(_CoordinatePosition);
    CK_PROPERTY(_CoordinateSize);
    CK_PROPERTY(_Color);
    CK_PROPERTY(_Rotation);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RenderTarget_DrawTexture, _Texture, _Position, _Size);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_Request_RenderTarget_DrawMaterial : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RenderTarget_DrawMaterial);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RenderTarget_DrawMaterial);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UMaterialInterface> _Material;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Position = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Size = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _Rotation = 0.0f;

public:
    CK_PROPERTY_GET(_Material);
    CK_PROPERTY_GET(_Position);
    CK_PROPERTY_GET(_Size);
    CK_PROPERTY(_Rotation);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RenderTarget_DrawMaterial, _Material, _Position, _Size);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_Request_RenderTarget_DrawText : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RenderTarget_DrawText);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RenderTarget_DrawText);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FString _Text;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Position = FVector2D::ZeroVector;

    // Optional — engine default (tiny) font is used when unset
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UFont> _Font;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Scale = FVector2D::UnitVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FLinearColor _Color = FLinearColor::White;

public:
    CK_PROPERTY_GET(_Text);
    CK_PROPERTY_GET(_Position);
    CK_PROPERTY(_Font);
    CK_PROPERTY(_Scale);
    CK_PROPERTY(_Color);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RenderTarget_DrawText, _Text, _Position);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_Request_RenderTarget_DrawBox : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RenderTarget_DrawBox);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RenderTarget_DrawBox);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Position = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Size = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _Thickness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FLinearColor _Color = FLinearColor::White;

public:
    CK_PROPERTY_GET(_Position);
    CK_PROPERTY_GET(_Size);
    CK_PROPERTY(_Thickness);
    CK_PROPERTY(_Color);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RenderTarget_DrawBox, _Position, _Size);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_Request_RenderTarget_Clear : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RenderTarget_Clear);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RenderTarget_Clear);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FLinearColor _ClearColor = FLinearColor::Black;

public:
    CK_PROPERTY(_ClearColor);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_Request_RenderTarget_DrawBorder : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RenderTarget_DrawBorder);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RenderTarget_DrawBorder);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture> _BorderTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture> _BackgroundTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture> _LeftBorderTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture> _RightBorderTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture> _TopBorderTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture> _BottomBorderTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Position = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Size = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _CoordinatePosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _CoordinateSize = FVector2D::UnitVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FLinearColor _Color = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _Rotation = 0.0f;

public:
    CK_PROPERTY_GET(_BorderTexture);
    CK_PROPERTY_GET(_BackgroundTexture);
    CK_PROPERTY_GET(_LeftBorderTexture);
    CK_PROPERTY_GET(_RightBorderTexture);
    CK_PROPERTY_GET(_TopBorderTexture);
    CK_PROPERTY_GET(_BottomBorderTexture);
    CK_PROPERTY_GET(_Position);
    CK_PROPERTY_GET(_Size);
    CK_PROPERTY(_CoordinatePosition);
    CK_PROPERTY(_CoordinateSize);
    CK_PROPERTY(_Color);
    CK_PROPERTY(_Rotation);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RenderTarget_DrawBorder,
        _BorderTexture, _BackgroundTexture, _LeftBorderTexture, _RightBorderTexture,
        _TopBorderTexture, _BottomBorderTexture, _Position, _Size);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_Request_RenderTarget_DrawTriangles : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RenderTarget_DrawTriangles);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RenderTarget_DrawTriangles);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture> _Texture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TArray<FCanvasUVTri> _Triangles;

public:
    CK_PROPERTY_GET(_Texture);
    CK_PROPERTY_GET(_Triangles);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RenderTarget_DrawTriangles, _Texture, _Triangles);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_Request_RenderTarget_DrawPolygon : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RenderTarget_DrawPolygon);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RenderTarget_DrawPolygon);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Position = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _Radius = FVector2D::UnitVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, ClampMin = "3"))
    int32 _NumberOfSides = 3;

    // Optional — null draws with the engine's default white texture
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture> _Texture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FLinearColor _Color = FLinearColor::White;

public:
    CK_PROPERTY_GET(_Position);
    CK_PROPERTY_GET(_Radius);
    CK_PROPERTY_GET(_NumberOfSides);
    CK_PROPERTY(_Texture);
    CK_PROPERTY(_Color);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RenderTarget_DrawPolygon, _Position, _Radius, _NumberOfSides);
};

// --------------------------------------------------------------------------------------------------------------------

// Triggers one server-side pixel capture → diff → compress pass (the Manual pixel-sync policy's
// entry point). No-ops when a capture is already in flight for this sync entity.
USTRUCT(BlueprintType)
struct CKRENDERTARGET_API FCk_Request_RenderTarget_SyncPixels : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RenderTarget_SyncPixels);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RenderTarget_SyncPixels);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_RenderTarget_OnInstructionsApplied,
    FCk_Handle_RenderTarget, InHandle,
    int32, InBatchSeq,
    int32, InNumCmds);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_RenderTarget_OnPixelPayloadProduced,
    FCk_Handle_RenderTarget, InHandle,
    ECk_RenderTarget_PixelPayloadKind, InKind,
    int32, InPayloadSeq);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_RenderTarget_OnPixelPayloadApplied,
    FCk_Handle_RenderTarget, InHandle,
    ECk_RenderTarget_PixelPayloadKind, InKind,
    int32, InPayloadSeq);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_RenderTarget_OnClientBaselineEstablished,
    FCk_Handle_RenderTarget, InHandle,
    APlayerState*, InPlayer);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_RenderTarget_OnClientUploadApplied,
    FCk_Handle_RenderTarget, InHandle,
    APlayerState*, InSender);

// --------------------------------------------------------------------------------------------------------------------
