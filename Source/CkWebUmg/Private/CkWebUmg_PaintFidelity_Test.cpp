#include "CkCore/Ensure/CkEnsure.h"
#include "CkWebUmg/Asset/CkWebUmg_PageAssetConvert.h"
#include "CkWebUmg/Builder/CkWebUmg_Builder.h"
#include "CkWebUmg/Ir/CkWebUmg_IrLoader.h"
#include "CkWebUmg_Log.h"

#include "Engine/TextureRenderTarget2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Slate/WidgetRenderer.h"
#include "TextureResource.h"

#if WITH_AUTOMATION_TESTS

// ====================================================================================================================
// Gate 3 pixel-diff fidelity harness (docs/campaigns/webumg/Plan/Gate_03_Paint_Layer.md, work item 1).
// Renders the built widget tree offscreen via FWidgetRenderer and compares against the extraction
// golden PNG. Text-leaf regions are masked out of the pixel metric (§10 gives text its own regime).
//
// Gate 3 bring-up posture: L-pages (solid colors) ASSERT near-zero diff — rect-green must imply
// pixel-green for solids, so a failure here is a harness defect (gamma, sRGB, DPI), not a paint
// gap. P/T pages REPORT scores only until the paint work lands and Adam ratifies a threshold.
// ====================================================================================================================

namespace ck_webumg_paintfidelity
{
    constexpr uint8 ChannelTolerance = 4;        // per-channel delta considered "same" (AA jitter)
    constexpr float SolidPageFailingBudget = 0.002f; // L-pages: ≤0.2% differing pixels (AA edges)

    auto
    CorpusGoldenDir()
        -> FString
    {
        const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation"));
        if (Plugin == nullptr)
        { return {}; }

        return FPaths::Combine(Plugin->GetBaseDir(),
            TEXT("Tools"), TEXT("ckwebumg-extract"), TEXT("corpus"), TEXT("golden"));
    }

    auto
    LoadGoldenPng(
        const FString& InPngPath,
        int32& OutWidth,
        int32& OutHeight)
        -> TArray<FColor>
    {
        auto FileData = TArray<uint8>{};
        if (NOT FFileHelper::LoadFileToArray(FileData, *InPngPath))
        { return {}; }

        auto& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
        const auto Wrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
        if (NOT Wrapper.IsValid() || NOT Wrapper->SetCompressed(FileData.GetData(), FileData.Num()))
        { return {}; }

        auto Raw = TArray<uint8>{};
        if (NOT Wrapper->GetRaw(ERGBFormat::BGRA, 8, Raw))
        { return {}; }

        OutWidth = Wrapper->GetWidth();
        OutHeight = Wrapper->GetHeight();

        auto Pixels = TArray<FColor>{};
        Pixels.SetNumUninitialized(OutWidth * OutHeight);
        FMemory::Memcpy(Pixels.GetData(), Raw.GetData(), Raw.Num());
        return Pixels;
    }

    auto
    CollectTextLeafRects(
        const TSharedPtr<const FCkWebUmg_IrNode>& InNode,
        TArray<FCkWebUmg_IrRect>& OutRects)
        -> void
    {
        if (InNode->Text.IsSet() && InNode->Children.Num() == 0)
        { OutRects.Add(InNode->Box.Border); }
        for (const auto& Child : InNode->Children)
        { CollectTextLeafRects(Child, OutRects); }
    }

