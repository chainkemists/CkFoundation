#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UTexture;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_TextureExportResult
{
    bool    Succeeded = false;
    FString PngFilePath;   // empty when the texture has no decodable 2D source (cubes, RTs) — metadata only
    FString JsonFilePath;
    FString ErrorMessage;
    FString AssetName;
};

// --------------------------------------------------------------------------------------------------------------------
// Exports a texture for visual analysis: the editor source image decoded to PNG (downscaled to a bounded size —
// the corpus is for looking at shapes/gradients, not for reimport) plus a metadata JSON (dims, format, sRGB,
// compression, LOD group, addressing). Non-2D textures get metadata only.
// --------------------------------------------------------------------------------------------------------------------
class CKASSETEXPORTER_API FCk_TextureExporter
{
public:
    static auto ExportTexture(UTexture* InTexture, const FString& InOutputDir) -> FCk_TextureExportResult;
};

// --------------------------------------------------------------------------------------------------------------------
