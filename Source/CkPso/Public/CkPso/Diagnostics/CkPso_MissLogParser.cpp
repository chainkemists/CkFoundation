#include "CkPso_MissLogParser.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include <Containers/ArrayView.h>
#include <Misc/Char.h>
#include <Misc/CString.h>
#include <String/ParseLines.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pso_miss_log_parser
{
    constexpr auto MissMarker = TEXTVIEW("PSO PRECACHING MISS:");

    constexpr auto Label_Frame = TEXTVIEW("Frame");
    constexpr auto Label_Type = TEXTVIEW("Type");
    constexpr auto Label_PrecachingState = TEXTVIEW("PSOPrecachingState");
    constexpr auto Label_Material = TEXTVIEW("Material");
    constexpr auto Label_VertexFactoryType = TEXTVIEW("VertexFactoryType");
    constexpr auto Label_MdcStatsCategory = TEXTVIEW("MDCStatsCategory");
    constexpr auto Label_MeshPassName = TEXTVIEW("MeshPassName");
    constexpr auto Label_GlobalTypeName = TEXTVIEW("GlobalTypeName");
    constexpr auto Label_PassName = TEXTVIEW("PassName");
    constexpr auto Label_ShaderHashes = TEXTVIEW("Shader Hashes");
    constexpr auto Label_VertexShader = TEXTVIEW("VertexShader");
    constexpr auto Label_PixelShader = TEXTVIEW("PixelShader");
    constexpr auto Label_GeometryShader = TEXTVIEW("GeometryShader");
    constexpr auto Label_MeshShader = TEXTVIEW("MeshShader");
    constexpr auto Label_AmplificationShader = TEXTVIEW("AmplificationShader");
    constexpr auto Label_ComputeShaderHash = TEXTVIEW("Compute Shader Hash");

    constexpr auto Token_ShadersOnly = TEXTVIEW("ShadersOnly");
    constexpr auto Token_MinimalPsoState = TEXTVIEW("MinimalPSOState");
    constexpr auto Token_FullPso = TEXTVIEW("FullPSO");
    constexpr auto Token_Compute = TEXTVIEW("Compute");
    constexpr auto Token_Unknown = TEXTVIEW("Unknown");

    constexpr auto Token_Missed = TEXTVIEW("Missed");
    constexpr auto Token_TooLate = TEXTVIEW("Too Late");
    constexpr auto Token_Untracked = TEXTVIEW("Untracked");

    constexpr auto DedupeKeySeparator = TEXT("|");

    // --------------------------------------------------------------------------------------------------------------------

    struct FLabeledLine
    {
        FStringView _Label;
        FStringView _Value;
    };

    struct FPassLine
    {
        FStringView _Name;
        ECk_Pso_MissCollectorScope _Scope = ECk_Pso_MissCollectorScope::Unknown;
    };

    // --------------------------------------------------------------------------------------------------------------------

    auto
        DoSplitLabeledLine(
            FStringView InLine)
        -> TOptional<FLabeledLine>
    {
        const auto Trimmed = InLine.TrimStartAndEnd();

        auto ColonIndex = int32{INDEX_NONE};
        if (NOT Trimmed.FindChar(TEXT(':'), ColonIndex))
        { return {}; }

        return FLabeledLine{Trimmed.Left(ColonIndex).TrimEnd(), Trimmed.RightChop(ColonIndex + 1).TrimStart()};
    }

    auto
        DoCollectHeaderLines(
            FStringView InTextAfterMarker)
        -> TArray<FLabeledLine>
    {
        auto Lines = TArray<FStringView>{};
        UE::String::ParseLines(InTextAfterMarker, [&Lines](FStringView InLine)
        {
            Lines.Add(InLine);
        });

        const auto LinesAfterMarkerLine = MakeArrayView(Lines).RightChop(1);

        const auto BlankLineIndex = ck::algo::FindIndex(LinesAfterMarkerLine, [](FStringView InLine)
        {
            return InLine.TrimStartAndEnd().IsEmpty();
        });

        const auto HeaderLineCount = BlankLineIndex == INDEX_NONE ? LinesAfterMarkerLine.Num() : BlankLineIndex;
        const auto HeaderLines = LinesAfterMarkerLine.Left(HeaderLineCount);

        auto LabeledLines = TArray<FLabeledLine>{};
        ck::algo::ForEach(HeaderLines, [&LabeledLines](FStringView InLine)
        {
            const auto LabeledLine = DoSplitLabeledLine(InLine);

            if (LabeledLine.IsSet())
            { LabeledLines.Add(*LabeledLine); }
        });

        return LabeledLines;
    }

    auto
        DoFindValue(
            const TArray<FLabeledLine>& InHeaderLines,
            FStringView InLabel)
        -> TOptional<FStringView>
    {
        const auto Found = ck::algo::FindIf(InHeaderLines, [InLabel](const FLabeledLine& InLine)
        {
            return InLine._Label.Equals(InLabel, ESearchCase::CaseSensitive);
        });

        if (NOT Found.IsSet())
        { return {}; }

        return Found->_Value;
    }

    auto
        DoFindValueAsString(
            const TArray<FLabeledLine>& InHeaderLines,
            FStringView InLabel)
        -> FString
    {
        return FString{DoFindValue(InHeaderLines, InLabel).Get(FStringView{})};
    }

    auto
        DoParseType(
            FStringView InToken)
        -> ECk_Pso_MissType
    {
        if (InToken.Equals(Token_ShadersOnly, ESearchCase::CaseSensitive))
        { return ECk_Pso_MissType::ShadersOnly; }

        if (InToken.Equals(Token_MinimalPsoState, ESearchCase::CaseSensitive))
        { return ECk_Pso_MissType::MinimalPSOState; }

        if (InToken.Equals(Token_FullPso, ESearchCase::CaseSensitive))
        { return ECk_Pso_MissType::FullPSO; }

        if (InToken.Equals(Token_Compute, ESearchCase::CaseSensitive))
        { return ECk_Pso_MissType::Compute; }

        return ECk_Pso_MissType::Unknown;
    }

    auto
        DoGet_TypeToken(
            ECk_Pso_MissType InType)
        -> FStringView
    {
        switch (InType)
        {
            case ECk_Pso_MissType::ShadersOnly:
            { return Token_ShadersOnly; }
            case ECk_Pso_MissType::MinimalPSOState:
            { return Token_MinimalPsoState; }
            case ECk_Pso_MissType::FullPSO:
            { return Token_FullPso; }
            case ECk_Pso_MissType::Compute:
            { return Token_Compute; }
            case ECk_Pso_MissType::Unknown:
            default:
            { return Token_Unknown; }
        }
    }

    auto
        DoParsePrecachingState(
            FStringView InToken)
        -> ECk_Pso_MissPrecachingState
    {
        if (InToken.Equals(Token_Missed, ESearchCase::CaseSensitive))
        { return ECk_Pso_MissPrecachingState::Missed; }

        if (InToken.Equals(Token_TooLate, ESearchCase::CaseSensitive))
        { return ECk_Pso_MissPrecachingState::TooLate; }

        if (InToken.Equals(Token_Untracked, ESearchCase::CaseSensitive))
        { return ECk_Pso_MissPrecachingState::Untracked; }

        return ECk_Pso_MissPrecachingState::Unknown;
    }

    // An unrecognised "Type:" token still has an unambiguous shape: only the compute overload writes a single
    // "Compute Shader Hash:" line, only the graphics overload writes the "Shader Hashes:" list.
    auto
        DoResolveKind(
            ECk_Pso_MissType InType,
            const TArray<FLabeledLine>& InHeaderLines)
        -> ECk_Pso_MissKind
    {
        switch (InType)
        {
            case ECk_Pso_MissType::ShadersOnly:
            case ECk_Pso_MissType::MinimalPSOState:
            case ECk_Pso_MissType::FullPSO:
            { return ECk_Pso_MissKind::Graphics; }
            case ECk_Pso_MissType::Compute:
            { return ECk_Pso_MissKind::Compute; }
            case ECk_Pso_MissType::Unknown:
            default:
            { break; }
        }

        if (DoFindValue(InHeaderLines, Label_ComputeShaderHash).IsSet())
        { return ECk_Pso_MissKind::Compute; }

        if (DoFindValue(InHeaderLines, Label_ShaderHashes).IsSet())
        { return ECk_Pso_MissKind::Graphics; }

        return ECk_Pso_MissKind::Unknown;
    }

    auto
        DoGet_HasRequiredLines(
            ECk_Pso_MissKind InKind,
            const TArray<FLabeledLine>& InHeaderLines)
        -> bool
    {
        const auto HasLabel = [&InHeaderLines](FStringView InLabel) -> bool
        {
            return DoFindValue(InHeaderLines, InLabel).IsSet();
        };

        const auto HasCommonLines = ck::algo::AllOf(
            TArray<FStringView>{Label_Type, Label_PrecachingState, Label_Material}, HasLabel);

        switch (InKind)
        {
            case ECk_Pso_MissKind::Graphics:
            {
                const auto HasPassLine = HasLabel(Label_MeshPassName) || HasLabel(Label_GlobalTypeName);
                const auto HasGraphicsLines = ck::algo::AllOf(
                    TArray<FStringView>{Label_VertexFactoryType, Label_ShaderHashes}, HasLabel);

                return HasCommonLines && HasPassLine && HasGraphicsLines;
            }
            case ECk_Pso_MissKind::Compute:
            {
                const auto HasComputeLines = ck::algo::AllOf(
                    TArray<FStringView>{Label_PassName, Label_ComputeShaderHash}, HasLabel);

                return HasCommonLines && HasComputeLines;
            }
            case ECk_Pso_MissKind::Unknown:
            default:
            { return false; }
        }
    }

    auto
        DoResolvePassLine(
            ECk_Pso_MissKind InKind,
            const TArray<FLabeledLine>& InHeaderLines)
        -> FPassLine
    {
        if (InKind == ECk_Pso_MissKind::Compute)
        {
            return FPassLine{
                DoFindValue(InHeaderLines, Label_PassName).Get(FStringView{}),
                ECk_Pso_MissCollectorScope::MeshPass};
        }

        if (const auto MeshPassName = DoFindValue(InHeaderLines, Label_MeshPassName);
            MeshPassName.IsSet())
        { return FPassLine{*MeshPassName, ECk_Pso_MissCollectorScope::MeshPass}; }

        if (const auto GlobalTypeName = DoFindValue(InHeaderLines, Label_GlobalTypeName);
            GlobalTypeName.IsSet())
        { return FPassLine{*GlobalTypeName, ECk_Pso_MissCollectorScope::Global}; }

        return FPassLine{};
    }

    auto
        DoParseFrameNumber(
            FStringView InValue)
        -> TOptional<int32>
    {
        constexpr auto MaxDigitsThatFitInt32 = 9;

        const auto IsPlainNonNegativeInteger =
            NOT InValue.IsEmpty() &&
            InValue.Len() <= MaxDigitsThatFitInt32 &&
            ck::algo::AllOf(InValue, [](TCHAR InChar) { return FChar::IsDigit(InChar); });

        if (NOT IsPlainNonNegativeInteger)
        { return {}; }

        return FCString::Atoi(*FString{InValue});
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Pso_MissLogParser::
    Get_IsMissBlock(
        FStringView InText)
    -> bool
{
    return InText.Contains(ck_pso_miss_log_parser::MissMarker, ESearchCase::CaseSensitive);
}

auto
    FCk_Pso_MissLogParser::
    TryParse(
        FStringView InText)
    -> TOptional<FCk_Pso_MissRecord>
{
    using namespace ck_pso_miss_log_parser;

    const auto MarkerIndex = InText.Find(MissMarker, 0, ESearchCase::CaseSensitive);

    if (MarkerIndex == INDEX_NONE)
    { return {}; }

    const auto HeaderLines = DoCollectHeaderLines(InText.RightChop(MarkerIndex + MissMarker.Len()));
    const auto Type = DoParseType(DoFindValue(HeaderLines, Label_Type).Get(FStringView{}));
    const auto Kind = DoResolveKind(Type, HeaderLines);

    if (NOT DoGet_HasRequiredLines(Kind, HeaderLines))
    { return {}; }

    const auto PassLine = DoResolvePassLine(Kind, HeaderLines);
    const auto FrameValue = DoFindValue(HeaderLines, Label_Frame);
    const auto FrameNumber = FrameValue.IsSet() ? DoParseFrameNumber(*FrameValue) : TOptional<int32>{};

    auto Record = FCk_Pso_MissRecord{};
    Record._Kind = Kind;
    Record._Type = Type;
    Record._PrecachingState = DoParsePrecachingState(DoFindValue(HeaderLines, Label_PrecachingState).Get(FStringView{}));
    Record._MaterialName = DoFindValueAsString(HeaderLines, Label_Material);
    Record._VertexFactoryTypeName = DoFindValueAsString(HeaderLines, Label_VertexFactoryType);
    Record._CollectorScope = PassLine._Scope;
    Record._PassName = FString{PassLine._Name};
    Record._MdcStatsCategory = DoFindValueAsString(HeaderLines, Label_MdcStatsCategory);
    Record._VertexShaderHash = DoFindValueAsString(HeaderLines, Label_VertexShader);
    Record._PixelShaderHash = DoFindValueAsString(HeaderLines, Label_PixelShader);
    Record._GeometryShaderHash = DoFindValueAsString(HeaderLines, Label_GeometryShader);
    Record._MeshShaderHash = DoFindValueAsString(HeaderLines, Label_MeshShader);
    Record._AmplificationShaderHash = DoFindValueAsString(HeaderLines, Label_AmplificationShader);
    Record._ComputeShaderHash = DoFindValueAsString(HeaderLines, Label_ComputeShaderHash);
    Record._CsvCapture = FrameNumber.IsSet() ? ECk_Pso_MissCsvCapture::Capturing : ECk_Pso_MissCsvCapture::NotCapturing;
    Record._CsvFrameNumber = FrameNumber.Get(0);
    Record._RawText = FString{InText.RightChop(MarkerIndex).TrimEnd()};

    return MoveTemp(Record);
}

auto
    FCk_Pso_MissLogParser::
    Get_DedupeKey(
        const FCk_Pso_MissRecord& InRecord)
    -> FString
{
    using namespace ck_pso_miss_log_parser;

    return FString::Join(TArray<FString>
    {
        FString{DoGet_TypeToken(InRecord.Get_Type())},
        InRecord.Get_MaterialName(),
        InRecord.Get_VertexFactoryTypeName(),
        InRecord.Get_PassName()
    }, DedupeKeySeparator);
}

// --------------------------------------------------------------------------------------------------------------------