    auto
    DumpDiffArtifacts(
        const FString& InPageName,
        const TArray<FColor>& InRendered,
        const TArray<FColor>& InGolden,
        const TArray<FCkWebUmg_IrRect>& InMaskedRects,
        int32 InWidth,
        int32 InHeight)
        -> void
    {
        const auto DumpDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("WebUmg"));
        IFileManager::Get().MakeDirectory(*DumpDir, true);
        auto& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));

        const auto WritePng = [&](const FString& InName, const TArray<FColor>& InPixels) -> void
        {
            const auto Wrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
            Wrapper->SetRaw(InPixels.GetData(), InPixels.Num() * sizeof(FColor),
                InWidth, InHeight, ERGBFormat::BGRA, 8);
            const auto Compressed = Wrapper->GetCompressed();
            FFileHelper::SaveArrayToFile(Compressed, *FPaths::Combine(DumpDir, InName));
        };

        auto Heatmap = TArray<FColor>{};
        Heatmap.SetNumUninitialized(InWidth * InHeight);
        for (auto Index = 0; Index < InWidth * InHeight; ++Index)
        {
            const auto Delta = FMath::Max3(
                FMath::Abs(static_cast<int32>(InRendered[Index].R) - InGolden[Index].R),
                FMath::Abs(static_cast<int32>(InRendered[Index].G) - InGolden[Index].G),
                FMath::Abs(static_cast<int32>(InRendered[Index].B) - InGolden[Index].B));
            const auto X = Index % InWidth;
            const auto Y = Index / InWidth;
            const auto IsMasked = InMaskedRects.ContainsByPredicate(
                [&](const FCkWebUmg_IrRect& InRect)
                {
                    return X >= InRect.X && X < InRect.X + InRect.W
                        && Y >= InRect.Y && Y < InRect.Y + InRect.H;
                });
            // red = counted failure; blue = difference inside a masked (text) region
            Heatmap[Index] = Delta > ChannelTolerance
                ? (IsMasked ? FColor(0, 90, 255, 255) : FColor(255, 0, 0, 255))
                : FColor(InGolden[Index].R / 4, InGolden[Index].G / 4, InGolden[Index].B / 4, 255);
        }
        WritePng(InPageName + TEXT(".rendered.png"), InRendered);
        WritePng(InPageName + TEXT(".diff.png"), Heatmap);
        ck::webumg::Display(TEXT("[{}] dumped rendered + diff PNGs to [{}]"), InPageName, DumpDir);
    }

    struct FPixelScore
    {
        int32 ComparedPixels = 0;
        int32 FailingPixels = 0;
        int32 MaskedPixels = 0;
        uint8 MaxChannelDelta = 0;

        auto Get_FailingFraction() const -> float
        {
            return ComparedPixels > 0
                ? static_cast<float>(FailingPixels) / static_cast<float>(ComparedPixels)
                : 1.0f;
        }
    };

    auto
    ComparePixels(
        const TArray<FColor>& InRendered,
        const TArray<FColor>& InGolden,
        int32 InWidth,
        int32 InHeight,
        const TArray<FCkWebUmg_IrRect>& InMaskedRects)
        -> FPixelScore
    {
        auto Score = FPixelScore{};

        for (auto Y = 0; Y < InHeight; ++Y)
        {
            for (auto X = 0; X < InWidth; ++X)
            {
                const auto IsMasked = InMaskedRects.ContainsByPredicate(
                    [&](const FCkWebUmg_IrRect& InRect)
                    {
                        return X >= InRect.X && X < InRect.X + InRect.W
                            && Y >= InRect.Y && Y < InRect.Y + InRect.H;
                    });
                if (IsMasked)
                {
                    ++Score.MaskedPixels;
                    continue;
                }

                const auto Index = Y * InWidth + X;
                const auto& A = InRendered[Index];
                const auto& B = InGolden[Index];
                const auto Delta = static_cast<uint8>(FMath::Max3(
                    FMath::Abs(static_cast<int32>(A.R) - B.R),
                    FMath::Abs(static_cast<int32>(A.G) - B.G),
                    FMath::Abs(static_cast<int32>(A.B) - B.B)));

                ++Score.ComparedPixels;
                Score.MaxChannelDelta = FMath::Max(Score.MaxChannelDelta, Delta);
                if (Delta > ChannelTolerance)
                { ++Score.FailingPixels; }
            }
        }
        return Score;
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FCkWebUmg_PaintFidelity_Test,
    "CkTests.UnitTests.CkWebUmg.PaintFidelity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void
    FCkWebUmg_PaintFidelity_Test::
    GetTests(
        TArray<FString>& OutBeautifiedNames,
        TArray<FString>& OutTestCommands) const
{
    const auto GoldenDir = ck_webumg_paintfidelity::CorpusGoldenDir();

    auto Files = TArray<FString>{};
    IFileManager::Get().FindFiles(Files, *FPaths::Combine(GoldenDir, TEXT("*.ckui.json")), true, false);
    Files.Sort();

    for (const auto& File : Files)
    {
        const auto Base = FPaths::GetBaseFilename(FPaths::GetBaseFilename(File)); // strip .ckui.json
        if (Base.StartsWith(TEXT("H1")))
        { continue; } // hostile page renders unsupported effects by design — never a pixel target
        OutBeautifiedNames.Add(Base);
        OutTestCommands.Add(FPaths::Combine(GoldenDir, File));
    }
}

bool
    FCkWebUmg_PaintFidelity_Test::
    RunTest(
        const FString& InIrFilePath)
{
    using namespace ck_webumg_paintfidelity;

    // The default automation lane runs -nullrhi, where FWidgetRenderer has no RT resource; the
    // pixel suite only measures anything in the --no-nullrhi lane. Skipping (not failing) keeps
    // the default-lane count stable without pretending these pages were compared.
    if (GUsingNullRHI)
    {
        AddInfo(TEXT("NullRHI lane — pixel comparison requires the --no-nullrhi lane; skipped"));
        return true;
    }

    const auto Loaded = ck::webumg::LoadIrDocumentFromFile(InIrFilePath);
    if (NOT TestTrue(TEXT("IR document loads"), Loaded.IsSet()))
    { return false; }

    // Gate 4 exit criterion: the ratified pixel budgets hold THROUGH the emission projection —
    // every page renders from IR -> PageAsset -> IR. Projection losslessness is separately proven
    // (AssetRoundTrip); a regression here would mean the emitted form paints differently.
    // Disk-bundle texture sources are spliced back (embedded textures are ImportIdempotence's subject).
    auto* PageAsset = NewObject<UCk_WebUmg_PageAsset_UE>();
    if (NOT TestTrue(TEXT("IR converts to asset"),
            ck::webumg::ConvertIrToAsset(*Loaded, TEXT("pixel-lane"), *PageAsset)))
    { return false; }
    auto Document = TOptional<FCkWebUmg_IrDocument>{ck::webumg::ConvertAssetToIr(*PageAsset)};
    Document->AssetSourcesById = Loaded->AssetSourcesById;

    const auto Built = ck::webumg::BuildWidgetTree(*Document,
        FPaths::GetPath(InIrFilePath)); // asset srcs are IR-relative (normalized ckui-assets/)
    FlushRenderingCommands(); // freshly imported textures must have RHI resources before the draw
    if (NOT TestTrue(TEXT("widget tree builds"), Built.RootWidget != nullptr))
    { return false; }

    const auto Width = Document->Viewport.X;
    const auto Height = Document->Viewport.Y;

    const auto RenderTarget = NewObject<UTextureRenderTarget2D>();
    RenderTarget->ClearColor = FLinearColor::Transparent;
    RenderTarget->InitCustomFormat(Width, Height, PF_B8G8R8A8, false);
    RenderTarget->UpdateResourceImmediate(true);
    FlushRenderingCommands();

    // Linear pipeline with sRGB-decoded input colors: solids round-trip exactly (proven 0.0000%
    // on all L-pages). The gamma-space alternative was tried and reverted — Slate quantizes
    // vertex colors through an sRGB encode regardless, so reinterpreted colors double-encode
    // (probe arithmetic in VERIFIED.md). Translucent compositing diverges from the browser by
    // design (linear vs sRGB blend space) — measured and recorded as a §8 position.
    constexpr auto UseGammaSpace = false;
    const auto Renderer = MakeUnique<FWidgetRenderer>(UseGammaSpace);
    Renderer->DrawWidget(RenderTarget, Built.RootWidget.ToSharedRef(),
        FVector2D(Width, Height), 0.0f);
    FlushRenderingCommands();

    auto Rendered = TArray<FColor>{};
    const auto Resource = RenderTarget->GameThread_GetRenderTargetResource();
    if (NOT TestTrue(TEXT("render target resource exists"), Resource != nullptr))
    { return false; }
    if (NOT TestTrue(TEXT("render target pixels read back"), Resource->ReadPixels(Rendered)))
    { return false; }

    const auto GoldenPath = InIrFilePath.Replace(TEXT(".ckui.json"), TEXT(".golden.png"));
    auto GoldenWidth = 0;
    auto GoldenHeight = 0;
    const auto Golden = LoadGoldenPng(GoldenPath, GoldenWidth, GoldenHeight);
    if (NOT TestTrue(TEXT("golden PNG loads with matching dimensions"),
            Golden.Num() > 0 && GoldenWidth == Width && GoldenHeight == Height))
    { return false; }

    auto MaskedRects = TArray<FCkWebUmg_IrRect>{};
    CollectTextLeafRects(Document->Root, MaskedRects);

    const auto Score = ComparePixels(Rendered, Golden, Width, Height, MaskedRects);

    const auto PageName = FPaths::GetBaseFilename(FPaths::GetBaseFilename(InIrFilePath));
    const auto ProbeAt = [&](int32 InX, int32 InY) -> FString
    {
        const auto Index = InY * Width + InX;
        return FString::Printf(TEXT("(%d,%d) rendered [%d,%d,%d,%d] golden [%d,%d,%d,%d]"),
            InX, InY,
            Rendered[Index].R, Rendered[Index].G, Rendered[Index].B, Rendered[Index].A,
            Golden[Index].R, Golden[Index].G, Golden[Index].B, Golden[Index].A);
    };
    AddInfo(FString::Printf(
        TEXT("[%s] failing fraction %.4f%% (%d/%d px over delta %d), max channel delta %d, masked %d text px; probes: %s | %s | %s"),
        *PageName, Score.Get_FailingFraction() * 100.0f, Score.FailingPixels, Score.ComparedPixels,
        ChannelTolerance, Score.MaxChannelDelta, Score.MaskedPixels,
        *ProbeAt(10, 10), *ProbeAt(100, 100), *ProbeAt(Width / 2, Height / 2)));

    // Gate 3 ratified thresholds (2026-08-01, per-class — measured floors in VERIFIED.md):
    // solids 0.2%; opaque paint pages 0.5% (AA floor 0.17%); translucency-bearing pages 7%
    // (the §8.4 compositing-space band, policy option 1 — floors 5.75%/4.39% are the platform
    // ceiling, not unbuilt features). Never tolerate via the channel threshold.
    const auto Budget = [&]() -> float
    {
        if (PageName.StartsWith(TEXT("L")) || PageName.StartsWith(TEXT("C")))
        { return SolidPageFailingBudget; }
        if (PageName.StartsWith(TEXT("P3")) || PageName.StartsWith(TEXT("P4")))
        { return 0.07f; }
        return 0.005f;
    }();

    const auto WithinBudget = Score.Get_FailingFraction() <= Budget;
    TestTrue(FString::Printf(TEXT("[%s] pixel parity — failing fraction %.4f%% within budget %.4f%%"),
            *PageName, Score.Get_FailingFraction() * 100.0f, Budget * 100.0f),
        WithinBudget);

    if (NOT WithinBudget || Score.Get_FailingFraction() > 0.01f)
    {
        DumpDiffArtifacts(PageName, Rendered, Golden, MaskedRects, Width, Height);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
