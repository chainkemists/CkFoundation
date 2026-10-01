#include "CkPso/Diagnostics/CkPso_MissLogParser.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Macros/CkMacros.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

/**
 * FIXTURES - constructed, not captured. Each piece below is one Appendf / operator<< of the engine's miss builder,
 * in emission order, so an engine upgrade re-derives them piece by piece. Names and hashes are illustrative; the
 * labels, tab runs, line breaks, ordering and placeholder tokens are the engine's.
 *
 * Source of truth: UnrealEngine-Angelscript fork 5.7.4, Engine/Source/Runtime/...
 *   Engine/Private/PSOPrecacheValidation.cpp
 *     :489-501  PSOMissStringBuilder, GetPSOMissTypeName  -> "ShadersOnly" | "MinimalPSOState" | "FullPSO"
 *     :503-511  AppendCSVProfilerInfo                     -> "\nFrame:\t\t\t\t\t%d" (no leading tab)
 *     :513-556  LogGeneralPSOMissInfo                     -> the graphics header, every fixture's first block
 *     :558-582  LogMaterialPSOPrecacheRequestData         -> "Precached with:" lines   (Get_ShadersOnly_Missed_DuringCsvCapture)
 *     :585-604  CompareStateAndLogChanges(Float)          -> "* X different:" triples  (both diff fixtures)
 *     :741-763  CompareRHIRasterizerStateAndLogChanges    -> "- RasterizerState different:"
 *     :787-807  GetEffectiveMaterial                      -> "\n\tUsing Default Material" INSIDE the header run
 *     :857-913  LogMinimalPSOStateMissInfo                -> repeats Material / VertexFactoryType / PassName
 *     :915-975  LogFullPSOStateMissInfo                   -> render-target diff, or :973 "No material found"
 *     :979-1045 LogPSOMissInfo (graphics)                 -> Untracked :997-1005, Missed :1007-1030, TooLate :1031-1036
 *     :1047-1074 LogPSOMissInfo (compute)                 -> "Compute" type, "PassName", "Compute Shader Hash"
 *     :1044, :1073  UE_LOG(LogEngine, Log, TEXT("%s\n"))  -> every bare message ends with one extra "\n"
 *     :402, :422, :445  callers: mesh-pass full PSO, compute, global graphics (null material / VF / proxy)
 *   Renderer/Private/MeshPassProcessor.cpp:2280, :2303    -> ShadersOnly and MinimalPSOState callers
 *   RHI/Private/PipelineStateCache.cpp:414-428            -> state tokens; TooLate prints "Too Late"
 *   Engine/Public/PSOPrecache.h:323-331, PSOPrecacheMaterial.h:65-75 -> pass name "Unknown" for INDEX_NONE
 *   Core/Public/Misc/SecureHash.h:242-245 + Containers/UnrealString.h:68-76 -> hashes are 40 uppercase hex
 *   Core/Private/Misc/OutputDeviceHelper.cpp:10-73, :108-139 -> log-file prefix "[time][frame]Category: " written
 *       once before the whole message, then LINE_TERMINATOR ("\r\n" on Windows); inner breaks stay "\n"
 *
 * Compile-time switches that change the shape:
 *   MDCStatsCategory line      MESH_DRAW_COMMAND_STATS = !UE_BUILD_SHIPPING (MeshDrawCommandStatsDefines.h:6)
 *   Frame line                 CSV_PROFILER_STATS (CsvProfilerConfig.h:35) AND a capture running
 *   Everything after the header PSO_PRECACHING_TRACKING (PSOPrecacheValidation.h:15): not Shipping, not Test
 */
namespace ck_test_pso_miss_log_parser
{
    constexpr auto Hash_Vertex = TEXT("1A2B3C4D5E6F708192A3B4C5D6E7F80910111213");
    constexpr auto Hash_Pixel = TEXT("F0E1D2C3B4A5968778695A4B3C2D1E0F00112233");
    constexpr auto Hash_Geometry = TEXT("9999888877776666555544443333222211110000");
    constexpr auto Hash_Mesh = TEXT("AAAABBBBCCCCDDDDEEEEFFFF0000111122223333");
    constexpr auto Hash_Amplification = TEXT("1234567890ABCDEF1234567890ABCDEF12345678");
    constexpr auto Hash_Compute = TEXT("C0FFEE00C0FFEE00C0FFEE00C0FFEE00C0FFEE00");

    constexpr auto Marker = TEXT("PSO PRECACHING MISS:");

    // --------------------------------------------------------------------------------------------------------------------

    struct FTextCase
    {
        FString _Text;
        const TCHAR* _What = nullptr;
    };

    // --------------------------------------------------------------------------------------------------------------------

    auto
        DoConcat(
            const TArray<const TCHAR*>& InPieces)
        -> FString
    {
        auto Result = FString{};
        ck::algo::ForEach(InPieces, [&Result](const TCHAR* InPiece)
        {
            Result += InPiece;
        });
        return Result;
    }

    // --------------------------------------------------------------------------------------------------------------------

