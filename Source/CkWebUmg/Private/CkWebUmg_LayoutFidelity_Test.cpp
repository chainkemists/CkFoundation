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

            // Line-run comparison (the §8.1 regime's measured datum): Slate advance width with the
            // mapped OS font vs Chromium's recorded line box. Single-line runs only — multi-line
            // needs identical wrap decisions first, which the tolerance decision may not require.
            if (IrNode->Text->LineBoxes.Num() == 1 && FSlateApplication::IsInitialized())
            {
                const auto FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
                const auto Measured = FontMeasure->Measure(
                    IrNode->Text->Content, ck::webumg::MakeWebFontInfo(*IrNode->Text), 1.0f);
                const auto& LineBox = IrNode->Text->LineBoxes[0];
                AddInfo(FString::Printf(
                    TEXT("[%s] line-run: chromium %.2fx%.2f, slate %.2fx%.2f, advance delta %.2fpx (%.1f%%)"),
                    *NodeId, LineBox.W, LineBox.H, Measured.X, Measured.Y,
                    Measured.X - LineBox.W,
                    LineBox.W > 0.0f ? (Measured.X - LineBox.W) / LineBox.W * 100.0f : 0.0f));
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

#endif // WITH_AUTOMATION_TESTS
