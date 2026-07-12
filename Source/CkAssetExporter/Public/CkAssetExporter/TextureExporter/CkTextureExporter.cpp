#include "CkTextureExporter.h"

#include "CkAssetExporter_Log.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"

#include <Dom/JsonObject.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter::texture
{
    // The corpus PNGs exist to be looked at, not reimported — bound the longest side so 4K sheets don't bloat
    // the corpus (metadata keeps the true dimensions).
    constexpr auto MaxPngSize = int32{1024};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_TextureExporter::
    ExportTexture(
        UTexture* InTexture,
        const FString& InOutputDir)
    -> FCk_TextureExportResult
{
    auto Result = FCk_TextureExportResult{};
    if (ck::Is_NOT_Valid(InTexture))
    {
        Result.ErrorMessage = TEXT("Invalid Texture asset");
        return Result;
    }

    Result.AssetName = InTexture->GetName();

    constexpr auto CreateTree = true;
    IFileManager::Get().MakeDirectory(*InOutputDir, CreateTree);

    auto Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("texture"), InTexture->GetName());
    Root->SetStringField(TEXT("packagePath"), InTexture->GetOutermost()->GetName());
    Root->SetStringField(TEXT("class"), InTexture->GetClass()->GetName());
    Root->SetBoolField(TEXT("sRGB"), InTexture->SRGB != 0);
    Root->SetStringField(TEXT("compression"), StaticEnum<TextureCompressionSettings>()->GetNameStringByValue(static_cast<int64>(InTexture->CompressionSettings)));
    Root->SetStringField(TEXT("lodGroup"), StaticEnum<TextureGroup>()->GetNameStringByValue(static_cast<int64>(InTexture->LODGroup)));

    if (const auto* Texture2D = Cast<UTexture2D>(InTexture))
    {
        Root->SetStringField(TEXT("addressX"), StaticEnum<TextureAddress>()->GetNameStringByValue(static_cast<int64>(Texture2D->AddressX)));
        Root->SetStringField(TEXT("addressY"), StaticEnum<TextureAddress>()->GetNameStringByValue(static_cast<int64>(Texture2D->AddressY)));
    }

#if WITH_EDITORONLY_DATA
    auto& Source = InTexture->Source;
    if (Source.IsValid())
    {
        Root->SetNumberField(TEXT("width"), Source.GetSizeX());
        Root->SetNumberField(TEXT("height"), Source.GetSizeY());
        Root->SetStringField(TEXT("sourceFormat"), StaticEnum<ETextureSourceFormat>()->GetNameStringByValue(static_cast<int64>(Source.GetFormat())));

        // Decode mip 0 and write the PNG (2D textures only; cubes/arrays keep metadata-only).
        if (Cast<UTexture2D>(InTexture) != nullptr)
        {
            auto Image = FImage{};
            constexpr auto Mip0 = 0;
            if (Source.GetMipImage(Image, Mip0))
            {
                const auto MaxSide = FMath::Max(Image.SizeX, Image.SizeY);
                const auto Gamma = InTexture->SRGB ? EGammaSpace::sRGB : EGammaSpace::Linear;

                auto Bgra = FImage{};
                if (MaxSide > ck::asset_exporter::texture::MaxPngSize)
                {
                    const auto Scale = static_cast<float>(ck::asset_exporter::texture::MaxPngSize) / static_cast<float>(MaxSide);
                    const auto NewX = FMath::Max(1, FMath::RoundToInt(Image.SizeX * Scale));
                    const auto NewY = FMath::Max(1, FMath::RoundToInt(Image.SizeY * Scale));
                    Image.ResizeTo(Bgra, NewX, NewY, ERawImageFormat::BGRA8, Gamma);
                    Root->SetBoolField(TEXT("downscaled"), true);
                }
                else
                { Image.CopyTo(Bgra, ERawImageFormat::BGRA8, Gamma); }

                const auto PngPath = FPaths::Combine(InOutputDir, InTexture->GetName() + TEXT(".png"));
                if (FImageUtils::SaveImageByExtension(*PngPath, Bgra))
                {
                    Result.PngFilePath = PngPath;
                    Root->SetStringField(TEXT("png"), FPaths::GetCleanFilename(PngPath));
                }
                else
                { Root->SetStringField(TEXT("pngError"), TEXT("SaveImageByExtension failed")); }
            }
            else
            { Root->SetStringField(TEXT("pngError"), TEXT("GetMipImage failed")); }
        }
    }
    else
    { Root->SetStringField(TEXT("pngError"), TEXT("no editor source data")); }
#endif

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(Root, JsonWriter);

    const auto JsonPath = FPaths::Combine(InOutputDir, InTexture->GetName() + TEXT(".json"));
    if (NOT FFileHelper::SaveStringToFile(JsonString, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        Result.ErrorMessage = ck::Format_UE(TEXT("Failed to write [{}]"), JsonPath);
        return Result;
    }

    Result.Succeeded = true;
    Result.JsonFilePath = JsonPath;
    return Result;
}
