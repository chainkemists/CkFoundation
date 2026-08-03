#include "CkAssetExporter/MaterialExporter/CkMaterialExporter_HeadlessTextures.h"

#include "CkCore/Macros/CkMacros.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <Engine/Texture.h>
#include <Materials/Material.h>
#include <Materials/MaterialInterface.h>
#include <Misc/App.h>
#include <Misc/AutomationTest.h>
#include <ShaderCompiler.h>

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck_test_material_headless_textures
{
    auto Get_EngineMaterialsSorted(int32 InMaxCount) -> TArray<UMaterialInterface*>
    {
        auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

        auto Filter = FARFilter{};
        Filter.PackagePaths.Add(TEXT("/Engine/EngineMaterials"));
        Filter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());
        Filter.bRecursivePaths = true;
        Filter.bRecursiveClasses = true;

        auto Assets = TArray<FAssetData>{};
        AssetRegistry.GetAssets(Filter, Assets);
        Assets.Sort([](const FAssetData& InA, const FAssetData& InB)
        {
            return InA.GetObjectPathString() < InB.GetObjectPathString();
        });

        auto Materials = TArray<UMaterialInterface*>{};
        for (const auto& Asset : Assets)
        {
            if (Materials.Num() >= InMaxCount)
            { break; }
            if (auto* Material = Cast<UMaterialInterface>(Asset.GetAsset()))
            { Materials.Add(Material); }
        }
        return Materials;
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_AssetExporter_MaterialHeadlessTextures_Parity_Test,
    "Ck.AssetExporter.MaterialHeadlessTextures.ParityWithGetUsedTextures",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_AssetExporter_MaterialHeadlessTextures_Parity_Test::RunTest(const FString& InParameters)
{
    constexpr auto MaxMaterialsToCheck = 15;
    const auto Materials = ck_test_material_headless_textures::Get_EngineMaterialsSorted(MaxMaterialsToCheck);
    TestTrue(TEXT("found engine materials to check"), Materials.Num() > 0);

    const auto ContextCanCompareAgainstLivePath = FApp::CanEverRender();
    if (ContextCanCompareAgainstLivePath && GShaderCompilingManager != nullptr)
    { GShaderCompilingManager->FinishAllCompilation(); }

    auto SupportedCount = 0;
    for (auto* Material : Materials)
    {
        const auto Walk = FCk_MaterialExporter_HeadlessTextures::EnumerateUsedTextures(Material);
        if (NOT Walk.Supported)
        { continue; }
        ++SupportedCount;

        const auto SecondWalk = FCk_MaterialExporter_HeadlessTextures::EnumerateUsedTextures(Material);
        TestTrue(FString::Printf(TEXT("[%s] walk is deterministic"), *Material->GetName()),
            Walk.Textures == SecondWalk.Textures);

        if (NOT ContextCanCompareAgainstLivePath)
        { continue; }

        auto LiveTextures = TArray<UTexture*>{};
        Material->GetUsedTextures(LiveTextures);

        if (Walk.Textures != LiveTextures)
        {
            const auto Describe = [](const TArray<UTexture*>& InTextures) -> FString
            {
                auto Paths = TArray<FString>{};
                for (const auto* Texture : InTextures)
                { Paths.Add(Texture != nullptr ? Texture->GetPathName() : TEXT("<null>")); }
                return FString::Join(Paths, TEXT(", "));
            };
            AddError(FString::Printf(TEXT("[%s] walk diverges from GetUsedTextures.\n  walk: [%s]\n  live: [%s]"),
                *Material->GetName(), *Describe(Walk.Textures), *Describe(LiveTextures)));
        }
    }

    TestTrue(TEXT("the walk supports at least one engine material (mechanism alive)"), SupportedCount > 0);

    return true;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
