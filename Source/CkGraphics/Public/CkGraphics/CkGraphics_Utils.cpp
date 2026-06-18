#include "CkGraphics_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkGraphics/CkGraphics_Common.h"

#include <Engine/Texture.h>
#include <Engine/World.h>
#include <EngineUtils.h>
#include <Materials/Material.h>
#include <Materials/MaterialInterface.h>
#include <Materials/MaterialInstanceDynamic.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Graphics_UE::
    Get_ModifiedColorIntensity(
        FColor InColor,
        float  InIntensity)
    -> FColor
{
    using ColorChannelType = decltype(InColor.R);

    const auto& IntensifyColor = [&](auto InChannel)
    {
        return static_cast<ColorChannelType>(static_cast<float>(InChannel) * InIntensity);
    };

    return FColor
    {
        IntensifyColor(InColor.R),
        IntensifyColor(InColor.G),
        IntensifyColor(InColor.B),
        InColor.A
    };
}

auto
    UCk_Utils_Graphics_UE::
    Get_WasActorRecentlyRendered(
        AActor*  InActor,
        FCk_Time InTimeTolerance)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to Get_WasActorRecentlyRendered"))
    { return {}; }

    return InActor->WasRecentlyRendered(InTimeTolerance.Get_Seconds());
}

auto
    UCk_Utils_Graphics_UE::
    Get_IsMaterialChildOf(
        UMaterialInterface* InMaterial,
        UMaterialInterface* InParentMaterial)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InMaterial), TEXT("Invalid Material supplied to Get_IsMaterialChildOf"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InParentMaterial), TEXT("Invalid Parent Material supplied to Get_IsMaterialChildOf"))
    { return {}; }

    return InMaterial->IsDependent(InParentMaterial);
}

auto
    UCk_Utils_Graphics_UE::
    Enable_MaterialUsageFlag_InstancedStaticMesh(
        UMaterialInterface* InMaterial)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InMaterial), TEXT("Invalid Material supplied to Enable_MaterialUsageFlag_InstancedStaticMesh"))
    { return false; }

    auto* RootMat = InMaterial->GetMaterial();

    CK_ENSURE_IF_NOT(ck::IsValid(RootMat), TEXT("Failed to get root UMaterial from Material [{}]"), InMaterial)
    { return false; }

    if (RootMat->bUsedWithInstancedStaticMeshes)
    { return true; }

#if WITH_EDITOR
    static auto NeedsRecompile = false;
    RootMat->SetMaterialUsage(NeedsRecompile, MATUSAGE_InstancedStaticMeshes);
    std::ignore = RootMat->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}

auto
    UCk_Utils_Graphics_UE::
    Get_MeshComponentEffectiveMaterials(
        const UMeshComponent* InMeshComponent)
    -> TArray<FCk_MeshMaterialOverride>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InMeshComponent), TEXT("Invalid Mesh Component supplied to Get_MeshComponentEffectiveMaterials"))
    { return {}; }

    const auto NumMaterials = InMeshComponent->GetNumMaterials();
    auto Result = TArray<FCk_MeshMaterialOverride>();
    Result.Reserve(NumMaterials);

    // Use GetMaterial(i) so we return the effective material for each slot —
    // the override if present, otherwise the asset's slot material. Reading
    // OverrideMaterials directly misses slots inherited from the mesh asset.
    for (auto Index = 0; Index < NumMaterials; ++Index)
    {
        Result.Add(FCk_MeshMaterialOverride{Index, InMeshComponent->GetMaterial(Index)});
    }

    return Result;
}

auto
    UCk_Utils_Graphics_UE::
    Apply_MaterialParameter(
        UMaterialInstanceDynamic*     InDynamicMaterial,
        const FCk_Material_Parameter& InParameter)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDynamicMaterial),
        TEXT("Invalid Dynamic Material supplied to Apply_MaterialParameter for parameter [{}]"), InParameter.Get_ParameterName())
    { return; }

    switch (InParameter.Get_Type())
    {
        case ECk_Material_ParameterType::Scalar:
        {
            InDynamicMaterial->SetScalarParameterValue(InParameter.Get_ParameterName(), InParameter.Get_ScalarValue());
            break;
        }
        case ECk_Material_ParameterType::Color:
        {
            InDynamicMaterial->SetVectorParameterValue(InParameter.Get_ParameterName(), InParameter.Get_ColorValue());
            break;
        }
        case ECk_Material_ParameterType::Texture:
        {
            if (ck::IsValid(InParameter.Get_TextureValue()))
            {
                InDynamicMaterial->SetTextureParameterValue(InParameter.Get_ParameterName(), InParameter.Get_TextureValue());
            }
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InParameter.Get_Type());
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