    /** Dev build, mesh-pass full PSO miss with the render-target diff. :524-556, then :925-966, then :1044. */
    auto
        Get_FullPso_Missed_RenderTargetDiff()
        -> FString
    {
        return DoConcat({
            TEXT("\n\nPSO PRECACHING MISS:"),
            TEXT("\n\tType:\t\t\t\t\tFullPSO"),
            TEXT("\n\tPSOPrecachingState:\t\tMissed"),
            TEXT("\n\tMaterial:\t\t\t\tMI_Rock_Wet"),
            TEXT("\n\tVertexFactoryType:\t\tFLocalVertexFactory"),
            TEXT("\n\tMDCStatsCategory:\t\tStaticMeshComponent"),
            TEXT("\n\tMeshPassName:\t\t\tBasePass"),
            TEXT("\n\tShader Hashes:"),
            TEXT("\n\t\tVertexShader:\t\t"), Hash_Vertex,
            TEXT("\n\t\tPixelShader:\t\t"), Hash_Pixel,
            TEXT("\n\n\tMissed Info:"),
            TEXT("\n\t\tFound PSO With same state PSO hash & different render target data:"),
            TEXT("\n\t\tDifferences:"),
            TEXT("\n\t\t\t\t* RenderTargetFormat 0 different:"),
            TEXT("\n\t\t\t\t\tPrecached:\t2"),
            TEXT("\n\t\t\t\t\tRequested:\t10"),
            TEXT("\n")
        });
    }

    /**
     * Dev build, minimal-state miss with the closest-precached-state diff. :524-556, :803 (inside the header run),
     * then :876-879 repeating Material / VertexFactoryType / PassName for the PRECACHED state, then :881-902.
     */
    auto
        Get_MinimalPsoState_Missed_ClosestStateDiff()
        -> FString
    {
        return DoConcat({
            TEXT("\n\nPSO PRECACHING MISS:"),
            TEXT("\n\tType:\t\t\t\t\tMinimalPSOState"),
            TEXT("\n\tPSOPrecachingState:\t\tMissed"),
            TEXT("\n\tMaterial:\t\t\t\tMI_Character_Skin"),
            TEXT("\n\tVertexFactoryType:\t\tFGPUSkinPassthroughVertexFactory"),
            TEXT("\n\tMDCStatsCategory:\t\tSkeletalMeshComponent"),
            TEXT("\n\tMeshPassName:\t\t\tDepthPass"),
            TEXT("\n\tShader Hashes:"),
            TEXT("\n\t\tVertexShader:\t\t"), Hash_Vertex,
            TEXT("\n\tUsing Default Material"),
            TEXT("\n\n\tShadersOnly precache information:"),
            TEXT("\n\tMaterial:\t\t\t\tWorldGridMaterial"),
            TEXT("\n\tVertexFactoryType:\t\tFLocalVertexFactory"),
            TEXT("\n\tPassName:\t\t\t\tBasePass"),
            TEXT("\n\n\tMissed Info:"),
            TEXT("\n\t\t- Found PSO With same shaders & different state:"),
            TEXT("\n\t\t\tVertexFactoryType:\t\tFGPUSkinPassthroughVertexFactory"),
            TEXT("\n\t\t\tPassName:\t\t\t\tDepthPass"),
            TEXT("\n\t\t  Differences:"),
            TEXT("\n\t\t\t- RasterizerState different:"),
            TEXT("\n\t\t\t\t* CullMode different:"),
            TEXT("\n\t\t\t\t\tPrecached:\t0"),
            TEXT("\n\t\t\t\t\tRequested:\t1"),
            TEXT("\n")
        });
    }

    /** Dev build during a CSV capture: :524, :508 (Frame, no leading tab), :526-556, then :1014 + :570. */
    auto
        Get_ShadersOnly_Missed_DuringCsvCapture()
        -> FString
    {
        return DoConcat({
            TEXT("\n\nPSO PRECACHING MISS:"),
            TEXT("\nFrame:\t\t\t\t\t4521"),
            TEXT("\n\tType:\t\t\t\t\tShadersOnly"),
            TEXT("\n\tPSOPrecachingState:\t\tMissed"),
            TEXT("\n\tMaterial:\t\t\t\tM_Water_Ocean"),
            TEXT("\n\tVertexFactoryType:\t\tFLocalVertexFactory"),
            TEXT("\n\tMDCStatsCategory:\t\tStaticMeshComponent"),
            TEXT("\n\tMeshPassName:\t\t\tTranslucencyStandard"),
            TEXT("\n\tShader Hashes:"),
            TEXT("\n\t\tVertexShader:\t\t"), Hash_Vertex,
            TEXT("\n\t\tPixelShader:\t\t"), Hash_Pixel,
            TEXT("\n\n\tMissed Info:"),
            TEXT("\n\t\tPrecached with:\t\tFLocalVertexFactory (PSOPrecacheParamData: 4096)"),
            TEXT("\n")
        });
    }

    /** Dev build, shaders-only untracked: :524-556, then :997-1001. Untracked only exists outside Test/Shipping (:221-224). */
    auto
        Get_ShadersOnly_Untracked()
        -> FString
    {
        return DoConcat({
            TEXT("\n\nPSO PRECACHING MISS:"),
            TEXT("\n\tType:\t\t\t\t\tShadersOnly"),
            TEXT("\n\tPSOPrecachingState:\t\tUntracked"),
            TEXT("\n\tMaterial:\t\t\t\tM_Spark_Additive"),
            TEXT("\n\tVertexFactoryType:\t\tFNiagaraSpriteVertexFactory"),
            TEXT("\n\tMDCStatsCategory:\t\tNiagaraComponent"),
            TEXT("\n\tMeshPassName:\t\t\tTranslucencyStandard"),
            TEXT("\n\tShader Hashes:"),
            TEXT("\n\t\tVertexShader:\t\t"), Hash_Vertex,
            TEXT("\n\t\tPixelShader:\t\t"), Hash_Pixel,
            TEXT("\n\n\tUntracked Info:"),
            TEXT("\n\t\t- VertexFactory doesn't support PSO precaching."),
            TEXT("\n")
        });
    }

