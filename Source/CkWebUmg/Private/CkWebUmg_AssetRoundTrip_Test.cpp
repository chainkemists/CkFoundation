#include "CkCore/Ensure/CkEnsure.h"
#include "CkWebUmg/Asset/CkWebUmg_PageAssetConvert.h"
#include "CkWebUmg/Builder/CkWebUmg_Builder.h"
#include "CkWebUmg/Ir/CkWebUmg_IrLoader.h"

#include "Interfaces/IPluginManager.h"
#include "Layout/ArrangedChildren.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#if WITH_AUTOMATION_TESTS

// ====================================================================================================================
// Gate 4 round-trip: golden IR -> PageAsset -> IR -> widget tree must arrange within the SAME ±1px
// contract the direct path holds, and the projection must be idempotent (asset -> IR -> asset is
// property-identical). The emission guard proves DECISION 3's duplicate-data-ck-name hard error.
// ====================================================================================================================

namespace ck_webumg_assetroundtrip
{
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
    ExportNodes(
        const UCk_WebUmg_PageAsset_UE& InAsset)
        -> FString
    {
        auto Result = FString{};
        for (const auto& Node : InAsset.Get_Nodes())
        {
            auto NodeText = FString{};
            FCk_WebUmg_NodeData::StaticStruct()->ExportText(
                NodeText, &Node, nullptr, nullptr, PPF_None, nullptr);
            Result += NodeText + TEXT("\n");
        }
        return Result;
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
        { CollectArrangedRects(Arranged[Index].Widget, Arranged[Index].Geometry, InOutRects); }
    }

