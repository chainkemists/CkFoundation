#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_DhPreSeed.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsSourceScanner.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_Dispatcher.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_RegenOwnership.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkDynamic/CkDynamic_AngelScript.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/PlatformTime.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::self_heal
{
    namespace ck_angelscript_generator_dh_pre_seed
    {
        // Reads the double-quoted string value that starts at the first `"`
        // at/after InFromIndex in the ORIGINAL text (offsets line up with the
        // blanked text 1:1 — Blank_CommentsAndStrings preserves quote chars
        // and lengths). Backslash-escape aware. Empty on malformed input.
        auto Extract_QuotedValue_FromOriginal(
            const FString& InOriginalText,
            int32 InFromIndex,
            int32 InScanEndIndex) -> FString
        {
            const auto OpenQuote = InOriginalText.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, InFromIndex);
            if (OpenQuote == INDEX_NONE || OpenQuote >= InScanEndIndex)
            { return FString{}; }

            auto Out = FString{};
            for (auto Index = OpenQuote + 1; Index < InOriginalText.Len(); ++Index)
            {
                const auto Char = InOriginalText[Index];
                if (Char == TEXT('\\') && Index + 1 < InOriginalText.Len())
                {
                    Out.AppendChar(InOriginalText[Index + 1]);
                    ++Index;
                    continue;
                }
                if (Char == TEXT('"'))
                { return Out; }
                Out.AppendChar(Char);
            }

            return FString{}; // unterminated — treat as malformed
        }

        // Finds `<InKeyword> = "` inside the blanked block span and returns the
        // value extracted from the original text. Empty when the keyword isn't
        // assigned in the block.
        auto Extract_AssignedString(
            const FString& InOriginalText,
            const FString& InBlankedText,
            int32 InBlockOpen,
            int32 InBlockClose,
            const TCHAR* InKeyword) -> FString
        {
            const auto BlankedBlock = InBlankedText.Mid(InBlockOpen, InBlockClose - InBlockOpen);

            const auto Pattern = FRegexPattern{FString::Printf(TEXT("\\b%s\\s*=\\s*\""), InKeyword)};
            auto Matcher = FRegexMatcher{Pattern, BlankedBlock};
            if (NOT Matcher.FindNext())
            { return FString{}; }

            // The match's closing `"` is the value's opening quote in the original.
            const auto QuoteIndexInBlock = Matcher.GetMatchEnding() - 1;
            return Extract_QuotedValue_FromOriginal(InOriginalText, InBlockOpen + QuoteIndexInBlock, InBlockClose);
        }

        // Walks blanked text from InOpenBraceIndex (a `{`) to its matching `}`.
        // Blanked text has no braces inside strings/comments, so a plain depth
        // counter is exact. INDEX_NONE when unbalanced.
        auto Find_MatchingCloseBrace(
            const FString& InBlankedText,
            int32 InOpenBraceIndex) -> int32
        {
            auto Depth = 0;
            for (auto Index = InOpenBraceIndex; Index < InBlankedText.Len(); ++Index)
            {
                const auto Char = InBlankedText[Index];
                if (Char == TEXT('{'))
                { ++Depth; }
                else if (Char == TEXT('}'))
                {
                    --Depth;
                    if (Depth == 0)
                    { return Index; }
                }
            }
            return INDEX_NONE;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsDhPreSeed::
        Parse_HandleDefinitions(
            const FString& InFileContents,
            const FString& InSourcePathForDiagnostics)
        -> TArray<FCk_DhDeclaration>
    {
        auto Out = TArray<FCk_DhDeclaration>{};

        if (NOT InFileContents.Contains(TEXT("UCkDynamic_HandleDefinition"), ESearchCase::CaseSensitive))
        { return Out; }

        const auto Blanked = FCkAsSourceScanner::Blank_CommentsAndStrings(InFileContents);

        static const auto HeaderPattern = FRegexPattern{TEXT("\\basset\\s+\\w+\\s+of\\s+UCkDynamic_HandleDefinition\\b")};
        auto Matcher = FRegexMatcher{HeaderPattern, Blanked};

        while (Matcher.FindNext())
        {
            const auto HeaderEnd = Matcher.GetMatchEnding();

            const auto BlockOpen = Blanked.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, HeaderEnd);
            if (BlockOpen == INDEX_NONE)
            {
                ck::angelscriptgenerator::Verbose(
                    TEXT("[DhPreSeed] Handle-definition header without a body block in '{}' — skipping."),
                    InSourcePathForDiagnostics);
                break;
            }

            const auto BlockClose = ck_angelscript_generator_dh_pre_seed::Find_MatchingCloseBrace(Blanked, BlockOpen);
            if (BlockClose == INDEX_NONE)
            {
                ck::angelscriptgenerator::Verbose(
                    TEXT("[DhPreSeed] Unbalanced handle-definition block in '{}' — skipping rest of file."),
                    InSourcePathForDiagnostics);
                break;
            }

            const auto TypeName = ck_angelscript_generator_dh_pre_seed::Extract_AssignedString(
                InFileContents, Blanked, BlockOpen, BlockClose, TEXT("TypeName"));

            if (TypeName.IsEmpty())
            {
                ck::angelscriptgenerator::Verbose(
                    TEXT("[DhPreSeed] Handle-definition block without a TypeName in '{}' — skipping block."),
                    InSourcePathForDiagnostics);
                continue;
            }

            auto ShortName = ck_angelscript_generator_dh_pre_seed::Extract_AssignedString(
                InFileContents, Blanked, BlockOpen, BlockClose, TEXT("ShortName"));

            if (ShortName.IsEmpty())
            { ShortName = FCkDynamic_HandleTypeRegistry::ExtractShortNameFromTypeName(TypeName); }

            Out.Add(FCk_DhDeclaration{TypeName, ShortName, InSourcePathForDiagnostics});
        }

        return Out;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsDhPreSeed::
        Scan_ScriptTrees_ForHandleDefinitions(
            const TArray<FString>& InScanRoots)
        -> TArray<FCk_DhDeclaration>
    {
        auto Out = TArray<FCk_DhDeclaration>{};
        auto SeenTypeNames = TSet<FString>{};

        // Enumerate_AsSourceFiles is sorted, so first-declaration-wins dedup
        // below is deterministic across boots.
        for (const auto& Path : FCkAsSourceScanner::Enumerate_AsSourceFiles(InScanRoots))
        {
            auto Contents = FString{};
            if (NOT FFileHelper::LoadFileToString(Contents, *Path))
            {
                // Not ensure-worthy: the heal path is the backstop for anything
                // the scan misses.
                ck::angelscriptgenerator::Verbose(TEXT("[DhPreSeed] Failed to read '{}' — skipping."), Path);
                continue;
            }

            for (auto& Declaration : Parse_HandleDefinitions(Contents, Path))
            {
                auto AlreadySeen = false;
                SeenTypeNames.Add(Declaration.TypeName, &AlreadySeen);
                if (AlreadySeen)
                { continue; }

                Out.Add(MoveTemp(Declaration));
            }
        }

        Out.Sort([](const FCk_DhDeclaration& InA, const FCk_DhDeclaration& InB)
        { return InA.TypeName < InB.TypeName; });

        return Out;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsDhPreSeed::
        PreSeed_MissingDynamicHandles()
        -> int32
    {
        // Single-writer gate (G12): the pre-seed writes a Script/Generated
        // sibling. A secondary instance skips — behavior then degrades to
        // exactly today's heal path (itself G9-gated on the owning instance).
        if (NOT FCkAngelscriptGenerator_RegenOwnership::Try_AcquireOrGet_IsOwner(TEXT("Module.DhPreSeed")))
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[DhPreSeed] SECONDARY instance — skipping boot pre-seed; the owning instance ")
                TEXT("(or the self-heal fallback) covers any missing DynamicHandle entries."));
            return 0;
        }

        const auto StartSeconds = FPlatformTime::Seconds();

        const auto Declarations = Scan_ScriptTrees_ForHandleDefinitions(FCkAsSourceScanner::Get_DefaultScanRoots());
        const auto SeededCount  = PreSeed_MissingDynamicHandles(
            FCkDynamic_HandleTypeRegistry::GetRegistryFilePath(), Declarations);

        const auto DurationMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;

        // Boot-cost visibility: the scan reads every non-generated .as file.
        // If this line ever reports a duration in the same order as the heal
        // cycle it prevents (~6-8s), the pre-seed has lost its reason to exist.
        if (SeededCount > 0)
        {
            ck::angelscriptgenerator::Log(
                TEXT("[DhPreSeed] Boot pre-seed scanned {} declaration(s) and seeded {} in {:.1f} ms."),
                Declarations.Num(), SeededCount, DurationMs);
        }
        else
        {
            ck::angelscriptgenerator::Verbose(
                TEXT("[DhPreSeed] Boot pre-seed scanned {} declaration(s), nothing missing ({:.1f} ms)."),
                Declarations.Num(), DurationMs);
        }

        return SeededCount;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsDhPreSeed::
        PreSeed_MissingDynamicHandles(
            const FString& InCanonicalJsonPath,
            const TArray<FCk_DhDeclaration>& InDeclarations)
        -> int32
    {
        if (InCanonicalJsonPath.IsEmpty())
        {
            ck::angelscriptgenerator::Verbose(
                TEXT("[DhPreSeed] Registry file path is empty — nothing to pre-seed against."));
            return 0;
        }

        if (InDeclarations.IsEmpty())
        { return 0; }

        // Collect the canonical's TypeNames. Missing file OR unparsable content
        // both count as "no known TypeNames" — seeding everything is the
        // productive recovery in either state (the alternative is one failed
        // compile + one heal cycle PER handle), and the post-init regen
        // rewrites the canonical from the data assets regardless.
        auto CanonicalTypeNames = TSet<FString>{};
        {
            auto CanonicalContent = FString{};
            if (FFileHelper::LoadFileToString(CanonicalContent, *InCanonicalJsonPath))
            {
                auto RootObj    = TSharedPtr<FJsonObject>{};
                auto JsonReader = TJsonReaderFactory<>::Create(CanonicalContent);
                if (FJsonSerializer::Deserialize(JsonReader, RootObj) && RootObj.IsValid()
                    && RootObj->HasField(TEXT("HandleTypes")))
                {
                    for (const auto& Entry : RootObj->GetArrayField(TEXT("HandleTypes")))
                    {
                        const auto Obj = Entry->AsObject();
                        if (NOT Obj.IsValid())
                        { continue; }

                        auto Name = FString{};
                        Obj->TryGetStringField(TEXT("TypeName"), Name);
                        if (NOT Name.IsEmpty())
                        { CanonicalTypeNames.Add(Name); }
                    }
                }
                else
                {
                    ck::angelscriptgenerator::Warning(
                        TEXT("[DhPreSeed] Canonical registry at '{}' failed to parse — pre-seeding ALL scanned ")
                        TEXT("declarations so the first compile isn't wedged; the post-init regen rewrites the canonical."),
                        InCanonicalJsonPath);
                }
            }
        }

        auto MissingEntries = TArray<FCk_DynamicHandleStubEntry>{};
        auto MissingNames   = TArray<FString>{};
        for (const auto& Declaration : InDeclarations)
        {
            if (CanonicalTypeNames.Contains(Declaration.TypeName))
            { continue; }

            MissingEntries.Add(FCk_DynamicHandleStubEntry{Declaration.TypeName, Declaration.ShortName});
            MissingNames.Add(Declaration.TypeName);
        }

        if (MissingEntries.IsEmpty())
        {
            ck::angelscriptgenerator::Verbose(
                TEXT("[DhPreSeed] Canonical registry covers all {} scanned declaration(s) — nothing to do."),
                InDeclarations.Num());
            return 0;
        }

        const auto StubJsonPath = FCkAsRecoveryDispatcher::Derive_DynamicHandleStubPath(InCanonicalJsonPath);
        const auto AppendedCount = FCkAsRecoveryDispatcher::Append_DynamicHandleStubEntries(StubJsonPath, MissingEntries);
        if (NOT AppendedCount.IsSet())
        {
            ck::angelscriptgenerator::Warning(
                TEXT("[DhPreSeed] Sibling stub write failed — the self-heal dispatcher remains the fallback for: [{}]"),
                FString::Join(MissingNames, TEXT(", ")));
            return 0;
        }

        if (*AppendedCount > 0)
        {
            // Arms the deferred OnPostEngineInit regen (strict-validator upgrade),
            // exactly as a heal-path synthesis would.
            FCkAsRecoveryDispatcher::Mark_JsonStubSynthesized();

            ck::angelscriptgenerator::Log(
                TEXT("[DhPreSeed] Pre-seeded {} DynamicHandle stub entry(ies) from AS source scan: [{}] -> {}"),
                *AppendedCount, FString::Join(MissingNames, TEXT(", ")), StubJsonPath);
        }

        return *AppendedCount;
    }
}

// --------------------------------------------------------------------------------------------------------------------
