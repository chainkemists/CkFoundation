#include "CkGraphics_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include <Engine/World.h>
#include <EngineUtils.h>
#include <Materials/Material.h>
#include <Materials/MaterialInterface.h>

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

// --------------------------------------------------------------------------------------------------------------------