    /**
     * Shipping-shaped (no MESH_DRAW_COMMAND_STATS, no PSO_PRECACHING_TRACKING, no capture): :524-529, :533-555 only.
     * TooLate gets no extra info even in dev builds (:1031-1036). Mesh + amplification stages from :554-555.
     */
    auto
        Get_FullPso_TooLate_WithoutOptionalLines()
        -> FString
    {
        return DoConcat({
            TEXT("\n\nPSO PRECACHING MISS:"),
            TEXT("\n\tType:\t\t\t\t\tFullPSO"),
            TEXT("\n\tPSOPrecachingState:\t\tToo Late"),
            TEXT("\n\tMaterial:\t\t\t\tM_Cliff"),
            TEXT("\n\tVertexFactoryType:\t\tFNaniteVertexFactory"),
            TEXT("\n\tMeshPassName:\t\t\tNaniteRaster"),
            TEXT("\n\tShader Hashes:"),
            TEXT("\n\t\tPixelShader:\t\t"), Hash_Pixel,
            TEXT("\n\t\tMeshShader:\t\t"), Hash_Mesh,
            TEXT("\n\t\tAmplificationShader:\t\t"), Hash_Amplification,
            TEXT("\n")
        });
    }

    /**
     * Dev build, global graphics PSO (:445 passes null material, VF and proxy): placeholders from :528, :529, :531,
     * GlobalTypeName from :539, then :973 because there is no material.
     */
    auto
        Get_Global_FullPso_Missed()
        -> FString
    {
        return DoConcat({
            TEXT("\n\nPSO PRECACHING MISS:"),
            TEXT("\n\tType:\t\t\t\t\tFullPSO"),
            TEXT("\n\tPSOPrecachingState:\t\tMissed"),
            TEXT("\n\tMaterial:\t\t\t\tUnknown"),
            TEXT("\n\tVertexFactoryType:\t\tNone"),
            TEXT("\n\tMDCStatsCategory:\t\tUnknown"),
            TEXT("\n\tGlobalTypeName:\t\t\tSlateGlobalPSOCollector"),
            TEXT("\n\tShader Hashes:"),
            TEXT("\n\t\tVertexShader:\t\t"), Hash_Vertex,
            TEXT("\n\t\tPixelShader:\t\t"), Hash_Pixel,
            TEXT("\n\t\tGeometryShader:\t\t"), Hash_Geometry,
            TEXT("\n\n\tNo material found so no extra information on miss"),
            TEXT("\n")
        });
    }

    /** Compute overload, :1054-1060 then :1073. Its tracking section is commented out in the engine (:1062-1071). */
    auto
        Get_Compute_Missed()
        -> FString
    {
        return DoConcat({
            TEXT("\n\nPSO PRECACHING MISS:"),
            TEXT("\n\tType:\t\t\t\t\tCompute"),
            TEXT("\n\tPSOPrecachingState:\t\tMissed"),
            TEXT("\n\tMaterial:\t\t\t\tM_Foliage_Nanite"),
            TEXT("\n\tPassName:\t\t\t\tNaniteShading"),
            TEXT("\n\tCompute Shader Hash:\t"), Hash_Compute,
            TEXT("\n")
        });
    }

    auto
        Get_AllWellFormedFixtures()
        -> TArray<FString>
    {
        return TArray<FString>
        {
            Get_FullPso_Missed_RenderTargetDiff(),
            Get_MinimalPsoState_Missed_ClosestStateDiff(),
            Get_ShadersOnly_Missed_DuringCsvCapture(),
            Get_ShadersOnly_Untracked(),
            Get_FullPso_TooLate_WithoutOptionalLines(),
            Get_Global_FullPso_Missed(),
            Get_Compute_Missed()
        };
    }

    /**
     * The same message as FOutputDeviceFile writes it (OutputDeviceHelper.cpp:20-35, :49-73, :128-138): one prefix,
     * then the message verbatim (so the marker lands two physical lines below the prefix), then "\r\n".
     */
    auto
        DoWrapAsLogFileEntry(
            const FString& InBareMessage)
        -> FString
    {
        return FString{TEXT("[2026.10.01-14.22.07:511][347]LogStreaming: Display: Flushing package /Game/Maps/Arena\r\n")}
            + TEXT("[2026.10.01-14.22.07:512][347]LogEngine: ")
            + InBareMessage
            + TEXT("\r\n");
    }

    auto
        DoConvertToCrlf(
            const FString& InText)
        -> FString
    {
        return InText.Replace(TEXT("\n"), TEXT("\r\n"), ESearchCase::CaseSensitive);
    }

    auto
        DoReplace(
            const FString& InText,
            const TCHAR* InFrom,
            const TCHAR* InTo)
        -> FString
    {
        return InText.Replace(InFrom, InTo, ESearchCase::CaseSensitive);
    }

