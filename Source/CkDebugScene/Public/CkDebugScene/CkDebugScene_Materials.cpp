#include "CkDebugScene/CkDebugScene_Materials.h"

#include <Materials/Material.h>
#include <Materials/MaterialInterface.h>
#include <UObject/StrongObjectPtr.h>

namespace ck::debug_scene::materials
{
auto
TryGet_Opaque()
    -> UMaterialInterface*
{
    static auto Material = TStrongObjectPtr<UMaterialInterface>{LoadObject<UMaterialInterface>(
        nullptr, TEXT("/CkFoundation/DebugScene/M_CkDebugScene_Opaque.M_CkDebugScene_Opaque"))};
    return Material.Get();
}

auto
TryGet_Translucent()
    -> UMaterialInterface*
{
    static auto Material = TStrongObjectPtr<UMaterialInterface>{LoadObject<UMaterialInterface>(
        nullptr, TEXT("/CkFoundation/DebugScene/M_CkDebugScene_Translucent.M_CkDebugScene_Translucent"))};
    return Material.Get();
}

auto
TryGet_Wireframe()
    -> UMaterialInterface*
{
    static auto Material = TStrongObjectPtr<UMaterialInterface>{LoadObject<UMaterialInterface>(
        nullptr, TEXT("/Engine/EngineDebugMaterials/WireframeMaterial.WireframeMaterial"))};
    return Material.Get();
}

auto
Is_IsmCompatible(
    UMaterialInterface* InMaterial)
    -> bool
{
    if (NOT IsValid(InMaterial))
    {
        return false;
    }

    const auto* BaseMaterial = InMaterial->GetMaterial();
    return IsValid(BaseMaterial) &&
           (BaseMaterial->bUsedAsSpecialEngineMaterial ||
            BaseMaterial->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes));
}
} // namespace ck::debug_scene::materials
