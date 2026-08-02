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

    static auto Surface_Sweep(float S, float T) -> FGridPoint
    {
        constexpr auto HalfArcRadians = 1.22173f; // 70 degrees either side of +X
        constexpr auto OuterRadius    = 100.0f;
        constexpr auto TipWidth       = 6.0f;
        constexpr auto BellyWidth     = 49.0f;

        const float Theta  = FMath::Lerp(-HalfArcRadians, HalfArcRadians, T);
        const float Belly  = FMath::Sin(PI * T);
        const float RInner = OuterRadius - (TipWidth + BellyWidth * Belly);
        const float R      = FMath::Lerp(RInner, OuterRadius, S);
        return { FVector3f(FMath::Cos(Theta) * R, FMath::Sin(Theta) * R, 0.0f), FVector2f(S, T) };
    }

    static auto Surface_Tube(float S, float T) -> FGridPoint
    {
        constexpr auto Radius = 30.0f;
        constexpr auto Length = 100.0f;

        const float Alpha = 2.0f * PI * S;
        return { FVector3f(T * Length, FMath::Cos(Alpha) * Radius, FMath::Sin(Alpha) * Radius), FVector2f(S, T) };
    }

    static auto Surface_Shell(float S, float T) -> FGridPoint
    {
        constexpr auto Radius  = 50.0f;
        constexpr auto PoleGap = 0.06f; // stops short of the poles so no degenerate triangles are emitted

        const float Phi   = FMath::Lerp(PoleGap, PI - PoleGap, T);
        const float Alpha = 2.0f * PI * S;
        const float SinPhi = FMath::Sin(Phi);
        return { FVector3f(SinPhi * FMath::Cos(Alpha), SinPhi * FMath::Sin(Alpha), FMath::Cos(Phi)) * Radius, FVector2f(S, T) };
    }

    static auto Surface_Disc(float S, float T) -> FGridPoint
    {
        constexpr auto InnerRadius = 55.0f;
        constexpr auto OuterRadius = 100.0f;

        const float Alpha = 2.0f * PI * S;
        const float R = FMath::Lerp(InnerRadius, OuterRadius, T);
        return { FVector3f(FMath::Cos(Alpha) * R, FMath::Sin(Alpha) * R, 0.0f), FVector2f(S, T) };
    }

    // ---- Spike: the Vefects SM_VFX_Spike01 carrier, from the corpus .obj measurements ---------------------------
    //
    // A square pyramid: base (+-100, +-100, 0), apex (0, 0, 200), the source's own dimensions. Its UV is the
    // measured fact that matters — corr(v, Z) = -1.000 and corr(v, radius) = +1.000, so v runs TIP (0) to BASE
    // (1), and u is derived from Y alone (corr(u, Y) = -1.000): 0 on the +Y side, 1 on -Y, 0.5 at the apex.
    //
    // The apex is a 1-unit square rather than a point, exactly as Surface_Shell stops short of its poles: a
    // grid surface whose top row collapses emits degenerate triangles. 1 unit on a 200-unit pyramid is 0.5%.
    // The base is closed by a second band that runs the perimeter back to the centre at Z = 0.
    static auto Surface_Spike(float S, float T) -> FGridPoint
    {
        constexpr auto HalfExtent = 100.0f;
        constexpr auto Height     = 200.0f;
        constexpr auto TipScale   = 0.01f;
        constexpr auto SideEnd    = 0.75f; // T past this closes the base cap

        // The unit square's perimeter, parameterized counter-clockwise from (+1, -1).
        const float Q = S * 4.0f;
        auto Px = 0.0f;
        auto Py = 0.0f;
        if      (Q < 1.0f) { Px =  1.0f;            Py = -1.0f + 2.0f * Q; }
        else if (Q < 2.0f) { Px =  1.0f - 2.0f * (Q - 1.0f); Py =  1.0f; }
        else if (Q < 3.0f) { Px = -1.0f;            Py =  1.0f - 2.0f * (Q - 2.0f); }
        else               { Px = -1.0f + 2.0f * (Q - 3.0f); Py = -1.0f; }

        auto RadiusScale = 0.0f;
        auto Z           = 0.0f;
        auto V           = 0.0f;

        if (T <= SideEnd)
        {
            const float K = T / SideEnd;
            RadiusScale = FMath::Lerp(TipScale, 1.0f, K);
            Z           = FMath::Lerp(Height, 0.0f, K);
            V           = K;
        }
        else
        {
            const float K = (T - SideEnd) / (1.0f - SideEnd);
            RadiusScale = FMath::Lerp(1.0f, TipScale, K);
            Z           = 0.0f;
            V           = 1.0f;
        }

        return { FVector3f(Px * HalfExtent * RadiusScale, Py * HalfExtent * RadiusScale, Z),
                 FVector2f(0.5f - 0.5f * Py * RadiusScale, V) };
    }

    // ---- Card: the Vefects SM_VFX_Plane01 carrier -------------------------------------------------------------
    //
    // A flat quad in the XZ plane, X +-100 and Z 0..200. Measured UV: corr(u, X) = 1.000 and corr(v, Z) = -1.000,
    // so u runs -X to +X and v runs TOP (0) to BOTTOM (1). The source is two coincident quads 0.0643 apart in Y —
    // 0.03% of the 200-unit span — collapsed here to one sheet with a two-sided look, exactly as the crescent did.
    static auto Surface_Card(float S, float T) -> FGridPoint
    {
        constexpr auto HalfWidth = 100.0f;
        constexpr auto Height    = 200.0f;

        return { FVector3f(FMath::Lerp(-HalfWidth, HalfWidth, S), 0.0f, FMath::Lerp(Height, 0.0f, T)),
                 FVector2f(S, T) };
    }

    // ---- Cylinder: the Vefects SM_VFX_Ring01 carrier, from the corpus .obj measurements -------------------------
    //
    // An open tube: radius 100, Z 0..50, 32 circumferential segments. The source is two coaxial walls 0.5 apart
    // — 0.5% of its radius — collapsed here to one sheet with a two-sided look, exactly as the card and the
    // crescent were.
    //
    // Its UV is measured and closed-form: v = 1 at Z = 0 and v = 0 at Z = 50 (and only those two values exist),
    // while u wraps once around the circumference DECREASING with polar angle — u = frac(0.75 - angle/360), so
    // the seam sits at -90 degrees and u = 0.75 at angle 0. The renderer's own (1, 1, 5) mesh scale is NOT
    // baked in: the behavior applies it, so this asset stays a faithful record of the source's dimensions.
    static auto Surface_Cylinder(float S, float T) -> FGridPoint
    {
        constexpr auto Radius = 100.0f;
        constexpr auto Height = 50.0f;

        const float AngleDegrees = 360.0f * (0.75f - S);
        const float Angle        = FMath::DegreesToRadians(AngleDegrees);

        return { FVector3f(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, FMath::Lerp(Height, 0.0f, T)),
                 FVector2f(S, T) };
    }

    // ---- Bomb: the stylized prop stand-in for SM_VFX_Bomb_01_Small ---------------------------------------------
    //
    // A procedural sphere plus a fuse, per the campaign's [C-D2] ruling — the source mesh is NOT imported. Its
    // silhouette is measurable and reproduced: radius-from-origin min 69.2 / mean 81.8 / max 94.1, symmetric
    // about Z and widest at the equator, so the ball takes the measured MEAN radius. What is not reproducible
    // is its UV, an authored atlas whose every correlation against angle, radius and Z is under 0.18 — there is
    // no projection to re-derive, and MI_VFX_Bomb bands its three flat colours by Step over exactly those UVs.
    // So v here is latitudinal by construction: 0 at the fuse tip, 1 at the underside, which turns the source's
    // arbitrary banding into a top-lit read. Both the fuse and the banding axis are recorded as shape deviations
    // in the recipe's §13 — the corpus .obj carries no fuse at all (its Z bounds are exactly the ball radius).
    static auto Surface_Bomb(float S, float T) -> FGridPoint
    {
        constexpr auto Radius     = 82.0f;  // the measured mean radius-from-origin, 81.818
        constexpr auto PoleGap    = 0.12f;  // stops short of the pole so the fuse meets a ring, not a point
        constexpr auto FuseEnd    = 0.15f;  // T past this is the ball
        constexpr auto FuseRadius = 10.0f;
        constexpr auto FuseTip    = 0.10f;  // tip radius as a fraction of the fuse's, so the top is closed
        constexpr auto FuseLength = 28.0f;

        const float Alpha = 2.0f * PI * S;

        auto RadiusXY = 0.0f;
        auto Z        = 0.0f;

        if (T <= FuseEnd)
        {
            const float K = T / FuseEnd;
            RadiusXY = FuseRadius * FMath::Lerp(FuseTip, 1.0f, K);
            Z        = Radius + FMath::Lerp(FuseLength, 0.0f, K);
        }
        else
        {
            const float K   = (T - FuseEnd) / (1.0f - FuseEnd);
            const float Phi = FMath::Lerp(PoleGap, PI - PoleGap, K);
            RadiusXY = Radius * FMath::Sin(Phi);
            Z        = Radius * FMath::Cos(Phi);
        }

        return { FVector3f(FMath::Cos(Alpha) * RadiusXY, FMath::Sin(Alpha) * RadiusXY, Z),
                 FVector2f(S, T) };
    }

    // ---- Crescent: the Vefects NS_BasicAttack slash carrier, from the recipe's MEASURED profile ----------------
    //
    // A flat tapered annulus in XY sweeping the full 360 degrees, widest radially at 0 degrees and narrowing to
    // its thinnest at +/-180. The knots below are corpus measurements of the source .obj at every 30 degrees
    // (+/-4 degree windows; the +/-angle halves agree exactly), quoted in
    // CkParticles/Cookbook/NS_BasicAttack.md §3; the profile is piecewise-linear between them and symmetric in
    // +/-angle. The generator never reads the .obj at bake time — these numbers ARE the source of truth. A
    // coarser 60-degree table read the tail band 1.3-2.4x too wide past 120 degrees (recipe §13).
    struct FCrescentKnot { float AngleDegrees; float InnerRadius; float OuterRadius; };
    struct FCrescentBand { float InnerRadius; float OuterRadius; };

    static constexpr FCrescentKnot CrescentKnots[] =
    {
        {   0.0f, 21.2f, 169.0f },
        {  30.0f, 23.4f, 165.4f },
        {  60.0f, 30.0f, 156.8f },
        {  90.0f, 44.1f, 147.1f },
        { 120.0f, 67.3f, 132.5f },
        { 150.0f, 88.5f, 117.4f },
        { 180.0f, 98.6f, 101.2f },
    };
    static constexpr int32 NumCrescentKnots = 7;

    static auto Get_CrescentBand(float InAbsAngleDegrees) -> FCrescentBand
    {
        const auto Clamped = FMath::Clamp(InAbsAngleDegrees, 0.0f, 180.0f);
        for (auto Index = 1; Index < NumCrescentKnots; ++Index)
        {
            const auto& Lo = CrescentKnots[Index - 1];
            const auto& Hi = CrescentKnots[Index];
            if (Clamped > Hi.AngleDegrees)
            { continue; }

            const auto Alpha = (Clamped - Lo.AngleDegrees) / (Hi.AngleDegrees - Lo.AngleDegrees);
            return { FMath::Lerp(Lo.InnerRadius, Hi.InnerRadius, Alpha),
                     FMath::Lerp(Lo.OuterRadius, Hi.OuterRadius, Alpha) };
        }
        return { CrescentKnots[NumCrescentKnots - 1].InnerRadius, CrescentKnots[NumCrescentKnots - 1].OuterRadius };
    }

    // u along the arc is measured, not derived: the source spreads its texture non-uniformly over the sweep
    // (arc-length-like — the belly u 0.25..0.75 covers ~215 degrees, not 180). Mean u per 30-degree station
    // from the source .obj, symmetrized (the +/- station pairs agree within uv quantization, 0.0015).
    static constexpr float CrescentUKnots[] =
    {
        0.0f, 0.1031f, 0.2092f, 0.3064f, 0.3666f, 0.4263f, 0.5f,
        0.5737f, 0.6334f, 0.6936f, 0.7908f, 0.8969f, 1.0f,
    };

    static auto Get_CrescentU(float InSweepDegrees) -> float
    {
        const auto Station = FMath::Clamp(InSweepDegrees / 30.0f, 0.0f, 12.0f);
        const auto Index   = FMath::Clamp(FMath::FloorToInt32(Station), 0, 11);
        return FMath::Lerp(CrescentUKnots[Index], CrescentUKnots[Index + 1], Station - Index);
    }

    // T sweeps the arc counter-clockwise from -180 degrees; S crosses the band. UV is the fact that fixes
    // sweep direction: u runs ALONG the arc CCW covering the full 360 degrees (u=0 at -180, u=1 back at +180,
    // through the measured station table above) and v runs ACROSS it, v=0 OUTER edge to v=1 INNER edge. The
    // material's offset channel pans along u, wrapping through the seam exactly as it does on the source mesh.
    static auto Surface_Crescent(float S, float T) -> FGridPoint
    {
        constexpr auto StartDegrees = -180.0f;

        const float SweepDegrees = 360.0f * T;
        const float AngleDegrees = StartDegrees + SweepDegrees;
        const float Angle        = FMath::DegreesToRadians(AngleDegrees);

        const auto  Band = Get_CrescentBand(FMath::Abs(FMath::UnwindDegrees(AngleDegrees)));
        const float R    = FMath::Lerp(Band.OuterRadius, Band.InnerRadius, S); // S = v: 0 outer, 1 inner

        return { FVector3f(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R, 0.0f),
                 FVector2f(Get_CrescentU(SweepDegrees), S) };
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

        // 3 band segments x 160 arc segments = 960 triangles, the source mesh's own count. Its slot material is
        // only a fallback: the Slash row's four crescent renderers each override it with their own CkUsf look.
        BakeOne(TEXT("SM_CkParticles_Crescent"), &Surface_Crescent, 3, 160, TEXT("SweepErode"));

        // The Vefects hit carriers. 4 perimeter columns is the source pyramid's own facet count; 8 rows keeps
        // the base cap and the side band on their own quads. The card needs a single quad and nothing more —
        // both slot materials are fallbacks, overridden by each row renderer's own CkUsf look.
        BakeOne(TEXT("SM_CkParticles_Spike"), &Surface_Spike, 4, 8, TEXT("SweepErode"));
        BakeOne(TEXT("SM_CkParticles_Card"),  &Surface_Card,  1, 1, TEXT("SweepErode"));

        // The Vefects arrow/bomb carriers. 32 columns is SM_VFX_Ring01's own segment count; the bomb's grid is
        // a stylization rather than a transcription (see Surface_Bomb) and is sized for a clean silhouette.
        BakeOne(TEXT("SM_CkParticles_Cylinder"), &Surface_Cylinder, 32, 1,  TEXT("SweepErode"));
        BakeOne(TEXT("SM_CkParticles_Bomb"),     &Surface_Bomb,     24, 20, TEXT("SweepErode"));

        Log(TEXT("Generated {}/{} CkParticles VFX carrier meshes under {}."),
            FString::FromInt(Ok), FString::FromInt(Total), FString(MeshDir));
    }
}

// --------------------------------------------------------------------------------------------------------------------