    auto
        DoCutBefore(
            const FString& InText,
            const TCHAR* InCutAt)
        -> FString
    {
        return InText.Left(InText.Find(InCutAt, ESearchCase::CaseSensitive));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        DoParseExpectingSuccess(
            FAutomationTestBase& InTest,
            const FString& InText,
            const TCHAR* InWhat)
        -> TOptional<FCk_Pso_MissRecord>
    {
        auto Parsed = FCk_Pso_MissLogParser::TryParse(InText);
        InTest.TestTrue(FString::Printf(TEXT("[%s] parses"), InWhat), Parsed.IsSet());
        return Parsed;
    }

    auto
        DoTestRejected(
            FAutomationTestBase& InTest,
            const FString& InText,
            const TCHAR* InWhat)
        -> void
    {
        InTest.TestFalse(FString::Printf(TEXT("[%s] is rejected, not half-filled"), InWhat),
            FCk_Pso_MissLogParser::TryParse(InText).IsSet());
    }

    auto
        DoTestSameDecodedFields(
            FAutomationTestBase& InTest,
            const TCHAR* InWhat,
            const FCk_Pso_MissRecord& InExpected,
            const FCk_Pso_MissRecord& InActual)
        -> void
    {
        const auto Context = FString{InWhat};

        InTest.TestTrue(Context + TEXT(": kind"), InActual.Get_Kind() == InExpected.Get_Kind());
        InTest.TestTrue(Context + TEXT(": type"), InActual.Get_Type() == InExpected.Get_Type());
        InTest.TestTrue(Context + TEXT(": state"), InActual.Get_PrecachingState() == InExpected.Get_PrecachingState());
        InTest.TestTrue(Context + TEXT(": collector scope"), InActual.Get_CollectorScope() == InExpected.Get_CollectorScope());
        InTest.TestTrue(Context + TEXT(": csv capture"), InActual.Get_CsvCapture() == InExpected.Get_CsvCapture());
        InTest.TestEqual(Context + TEXT(": csv frame"), InActual.Get_CsvFrameNumber(), InExpected.Get_CsvFrameNumber());
        InTest.TestEqual(Context + TEXT(": material"), InActual.Get_MaterialName(), InExpected.Get_MaterialName());
        InTest.TestEqual(Context + TEXT(": vertex factory"), InActual.Get_VertexFactoryTypeName(), InExpected.Get_VertexFactoryTypeName());
        InTest.TestEqual(Context + TEXT(": pass"), InActual.Get_PassName(), InExpected.Get_PassName());
        InTest.TestEqual(Context + TEXT(": mdc category"), InActual.Get_MdcStatsCategory(), InExpected.Get_MdcStatsCategory());
        InTest.TestEqual(Context + TEXT(": vertex hash"), InActual.Get_VertexShaderHash(), InExpected.Get_VertexShaderHash());
        InTest.TestEqual(Context + TEXT(": pixel hash"), InActual.Get_PixelShaderHash(), InExpected.Get_PixelShaderHash());
        InTest.TestEqual(Context + TEXT(": geometry hash"), InActual.Get_GeometryShaderHash(), InExpected.Get_GeometryShaderHash());
        InTest.TestEqual(Context + TEXT(": mesh hash"), InActual.Get_MeshShaderHash(), InExpected.Get_MeshShaderHash());
        InTest.TestEqual(Context + TEXT(": amplification hash"), InActual.Get_AmplificationShaderHash(), InExpected.Get_AmplificationShaderHash());
        InTest.TestEqual(Context + TEXT(": compute hash"), InActual.Get_ComputeShaderHash(), InExpected.Get_ComputeShaderHash());
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_Graphics_FullPsoMissed,
    "Ck.Pso.MissLogParser.Graphics.FullPsoMissed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_Graphics_HeaderWinsOverClosestStateSection,
    "Ck.Pso.MissLogParser.Graphics.HeaderWinsOverClosestStateSection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_Graphics_ShadersOnlyDuringCsvCapture,
    "Ck.Pso.MissLogParser.Graphics.ShadersOnlyDuringCsvCapture",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_Graphics_GlobalCollector,
    "Ck.Pso.MissLogParser.Graphics.GlobalCollector",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_Compute_Missed,
    "Ck.Pso.MissLogParser.Compute.Missed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_PrecachingStates,
    "Ck.Pso.MissLogParser.PrecachingStates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_OptionalLinesAbsent,
    "Ck.Pso.MissLogParser.OptionalLinesAbsent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_CrlfMatchesLf,
    "Ck.Pso.MissLogParser.CrlfMatchesLf",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_LogFileEntryMatchesBareMessage,
    "Ck.Pso.MissLogParser.LogFileEntryMatchesBareMessage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_ToleratesUnknownTokensAndExtraLines,
    "Ck.Pso.MissLogParser.ToleratesUnknownTokensAndExtraLines",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_RejectsNonMissText,
    "Ck.Pso.MissLogParser.RejectsNonMissText",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_RejectsTruncatedBlocks,
    "Ck.Pso.MissLogParser.RejectsTruncatedBlocks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_DedupeKey,
    "Ck.Pso.MissLogParser.DedupeKey",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_MissLogParser_Prefilter,
    "Ck.Pso.MissLogParser.Prefilter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// --------------------------------------------------------------------------------------------------------------------

bool FCkTest_Pso_MissLogParser_Graphics_FullPsoMissed::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    const auto Parsed = DoParseExpectingSuccess(*this, Get_FullPso_Missed_RenderTargetDiff(), TEXT("full PSO miss"));
    if (NOT Parsed.IsSet())
    { return false; }

    const auto& Record = *Parsed;
    TestTrue(TEXT("kind is graphics"), Record.Get_Kind() == ECk_Pso_MissKind::Graphics);
    TestTrue(TEXT("type is FullPSO"), Record.Get_Type() == ECk_Pso_MissType::FullPSO);
    TestTrue(TEXT("state is Missed"), Record.Get_PrecachingState() == ECk_Pso_MissPrecachingState::Missed);
    TestEqual(TEXT("material"), Record.Get_MaterialName(), FString{TEXT("MI_Rock_Wet")});
    TestEqual(TEXT("vertex factory"), Record.Get_VertexFactoryTypeName(), FString{TEXT("FLocalVertexFactory")});
    TestTrue(TEXT("pass came from a mesh pass collector"), Record.Get_CollectorScope() == ECk_Pso_MissCollectorScope::MeshPass);
    TestEqual(TEXT("pass"), Record.Get_PassName(), FString{TEXT("BasePass")});
    TestEqual(TEXT("mdc category"), Record.Get_MdcStatsCategory(), FString{TEXT("StaticMeshComponent")});
    TestEqual(TEXT("vertex hash"), Record.Get_VertexShaderHash(), FString{Hash_Vertex});
    TestEqual(TEXT("pixel hash"), Record.Get_PixelShaderHash(), FString{Hash_Pixel});
    TestTrue(TEXT("stages the engine did not print stay empty"),
        Record.Get_GeometryShaderHash().IsEmpty() && Record.Get_MeshShaderHash().IsEmpty() &&
        Record.Get_AmplificationShaderHash().IsEmpty() && Record.Get_ComputeShaderHash().IsEmpty());
    TestTrue(TEXT("no Frame line means no capture"), Record.Get_CsvCapture() == ECk_Pso_MissCsvCapture::NotCapturing);
    TestTrue(TEXT("raw text starts at the marker"), Record.Get_RawText().StartsWith(Marker, ESearchCase::CaseSensitive));
    TestTrue(TEXT("raw text keeps the render-target diff verbatim"),
        Record.Get_RawText().Contains(TEXT("RenderTargetFormat 0 different:"), ESearchCase::CaseSensitive));
    TestFalse(TEXT("raw text drops the trailing UE_LOG newline"), Record.Get_RawText().EndsWith(TEXT("\n")));

    return true;
}

bool FCkTest_Pso_MissLogParser_Graphics_HeaderWinsOverClosestStateSection::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    const auto Parsed = DoParseExpectingSuccess(*this, Get_MinimalPsoState_Missed_ClosestStateDiff(), TEXT("minimal state miss"));
    if (NOT Parsed.IsSet())
    { return false; }

    const auto& Record = *Parsed;
    TestTrue(TEXT("type is MinimalPSOState"), Record.Get_Type() == ECk_Pso_MissType::MinimalPSOState);

    // The diagnosis repeats these labels for the closest PRECACHED state at the header's own indent; reporting
    // those would point the manifest at the material that was fine.
    TestEqual(TEXT("material is the missed one, not the precached one"),
        Record.Get_MaterialName(), FString{TEXT("MI_Character_Skin")});
    TestEqual(TEXT("vertex factory is the missed one, not the precached one"),
        Record.Get_VertexFactoryTypeName(), FString{TEXT("FGPUSkinPassthroughVertexFactory")});
    TestEqual(TEXT("pass is the missed one"), Record.Get_PassName(), FString{TEXT("DepthPass")});
    TestEqual(TEXT("vertex hash"), Record.Get_VertexShaderHash(), FString{Hash_Vertex});
    TestTrue(TEXT("a depth-only PSO has no pixel hash"), Record.Get_PixelShaderHash().IsEmpty());
    TestTrue(TEXT("raw text keeps the closest-state diff verbatim"),
        Record.Get_RawText().Contains(TEXT("CullMode different:"), ESearchCase::CaseSensitive));

    return true;
}

bool FCkTest_Pso_MissLogParser_Graphics_ShadersOnlyDuringCsvCapture::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    const auto Parsed = DoParseExpectingSuccess(*this, Get_ShadersOnly_Missed_DuringCsvCapture(), TEXT("shaders-only miss in a capture"));
    if (NOT Parsed.IsSet())
    { return false; }

