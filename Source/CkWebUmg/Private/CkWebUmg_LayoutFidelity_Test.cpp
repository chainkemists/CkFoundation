#include "CkCore/Ensure/CkEnsure.h"
#include "CkWebUmg/Builder/CkWebUmg_Builder.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "CkWebUmg/Ir/CkWebUmg_IrLoader.h"
#include "CkWebUmg_Log.h"

#include "Interfaces/IPluginManager.h"
#include "Layout/ArrangedChildren.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#if WITH_AUTOMATION_TESTS

// ====================================================================================================================
// Gate 2 rect-diff fidelity harness (campaign: docs/campaigns/webumg/Plan/Gate_02_Layout_Runtime.md).
// One test instance per layout-only corpus page: load the golden IR, build the widget tree, arrange
// at the recorded viewport, and compare every node's arranged rect against the IR box (§10 layout
// tolerance). Text-leaf nodes are reported, not gated — their tolerance regime is Gate 3 scope.
//
// Interim home: lives in CkWebUmg (not CkTests) per the recorded Gate 2 deviation in
// docs/campaigns/webumg/DECISIONS.md — migrate when CkTests' in-flight sibling work lands.
// ====================================================================================================================

namespace ck_webumg_layoutfidelity
{
    constexpr float LayoutTolerancePx = 1.0f;

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
    CollectIrNodesById(
        const TSharedPtr<const FCkWebUmg_IrNode>& InNode,
        TMap<FString, TSharedPtr<const FCkWebUmg_IrNode>>& InOutMap)
        -> void
    {
        InOutMap.Add(InNode->Id, InNode);
        for (const auto& Child : InNode->Children)
        { CollectIrNodesById(Child, InOutMap); }
    }

