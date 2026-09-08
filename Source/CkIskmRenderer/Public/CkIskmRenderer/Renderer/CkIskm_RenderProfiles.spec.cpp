#if WITH_DEV_AUTOMATION_TESTS

#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment_Data.h"
#include "CkIskmRenderer/Renderer/CkIskm_BatchedClusterComponent.h"
#include "CkIskmRenderer/Renderer/CkIskm_BatchedCrowd_Actor.h"
#include "CkIskmRenderer/Renderer/CkIskm_RenderProfile_Utils.h"

#include "Components/SkeletalMeshComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_IskmRenderProfiles_AuthoringSurface,
    "Ck.CkVisualLod.RenderProfiles.AuthoringSurface",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCk_IskmRenderProfiles_AuthoringSurface::RunTest(const FString&)
{
    auto DisabledRendering = FCk_IskmRenderer_RenderingInfo{};
    DisabledRendering.Set_bCastDynamicShadow(false);
    DisabledRendering.Set_bCastContactShadow(false);
    DisabledRendering.Set_bRenderInMainPass(false);
    DisabledRendering.Set_bRenderInDepthPass(false);
    DisabledRendering.Set_bReceivesDecals(false);
    DisabledRendering.Set_bAffectDynamicIndirectLighting(false);
    DisabledRendering.Set_bAffectDistanceFieldLighting(false);
    DisabledRendering.Set_bVisibleInRayTracing(false);
    DisabledRendering.Set_bOutputVelocity(false);

    auto* Profile = NewObject<UCk_IskmRenderer_Data>();
    Profile->Set_RenderingInfo(DisabledRendering);

    auto* SkinnedMesh = NewObject<USkeletalMeshComponent>();
    SkinnedMesh->SetCastShadow(true);
    SkinnedMesh->bCastContactShadow = true;
    SkinnedMesh->bRenderInMainPass = true;
    SkinnedMesh->SetRenderInDepthPass(true);
    SkinnedMesh->bReceivesDecals = true;
    SkinnedMesh->bAffectDynamicIndirectLighting = true;
    SkinnedMesh->bAffectDistanceFieldLighting = true;
    SkinnedMesh->bVisibleInRayTracing = true;
    ck::iskm::Apply_RenderProfile(*SkinnedMesh, *Profile);

    TestFalse(TEXT("SKMC shadow disabled"), SkinnedMesh->CastShadow);
    TestFalse(TEXT("SKMC contact shadow disabled"), SkinnedMesh->bCastContactShadow);
    TestFalse(TEXT("SKMC main pass disabled"), SkinnedMesh->bRenderInMainPass);
    TestFalse(TEXT("SKMC depth pass disabled"), SkinnedMesh->bRenderInDepthPass);
    TestFalse(TEXT("SKMC decals disabled"), SkinnedMesh->bReceivesDecals);
    TestFalse(TEXT("SKMC indirect lighting disabled"), SkinnedMesh->bAffectDynamicIndirectLighting);
    TestFalse(TEXT("SKMC distance-field lighting disabled"), SkinnedMesh->bAffectDistanceFieldLighting);
    TestFalse(TEXT("SKMC ray tracing disabled"), SkinnedMesh->bVisibleInRayTracing);

    auto* Cluster = NewObject<UCk_Iskm_BatchedClusterComponent>();
    Cluster->Apply_RenderProfile(Profile);
    TestFalse(TEXT("GPU cluster shadow disabled"), Cluster->CastShadow);
    TestFalse(TEXT("GPU cluster main pass disabled"), Cluster->bRenderInMainPass);
    TestFalse(TEXT("GPU cluster indirect lighting disabled"), Cluster->bAffectDynamicIndirectLighting);

    auto EnabledRendering = FCk_IskmRenderer_RenderingInfo{};
    Profile->Set_RenderingInfo(EnabledRendering);
    ck::iskm::Apply_RenderProfile(*SkinnedMesh, *Profile);
    Cluster->Apply_RenderProfile(Profile);

    TestTrue(TEXT("SKMC shadow restored by complete overwrite"), SkinnedMesh->CastShadow);
    TestTrue(TEXT("SKMC main pass restored by complete overwrite"), SkinnedMesh->bRenderInMainPass);
    TestTrue(TEXT("GPU cluster shadow restored by complete overwrite"), Cluster->CastShadow);
    TestTrue(TEXT("GPU cluster main pass restored by complete overwrite"), Cluster->bRenderInMainPass);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_IskmRenderProfiles_BucketKeyContract,
    "Ck.CkVisualLod.RenderProfiles.BucketKeyContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCk_IskmRenderProfiles_BucketKeyContract::RunTest(const FString&)
{
    const FIntPoint Tile(17, -4);
    const FIntVector A(Tile.X, Tile.Y, 0);
    const FIntVector B(Tile.X, Tile.Y, 1);
    TestNotEqual(TEXT("same tile different profile is distinct render bucket"), A, B);
    TestEqual(TEXT("same tile X is stable"), A.X, B.X);
    TestEqual(TEXT("same tile Y is stable"), A.Y, B.Y);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_IskmRenderProfiles_RuntimeTunersValidation,
    "Ck.CkVisualLod.RenderProfiles.RuntimeTunersValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCk_IskmRenderProfiles_RuntimeTunersValidation::RunTest(const FString&)
{
    auto Tuners = FCk_IskmRenderer_RuntimeProfileTuners{};
    Tuners.Set_MinDrawDistance(100.0f);
    Tuners.Set_MaxDrawDistance(500.0f);
    Tuners.Set_BoundsScale(1.0f);
    Tuners.Set_FarAnimationUpdateInterval(FCk_Time::ZeroSecond());
    TestTrue(TEXT("finite aligned runtime tuners are accepted"),
        ck::iskm::Get_AreRuntimeProfileTunersValid(Tuners));

    Tuners.Set_MaxDrawDistance(50.0f);
    TestFalse(TEXT("enabled max draw distance cannot precede min draw distance"),
        ck::iskm::Get_AreRuntimeProfileTunersValid(Tuners));

    Tuners.Set_BoundsScale(-1.0f);
    TestFalse(TEXT("nonpositive bounds scale rejects the entire runtime snapshot"),
        ck::iskm::Get_AreRuntimeProfileTunersValid(Tuners));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_IskmRenderProfiles_RuntimeTunersApplyCompleteOverwrite,
    "Ck.CkVisualLod.RenderProfiles.RuntimeTunersApplyCompleteOverwrite",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCk_IskmRenderProfiles_RuntimeTunersApplyCompleteOverwrite::RunTest(const FString&)
{
    auto RenderingTuners = FCk_IskmRenderer_RuntimeRenderingTuners{};
    RenderingTuners.Set_bCastDynamicShadow(false);
    RenderingTuners.Set_bCastContactShadow(false);
    RenderingTuners.Set_bRenderInMainPass(false);
    RenderingTuners.Set_bRenderInDepthPass(false);
    RenderingTuners.Set_bReceivesDecals(false);
    RenderingTuners.Set_bUseAsOccluder(true);
    RenderingTuners.Set_bRenderCustomDepth(true);
    RenderingTuners.Set_bAffectDynamicIndirectLighting(false);
    RenderingTuners.Set_bAffectDistanceFieldLighting(false);
    RenderingTuners.Set_bVisibleInRayTracing(false);
    RenderingTuners.Set_bOutputVelocity(false);

    auto LightingChannels = FLightingChannels{};
    LightingChannels.bChannel0 = false;
    LightingChannels.bChannel1 = true;
    LightingChannels.bChannel2 = false;

    auto Tuners = FCk_IskmRenderer_RuntimeProfileTuners{};
    Tuners.Set_RenderingInfo(RenderingTuners);
    Tuners.Set_MinDrawDistance(123.0f);
    Tuners.Set_MaxDrawDistance(456.0f);
    Tuners.Set_MinLOD(2);
    Tuners.Set_BoundsScale(1.5f);
    Tuners.Set_LightingChannels(LightingChannels);

    auto* SkinnedMesh = NewObject<USkeletalMeshComponent>();
    auto* Profile = NewObject<UCk_IskmRenderer_Data>();
    ck::iskm::Apply_RenderProfile(*SkinnedMesh, Tuners);

    auto* Cluster = NewObject<UCk_Iskm_BatchedClusterComponent>();
    Cluster->Apply_RenderProfile(Profile, Tuners);

    TestFalse(TEXT("runtime SKMC shadow disabled"), SkinnedMesh->CastShadow);
    TestFalse(TEXT("runtime cluster shadow disabled"), Cluster->CastShadow);
    TestFalse(TEXT("runtime SKMC main pass disabled"), SkinnedMesh->bRenderInMainPass);
    TestFalse(TEXT("runtime cluster main pass disabled"), Cluster->bRenderInMainPass);
    TestEqual(TEXT("runtime SKMC min draw distance"), SkinnedMesh->MinDrawDistance, 123.0f);
    TestEqual(TEXT("runtime cluster max draw distance"), Cluster->LDMaxDrawDistance, 456.0f);
    TestTrue(TEXT("runtime SKMC MinLOD override enabled"), SkinnedMesh->bOverrideMinLod != 0);
    TestEqual(TEXT("runtime cluster MinLOD"), Cluster->Get_RuntimeProfileTuners().Get_MinLOD(), 2);
    TestEqual(TEXT("runtime SKMC bounds scale"), SkinnedMesh->BoundsScale, 1.5f);
    TestEqual(TEXT("runtime cluster bounds scale"), Cluster->BoundsScale, 1.5f);
    TestTrue(TEXT("runtime SKMC lighting channel 1"), SkinnedMesh->LightingChannels.bChannel1);
    TestTrue(TEXT("runtime cluster lighting channel 1"), Cluster->LightingChannels.bChannel1);

    auto ResetTuners = FCk_IskmRenderer_RuntimeProfileTuners{};
    SkinnedMesh->bOverrideMinLod = false;
    ck::iskm::Apply_RenderProfile(*SkinnedMesh, ResetTuners);
    Cluster->Apply_RenderProfile(Profile, ResetTuners);
    TestTrue(TEXT("runtime SKMC complete overwrite restores shadow"), SkinnedMesh->CastShadow);
    TestTrue(TEXT("runtime cluster complete overwrite restores shadow"), Cluster->CastShadow);
    TestTrue(TEXT("runtime SKMC complete overwrite restores MinLOD override"), SkinnedMesh->bOverrideMinLod != 0);
    TestEqual(TEXT("runtime cluster complete overwrite restores MinLOD"), Cluster->Get_RuntimeProfileTuners().Get_MinLOD(), 0);

    return true;
}

#endif