    const auto& Record = *Parsed;
    TestTrue(TEXT("type is ShadersOnly"), Record.Get_Type() == ECk_Pso_MissType::ShadersOnly);
    TestTrue(TEXT("the untabbed Frame line is read"), Record.Get_CsvCapture() == ECk_Pso_MissCsvCapture::Capturing);
    TestEqual(TEXT("frame number"), Record.Get_CsvFrameNumber(), 4521);
    TestEqual(TEXT("pass"), Record.Get_PassName(), FString{TEXT("TranslucencyStandard")});
    TestEqual(TEXT("material"), Record.Get_MaterialName(), FString{TEXT("M_Water_Ocean")});

    return true;
}

bool FCkTest_Pso_MissLogParser_Graphics_GlobalCollector::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    const auto Parsed = DoParseExpectingSuccess(*this, Get_Global_FullPso_Missed(), TEXT("global graphics miss"));
    if (NOT Parsed.IsSet())
    { return false; }

    const auto& Record = *Parsed;
    TestTrue(TEXT("kind is graphics"), Record.Get_Kind() == ECk_Pso_MissKind::Graphics);
    TestTrue(TEXT("pass came from a global collector"), Record.Get_CollectorScope() == ECk_Pso_MissCollectorScope::Global);
    TestEqual(TEXT("global type name"), Record.Get_PassName(), FString{TEXT("SlateGlobalPSOCollector")});
    TestEqual(TEXT("null material keeps the engine placeholder"), Record.Get_MaterialName(), FString{TEXT("Unknown")});
    TestEqual(TEXT("null vertex factory keeps the engine placeholder"), Record.Get_VertexFactoryTypeName(), FString{TEXT("None")});
    TestEqual(TEXT("null proxy keeps the engine placeholder"), Record.Get_MdcStatsCategory(), FString{TEXT("Unknown")});
    TestEqual(TEXT("geometry hash"), Record.Get_GeometryShaderHash(), FString{Hash_Geometry});

    return true;
}

