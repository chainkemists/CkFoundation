#include "CkMesh_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "Algo/Accumulate.h"

#include <Rendering/SkeletalMeshRenderData.h>
#include <Kismet/GameplayStatics.h>


// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StaticMesh_UE::
    Get_TriangleCount(
        UStaticMesh* InStaticMesh,
        int32 InLODIndex)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InStaticMesh), TEXT("Invalid Static Mesh, cannot get the triangle count"))
    { return {}; }

    const auto& RenderData =  InStaticMesh->GetRenderData();

    CK_ENSURE_IF_NOT(ck::IsValid(RenderData, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Invalid Render Data for Static Mesh [{}], cannot get the triangle count"), InStaticMesh)
    { return {}; }

    const auto& LODResources = RenderData->LODResources;

    CK_ENSURE_IF_NOT(LODResources.IsValidIndex(InLODIndex),
        TEXT("Static Mesh [{}] does NOT support LOD [{}], cannot get the triangle count"), InStaticMesh, InLODIndex)
    { return {}; }

    return LODResources[InLODIndex].GetNumTriangles();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SkeletalMesh_UE::
    Get_TriangleCount(
        USkeletalMesh* InSkeletalMesh,
        int32 InLODIndex)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSkeletalMesh), TEXT("Invalid Static Mesh, cannot get the triangle count"))
    { return {}; }

    const auto& RenderData =  InSkeletalMesh->GetResourceForRendering();

    CK_ENSURE_IF_NOT(ck::IsValid(RenderData, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Invalid Render Data for Skeletal Mesh [{}], cannot get the triangle count"), InSkeletalMesh)
    { return {}; }

    const auto& LODRenderData = RenderData->LODRenderData;

    CK_ENSURE_IF_NOT(LODRenderData.IsValidIndex(InLODIndex),
        TEXT("Skeletal Mesh [{}] does NOT support LOD [{}], cannot get the triangle count"), InSkeletalMesh, InLODIndex)
    { return {}; }

    const auto& RenderSections = LODRenderData[InLODIndex].RenderSections;

    return Algo::Accumulate(RenderSections, 0, [](int32 InSum, const FSkelMeshRenderSection& InMeshSection)
    {
        return InSum + InMeshSection.NumTriangles;
    });
}

// --------------------------------------------------------------------------------------------------------------------

