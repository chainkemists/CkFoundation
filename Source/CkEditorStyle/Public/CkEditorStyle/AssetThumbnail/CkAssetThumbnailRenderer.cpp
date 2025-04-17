#include "CkAssetThumbnailRenderer.h"

#include "CkCore/Validation/CkIsValid.h"

#include <ThumbnailRendering/ThumbnailManager.h>
#include <Engine/Texture2D.h>
#include <TextureResource.h>
#include <CanvasItem.h>
#include <CanvasTypes.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CustomBlueprintThumbnailRenderer_UE::
    GetThumbnailSize(
        UObject* InObject,
        float InZoom,
        uint32& OutWidth,
        uint32& OutHeight) const
    -> void
{
    if (const auto* Blueprint = Cast<UBlueprint>(InObject);
        ck::IsValid(Blueprint))
    {
        auto BackgroundColor = FLinearColor{};

        if (const auto& ThumbnailIcon = TryGet_ThumbnailFromInterface(Blueprint->GeneratedClass, BackgroundColor);
            ck::IsValid(ThumbnailIcon))
        {
            OutWidth = FMath::TruncToInt(InZoom * ThumbnailIcon->GetSurfaceWidth());
            OutHeight = FMath::TruncToInt(InZoom * ThumbnailIcon->GetSurfaceHeight());
        }
    }

    Super::GetThumbnailSize(InObject, InZoom, OutWidth, OutHeight);
}

auto
    UCk_CustomBlueprintThumbnailRenderer_UE::
    Draw(
        UObject* InObject,
        int32 InX,
        int32 InY,
        uint32 InWidth,
        uint32 InHeight,
        FRenderTarget* InRenderTarget,
        FCanvas* InCanvas,
        bool InAdditionalViewFamily)
    -> void
{
    if (const auto* Blueprint = Cast<UBlueprint>(InObject);
        ck::IsValid(Blueprint))
    {
        auto BackgroundColor = FLinearColor{};

        if (const auto& ThumbnailIcon = TryGet_ThumbnailFromInterface(Blueprint->GeneratedClass, BackgroundColor);
            ck::IsValid(ThumbnailIcon))
        {
            InCanvas->DrawTile(0.0f, 0.0f, InWidth, InHeight, 0.0f, 0.0f, 1.0f, 1.0f, BackgroundColor, nullptr);
            InCanvas->DrawTile(InX, InY, InWidth, InHeight, 0.0f, 0.0f, 1.0f, 1.0f, FLinearColor::White, ThumbnailIcon->GetResource(), true);
            return;
        }
    }

    Super::Draw(InObject, InX, InY, InWidth, InHeight, InRenderTarget, InCanvas, InAdditionalViewFamily);
}

auto
    UCk_CustomBlueprintThumbnailRenderer_UE::
    CanVisualizeAsset(
        UObject* InObject)
    -> bool
{
    if (Super::CanVisualizeAsset(InObject))
    { return true; }

    const auto& Blueprint = Cast<UBlueprint>(InObject);

    if (ck::Is_NOT_Valid(Blueprint))
    { return {}; }

    auto BackgroundColor = FLinearColor{};
    const auto& ThumbnailIcon = TryGet_ThumbnailFromInterface(Blueprint->GeneratedClass, BackgroundColor);

    return ck::IsValid(ThumbnailIcon);
}

auto
    UCk_CustomBlueprintThumbnailRenderer_UE::
    TryGet_ThumbnailFromInterface(
        UClass* InClass,
        FLinearColor& OutBackgroundColor)
    -> UTexture2D*
{
    if (ck::Is_NOT_Valid(InClass))
    { return {}; }

    if (const auto* CDO = InClass->GetDefaultObject();
        ck::IsValid(CDO))
    {
        if (CDO->Implements<UCk_CustomAssetThumbnail_Interface>())
        {
            return ICk_CustomAssetThumbnail_Interface::Execute_Get_ThumbnailIconAndColor(CDO, OutBackgroundColor);
        }
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CustomDataAssetThumbnailRenderer_UE::
    GetThumbnailSize(
        UObject* InObject,
        float InZoom,
        uint32& OutWidth,
        uint32& OutHeight) const
    -> void
{
    OutWidth = 64;
    OutHeight = 64;

    Super::GetThumbnailSize(InObject, 0.2f, OutWidth, OutHeight);
}

auto
    UCk_CustomDataAssetThumbnailRenderer_UE::
    Draw(
        UObject* InObject,
        int32 InX,
        int32 InY,
        uint32 InWidth,
        uint32 InHeight,
        FRenderTarget* InRenderTarget,
        FCanvas* InCanvas,
        bool InAdditionalViewFamily)
    -> void
{
    auto BackgroundColor = FLinearColor{};
    if (const auto& ThumbnailIcon = TryGet_ThumbnailFromInterface(Cast<UDataAsset>(InObject), BackgroundColor);
        ck::IsValid(ThumbnailIcon))
    {
        const auto UseTranslucentBlend = ThumbnailIcon->HasAlphaChannel() && (ThumbnailIcon->LODGroup == TEXTUREGROUP_UI || ThumbnailIcon->LODGroup == TEXTUREGROUP_Pixels2D);
        TRefCountPtr<FBatchedElementParameters> BatchedElementParameters;

        if (UseTranslucentBlend)
        {
            constexpr int32 CheckerDensity = 1;
            auto* Checker = UThumbnailManager::Get().CheckerboardTexture.Get();

            InCanvas->DrawTile(InX, InY, InWidth, InHeight, 0.0f, 0.0f, CheckerDensity, CheckerDensity, BackgroundColor, Checker->GetResource());
        }

        InCanvas->DrawTile(InX, InY, InWidth, InHeight, 0.0f, 0.0f, 1.0f, 1.0f, FLinearColor::White, ThumbnailIcon->GetResource(), true);
        return;
    }

    Super::Draw(InObject, InX, InY, InWidth, InHeight, InRenderTarget, InCanvas, InAdditionalViewFamily);
}

auto
    UCk_CustomDataAssetThumbnailRenderer_UE::
    CanVisualizeAsset(
        UObject* InObject)
    -> bool
{
    if (Super::CanVisualizeAsset(InObject))
    { return true; }

    if (const auto* DataAsset = Cast<UDataAsset>(InObject);
        ck::IsValid(DataAsset) && DataAsset->Implements<UCk_CustomAssetThumbnail_Interface>())
    { return true; }

    return {};
}

auto
    UCk_CustomDataAssetThumbnailRenderer_UE::
    TryGet_ThumbnailFromInterface(
        const UDataAsset* InDataAsset,
        FLinearColor& OutBackgroundColor)
    -> UTexture2D*
{
    if (InDataAsset->Implements<UCk_CustomAssetThumbnail_Interface>())
    {
        return ICk_CustomAssetThumbnail_Interface::Execute_Get_ThumbnailIconAndColor(InDataAsset, OutBackgroundColor);
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------
