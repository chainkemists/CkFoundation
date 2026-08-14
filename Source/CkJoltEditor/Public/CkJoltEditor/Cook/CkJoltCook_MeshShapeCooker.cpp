#include "CkJoltCook_MeshShapeCooker.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJolt/StaticWorld/CkJoltBakeExtraction.h"
#include "CkJolt/StaticWorld/CkJoltMeshShape_Utils.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Data.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <AssetRegistry/IAssetRegistry.h>
#include <Engine/StaticMesh.h>
#include <Misc/ScopeExit.h>
#include <PhysicsEngine/BodySetup.h>
#include <UObject/Package.h>
#include <UObject/SavePackage.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Core/Core.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>

#include <sstream>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_cook_mesh_shape_cooker
{
    using namespace ck::jolt;
    using namespace ck::jolt::bake;

    static auto DoSave_Asset(UObject& InAsset) -> bool
    {
        auto* Package = InAsset.GetOutermost();

        FAssetRegistryModule::AssetCreated(&InAsset);
        Package->MarkPackageDirty();

        const auto FileName = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());

        auto SaveArgs = FSavePackageArgs{};
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

        return UPackage::SavePackage(Package, &InAsset, *FileName, SaveArgs);
    }

    static auto DoSerialize_ScaleOneShape(
        const UBodySetup& InBodySetup,
        const FString& InDebugName,
        TArray<uint8>& OutBlob) -> bool
    {
        const auto Shape = BuildShape_FromBodySetup(InBodySetup, FVector::OneVector, InDebugName);
        if (ck::Is_NOT_Valid(Shape))
        { return false; }

        auto BlobStream = std::ostringstream{};
        auto StreamWrapper = JPH::StreamOutWrapper{BlobStream};
        auto ShapeToId = JPH::Shape::ShapeToIDMap{};
        auto MaterialToId = JPH::Shape::MaterialToIDMap{};

        Shape->SaveWithChildren(StreamWrapper, ShapeToId, MaterialToId);

        const auto BlobString = BlobStream.str();
        OutBlob.Reset();
        OutBlob.Append(reinterpret_cast<const uint8*>(BlobString.data()), BlobString.size());

        return OutBlob.Num() > 0;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_MeshShapeCooker::
    Cook_MeshShapes(
        bool InDryRun)
    -> FCookStats
{
    using namespace ck_jolt_cook_mesh_shape_cooker;
    using namespace ck::jolt;
    using namespace ck::jolt::bake;

    auto Stats = FCookStats{};

    // Cook vehicles (commandlet, editor subsystem) have no game world, so nothing else has
    // registered Jolt's allocator/factory/types — shape creation crashes without this.
    Request_GlobalJoltInit();
    ON_SCOPE_EXIT { Request_GlobalJoltShutdown(); };

    const auto Roots = UCk_Utils_Jolt_ProjectSettings::Get_BakedMeshShapeRoots();
    const auto CookedRootPath = UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath();

    if (Roots.IsEmpty())
    {
        ck::jolt::Log(TEXT("JoltMeshCook: _BakedMeshShapeRoots is empty — the per-mesh pre-bake is off, "
            "nothing to cook"));
        Stats._Success = true;
        return Stats;
    }

    auto& AssetRegistry = FAssetRegistryModule::GetRegistry();

    auto Filter = FARFilter{};
    Filter.ClassPaths.Emplace(UStaticMesh::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Filter.bRecursivePaths = true;
    for (const auto& Root : Roots)
    { Filter.PackagePaths.Emplace(*Root); }

    auto MeshAssets = TArray<FAssetData>{};
    AssetRegistry.GetAssets(Filter, MeshAssets);

    auto CookedAssetPathsInUse = TSet<FString>{};

    for (const auto& MeshAsset : MeshAssets)
    {
        ++Stats._NumMeshesConsidered;

        const auto* Mesh = Cast<UStaticMesh>(MeshAsset.GetAsset());
        if (ck::Is_NOT_Valid(Mesh))
        {
            ++Stats._NumFailed;
            continue;
        }

        const auto MeshPackagePath = Mesh->GetOutermost()->GetName();
        const auto* BodySetup = Mesh->GetBodySetup();

        if (ck::Is_NOT_Valid(BodySetup) ||
            NOT mesh_shape_utils::Get_IsWorthPreBaking(*BodySetup))
        {
            ++Stats._NumSkippedNotWorthBaking;
            continue;
        }

        const auto AssetPath = mesh_shape_utils::Get_CookedMeshShapeAssetPath(
            CookedRootPath, MeshPackagePath);

        CK_ENSURE_IF_NOT(NOT AssetPath.IsEmpty(),
            TEXT("JoltMeshCook: mesh [{}] is not /Game-rooted — cannot derive a cooked-shape path, skipping"),
            MeshPackagePath)
        {
            ++Stats._NumFailed;
            continue;
        }

        CookedAssetPathsInUse.Add(AssetPath);

        // Incremental: an existing asset that already matches the source is up to date.
        if (const auto* Existing = LoadObject<UCk_Jolt_CookedMeshShape_UE>(nullptr, *AssetPath))
        {
            const auto UpToDate = Existing->Get_CookVersion() == CookVersion_Current
                && Existing->Get_JoltVersionId() == static_cast<uint32>(JPH_VERSION_ID)
                && Existing->Get_BodySetupGuid() == BodySetup->BodySetupGuid
                && Existing->Get_TraceFlag() == static_cast<uint8>(BodySetup->GetCollisionTraceFlag());

            if (UpToDate)
            {
                ++Stats._NumUpToDate;
                continue;
            }
        }

        auto Blob = TArray<uint8>{};
        if (NOT DoSerialize_ScaleOneShape(*BodySetup, MeshPackagePath, Blob))
        {
            // BuildShape already ensured with the specific defect (bad collision authoring).
            ++Stats._NumFailed;
            continue;
        }

        if (InDryRun)
        {
            ++Stats._NumShapesCooked;
            continue;
        }

        const auto PackageName = FPackageName::ObjectPathToPackageName(AssetPath);
        const auto AssetName = FPackageName::GetShortName(PackageName);

        auto* Package = CreatePackage(*PackageName);
        CK_ENSURE_IF_NOT(Package != nullptr,
            TEXT("JoltMeshCook: failed to create package [{}]"), PackageName)
        {
            ++Stats._NumFailed;
            continue;
        }
        Package->FullyLoad();

        auto* ShapeAsset = NewObject<UCk_Jolt_CookedMeshShape_UE>(Package, *AssetName, RF_Public | RF_Standalone);
        CK_ENSURE_IF_NOT(ck::IsValid(ShapeAsset),
            TEXT("JoltMeshCook: failed to create shape asset [{}]"), AssetPath)
        {
            ++Stats._NumFailed;
            continue;
        }

        ShapeAsset->Set_CookVersion(CookVersion_Current);
        ShapeAsset->Set_JoltVersionId(static_cast<uint32>(JPH_VERSION_ID));
        ShapeAsset->Set_SourceMesh(FSoftObjectPath{Mesh});
        ShapeAsset->Set_BodySetupGuid(BodySetup->BodySetupGuid);
        ShapeAsset->Set_TraceFlag(static_cast<uint8>(BodySetup->GetCollisionTraceFlag()));
        ShapeAsset->Set_ShapeBlob(MoveTemp(Blob));

        CK_ENSURE_IF_NOT(DoSave_Asset(*ShapeAsset),
            TEXT("JoltMeshCook: failed to SAVE shape asset [{}]"), AssetPath)
        {
            ++Stats._NumFailed;
            continue;
        }

        ++Stats._NumShapesCooked;
    }

    // Orphan sweep: cooked shape assets under <Root>/Meshes whose source mesh is gone or no longer
    // worth baking. Logged, never auto-deleted (v1 — same policy as map-cook cells).
    {
        auto OrphanFilter = FARFilter{};
        OrphanFilter.ClassPaths.Emplace(UCk_Jolt_CookedMeshShape_UE::StaticClass()->GetClassPathName());
        OrphanFilter.bRecursivePaths = true;
        OrphanFilter.PackagePaths.Emplace(*ck::Format_UE(TEXT("{}/Meshes"), CookedRootPath));

        auto ExistingShapeAssets = TArray<FAssetData>{};
        AssetRegistry.GetAssets(OrphanFilter, ExistingShapeAssets);

        for (const auto& ShapeAssetData : ExistingShapeAssets)
        {
            if (CookedAssetPathsInUse.Contains(ShapeAssetData.GetObjectPathString()))
            { continue; }

            ++Stats._NumOrphans;
            ck::jolt::Warning(TEXT("JoltMeshCook: ORPHANED cooked mesh shape [{}] (source mesh gone, moved, "
                "or no longer worth baking) — delete it by hand"), ShapeAssetData.GetObjectPathString());
        }
    }

    Stats._Success = Stats._NumFailed == 0;

    ck::jolt::Log(TEXT("JoltMeshCook{}: [{}] meshes considered — [{}] cooked, [{}] up to date, [{}] not worth "
        "pre-baking (primitive-only or no collision), [{}] failed, [{}] orphans"),
        InDryRun ? TEXT(" DRY RUN") : TEXT(""), Stats._NumMeshesConsidered, Stats._NumShapesCooked,
        Stats._NumUpToDate, Stats._NumSkippedNotWorthBaking, Stats._NumFailed, Stats._NumOrphans);

    return Stats;
}

// --------------------------------------------------------------------------------------------------------------------
