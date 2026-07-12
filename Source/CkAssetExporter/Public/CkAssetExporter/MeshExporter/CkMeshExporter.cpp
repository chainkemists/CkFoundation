#include "CkMeshExporter.h"

#include "CkAssetExporter_Log.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/Paths.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::asset_exporter::mesh
{
    // VFX carrier meshes are tiny (sweeps/cones/shells run well under 10k verts). Anything past this cap is not a
    // recipe-relevant carrier — still record its stats, but skip the OBJ body.
    constexpr auto MaxObjVertices = int32{100000};

    static auto ResolveOutputPath(
        const UStaticMesh* InMesh,
        const FString&     InExtension,
        const FString&     InOutputDir)
        -> FString
    {
        if (NOT InOutputDir.IsEmpty())
        {
            IFileManager::Get().MakeDirectory(*InOutputDir, true);
            return FPaths::Combine(InOutputDir, InMesh->GetName() + InExtension);
        }

        const auto PackagePath = InMesh->GetOutermost()->GetName();
        const auto AbsolutePath = FPackageName::LongPackageNameToFilename(PackagePath, InExtension);
        return AbsolutePath;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_MeshExporter::
    ExportStaticMesh(
        UStaticMesh* InMesh,
        const FString& InOutputDir)
    -> FCk_MeshExportResult
{
    auto Result = FCk_MeshExportResult{};

    if (InMesh == nullptr)
    {
        Result.ErrorMessage = TEXT("Invalid StaticMesh asset");
        return Result;
    }

    Result.AssetName = InMesh->GetName();

    const auto* RenderData = InMesh->GetRenderData();
    if (RenderData == nullptr || RenderData->LODResources.Num() == 0)
    {
        Result.ErrorMessage = TEXT("StaticMesh has no render data (LOD 0 missing)");
        return Result;
    }

    const auto& Lod = RenderData->LODResources[0];
    const auto& Positions = Lod.VertexBuffers.PositionVertexBuffer;
    const auto& VertexData = Lod.VertexBuffers.StaticMeshVertexBuffer;

    const auto NumVertices = static_cast<int32>(Positions.GetNumVertices());
    const auto NumTexCoords = static_cast<int32>(VertexData.GetNumTexCoords());

    auto Indices = TArray<uint32>{};
    Lod.IndexBuffer.GetCopy(Indices);
    const auto NumTriangles = Indices.Num() / 3;

    // ---- Stats JSON ----
    const auto Bounds = InMesh->GetBoundingBox();

    auto UvMin = FVector2f(FLT_MAX, FLT_MAX);
    auto UvMax = FVector2f(-FLT_MAX, -FLT_MAX);
    if (NumTexCoords > 0)
    {
        for (auto Index = 0; Index < NumVertices; ++Index)
        {
            const auto Uv = VertexData.GetVertexUV(Index, 0);
            UvMin = FVector2f(FMath::Min(UvMin.X, Uv.X), FMath::Min(UvMin.Y, Uv.Y));
            UvMax = FVector2f(FMath::Max(UvMax.X, Uv.X), FMath::Max(UvMax.Y, Uv.Y));
        }
    }

    const auto& StaticMaterials = InMesh->GetStaticMaterials();
    const auto Get_SlotName = [&](int32 InMaterialIndex) -> FString
    {
        return StaticMaterials.IsValidIndex(InMaterialIndex)
            ? StaticMaterials[InMaterialIndex].MaterialSlotName.ToString()
            : ck::Format_UE(TEXT("Slot{}"), InMaterialIndex);
    };

    const auto Json = MakeShared<FJsonObject>();
    Json->SetStringField(TEXT("mesh"), InMesh->GetName());
    Json->SetStringField(TEXT("packagePath"), InMesh->GetOutermost()->GetName());
    Json->SetNumberField(TEXT("numVertices"), NumVertices);
    Json->SetNumberField(TEXT("numTriangles"), NumTriangles);
    Json->SetNumberField(TEXT("numTexCoords"), NumTexCoords);
    Json->SetStringField(TEXT("boundsMin"), ck::Format_UE(TEXT("({}, {}, {})"), Bounds.Min.X, Bounds.Min.Y, Bounds.Min.Z));
    Json->SetStringField(TEXT("boundsMax"), ck::Format_UE(TEXT("({}, {}, {})"), Bounds.Max.X, Bounds.Max.Y, Bounds.Max.Z));
    const auto BoundsSize = Bounds.GetSize();
    Json->SetStringField(TEXT("boundsSize"), ck::Format_UE(TEXT("({}, {}, {})"), BoundsSize.X, BoundsSize.Y, BoundsSize.Z));
    if (NumTexCoords > 0)
    {
        Json->SetStringField(TEXT("uv0Min"), ck::Format_UE(TEXT("({}, {})"), UvMin.X, UvMin.Y));
        Json->SetStringField(TEXT("uv0Max"), ck::Format_UE(TEXT("({}, {})"), UvMax.X, UvMax.Y));
    }

    auto SectionsArr = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto& Section : Lod.Sections)
    {
        const auto SectionObj = MakeShared<FJsonObject>();
        SectionObj->SetStringField(TEXT("materialSlot"), Get_SlotName(Section.MaterialIndex));
        SectionObj->SetStringField(TEXT("material"),
            StaticMaterials.IsValidIndex(Section.MaterialIndex) && StaticMaterials[Section.MaterialIndex].MaterialInterface != nullptr
                ? StaticMaterials[Section.MaterialIndex].MaterialInterface->GetPathName()
                : FString(TEXT("<none>")));
        SectionObj->SetNumberField(TEXT("numTriangles"), static_cast<int32>(Section.NumTriangles));
        SectionsArr.Add(MakeShared<FJsonValueObject>(SectionObj));
    }
    Json->SetArrayField(TEXT("sections"), SectionsArr);

    const auto bWriteObj = NumVertices > 0 && NumVertices <= ck::asset_exporter::mesh::MaxObjVertices;
    if (NOT bWriteObj)
    {
        Json->SetStringField(TEXT("objSkipped"),
            ck::Format_UE(TEXT("vertex count {} exceeds OBJ cap {} (or mesh is empty)"),
                NumVertices, ck::asset_exporter::mesh::MaxObjVertices));
    }

    // ---- OBJ (positions / normals / UV0 share the render-data vertex indexing, so faces are i/i/i) ----
    if (bWriteObj)
    {
        auto Obj = FString{};
        Obj.Reserve(NumVertices * 96 + NumTriangles * 32);
        Obj += ck::Format_UE(TEXT("# CkMeshExporter — LOD0 render geometry of {}\n"), InMesh->GetName());
        Obj += TEXT("# UVs are in UE convention (V is DOWN); no flip applied.\n");
        Obj += ck::Format_UE(TEXT("o {}\n"), InMesh->GetName());

        for (auto Index = 0; Index < NumVertices; ++Index)
        {
            const auto Pos = Positions.VertexPosition(Index);
            Obj += FString::Printf(TEXT("v %g %g %g\n"), Pos.X, Pos.Y, Pos.Z);
        }
        if (NumTexCoords > 0)
        {
            for (auto Index = 0; Index < NumVertices; ++Index)
            {
                const auto Uv = VertexData.GetVertexUV(Index, 0);
                Obj += FString::Printf(TEXT("vt %g %g\n"), Uv.X, Uv.Y);
            }
        }
        for (auto Index = 0; Index < NumVertices; ++Index)
        {
            const auto Normal = VertexData.VertexTangentZ(Index);
            Obj += FString::Printf(TEXT("vn %g %g %g\n"), Normal.X, Normal.Y, Normal.Z);
        }

        for (const auto& Section : Lod.Sections)
        {
            Obj += ck::Format_UE(TEXT("usemtl {}\n"), Get_SlotName(Section.MaterialIndex));
            const auto FirstIndex = static_cast<int32>(Section.FirstIndex);
            const auto LastIndex = FirstIndex + static_cast<int32>(Section.NumTriangles) * 3;
            for (auto Index = FirstIndex; Index + 2 < LastIndex && Index + 2 < Indices.Num(); Index += 3)
            {
                const auto A = Indices[Index] + 1;
                const auto B = Indices[Index + 1] + 1;
                const auto C = Indices[Index + 2] + 1;
                Obj += NumTexCoords > 0
                    ? FString::Printf(TEXT("f %u/%u/%u %u/%u/%u %u/%u/%u\n"), A, A, A, B, B, B, C, C, C)
                    : FString::Printf(TEXT("f %u//%u %u//%u %u//%u\n"), A, A, B, B, C, C);
            }
        }

        Result.ObjFilePath = ck::asset_exporter::mesh::ResolveOutputPath(InMesh, TEXT(".obj"), InOutputDir);
        if (NOT FFileHelper::SaveStringToFile(Obj, *Result.ObjFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        {
            Result.ErrorMessage = ck::Format_UE(TEXT("Failed to write OBJ [{}]"), Result.ObjFilePath);
            return Result;
        }
        Json->SetStringField(TEXT("obj"), FPaths::GetCleanFilename(Result.ObjFilePath));
    }

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(Json, JsonWriter);

    Result.JsonFilePath = ck::asset_exporter::mesh::ResolveOutputPath(InMesh, TEXT(".json"), InOutputDir);
    if (NOT FFileHelper::SaveStringToFile(JsonString, *Result.JsonFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        Result.ErrorMessage = ck::Format_UE(TEXT("Failed to write JSON [{}]"), Result.JsonFilePath);
        return Result;
    }

    Result.Succeeded = true;
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
