#include "CkParticlesEditor/Generator/CkParticles_MeshGenerator.h"

#include "CkParticlesEditor_Log.h"

#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::particles_editor::MeshGenLocal
{
    static const TCHAR* MeshDir = TEXT("/CkFoundation/CkParticles/Meshes");

    struct FGridPoint
    {
        FVector3f Position = FVector3f::ZeroVector;
        FVector2f Uv       = FVector2f::ZeroVector;
    };

    // (S, T) in [0,1]^2 -> position + UV. S maps to U (grid columns), T maps to V (grid rows).
    using FSurfaceFn = FGridPoint (*)(float, float);

    // Fills a MeshDescription with a (InCols x InRows)-cell parametric grid — shared vertex instances, one
    // polygon group whose material slot is named "VfxMaterial" (the slot the static-mesh material fills).
    static auto Build_GridDescription(
        FMeshDescription& OutDescription,
        FSurfaceFn        InSurface,
        int32             InCols,
        int32             InRows) -> void
    {
        auto Attributes = FStaticMeshAttributes{OutDescription};
        Attributes.Register();

        const auto Group = OutDescription.CreatePolygonGroup();
        Attributes.GetPolygonGroupMaterialSlotNames()[Group] = TEXT("VfxMaterial");

        auto Positions = Attributes.GetVertexPositions();
        auto Uvs = Attributes.GetVertexInstanceUVs();

        auto Instances = TArray<FVertexInstanceID>{};
        Instances.Reserve((InCols + 1) * (InRows + 1));

        for (auto Row = 0; Row <= InRows; ++Row)
        {
            for (auto Col = 0; Col <= InCols; ++Col)
            {
                const auto Point = InSurface(static_cast<float>(Col) / InCols, static_cast<float>(Row) / InRows);
                const auto Vertex = OutDescription.CreateVertex();
                Positions[Vertex] = Point.Position;

                const auto Instance = OutDescription.CreateVertexInstance(Vertex);
                Uvs.Set(Instance, 0, Point.Uv);
                Instances.Add(Instance);
            }
        }

        const auto Stride = InCols + 1;
        for (auto Row = 0; Row < InRows; ++Row)
        {
            for (auto Col = 0; Col < InCols; ++Col)
            {
                const auto A = Instances[Row * Stride + Col];
                const auto B = Instances[Row * Stride + Col + 1];
                const auto C = Instances[(Row + 1) * Stride + Col];
                const auto D = Instances[(Row + 1) * Stride + Col + 1];

                OutDescription.CreatePolygon(Group, TArray<FVertexInstanceID>{A, B, C});
                OutDescription.CreatePolygon(Group, TArray<FVertexInstanceID>{C, B, D});
            }
        }
    }

    static auto Bake_Mesh(
        const TCHAR* InName,
        FSurfaceFn   InSurface,
        int32        InCols,
        int32        InRows,
        const FName  InMasterMaterialName) -> bool
    {
        const FString PkgPath = FString::Printf(TEXT("%s/%s"), MeshDir, InName);

        UPackage* Package = FPackageName::DoesPackageExist(PkgPath)
            ? LoadPackage(nullptr, *PkgPath, LOAD_None)
            : nullptr;
        if (Package == nullptr) { Package = CreatePackage(*PkgPath); }
        if (Package == nullptr) { return false; }

        if (auto* Old = StaticFindObject(UStaticMesh::StaticClass(), Package, InName))
        {
            Old->ClearFlags(RF_Standalone | RF_Public);
            Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
        }

        auto* Mesh = NewObject<UStaticMesh>(Package, InName, RF_Public | RF_Standalone);
        if (Mesh == nullptr) { return false; }

        auto Description = FMeshDescription{};
        Build_GridDescription(Description, InSurface, InCols, InRows);

        FStaticMeshOperations::ComputeTriangleTangentsAndNormals(Description);
        FStaticMeshOperations::ComputeTangentsAndNormals(Description,
            EComputeNTBsFlags::Normals | EComputeNTBsFlags::Tangents);

        auto* Material = LoadObject<UMaterialInterface>(nullptr,
            *ck::particles::Get_VfxMasterMaterialObjectPath(InMasterMaterialName));
        if (Material == nullptr)
        { Log(TEXT("VFX mesh [{}]: master material [{}] not found — slot left empty"), FString(InName), InMasterMaterialName); }
        Mesh->SetStaticMaterials({ FStaticMaterial(Material, TEXT("VfxMaterial")) });

        auto Params = UStaticMesh::FBuildMeshDescriptionsParams{};
        Params.bCommitMeshDescription = true;
        Params.bBuildSimpleCollision  = false;
        Params.bAllowCpuAccess        = true;
        Params.bMarkPackageDirty      = true;
        Params.bFastBuild             = true;

        if (NOT Mesh->BuildFromMeshDescriptions({ &Description }, Params))
        { return false; }

        // The fast build fills render data for immediate use; the editor build path (DDC) then produces the
        // full persistent artifact so the saved asset loads clean without a rebuild-on-load.
        Mesh->Build();
        Mesh->PostEditChange();

        Mesh->MarkPackageDirty();
        FAssetRegistryModule::AssetCreated(Mesh);

        const FString FileName = FPackageName::LongPackageNameToFilename(PkgPath, FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(Package, Mesh, *FileName, SaveArgs);
    }

    // ---- Surfaces. All sized around a ~100-unit base radius; behaviors scale via Particles.Scale. ------------------

    // Crescent arc band: 140-degree arc around +Z centered on +X, outer edge circular, inner edge bellied so the
    // band is razor-thin at the tips and ~55 units wide mid-swing (the marketplace slash silhouette).
    static auto Surface_Sweep(float S, float T) -> FGridPoint
    {
        const float Theta = FMath::Lerp(-1.22173f, 1.22173f, T);
        const float Belly = FMath::Sin(PI * T);
        const float ROuter = 100.0f;
        const float RInner = ROuter - (6.0f + 49.0f * Belly);
        const float R = FMath::Lerp(RInner, ROuter, S);
        return { FVector3f(FMath::Cos(Theta) * R, FMath::Sin(Theta) * R, 0.0f), FVector2f(S, T) };
    }

    // Open cylinder along +X: radius 30, length 100, no caps.
    static auto Surface_Tube(float S, float T) -> FGridPoint
    {
        const float Alpha = 2.0f * PI * S;
        return { FVector3f(T * 100.0f, FMath::Cos(Alpha) * 30.0f, FMath::Sin(Alpha) * 30.0f), FVector2f(S, T) };
    }

    // Sphere, radius 50. Polar angle clamped slightly off the poles so no degenerate triangles are emitted.
    static auto Surface_Shell(float S, float T) -> FGridPoint
    {
        const float Phi   = FMath::Lerp(0.06f, PI - 0.06f, T);
        const float Alpha = 2.0f * PI * S;
        const float SinPhi = FMath::Sin(Phi);
        return { FVector3f(SinPhi * FMath::Cos(Alpha), SinPhi * FMath::Sin(Alpha), FMath::Cos(Phi)) * 50.0f, FVector2f(S, T) };
    }

    // Flat ring in the XY plane: inner radius 55, outer 100. V runs radially so a V-pan pulses outward.
    static auto Surface_Disc(float S, float T) -> FGridPoint
    {
        const float Alpha = 2.0f * PI * S;
        const float R = FMath::Lerp(55.0f, 100.0f, T);
        return { FVector3f(FMath::Cos(Alpha) * R, FMath::Sin(Alpha) * R, 0.0f), FVector2f(S, T) };
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::particles_editor
{
    auto Generate_AllVfxMeshes() -> void
    {
        using namespace MeshGenLocal;

        int32 Ok = 0, Total = 0;
        const auto BakeOne = [&](const TCHAR* InName, FSurfaceFn InSurface, int32 InCols, int32 InRows, const FName InMaterial)
        {
            ++Total;
            if (Bake_Mesh(InName, InSurface, InCols, InRows, InMaterial)) { ++Ok; }
            else { Log(TEXT("Failed to bake VFX mesh: {}"), FString(InName)); }
        };

        BakeOne(TEXT("SM_CkParticles_Sweep"), &Surface_Sweep, 4,  48, TEXT("SweepErode"));
        BakeOne(TEXT("SM_CkParticles_Tube"),  &Surface_Tube,  32, 8,  TEXT("SweepErode"));
        BakeOne(TEXT("SM_CkParticles_Shell"), &Surface_Shell, 32, 16, TEXT("FresnelShell"));
        BakeOne(TEXT("SM_CkParticles_Disc"),  &Surface_Disc,  48, 3,  TEXT("SweepErode"));

        Log(TEXT("Generated {}/{} CkParticles VFX carrier meshes under {}."),
            FString::FromInt(Ok), FString::FromInt(Total), FString(MeshDir));
    }
}

// --------------------------------------------------------------------------------------------------------------------
