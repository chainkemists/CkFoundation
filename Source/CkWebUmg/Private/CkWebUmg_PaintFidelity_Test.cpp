#include "CkCore/Ensure/CkEnsure.h"
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

    const auto Document = ck::webumg::LoadIrDocumentFromFile(InIrFilePath);
    if (NOT TestTrue(TEXT("IR document loads"), Document.IsSet()))
    { return false; }

    const auto Built = ck::webumg::BuildWidgetTree(Document->Root);
    if (NOT TestTrue(TEXT("widget tree builds"), Built.RootWidget != nullptr))
    { return false; }

    const auto Width = Document->Viewport.X;
    const auto Height = Document->Viewport.Y;

    const auto RenderTarget = NewObject<UTextureRenderTarget2D>();
    RenderTarget->ClearColor = FLinearColor::Transparent;
    RenderTarget->InitCustomFormat(Width, Height, PF_B8G8R8A8, false);
    RenderTarget->UpdateResourceImmediate(true);
    FlushRenderingCommands();

    // FWidgetRenderer's ctor arg is bUseGammaSpace: true ADDS a linear->sRGB encode on write,
    // which double-encodes against an sRGB golden (probes matched sRGBencode(golden) exactly).
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

    // Solid-color layout pages gate NOW; paint/text pages (and smoke, which uses radii) report
    // until the Gate 3 paint work lands and Adam ratifies the paint threshold.
    if (PageName.StartsWith(TEXT("L")))
    {
        TestTrue(FString::Printf(TEXT("[%s] solid-page pixel parity — failing fraction %.4f%% within budget %.4f%%"),
                *PageName, Score.Get_FailingFraction() * 100.0f, SolidPageFailingBudget * 100.0f),
            Score.Get_FailingFraction() <= SolidPageFailingBudget);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