bool FCkTest_Pso_MissLogParser_Compute_Missed::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    const auto Parsed = DoParseExpectingSuccess(*this, Get_Compute_Missed(), TEXT("compute miss"));
    if (NOT Parsed.IsSet())
    { return false; }

    const auto& Record = *Parsed;
    TestTrue(TEXT("kind is compute"), Record.Get_Kind() == ECk_Pso_MissKind::Compute);
    TestTrue(TEXT("type is the engine's Compute token"), Record.Get_Type() == ECk_Pso_MissType::Compute);
    TestTrue(TEXT("state is Missed"), Record.Get_PrecachingState() == ECk_Pso_MissPrecachingState::Missed);
    TestEqual(TEXT("material"), Record.Get_MaterialName(), FString{TEXT("M_Foliage_Nanite")});
    TestEqual(TEXT("PassName is read for compute"), Record.Get_PassName(), FString{TEXT("NaniteShading")});
    TestTrue(TEXT("compute pass names come from the mesh pass collectors"),
        Record.Get_CollectorScope() == ECk_Pso_MissCollectorScope::MeshPass);
    TestEqual(TEXT("compute hash"), Record.Get_ComputeShaderHash(), FString{Hash_Compute});
    TestTrue(TEXT("compute writes no vertex factory line"), Record.Get_VertexFactoryTypeName().IsEmpty());
    TestTrue(TEXT("compute writes no MDC line"), Record.Get_MdcStatsCategory().IsEmpty());
    TestTrue(TEXT("compute writes no graphics stage hashes"),
        Record.Get_VertexShaderHash().IsEmpty() && Record.Get_PixelShaderHash().IsEmpty());

    return true;
}

bool FCkTest_Pso_MissLogParser_PrecachingStates::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    const auto Missed = DoParseExpectingSuccess(*this, Get_FullPso_Missed_RenderTargetDiff(), TEXT("Missed"));
    const auto TooLate = DoParseExpectingSuccess(*this, Get_FullPso_TooLate_WithoutOptionalLines(), TEXT("Too Late"));
    const auto Untracked = DoParseExpectingSuccess(*this, Get_ShadersOnly_Untracked(), TEXT("Untracked"));
    const auto Unrecognised = DoParseExpectingSuccess(*this,
        DoReplace(Get_FullPso_Missed_RenderTargetDiff(), TEXT("PSOPrecachingState:\t\tMissed"), TEXT("PSOPrecachingState:\t\tPrecaching")),
        TEXT("unrecognised state"));

    if (NOT Missed.IsSet() || NOT TooLate.IsSet() || NOT Untracked.IsSet() || NOT Unrecognised.IsSet())
    { return false; }

    TestTrue(TEXT("Missed"), Missed->Get_PrecachingState() == ECk_Pso_MissPrecachingState::Missed);
    TestTrue(TEXT("the engine's spaced \"Too Late\" token"), TooLate->Get_PrecachingState() == ECk_Pso_MissPrecachingState::TooLate);
    TestTrue(TEXT("Untracked"), Untracked->Get_PrecachingState() == ECk_Pso_MissPrecachingState::Untracked);
    TestTrue(TEXT("any other token is Unknown"), Unrecognised->Get_PrecachingState() == ECk_Pso_MissPrecachingState::Unknown);

    return true;
}

bool FCkTest_Pso_MissLogParser_OptionalLinesAbsent::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    const auto Parsed = DoParseExpectingSuccess(*this, Get_FullPso_TooLate_WithoutOptionalLines(), TEXT("shipping-shaped block"));
    if (NOT Parsed.IsSet())
    { return false; }

    const auto& Record = *Parsed;
    TestTrue(TEXT("an absent MDC line leaves the field empty"), Record.Get_MdcStatsCategory().IsEmpty());
    TestTrue(TEXT("an absent Frame line means no capture"), Record.Get_CsvCapture() == ECk_Pso_MissCsvCapture::NotCapturing);
    TestEqual(TEXT("frame number stays at its default"), Record.Get_CsvFrameNumber(), 0);
    TestTrue(TEXT("an absent stage leaves its hash empty"), Record.Get_VertexShaderHash().IsEmpty());
    TestEqual(TEXT("pixel hash"), Record.Get_PixelShaderHash(), FString{Hash_Pixel});
    TestEqual(TEXT("mesh hash"), Record.Get_MeshShaderHash(), FString{Hash_Mesh});
    TestEqual(TEXT("amplification hash"), Record.Get_AmplificationShaderHash(), FString{Hash_Amplification});
    TestEqual(TEXT("pass"), Record.Get_PassName(), FString{TEXT("NaniteRaster")});

    return true;
}

bool FCkTest_Pso_MissLogParser_CrlfMatchesLf::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    ck::algo::ForEach(Get_AllWellFormedFixtures(), [this](const FString& InFixture)
    {
        const auto FromLf = DoParseExpectingSuccess(*this, InFixture, TEXT("LF"));
        const auto FromCrlf = DoParseExpectingSuccess(*this, DoConvertToCrlf(InFixture), TEXT("CRLF"));

        if (FromLf.IsSet() && FromCrlf.IsSet())
        { DoTestSameDecodedFields(*this, TEXT("CRLF vs LF"), *FromLf, *FromCrlf); }
    });

    return true;
}

