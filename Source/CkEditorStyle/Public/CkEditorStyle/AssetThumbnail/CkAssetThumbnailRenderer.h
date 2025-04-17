#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <ThumbnailRendering/BlueprintThumbnailRenderer.h>
#include <Engine/DataAsset.h>

#include "CkAssetThumbnailRenderer.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(Blueprintable)
class UCk_CustomAssetThumbnail_Interface : public UInterface { GENERATED_BODY() };
class CKEDITORSTYLE_API ICk_CustomAssetThumbnail_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ICk_CustomAssetThumbnail_Interface);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
        Category = "Ck|EditorStyle|AssetThumbnail")
    UTexture2D*
    Get_ThumbnailIconAndColor(
        FLinearColor& OutBackgroundColor) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class UCk_CustomBlueprintThumbnailRenderer_UE : public UBlueprintThumbnailRenderer
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CustomBlueprintThumbnailRenderer_UE);

private:
    auto GetThumbnailSize(UObject* InObject, float InZoom, uint32& OutWidth, uint32& OutHeight) const -> void override;
    auto Draw(UObject* InObject, int32 InX, int32 InY, uint32 InWidth, uint32 InHeight, FRenderTarget* InRenderTarget, FCanvas* InCanvas, bool InAdditionalViewFamily) -> void override;
    auto CanVisualizeAsset(UObject* InObject) -> bool override;

protected:
    static auto TryGet_ThumbnailFromInterface(UClass* InClass, FLinearColor& OutBackgroundColor) -> UTexture2D*;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class UCk_CustomDataAssetThumbnailRenderer_UE : public UBlueprintThumbnailRenderer
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CustomDataAssetThumbnailRenderer_UE);

private:
    auto GetThumbnailSize(UObject* InObject, float InZoom, uint32& OutWidth, uint32& OutHeight) const -> void override;
    auto Draw(UObject* InObject, int32 InX, int32 InY, uint32 InWidth, uint32 InHeight, FRenderTarget* InRenderTarget, FCanvas* InCanvas, bool InAdditionalViewFamily) -> void override;
    auto CanVisualizeAsset(UObject* InObject) -> bool override;

protected:
    static auto TryGet_ThumbnailFromInterface(
        const UDataAsset* InDataAsset, FLinearColor& OutBackgroundColor) -> UTexture2D*;
};

// --------------------------------------------------------------------------------------------------------------------