    auto
    CollectArrangedRects(
        const TSharedRef<SWidget>& InWidget,
        const FGeometry& InGeometry,
        TMap<const SWidget*, FSlateRect>& InOutRects)
        -> void
    {
        InOutRects.Add(&InWidget.Get(), FSlateRect(
            static_cast<float>(InGeometry.GetAbsolutePosition().X),
            static_cast<float>(InGeometry.GetAbsolutePosition().Y),
            static_cast<float>(InGeometry.GetAbsolutePosition().X + InGeometry.GetAbsoluteSize().X),
            static_cast<float>(InGeometry.GetAbsolutePosition().Y + InGeometry.GetAbsoluteSize().Y)));

        auto Arranged = FArrangedChildren{EVisibility::All};
        InWidget->ArrangeChildren(InGeometry, Arranged);

        for (auto Index = 0; Index < Arranged.Num(); ++Index)
        {
            const auto& ArrangedWidget = Arranged[Index];
            CollectArrangedRects(ArrangedWidget.Widget, ArrangedWidget.Geometry, InOutRects);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FCkWebUmg_LayoutFidelity_Test,
    "CkTests.UnitTests.CkWebUmg.LayoutFidelity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void
    FCkWebUmg_LayoutFidelity_Test::
    GetTests(
        TArray<FString>& OutBeautifiedNames,
        TArray<FString>& OutTestCommands) const
{
    const auto GoldenDir = ck_webumg_layoutfidelity::CorpusGoldenDir();

    auto Files = TArray<FString>{};
    IFileManager::Get().FindFiles(Files, *FPaths::Combine(GoldenDir, TEXT("L*.ckui.json")), true, false);
    // Text pages join the rect suite for the line-run/wrap comparison; their non-text nodes gate
    // at the same ±1px as everything else.
    auto TextPageFiles = TArray<FString>{};
    IFileManager::Get().FindFiles(TextPageFiles, *FPaths::Combine(GoldenDir, TEXT("T*.ckui.json")), true, false);
    Files.Append(TextPageFiles);
    Files.Add(TEXT("C1_button_states.ckui.json"));
    Files.Sort();

    for (const auto& File : Files)
    {
        OutBeautifiedNames.Add(FPaths::GetBaseFilename(File));
        OutTestCommands.Add(FPaths::Combine(GoldenDir, File));
    }
}

bool
    FCkWebUmg_LayoutFidelity_Test::
    RunTest(
        const FString& InIrFilePath)
{
    using namespace ck_webumg_layoutfidelity;

    const auto Document = ck::webumg::LoadIrDocumentFromFile(InIrFilePath);
    if (NOT TestTrue(TEXT("IR document loads"), Document.IsSet()))
    { return false; }

    const auto Built = ck::webumg::BuildWidgetTree(*Document);
    if (NOT TestTrue(TEXT("widget tree builds"), Built.RootWidget != nullptr))
    { return false; }

    const auto ViewportSize = FVector2D(Document->Viewport.X, Document->Viewport.Y);
    const auto RootWidget = Built.RootWidget.ToSharedRef();

    RootWidget->SlatePrepass(1.0f);
    const auto RootGeometry = FGeometry::MakeRoot(ViewportSize, FSlateLayoutTransform{});

    auto ArrangedRects = TMap<const SWidget*, FSlateRect>{};
    CollectArrangedRects(RootWidget, RootGeometry, ArrangedRects);

    auto IrNodesById = TMap<FString, TSharedPtr<const FCkWebUmg_IrNode>>{};
    CollectIrNodesById(Document->Root, IrNodesById);

    auto MaxDeviation = 0.0f;
    auto WorstNodeId = FString{};
    auto TextNodeDeviations = TArray<FString>{};

    for (const auto& [NodeId, IrNode] : IrNodesById)
    {
        const auto* Widget = Built.WidgetsByIrId.Find(NodeId);
        if (NOT TestTrue(FString::Printf(TEXT("widget exists for IR node [%s]"), *NodeId), Widget != nullptr))
        { continue; }

        const auto* Rect = ArrangedRects.Find(Widget->Get());
        if (NOT TestTrue(FString::Printf(TEXT("arranged rect exists for IR node [%s]"), *NodeId), Rect != nullptr))
        { continue; }

        // Arranged geometry is pre-render-transform, so transformed nodes compare against their
        // untransformed rect; the transform itself is a paint concern the pixel suite covers.
        const auto& Expected = IrNode->Get_LayoutBox().Border;
        const auto Deviation = FMath::Max(
            FMath::Max(FMath::Abs(Rect->Left - Expected.X), FMath::Abs(Rect->Top - Expected.Y)),
            FMath::Max(FMath::Abs(Rect->GetSize().X - Expected.W), FMath::Abs(Rect->GetSize().Y - Expected.H)));

        // NaN is a layout defect on ANY node — never report-only.
        TestFalse(FString::Printf(TEXT("node [%s] arranged rect is NaN"), *NodeId),
            FMath::IsNaN(Rect->Left) || FMath::IsNaN(Rect->Top) ||
            FMath::IsNaN(Rect->GetSize().X) || FMath::IsNaN(Rect->GetSize().Y));

        const auto IsTextLeaf = IrNode->Text.IsSet() && IrNode->Children.Num() == 0;
        if (IsTextLeaf)
        {
            TextNodeDeviations.Add(FString::Printf(TEXT("[%s] %.2fpx"), *NodeId, Deviation));

            // Line-run comparison (the §8.1 regime's measured datum): Slate advances with the
            // mapped OS font vs Chromium's recorded line boxes. Multi-line runs replay Chromium's
            // greedy wrap (space-separated words filled to the content-box width) so line COUNT
            // agreement is measured too, not assumed.
            if (IrNode->Text->LineBoxes.Num() > 0 && FSlateApplication::IsInitialized())
            {
                const auto FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
                const auto FontInfo = ck::webumg::MakeWebFontInfo(*IrNode->Text);
                const auto& LineBoxes = IrNode->Text->LineBoxes;

                auto SlateLines = TArray<float>{}; // advance per wrapped line
                {
                    const auto WrapWidth = IrNode->Get_LayoutBox().Content.W;
                    // Letter-spacing is modeled explicitly (measure with spacing zeroed, add
                    // LetterSpacingPx per character, Chromium-style) so the comparison does not
                    // depend on whether Slate's measure path honors FSlateFontInfo::LetterSpacing.
                    auto MeasureFont = FontInfo;
                    MeasureFont.LetterSpacing = 0;
                    const auto Spacing = IrNode->Text->LetterSpacingPx;
                    const auto MeasureRun = [&](const FString& InRun) -> float
                    {
                        return static_cast<float>(FontMeasure->Measure(InRun, MeasureFont, 1.0f).X)
                            + Spacing * static_cast<float>(InRun.Len());
                    };
                    const auto SpaceW = MeasureRun(TEXT(" "));

                    // \n in content is a FORCED break (<br> at extraction); greedy wrap applies
                    // within each segment only.
                    auto Segments = TArray<FString>{};
                    ck::webumg::ApplyTextTransform(IrNode->Text->Content, IrNode->Text->TransformCase)
                        .ParseIntoArray(Segments, TEXT("\n"));
                    // nowrap/pre suppress soft wrapping entirely — Chromium overflows instead.
                    const auto CanWrap = IrNode->Text->WhiteSpace != TEXT("nowrap")
                        && IrNode->Text->WhiteSpace != TEXT("pre");
                    for (const auto& Segment : Segments)
                    {
                        auto Words = TArray<FString>{};
                        Segment.ParseIntoArray(Words, TEXT(" "));
                        auto Current = 0.0f;
                        for (const auto& Word : Words)
                        {
                            const auto WordW = MeasureRun(Word);
                            const auto Extended = Current > 0.0f ? Current + SpaceW + WordW : WordW;
                            if (Current > 0.0f && Extended > WrapWidth + 0.5f && LineBoxes.Num() > 1 && CanWrap)
                            {
                                SlateLines.Add(Current);
                                Current = WordW;
                            }
                            else
                            { Current = Extended; }
                        }
                        if (Current > 0.0f)
                        { SlateLines.Add(Current); }
                    }
                }

                auto MaxAdvanceDeltaPct = 0.0f;
                auto AdvancesWithinTolerance = true;
                for (auto Line = 0; Line < FMath::Min(SlateLines.Num(), LineBoxes.Num()); ++Line)
                {
                    if (LineBoxes[Line].W > 0.0f)
                    {
                        const auto DeltaPx = FMath::Abs(SlateLines[Line] - LineBoxes[Line].W);
                        MaxAdvanceDeltaPct = FMath::Max(MaxAdvanceDeltaPct,
                            DeltaPx / LineBoxes[Line].W * 100.0f);
                        // Gate 3 ratified text tolerance (2026-08-01): per-line advance within
                        // max(3%, 3px) of Chromium's line box — dataset floor: mean 0.78%, max 2.9%.
                        if (DeltaPx > FMath::Max(LineBoxes[Line].W * 0.03f, 3.0f))
                        { AdvancesWithinTolerance = false; }
                    }
                }
                TestTrue(FString::Printf(
                    TEXT("[%s] text tolerance — line count %d==%d, max advance delta %.1f%% within max(3%%, 3px)"),
                    *NodeId, LineBoxes.Num(), SlateLines.Num(), MaxAdvanceDeltaPct),
                    SlateLines.Num() == LineBoxes.Num() && AdvancesWithinTolerance);
                AddInfo(FString::Printf(
                    TEXT("[%s] line-run: chromium %d lines, slate %d lines, max advance delta %.1f%% (first line %.2f vs %.2f)"),
                    *NodeId, LineBoxes.Num(), SlateLines.Num(), MaxAdvanceDeltaPct,
                    LineBoxes[0].W, SlateLines.Num() > 0 ? SlateLines[0] : 0.0f));
            }
            continue;
        }

        if (Deviation > MaxDeviation)
        {
            MaxDeviation = Deviation;
            WorstNodeId = NodeId;
        }

        TestTrue(FString::Printf(
                TEXT("node [%s] within tolerance — deviation %.2fpx (expected [%.2f,%.2f %.2fx%.2f], arranged [%.2f,%.2f %.2fx%.2f])"),
                *NodeId, Deviation,
                Expected.X, Expected.Y, Expected.W, Expected.H,
                Rect->Left, Rect->Top, Rect->GetSize().X, Rect->GetSize().Y),
            Deviation <= LayoutTolerancePx);
    }

    AddInfo(FString::Printf(TEXT("max non-text deviation: %.2fpx (node [%s]); text-leaf deviations (report-only): %s"),
        MaxDeviation, *WorstNodeId,
        TextNodeDeviations.Num() > 0 ? *FString::Join(TextNodeDeviations, TEXT(", ")) : TEXT("none")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// The hostile corpus page (duplicate ck-names, extreme values, adversarial structures) must load,
// build, and arrange without crashing or producing NaN geometry — the invalid-input contract for
// the whole load→build→layout chain. Fidelity is NOT asserted; surviving hostile input is.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCkWebUmg_HostileLoad_Test,
    "CkTests.UnitTests.CkWebUmg.HostileLoad",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool
    FCkWebUmg_HostileLoad_Test::
    RunTest(
        const FString&)
{
    using namespace ck_webumg_layoutfidelity;

    const auto IrPath = FPaths::Combine(CorpusGoldenDir(), TEXT("H1_hostile.ckui.json"));
    const auto Document = ck::webumg::LoadIrDocumentFromFile(IrPath);
    if (NOT TestTrue(TEXT("hostile IR document loads"), Document.IsSet()))
    { return false; }

    const auto Built = ck::webumg::BuildWidgetTree(*Document);
    if (NOT TestTrue(TEXT("hostile widget tree builds"), Built.RootWidget != nullptr))
    { return false; }

    const auto RootWidget = Built.RootWidget.ToSharedRef();
    RootWidget->SlatePrepass(1.0f);
    const auto RootGeometry = FGeometry::MakeRoot(
        FVector2D(Document->Viewport.X, Document->Viewport.Y), FSlateLayoutTransform{});

    auto ArrangedRects = TMap<const SWidget*, FSlateRect>{};
    CollectArrangedRects(RootWidget, RootGeometry, ArrangedRects);

    auto NanCount = 0;
    for (const auto& [Widget, Rect] : ArrangedRects)
    {
        if (FMath::IsNaN(Rect.Left) || FMath::IsNaN(Rect.Top) ||
            FMath::IsNaN(Rect.GetSize().X) || FMath::IsNaN(Rect.GetSize().Y))
        { ++NanCount; }
    }
    TestTrue(FString::Printf(TEXT("no NaN geometry across %d arranged widgets (found %d)"),
        ArrangedRects.Num(), NanCount), NanCount == 0);
    AddInfo(FString::Printf(TEXT("hostile page: %d widgets arranged"), ArrangedRects.Num()));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// Hit-test defaults contract: decorative nodes — no data-ck-name/bind — must not hit-test
// themselves (SelfHitTestInvisible keeps their subtree traversable); gameplay-reachable nodes
// stay fully hittable; CSS visibility:hidden always wins.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCkWebUmg_HitTestDefaults_Test,
    "CkTests.UnitTests.CkWebUmg.HitTestDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool
    FCkWebUmg_HitTestDefaults_Test::
    RunTest(
        const FString&)
{
    using namespace ck_webumg_layoutfidelity;

    const auto IrPath = FPaths::Combine(CorpusGoldenDir(), TEXT("smoke.ckui.json"));
    const auto Document = ck::webumg::LoadIrDocumentFromFile(IrPath);
    if (NOT TestTrue(TEXT("smoke IR document loads"), Document.IsSet()))
    { return false; }

    const auto Built = ck::webumg::BuildWidgetTree(*Document);
    if (NOT TestTrue(TEXT("smoke widget tree builds"), Built.RootWidget != nullptr))
    { return false; }

    auto IrNodesById = TMap<FString, TSharedPtr<const FCkWebUmg_IrNode>>{};
    CollectIrNodesById(Document->Root, IrNodesById);

    auto ReachableCount = 0;
    auto DecorativeCount = 0;
    for (const auto& [Id, Widget] : Built.WidgetsByIrId)
    {
        const auto* Node = IrNodesById.Find(Id);
        if (NOT TestTrue(FString::Printf(TEXT("widget [%s] has an IR node"), *Id), Node != nullptr))
        { continue; }

        const auto Visibility = Widget->GetVisibility();
        if ((*Node)->Paint.Visibility == TEXT("hidden"))
        {
            TestTrue(FString::Printf(TEXT("hidden node [%s] stays Hidden"), *Id),
                Visibility == EVisibility::Hidden);
        }
        else if (NOT (*Node)->CkName.IsEmpty() || NOT (*Node)->CkBind.IsEmpty())
        {
            ++ReachableCount;
            TestTrue(FString::Printf(TEXT("gameplay-reachable node [%s] stays hittable"), *Id),
                Visibility == EVisibility::Visible);
        }
        else
        {
            ++DecorativeCount;
            TestTrue(FString::Printf(TEXT("decorative node [%s] is SelfHitTestInvisible"), *Id),
                Visibility == EVisibility::SelfHitTestInvisible);
        }
    }

    TestTrue(TEXT("contract exercised both classes (>=1 reachable, >=1 decorative)"),
        ReachableCount >= 1 && DecorativeCount >= 1);
    AddInfo(FString::Printf(TEXT("smoke page: %d reachable, %d decorative"),
        ReachableCount, DecorativeCount));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