bool FCkTest_Pso_MissLogParser_LogFileEntryMatchesBareMessage::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    ck::algo::ForEach(Get_AllWellFormedFixtures(), [this](const FString& InFixture)
    {
        const auto FromDevice = DoParseExpectingSuccess(*this, InFixture, TEXT("bare FOutputDevice message"));
        const auto FromLogFile = DoParseExpectingSuccess(*this, DoWrapAsLogFileEntry(InFixture), TEXT("log file entry"));

        if (NOT FromDevice.IsSet() || NOT FromLogFile.IsSet())
        { return; }

        DoTestSameDecodedFields(*this, TEXT("log file vs device"), *FromDevice, *FromLogFile);
        TestEqual(TEXT("raw text excludes the log prefix and the preceding entry"),
            FromLogFile->Get_RawText(), FromDevice->Get_RawText());
    });

    return true;
}

bool FCkTest_Pso_MissLogParser_ToleratesUnknownTokensAndExtraLines::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    const auto WithExtraLines = DoReplace(Get_FullPso_Missed_RenderTargetDiff(),
        TEXT("\n\tVertexFactoryType:"),
        TEXT("\n\tFutureField:\t\t\tSomething: with a colon\n\tan unlabeled line\n\tVertexFactoryType:"));

    if (const auto Parsed = DoParseExpectingSuccess(*this, WithExtraLines, TEXT("unknown header lines"));
        Parsed.IsSet())
    {
        TestEqual(TEXT("known fields survive unknown neighbours"), Parsed->Get_MaterialName(), FString{TEXT("MI_Rock_Wet")});
        TestEqual(TEXT("known fields survive unknown neighbours"), Parsed->Get_VertexFactoryTypeName(), FString{TEXT("FLocalVertexFactory")});
    }

    const auto GraphicsWithUnknownType = DoReplace(Get_FullPso_Missed_RenderTargetDiff(),
        TEXT("Type:\t\t\t\t\tFullPSO"), TEXT("Type:\t\t\t\t\tFuturePSO"));

    if (const auto Parsed = DoParseExpectingSuccess(*this, GraphicsWithUnknownType, TEXT("unknown graphics type token"));
        Parsed.IsSet())
    {
        TestTrue(TEXT("unrecognised type token is Unknown"), Parsed->Get_Type() == ECk_Pso_MissType::Unknown);
        TestTrue(TEXT("kind still resolves from the Shader Hashes list"), Parsed->Get_Kind() == ECk_Pso_MissKind::Graphics);
    }

    // GetPSOMissTypeName (:491-501) returns nullptr for an out-of-range value; whatever %s prints for it is not a
    // known token, and "(null)" stands in for it here.
    const auto ComputeWithNullType = DoReplace(Get_Compute_Missed(),
        TEXT("Type:\t\t\t\t\tCompute"), TEXT("Type:\t\t\t\t\t(null)"));

    if (const auto Parsed = DoParseExpectingSuccess(*this, ComputeWithNullType, TEXT("null type token"));
        Parsed.IsSet())
    {
        TestTrue(TEXT("(null) type token is Unknown"), Parsed->Get_Type() == ECk_Pso_MissType::Unknown);
        TestTrue(TEXT("kind still resolves from the Compute Shader Hash line"), Parsed->Get_Kind() == ECk_Pso_MissKind::Compute);
    }

    const auto GarbageFrame = DoReplace(Get_ShadersOnly_Missed_DuringCsvCapture(),
        TEXT("Frame:\t\t\t\t\t4521"), TEXT("Frame:\t\t\t\t\t45x1"));

    if (const auto Parsed = DoParseExpectingSuccess(*this, GarbageFrame, TEXT("unreadable frame number"));
        Parsed.IsSet())
    {
        TestTrue(TEXT("an unreadable frame number is treated as absent"),
            Parsed->Get_CsvCapture() == ECk_Pso_MissCsvCapture::NotCapturing);
    }

    const auto CutMidHashList = DoCutBefore(Get_FullPso_Missed_RenderTargetDiff(), TEXT("\n\t\tPixelShader:"));

    if (const auto Parsed = DoParseExpectingSuccess(*this, CutMidHashList, TEXT("cut inside the optional hash list"));
        Parsed.IsSet())
    {
        TestEqual(TEXT("hashes before the cut are kept"), Parsed->Get_VertexShaderHash(), FString{Hash_Vertex});
        TestTrue(TEXT("hashes after the cut are absent, not invented"), Parsed->Get_PixelShaderHash().IsEmpty());
    }

    return true;
}

bool FCkTest_Pso_MissLogParser_RejectsNonMissText::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    DoTestRejected(*this, FString{}, TEXT("empty string"));
    DoTestRejected(*this, TEXT("Display: PSO precaching complete: 1203 PSOs"), TEXT("ordinary LogEngine message"));
    DoTestRejected(*this, TEXT("[2026.10.01-14.22.07:512][347]LogEngine: Display: Loading map Arena\r\n"), TEXT("ordinary log file line"));
    DoTestRejected(*this,
        DoReplace(Get_FullPso_Missed_RenderTargetDiff(), Marker, TEXT("pso precaching miss:")),
        TEXT("marker in the wrong case"));
    DoTestRejected(*this,
        DoReplace(Get_FullPso_Missed_RenderTargetDiff(), Marker, TEXT("")),
        TEXT("header lines without the marker"));

    return true;
}