    // Transform-bearing subtrees are excluded from the rect check: ArrangeChildren geometry
    // accumulates RENDER transforms (GetAbsolutePosition is post-transform), so those nodes
    // compare against the wrong space — the pixel suite owns transformed output, and the
    // idempotence check already proves the transform fields survive the projection.
    auto
    CollectIrNodes(
        const TSharedPtr<const FCkWebUmg_IrNode>& InNode,
        TArray<TSharedPtr<const FCkWebUmg_IrNode>>& InOutNodes)
        -> void
    {
        if (InNode->Paint.Transform.IsSet())
        { return; }
        InOutNodes.Add(InNode);
        for (const auto& Child : InNode->Children)
        { CollectIrNodes(Child, InOutNodes); }
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FCkWebUmg_AssetRoundTrip_Test,
    "CkTests.UnitTests.CkWebUmg.AssetRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void
    FCkWebUmg_AssetRoundTrip_Test::
    GetTests(
        TArray<FString>& OutBeautifiedNames,
        TArray<FString>& OutTestCommands) const
{
    const auto GoldenDir = ck_webumg_assetroundtrip::CorpusGoldenDir();
    auto Files = TArray<FString>{};
    IFileManager::Get().FindFiles(Files, *FPaths::Combine(GoldenDir, TEXT("*.ckui.json")), true, false);
    Files.Sort();
    for (const auto& File : Files)
    {
        if (File.StartsWith(TEXT("H1")))
        { continue; } // the hostile page is the emission guard's subject — it must FAIL to convert
        OutBeautifiedNames.Add(FPaths::GetBaseFilename(FPaths::GetBaseFilename(File)));
        OutTestCommands.Add(FPaths::Combine(GoldenDir, File));
    }
}

bool
    FCkWebUmg_AssetRoundTrip_Test::
    RunTest(
        const FString& InIrFilePath)
{
    using namespace ck_webumg_assetroundtrip;

    const auto Document = ck::webumg::LoadIrDocumentFromFile(InIrFilePath);
    if (NOT TestTrue(TEXT("IR loads"), Document.IsSet()))
    { return false; }

    auto* Asset = NewObject<UCk_WebUmg_PageAsset_UE>();
    if (NOT TestTrue(TEXT("IR converts to asset"),
            ck::webumg::ConvertIrToAsset(*Document, TEXT("test-hash"), *Asset)))
    { return false; }

    // Idempotence: asset -> IR -> asset must be property-identical (regeneration is a no-op).
    const auto RoundTrippedIr = ck::webumg::ConvertAssetToIr(*Asset);
    auto* SecondAsset = NewObject<UCk_WebUmg_PageAsset_UE>();
    if (NOT TestTrue(TEXT("round-tripped IR converts again"),
            ck::webumg::ConvertIrToAsset(RoundTrippedIr, TEXT("test-hash"), *SecondAsset)))
    { return false; }
    TestTrue(TEXT("projection is idempotent (node arrays property-identical)"),
        ExportNodes(*Asset) == ExportNodes(*SecondAsset));

    // Fidelity through the asset: the rebuilt tree arranges within the same ±1px contract.
    const auto Built = ck::webumg::BuildWidgetTree(RoundTrippedIr);
    if (NOT TestTrue(TEXT("asset-backed widget tree builds"), Built.RootWidget != nullptr))
    { return false; }

    const auto RootWidget = Built.RootWidget.ToSharedRef();
    RootWidget->SlatePrepass(1.0f);
    const auto RootGeometry = FGeometry::MakeRoot(
        FVector2D(RoundTrippedIr.Viewport.X, RoundTrippedIr.Viewport.Y), FSlateLayoutTransform{});

    auto ArrangedRects = TMap<const SWidget*, FSlateRect>{};
    CollectArrangedRects(RootWidget, RootGeometry, ArrangedRects);

    auto IrNodes = TArray<TSharedPtr<const FCkWebUmg_IrNode>>{};
    CollectIrNodes(RoundTrippedIr.Root, IrNodes);

    // The data-ck-name binding surface: every named node in the (unique-name-guaranteed) tree
    // must resolve to its widget through the asset path.
    {
        const auto CountNames = [](const auto& InSelf, const TSharedPtr<const FCkWebUmg_IrNode>& InNode) -> int32
        {
            auto Count = InNode->CkName.IsEmpty() ? 0 : 1;
            for (const auto& Child : InNode->Children)
            { Count += InSelf(InSelf, Child); }
            return Count;
        };
        TestTrue(TEXT("every data-ck-name resolves to a widget"),
            Built.WidgetsByCkName.Num() == CountNames(CountNames, RoundTrippedIr.Root));

        const auto CountBinds = [](const auto& InSelf, const TSharedPtr<const FCkWebUmg_IrNode>& InNode) -> int32
        {
            auto Count = InNode->CkBind.IsEmpty() ? 0 : 1;
            for (const auto& Child : InNode->Children)
            { Count += InSelf(InSelf, Child); }
            return Count;
        };
        const auto CountOriginalBinds = CountBinds(CountBinds, Document->Root);
        TestTrue(TEXT("data-ck-bind survives the projection"),
            CountBinds(CountBinds, RoundTrippedIr.Root) == CountOriginalBinds);
    }

    for (const auto& IrNode : IrNodes)
    {
        if (IrNode->Text.IsSet() && IrNode->Children.Num() == 0)
        { continue; } // text leaves are the text regime's concern, same as the direct path

        const auto* Widget = Built.WidgetsByIrId.Find(IrNode->Id);
        if (NOT TestTrue(FString::Printf(TEXT("widget exists for [%s]"), *IrNode->Id), Widget != nullptr))
        { continue; }
        const auto* Rect = ArrangedRects.Find(Widget->Get());
        if (NOT TestTrue(FString::Printf(TEXT("arranged rect for [%s]"), *IrNode->Id), Rect != nullptr))
        { continue; }

        const auto& Expected = IrNode->Get_LayoutBox().Border;
        const auto Deviation = FMath::Max(
            FMath::Max(FMath::Abs(Rect->Left - Expected.X), FMath::Abs(Rect->Top - Expected.Y)),
            FMath::Max(FMath::Abs(Rect->GetSize().X - Expected.W), FMath::Abs(Rect->GetSize().Y - Expected.H)));

        TestFalse(FString::Printf(TEXT("[%s] NaN geometry"), *IrNode->Id),
            FMath::IsNaN(Rect->Left) || FMath::IsNaN(Rect->GetSize().X));
        TestTrue(FString::Printf(TEXT("[%s] asset-path rect within ±1px (%.2fpx)"), *IrNode->Id, Deviation),
            Deviation <= 1.0f);
    }
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCkWebUmg_AssetEmitGuard_Test,
    "CkTests.UnitTests.CkWebUmg.AssetEmitGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool
    FCkWebUmg_AssetEmitGuard_Test::
    RunTest(
        const FString&)
{
    using namespace ck_webumg_assetroundtrip;

    const auto Document = ck::webumg::LoadIrDocumentFromFile(
        FPaths::Combine(CorpusGoldenDir(), TEXT("H1_hostile.ckui.json")));
    if (NOT TestTrue(TEXT("hostile IR loads"), Document.IsSet()))
    { return false; }

    // DECISION 3: a duplicate data-ck-name is a hard emit error, not a warning.
    AddExpectedError(TEXT("duplicate data-ck-name"), EAutomationExpectedErrorFlags::Contains, 0);
    auto* Asset = NewObject<UCk_WebUmg_PageAsset_UE>();
    const auto Converted = ck::webumg::ConvertIrToAsset(*Document, TEXT("test-hash"), *Asset);

    TestFalse(TEXT("duplicate data-ck-name rejects emission"), Converted);
    TestTrue(TEXT("rejected asset stays untouched (atomic emission)"), Asset->Get_Nodes().Num() == 0);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

// The no-silent-drops contract survives into the asset: node-level unsupported entries and
// page-level diagnostics both land in the conversion report with provenance intact.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCkWebUmg_ConversionReport_Test,
    "CkTests.UnitTests.CkWebUmg.ConversionReport",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool
    FCkWebUmg_ConversionReport_Test::
    RunTest(
        const FString&)
{
    auto Document = FCkWebUmg_IrDocument{};
    Document.Schema = 1;
    Document.Viewport = FIntPoint(100, 100);
    Document.Root = MakeShared<FCkWebUmg_IrNode>();
    Document.Root->Id = TEXT("n0");
    Document.Root->Unsupported.Add(
        FCkWebUmg_IrUnsupported{TEXT("backdrop-filter"), TEXT("blur(4px)"), TEXT("styles.css:12")});
    Document.Diagnostics.Add(
        FCkWebUmg_IrDiagnostic{TEXT("unknown-ck-attribute"), TEXT("n0"), TEXT("data-ck-bogus")});

    auto* Asset = NewObject<UCk_WebUmg_PageAsset_UE>();
    if (NOT TestTrue(TEXT("converts"), ck::webumg::ConvertIrToAsset(Document, TEXT("h"), *Asset)))
    { return false; }

    const auto& Report = Asset->Get_ConversionReport();
    TestTrue(TEXT("both report rows present"), Report.Num() == 2);
    TestTrue(TEXT("unsupported row carries provenance"),
        Report.Num() == 2 && Report[0].Get_Property() == TEXT("backdrop-filter")
        && Report[0].Get_Source() == TEXT("styles.css:12"));
    TestTrue(TEXT("page diagnostic row present"),
        Report.Num() == 2 && Report[1].Get_Property() == TEXT("unknown-ck-attribute")
        && Report[1].Get_Source() == TEXT("page-diagnostic"));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
