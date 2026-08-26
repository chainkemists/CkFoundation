#include "CkJoltCook_MeshShapeCooker.h"

#include "CkJoltCook_AssetSave.h"

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
#include <Misc/PackageName.h>
#include <Misc/ScopeExit.h>
#include <PhysicsEngine/BodySetup.h>
#include <UObject/Package.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Core.h>
#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>

#include <sstream>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_cook_mesh_shape_cooker
{
    using namespace ck::jolt;
    using namespace ck::jolt::bake;
    using namespace ck::jolt::cook;

    static auto DoSave_Asset(UObject& InAsset) -> bool
    {
        return ck::jolt::cook::Save_CookedAsset(InAsset);
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
    Collect_Candidates()
    -> TArray<FAssetData>
{
    const auto Roots = UCk_Utils_Jolt_ProjectSettings::Get_BakedMeshShapeRoots();

    if (Roots.IsEmpty())
    { return {}; }

    auto& AssetRegistry = FAssetRegistryModule::GetRegistry();

    auto Filter = FARFilter{};
    Filter.ClassPaths.Emplace(UStaticMesh::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Filter.bRecursivePaths = true;
    for (const auto& Root : Roots)
    { Filter.PackagePaths.Emplace(*Root); }

    auto MeshAssets = TArray<FAssetData>{};
    AssetRegistry.GetAssets(Filter, MeshAssets);

    return MeshAssets;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_MeshShapeCooker::
    Cook_SingleMeshShape(
        const UStaticMesh& InMesh,
        ck::jolt::cook::ECk_Jolt_CookMode InMode,
        FString* OutCookedAssetPath)
    -> ck::jolt::cook::ECk_Jolt_MeshShapeCookResult
{
    using namespace ck_jolt_cook_mesh_shape_cooker;
    using namespace ck::jolt;
    using namespace ck::jolt::bake;
    using namespace ck::jolt::cook;

    // No game world here, so nothing else registers Jolt's allocator/factory/types. Refcounted, so
    // a caller already holding them across a sliced sweep pays nothing.
    Request_GlobalJoltInit();
    ON_SCOPE_EXIT { Request_GlobalJoltShutdown(); };

    const auto MeshPackagePath = InMesh.GetOutermost()->GetName();
    const auto* BodySetup = InMesh.GetBodySetup();

    if (ck::Is_NOT_Valid(BodySetup) || NOT mesh_shape_utils::Get_IsWorthPreBaking(*BodySetup))
    { return ECk_Jolt_MeshShapeCookResult::NotWorthPreBaking; }

    const auto AssetPath = mesh_shape_utils::Get_CookedMeshShapeAssetPath(
        UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath(), MeshPackagePath);

    const auto AssetPathIsDerivable = NOT AssetPath.IsEmpty();
    CK_ENSURE_IF_NOT(AssetPathIsDerivable,
        TEXT("JoltMeshCook: mesh [{}] is not /Game-rooted — cannot derive a cooked-shape path, skipping"),
        MeshPackagePath)
    { return ECk_Jolt_MeshShapeCookResult::Failed; }

    if (OutCookedAssetPath != nullptr)
    { *OutCookedAssetPath = AssetPath; }

    if (const auto* Existing = LoadObject<UCk_Jolt_CookedMeshShape_UE>(nullptr, *AssetPath, nullptr,
        LOAD_NoWarn | LOAD_Quiet))
    {
        const auto SourceMatches = Existing->Get_JoltVersionId() == static_cast<uint32>(JPH_VERSION_ID)
            && Existing->Get_BodySetupGuid() == BodySetup->BodySetupGuid
            && Existing->Get_TraceFlag() == static_cast<uint8>(BodySetup->GetCollisionTraceFlag());

        if (SourceMatches && Existing->Get_CookVersion() == MeshShapeCookVersion_Current)
        { return ECk_Jolt_MeshShapeCookResult::UpToDate; }

        // A pre-winding-fix (v2) blob shares the current encoding, and only its TRI-MESH content is
        // wrong (inverted by the bake's pre-fix b/c swap). Peek the blob: a convex v2 blob is
        // declared up to date rather than rewritten, keeping the fix's re-cook — and its Git LFS
        // lock footprint — to the blobs that are actually defective. Mirrors the runtime rule in
        // TryGet_ScaleOneShape.
        if (SourceMatches && Existing->Get_CookVersion() == mesh_shape_utils::PreWindingFixMeshShapeCookVersion)
        {
            const auto Restored = mesh_shape_utils::TryRestore_ShapeBlob(
                Existing->Get_ShapeBlob(), MeshPackagePath);

            if (ck::IsValid(Restored) && Restored->GetSubType() != JPH::EShapeSubType::Mesh)
            { return ECk_Jolt_MeshShapeCookResult::UpToDate; }
        }
    }

    auto Blob = TArray<uint8>{};
    if (NOT DoSerialize_ScaleOneShape(*BodySetup, MeshPackagePath, Blob))
    {
        // BuildShape_FromBodySetup already ensured with the specific defect (bad collision authoring).
        return ECk_Jolt_MeshShapeCookResult::Failed;
    }

    if (InMode == ECk_Jolt_CookMode::DryRun)
    { return ECk_Jolt_MeshShapeCookResult::Cooked; }

    const auto PackageName = FPackageName::ObjectPathToPackageName(AssetPath);
    const auto AssetName = FPackageName::GetShortName(PackageName);

    auto* Package = CreatePackage(*PackageName);
    CK_ENSURE_IF_NOT(Package != nullptr,
        TEXT("JoltMeshCook: failed to create package [{}]"), PackageName)
    { return ECk_Jolt_MeshShapeCookResult::Failed; }
    Package->FullyLoad();

    auto* ShapeAsset = NewObject<UCk_Jolt_CookedMeshShape_UE>(Package, *AssetName, RF_Public | RF_Standalone);
    CK_ENSURE_IF_NOT(ck::IsValid(ShapeAsset),
        TEXT("JoltMeshCook: failed to create shape asset [{}]"), AssetPath)
    { return ECk_Jolt_MeshShapeCookResult::Failed; }

    ShapeAsset->Set_CookVersion(MeshShapeCookVersion_Current);
    ShapeAsset->Set_JoltVersionId(static_cast<uint32>(JPH_VERSION_ID));
    ShapeAsset->Set_SourceMesh(FSoftObjectPath{&InMesh});
    ShapeAsset->Set_BodySetupGuid(BodySetup->BodySetupGuid);
    ShapeAsset->Set_TraceFlag(static_cast<uint8>(BodySetup->GetCollisionTraceFlag()));
    ShapeAsset->Set_ShapeBlob(MoveTemp(Blob));

    const auto ShapeSaved = DoSave_Asset(*ShapeAsset);
    CK_ENSURE_IF_NOT(ShapeSaved,
        TEXT("JoltMeshCook: failed to SAVE shape asset [{}]"), AssetPath)
    { return ECk_Jolt_MeshShapeCookResult::Failed; }

    mesh_shape_utils::Invalidate_CacheForMesh(MeshPackagePath);

    return ECk_Jolt_MeshShapeCookResult::Cooked;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_MeshShapeCooker::
    Accumulate_SingleResult(
        ck::jolt::cook::ECk_Jolt_MeshShapeCookResult InResult,
        FCookStats& InOutStats)
    -> void
{
    using namespace ck::jolt::cook;

    switch (InResult)
    {
        case ECk_Jolt_MeshShapeCookResult::Cooked:            ++InOutStats._NumShapesCooked; break;
        case ECk_Jolt_MeshShapeCookResult::UpToDate:          ++InOutStats._NumUpToDate; break;
        case ECk_Jolt_MeshShapeCookResult::NotWorthPreBaking: ++InOutStats._NumSkippedNotWorthBaking; break;
        case ECk_Jolt_MeshShapeCookResult::Failed:            ++InOutStats._NumFailed; break;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_MeshShapeCooker::
    Report_Orphans(
        const TSet<FString>& InCookedAssetPathsInUse)
    -> int32
{
    const auto CookedRootPath = UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath();

    auto& AssetRegistry = FAssetRegistryModule::GetRegistry();

    auto OrphanFilter = FARFilter{};
    OrphanFilter.ClassPaths.Emplace(UCk_Jolt_CookedMeshShape_UE::StaticClass()->GetClassPathName());
    OrphanFilter.bRecursivePaths = true;
    OrphanFilter.PackagePaths.Emplace(*ck::Format_UE(TEXT("{}/Meshes"), CookedRootPath));

    auto ExistingShapeAssets = TArray<FAssetData>{};
    AssetRegistry.GetAssets(OrphanFilter, ExistingShapeAssets);

    auto NumOrphans = 0;

    for (const auto& ShapeAssetData : ExistingShapeAssets)
    {
        if (InCookedAssetPathsInUse.Contains(ShapeAssetData.GetObjectPathString()))
        { continue; }

        ++NumOrphans;
        ck::jolt::Warning(TEXT("JoltMeshCook: ORPHANED cooked mesh shape [{}] (source mesh gone, moved, "
            "or no longer worth baking) — delete it by hand"), ShapeAssetData.GetObjectPathString());
    }

    return NumOrphans;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_MeshShapeCooker::
    Log_CookStats(
        const FCookStats& InStats,
        ck::jolt::cook::ECk_Jolt_CookMode InMode)
    -> void
{
    using namespace ck::jolt::cook;

    ck::jolt::Log(TEXT("JoltMeshCook{}: [{}] meshes considered — [{}] cooked, [{}] up to date, [{}] not worth "
        "pre-baking (primitive-only or no collision), [{}] failed, [{}] orphans"),
        InMode == ECk_Jolt_CookMode::DryRun ? TEXT(" DRY RUN") : TEXT(""), InStats._NumMeshesConsidered,
        InStats._NumShapesCooked, InStats._NumUpToDate, InStats._NumSkippedNotWorthBaking, InStats._NumFailed,
        InStats._NumOrphans);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Jolt_MeshShapeCooker::
    Cook_MeshShapes(
        ck::jolt::cook::ECk_Jolt_CookMode InMode)
    -> FCookStats
{
    using namespace ck_jolt_cook_mesh_shape_cooker;
    using namespace ck::jolt;
    using namespace ck::jolt::cook;

    auto Stats = FCookStats{};

    if (UCk_Utils_Jolt_ProjectSettings::Get_BakedMeshShapeRoots().IsEmpty())
    {
        ck::jolt::Log(TEXT("JoltMeshCook: _BakedMeshShapeRoots is empty — the per-mesh pre-bake is off, "
            "nothing to cook"));
        Stats._Success = true;
        return Stats;
    }

    // Held across the whole sweep so the per-mesh acquire never tears the Jolt globals down between meshes.
    Request_GlobalJoltInit();
    ON_SCOPE_EXIT { Request_GlobalJoltShutdown(); };

    const auto MeshAssets = Collect_Candidates();
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

        auto CookedAssetPath = FString{};
        const auto Result = Cook_SingleMeshShape(*Mesh, InMode, &CookedAssetPath);

        if (NOT CookedAssetPath.IsEmpty())
        { CookedAssetPathsInUse.Add(CookedAssetPath); }

        Accumulate_SingleResult(Result, Stats);
    }

    Stats._NumOrphans = Report_Orphans(CookedAssetPathsInUse);
    Stats._Success = Stats._NumFailed == 0;

    Log_CookStats(Stats, InMode);

    return Stats;
}

// --------------------------------------------------------------------------------------------------------------------