bool FCkTest_Pso_MissLogParser_RejectsTruncatedBlocks::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    const auto Truncations = TArray<FTextCase>
    {
        {TEXT("\n\nPSO PRECACHING MISS:"), TEXT("marker only")},
        {TEXT("PSO PRECACHING MISS:"), TEXT("marker without any line break")},
        {DoCutBefore(Get_FullPso_Missed_RenderTargetDiff(), TEXT("\n\tPSOPrecachingState:")), TEXT("cut after Type")},
        {DoCutBefore(Get_FullPso_Missed_RenderTargetDiff(), TEXT("\n\tVertexFactoryType:")), TEXT("cut after Material")},
        {DoCutBefore(Get_FullPso_Missed_RenderTargetDiff(), TEXT("\n\tShader Hashes:")), TEXT("graphics cut before Shader Hashes")},
        {DoCutBefore(Get_Compute_Missed(), TEXT("\n\tCompute Shader Hash:")), TEXT("compute cut before its hash")},
        {DoReplace(Get_FullPso_Missed_RenderTargetDiff(), TEXT("\n\tType:\t\t\t\t\tFullPSO"), TEXT("")), TEXT("graphics block missing its Type line")}
    };

    ck::algo::ForEach(Truncations, [this](const FTextCase& InTruncation)
    {
        TestTrue(FString::Printf(TEXT("[%s] still passes the prefilter"), InTruncation._What),
            FCk_Pso_MissLogParser::Get_IsMissBlock(InTruncation._Text));
        DoTestRejected(*this, InTruncation._Text, InTruncation._What);
    });

    return true;
}

bool FCkTest_Pso_MissLogParser_DedupeKey::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    const auto Base = Get_FullPso_Missed_RenderTargetDiff();

    const auto Parse = [this](const FString& InText, const TCHAR* InWhat) -> FString
    {
        const auto Parsed = DoParseExpectingSuccess(*this, InText, InWhat);
        return Parsed.IsSet() ? FCk_Pso_MissLogParser::Get_DedupeKey(*Parsed) : FString{};
    };

    const auto BaseKey = Parse(Base, TEXT("base"));

    TestEqual(TEXT("the key is the engine's tokens joined in a fixed order"),
        BaseKey, FString{TEXT("FullPSO|MI_Rock_Wet|FLocalVertexFactory|BasePass")});
    TestEqual(TEXT("compute keys carry an empty vertex factory"),
        Parse(Get_Compute_Missed(), TEXT("compute")), FString{TEXT("Compute|M_Foliage_Nanite||NaniteShading")});

    const auto SameDrawDifferentStateAndHash = DoReplace(
        DoReplace(Base, TEXT("PSOPrecachingState:\t\tMissed"), TEXT("PSOPrecachingState:\t\tToo Late")),
        Hash_Pixel, Hash_Amplification);

    TestEqual(TEXT("state and shader hashes do not split a key"),
        Parse(SameDrawDifferentStateAndHash, TEXT("same draw, other state and hash")), BaseKey);
    TestEqual(TEXT("CRLF and log-file framing do not split a key"),
        Parse(DoWrapAsLogFileEntry(DoConvertToCrlf(Base)), TEXT("framed")), BaseKey);

    const auto Variants = TArray<FTextCase>
    {
        {DoReplace(Base, TEXT("\tBasePass"), TEXT("\tDepthPass")), TEXT("other pass")},
        {DoReplace(Base, TEXT("FLocalVertexFactory"), TEXT("FInstancedStaticMeshVertexFactory")), TEXT("other vertex factory")},
        {DoReplace(Base, TEXT("MI_Rock_Wet"), TEXT("MI_Rock_Dry")), TEXT("other material")},
        {DoReplace(Base, TEXT("Type:\t\t\t\t\tFullPSO"), TEXT("Type:\t\t\t\t\tShadersOnly")), TEXT("other type")}
    };

    ck::algo::ForEach(Variants, [&](const FTextCase& InVariant)
    {
        const auto VariantKey = Parse(InVariant._Text, InVariant._What);
        TestFalse(FString::Printf(TEXT("[%s] gets its own key"), InVariant._What),
            VariantKey.Equals(BaseKey, ESearchCase::CaseSensitive));
    });

    return true;
}

bool FCkTest_Pso_MissLogParser_Prefilter::RunTest(const FString&)
{
    using namespace ck_test_pso_miss_log_parser;

    ck::algo::ForEach(Get_AllWellFormedFixtures(), [this](const FString& InFixture)
    {
        TestTrue(TEXT("bare message passes"), FCk_Pso_MissLogParser::Get_IsMissBlock(InFixture));
        TestTrue(TEXT("CRLF message passes"), FCk_Pso_MissLogParser::Get_IsMissBlock(DoConvertToCrlf(InFixture)));
        TestTrue(TEXT("log file entry passes"), FCk_Pso_MissLogParser::Get_IsMissBlock(DoWrapAsLogFileEntry(InFixture)));
    });

    TestFalse(TEXT("empty string"), FCk_Pso_MissLogParser::Get_IsMissBlock(FString{}));
    TestFalse(TEXT("ordinary LogEngine message"),
        FCk_Pso_MissLogParser::Get_IsMissBlock(TEXT("Display: PSO precaching complete: 1203 PSOs")));
    TestFalse(TEXT("marker in the wrong case"),
        FCk_Pso_MissLogParser::Get_IsMissBlock(TEXT("pso precaching miss:")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif
