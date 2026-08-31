#include "CkIskmRenderer/Renderer/CkIskm_RenderProfile_Utils.h"

#include "Components/SkeletalMeshComponent.h"

namespace ck::iskm
{
    auto
    MakeRuntimeProfileTuners(
        const UCk_IskmRenderer_Data& InData) -> FCk_IskmRenderer_RuntimeProfileTuners
    {
        auto Result = FCk_IskmRenderer_RuntimeProfileTuners{};
        auto RenderingTuners = FCk_IskmRenderer_RuntimeRenderingTuners{};
        const auto& Rendering = InData.Get_RenderingInfo();
        RenderingTuners.Set_bCastDynamicShadow(Rendering.Get_bCastDynamicShadow());
        RenderingTuners.Set_bRenderInMainPass(Rendering.Get_bRenderInMainPass());
        RenderingTuners.Set_bRenderInDepthPass(Rendering.Get_bRenderInDepthPass());
        RenderingTuners.Set_bReceivesDecals(Rendering.Get_bReceivesDecals());
        RenderingTuners.Set_bUseAsOccluder(Rendering.Get_bUseAsOccluder());
        RenderingTuners.Set_bRenderCustomDepth(Rendering.Get_bRenderCustomDepth());
        RenderingTuners.Set_bCastContactShadow(Rendering.Get_bCastContactShadow());
        RenderingTuners.Set_bAffectDynamicIndirectLighting(Rendering.Get_bAffectDynamicIndirectLighting());
        RenderingTuners.Set_bAffectDistanceFieldLighting(Rendering.Get_bAffectDistanceFieldLighting());
        RenderingTuners.Set_bVisibleInRayTracing(Rendering.Get_bVisibleInRayTracing());
        RenderingTuners.Set_bOutputVelocity(Rendering.Get_bOutputVelocity());
        Result.Set_RenderingInfo(RenderingTuners);
        const auto& Culling = InData.Get_CullingInfo();
        Result.Set_MinDrawDistance(Culling.Get_MinDrawDistance().GetValue());
        Result.Set_MaxDrawDistance(Culling.Get_MaxDrawDistance().GetValue());
        Result.Set_MinLOD(Culling.Get_MinLOD());
        Result.Set_BoundsScale(InData.Get_BoundsScale());
        Result.Set_LightingChannels(InData.Get_LightingChannels());
        Result.Set_FarAnimationUpdateInterval(InData.Get_FarAnimationUpdateInterval());
        Result.Set_FreezeFarAnimation(InData.Get_FreezeFarAnimation());
        return Result;
    }

    auto
    Get_AreRuntimeProfileTunersValid(
        const FCk_IskmRenderer_RuntimeProfileTuners& InTuners) -> bool
    {
        const auto IsFiniteNonNegative = [](float InValue)
        { return FMath::IsFinite(InValue) && InValue >= 0.0f; };
        const auto HasValidDistances = IsFiniteNonNegative(InTuners.Get_MinDrawDistance()) &&
            IsFiniteNonNegative(InTuners.Get_MaxDrawDistance()) &&
            (InTuners.Get_MaxDrawDistance() == 0.0f ||
                InTuners.Get_MinDrawDistance() <= InTuners.Get_MaxDrawDistance());
        if (NOT HasValidDistances || InTuners.Get_MinLOD() < 0 ||
            NOT FMath::IsFinite(InTuners.Get_BoundsScale()) || InTuners.Get_BoundsScale() <= 0.0f ||
            NOT IsFiniteNonNegative(static_cast<float>(InTuners.Get_FarAnimationUpdateInterval().Get_Seconds())))
        { return false; }

        return InTuners.Get_FreezeFarAnimation() == ECk_EnableDisable::Enable ||
            InTuners.Get_FreezeFarAnimation() == ECk_EnableDisable::Disable;
    }

    auto
    Apply_RenderProfile(
        USkeletalMeshComponent& InComponent,
        const FCk_IskmRenderer_RuntimeProfileTuners& InTuners)
        -> void
    {
        const auto& Rendering = InTuners.Get_RenderingInfo();
        InComponent.SetCastShadow(Rendering.Get_bCastDynamicShadow() != 0);
        InComponent.bCastContactShadow = Rendering.Get_bCastContactShadow();
        InComponent.bRenderInMainPass = Rendering.Get_bRenderInMainPass();
        InComponent.SetRenderInDepthPass(Rendering.Get_bRenderInDepthPass() != 0);
        InComponent.bReceivesDecals = Rendering.Get_bReceivesDecals();
        InComponent.bUseAsOccluder = Rendering.Get_bUseAsOccluder();
        InComponent.SetRenderCustomDepth(Rendering.Get_bRenderCustomDepth() != 0);
        InComponent.bAffectDynamicIndirectLighting = Rendering.Get_bAffectDynamicIndirectLighting();
        InComponent.bAffectDistanceFieldLighting = Rendering.Get_bAffectDistanceFieldLighting();
        InComponent.bVisibleInRayTracing = Rendering.Get_bVisibleInRayTracing();
        InComponent.LightingChannels = InTuners.Get_LightingChannels();
        InComponent.BoundsScale = InTuners.Get_BoundsScale();
        InComponent.MinDrawDistance = InTuners.Get_MinDrawDistance();
        InComponent.SetCullDistance(InTuners.Get_MaxDrawDistance());
        InComponent.OverrideMinLOD(InTuners.Get_MinLOD());
        InComponent.MarkRenderStateDirty();
    }

    auto
    Apply_RenderProfile(
        USkeletalMeshComponent& InComponent,
        const UCk_IskmRenderer_Data& InData)
        -> void
    {
        Apply_RenderProfile(InComponent, MakeRuntimeProfileTuners(InData));
    }
}
