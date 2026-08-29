#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AssetBlockPatcher.h"

#include "CkCore/Macros/CkMacros.h"

#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::write_back
{
    namespace ck_angelscript_generator_asset_block_patcher
    {
        constexpr auto DefaultIndentWidth = 4;

        auto Is_IdentChar(
            TCHAR InChar) -> bool
        {
            return FChar::IsAlnum(InChar) || InChar == TEXT('_');
        }

        auto Get_LineNumberAt(
            const FString& InText,
            int32          InOffset) -> int32
        {
            auto Line = 1;
            const auto Limit = FMath::Min(InOffset, InText.Len());
            for (auto Index = 0; Index < Limit; ++Index)
            {
                if (InText[Index] == TEXT('\n'))
                { ++Line; }
            }
            return Line;
        }

        auto Get_LineStartAt(
            const FString& InText,
            int32          InOffset) -> int32
        {
            auto Index = FMath::Clamp(InOffset, 0, InText.Len());
            while (Index > 0 && InText[Index - 1] != TEXT('\n'))
            { --Index; }
            return Index;
        }

        // One past the line terminator, so a caller splicing [Start, End) also removes the newline.
        auto Get_NextLineStartAt(
            const FString& InText,
            int32          InOffset) -> int32
        {
            const auto N = InText.Len();
            auto Index = FMath::Clamp(InOffset, 0, N);
            while (Index < N && InText[Index] != TEXT('\n'))
            { ++Index; }
            return Index < N ? Index + 1 : N;
        }

        auto Get_LineTextAt(
            const FString& InText,
            int32          InOffset) -> FString
        {
            const auto Start = Get_LineStartAt(InText, InOffset);
            auto End = Start;
            const auto N = InText.Len();
            while (End < N && InText[End] != TEXT('\n'))
            { ++End; }
            return InText.Mid(Start, End - Start).TrimEnd();
        }

        auto Get_LeadingWhitespace(
            const FString& InLine) -> FString
        {
            auto Count = 0;
            while (Count < InLine.Len() && FChar::IsWhitespace(InLine[Count]))
            { ++Count; }
            return InLine.Left(Count);
        }

        // A statement the patcher recognises as `<Property>[.<SubPath>] = <RHS>;` at body depth 0.
        // Every other line shape — locals, `.Add()` calls, `[]` element writes, comments — parses as
        // "not ours" and is therefore never touched.
        struct FParsedAssignment
        {
            FString PropertyName;
            FString SubPath;
            int32   NameStart      = INDEX_NONE;
            int32   EqualsPos      = INDEX_NONE;
            int32   ValueStart     = INDEX_NONE; // first char after `=` and its trailing whitespace
            int32   SemicolonPos   = INDEX_NONE;
            int32   LineStart      = INDEX_NONE;
            int32   DeleteEnd      = INDEX_NONE; // exclusive; spans the terminator when the statement owns its line
            bool    OwnsWholeLine  = false;
            FString Indent;
        };

        auto Try_ParseAssignment(
            const FString&    InBlanked,
            int32             InStart,
            int32             InBodyClose,
            FParsedAssignment& OutParsed) -> bool
        {
            auto Pos = InStart;

            if (Pos >= InBodyClose || NOT Is_IdentChar(InBlanked[Pos]) || FChar::IsDigit(InBlanked[Pos]))
            { return false; }

            const auto NameStart = Pos;
            while (Pos < InBodyClose && Is_IdentChar(InBlanked[Pos]))
            { ++Pos; }

            auto Parsed = FParsedAssignment{};
            Parsed.PropertyName = InBlanked.Mid(NameStart, Pos - NameStart);
            Parsed.NameStart    = NameStart;

            while (Pos < InBodyClose && InBlanked[Pos] == TEXT('.'))
            {
                const auto FieldStart = Pos + 1;
                auto FieldEnd = FieldStart;
                while (FieldEnd < InBodyClose && Is_IdentChar(InBlanked[FieldEnd]))
                { ++FieldEnd; }

                if (FieldEnd == FieldStart)
                { return false; }

                Parsed.SubPath += TEXT(".") + InBlanked.Mid(FieldStart, FieldEnd - FieldStart);
                Pos = FieldEnd;
            }

            while (Pos < InBodyClose && FChar::IsWhitespace(InBlanked[Pos]))
            { ++Pos; }

            if (Pos >= InBodyClose || InBlanked[Pos] != TEXT('='))
            { return false; }

            // `==` is a comparison, not an assignment. Compound forms (`+=`) never reach here: the
            // operator character precedes the `=`, so the identifier scan above already stopped short.
            if (Pos + 1 < InBodyClose && InBlanked[Pos + 1] == TEXT('='))
            { return false; }

            Parsed.EqualsPos = Pos;
            ++Pos;

            while (Pos < InBodyClose && FChar::IsWhitespace(InBlanked[Pos]))
            { ++Pos; }
            Parsed.ValueStart = Pos;

            auto Nesting = 0;
            auto SemicolonPos = int32{INDEX_NONE};
            for (auto Scan = Pos; Scan < InBodyClose; ++Scan)
            {
                const auto Char = InBlanked[Scan];
                if (Char == TEXT('(') || Char == TEXT('[') || Char == TEXT('{'))
                { ++Nesting; }
                else if (Char == TEXT(')') || Char == TEXT(']') || Char == TEXT('}'))
                { --Nesting; }
                else if (Char == TEXT(';') && Nesting == 0)
                { SemicolonPos = Scan; break; }
            }

            if (SemicolonPos == INDEX_NONE)
            { return false; }

            Parsed.SemicolonPos = SemicolonPos;
            Parsed.LineStart    = Get_LineStartAt(InBlanked, NameStart);

            const auto Preamble = InBlanked.Mid(Parsed.LineStart, NameStart - Parsed.LineStart);
            const auto PreambleIsBlank = Preamble.TrimStartAndEnd().IsEmpty();

            // A trailing comment blanks to spaces, so "only whitespace follows" also covers
            // `_Foo = 1; // why` — the comment goes with the line it annotates.
            auto TailEnd = SemicolonPos + 1;
            while (TailEnd < InBlanked.Len() && InBlanked[TailEnd] != TEXT('\n'))
            { ++TailEnd; }
            const auto TailIsBlank = InBlanked.Mid(SemicolonPos + 1, TailEnd - SemicolonPos - 1)
                                              .TrimStartAndEnd().IsEmpty();

            Parsed.OwnsWholeLine = PreambleIsBlank && TailIsBlank;
            Parsed.Indent        = PreambleIsBlank ? Preamble : FString{};
            Parsed.DeleteEnd     = Parsed.OwnsWholeLine
                ? Get_NextLineStartAt(InBlanked, SemicolonPos)
                : SemicolonPos + 1;

            OutParsed = MoveTemp(Parsed);
            return true;
        }

        auto Collect_Assignments(
            const FString& InBlanked,
            int32          InBodyOpen,
            int32          InBodyClose) -> TArray<FParsedAssignment>
        {
            auto Out = TArray<FParsedAssignment>{};

            auto BraceDepth      = 0;
            auto Nesting         = 0;
            auto AtStatementHead = true;
            auto Index           = InBodyOpen + 1;

            while (Index < InBodyClose)
            {
                const auto Char = InBlanked[Index];

                if (FChar::IsWhitespace(Char))
                { ++Index; continue; }

                if (Nesting == 0 && Char == TEXT('{'))
                { ++BraceDepth; AtStatementHead = true; ++Index; continue; }

                if (Nesting == 0 && Char == TEXT('}'))
                { --BraceDepth; AtStatementHead = true; ++Index; continue; }

                if (Char == TEXT('(') || Char == TEXT('['))
                { ++Nesting; AtStatementHead = false; ++Index; continue; }

                if (Char == TEXT(')') || Char == TEXT(']'))
                { --Nesting; AtStatementHead = false; ++Index; continue; }

                if (Nesting == 0 && Char == TEXT(';'))
                { AtStatementHead = true; ++Index; continue; }

                if (AtStatementHead && BraceDepth == 0 && Nesting == 0)
                {
                    auto Parsed = FParsedAssignment{};
                    if (Try_ParseAssignment(InBlanked, Index, InBodyClose, Parsed))
                    {
                        Index = Parsed.SemicolonPos + 1;
                        Out.Add(MoveTemp(Parsed));
                        AtStatementHead = true;
                        continue;
                    }
                }

                AtStatementHead = false;
                ++Index;
            }

            return Out;
        }

        // Descending by Start so earlier splices never shift a later one's offsets.
        struct FTextEdit
        {
            int32   Start = 0;
            int32   End   = 0; // exclusive
            FString Replacement;
        };
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_AssetBlockPatchEntry::
        Make_Assign(
            const FString&                          InPropertyName,
            const TArray<FCk_AssetBlockAssignment>& InAssignments)
        -> FCk_AssetBlockPatchEntry
    {
        auto Entry = FCk_AssetBlockPatchEntry{};
        Entry.PropertyName = InPropertyName;
        Entry.Assignments  = InAssignments;
        Entry.Delete       = false;
        return Entry;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_AssetBlockPatchEntry::
        Make_Delete(
            const FString& InPropertyName)
        -> FCk_AssetBlockPatchEntry
    {
        auto Entry = FCk_AssetBlockPatchEntry{};
        Entry.PropertyName = InPropertyName;
        Entry.Delete       = true;
        return Entry;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetBlockPatcher::
        Blank_CommentsAndStrings(
            const FString& InText)
        -> FString
    {
        auto Out = InText;
        const auto N = InText.Len();
        auto Index = 0;

        while (Index < N)
        {
            const auto Char = InText[Index];
            const auto Next = (Index + 1 < N) ? InText[Index + 1] : TCHAR{0};

            if (Char == TEXT('/') && Next == TEXT('/'))
            {
                while (Index < N && InText[Index] != TEXT('\n'))
                { Out[Index] = TEXT(' '); ++Index; }
                continue;
            }

            if (Char == TEXT('/') && Next == TEXT('*'))
            {
                Out[Index] = TEXT(' '); Out[Index + 1] = TEXT(' ');
                Index += 2;
                while (Index < N)
                {
                    if (InText[Index] == TEXT('*') && Index + 1 < N && InText[Index + 1] == TEXT('/'))
                    {
                        Out[Index] = TEXT(' '); Out[Index + 1] = TEXT(' ');
                        Index += 2;
                        break;
                    }
                    if (InText[Index] != TEXT('\n'))
                    { Out[Index] = TEXT(' '); }
                    ++Index;
                }
                continue;
            }

            if (Char == TEXT('"') || Char == TEXT('\''))
            {
                const auto Quote = Char;
                ++Index;
                while (Index < N)
                {
                    if (InText[Index] == TEXT('\\') && Index + 1 < N)
                    {
                        Out[Index] = TEXT(' '); Out[Index + 1] = TEXT(' ');
                        Index += 2;
                        continue;
                    }
                    if (InText[Index] == Quote)
                    { ++Index; break; }
                    if (InText[Index] != TEXT('\n'))
                    { Out[Index] = TEXT(' '); }
                    ++Index;
                }
                continue;
            }

            ++Index;
        }

        return Out;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetBlockPatcher::
        Find_AssetBlock(
            const FString& InFileContents,
            const FString& InAssetName)
        -> FCk_AssetBlockLocation
    {
        auto Location = FCk_AssetBlockLocation{};

        if (InAssetName.IsEmpty())
        {
            Location.FailReason = ECk_AssetBlockPatch_FailReason::DeclarationNotFound;
            return Location;
        }

        const auto Blanked = Blank_CommentsAndStrings(InFileContents);

        // Mirrors AngelscriptPreprocessor.cpp:3953 — the declaration must end its own line, so the
        // body's `{` is always on a following line.
        // The leading boundary stops `superasset Foo of Bar` matching as `asset Foo of Bar`.
        const auto Pattern = FRegexPattern{FString::Printf(
            TEXT("(^|[^A-Za-z0-9_])asset\\s+%s\\s+of\\s+([A-Za-z0-9_]+)\\s*(\\r|\\n|$)"), *InAssetName)};

        auto Matcher = FRegexMatcher{Pattern, Blanked};
        if (NOT Matcher.FindNext())
        {
            Location.FailReason = ECk_AssetBlockPatch_FailReason::DeclarationNotFound;
            return Location;
        }

        // Group 1 is the leading boundary character, which is not part of the declaration — skip it
        // so DeclStart still points at the `a` of `asset`.
        const auto BoundaryLen = Matcher.GetCaptureGroup(1).Len();
        const auto DeclStart   = Matcher.GetMatchBeginning() + BoundaryLen;
        const auto DeclEnd     = Matcher.GetMatchEnding();

        auto BodyOpen = DeclEnd;
        while (BodyOpen < Blanked.Len() && FChar::IsWhitespace(Blanked[BodyOpen]))
        { ++BodyOpen; }

        if (BodyOpen >= Blanked.Len() || Blanked[BodyOpen] != TEXT('{'))
        {
            Location.FailReason = ECk_AssetBlockPatch_FailReason::BodyOpenNotFound;
            return Location;
        }

        auto Depth = 1;
        auto BodyClose = int32{INDEX_NONE};
        for (auto Index = BodyOpen + 1; Index < Blanked.Len(); ++Index)
        {
            if (Blanked[Index] == TEXT('{'))
            { ++Depth; }
            else if (Blanked[Index] == TEXT('}'))
            {
                --Depth;
                if (Depth == 0)
                { BodyClose = Index; break; }
            }
        }

        if (BodyClose == INDEX_NONE)
        {
            Location.FailReason = ECk_AssetBlockPatch_FailReason::BodyUnterminated;
            return Location;
        }

        Location.Found      = true;
        Location.TypeName   = Matcher.GetCaptureGroup(2);
        Location.DeclStart  = DeclStart;
        Location.BodyOpen   = BodyOpen;
        Location.BodyClose  = BodyClose;
        Location.DeclLine   = ck_angelscript_generator_asset_block_patcher::Get_LineNumberAt(InFileContents, DeclStart);
        Location.DeclIndent = ck_angelscript_generator_asset_block_patcher::Get_LeadingWhitespace(
            ck_angelscript_generator_asset_block_patcher::Get_LineTextAt(InFileContents, DeclStart));

        return Location;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetBlockPatcher::
        Find_AllAssetDeclarations(
            const FString& InFileContents)
        -> TArray<FString>
    {
        const auto Blanked = Blank_CommentsAndStrings(InFileContents);

        static const auto Pattern = FRegexPattern{
            TEXT("(^|[^A-Za-z0-9_])asset\\s+([A-Za-z0-9_]+)\\s+of\\s+([A-Za-z0-9_]+)\\s*(\\r|\\n|$)")};

        auto Matcher = FRegexMatcher{Pattern, Blanked};
        auto Names = TArray<FString>{};

        while (Matcher.FindNext())
        { Names.Add(Matcher.GetCaptureGroup(2)); }

        return Names;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetBlockPatcher::
        Apply_Patch(
            const FString&                          InFileContents,
            const FString&                          InAssetName,
            const TArray<FCk_AssetBlockPatchEntry>& InEntries)
        -> FCk_AssetBlockPatchResult
    {
        namespace detail = ck_angelscript_generator_asset_block_patcher;

        auto Result = FCk_AssetBlockPatchResult{};

        if (InEntries.IsEmpty())
        {
            Result.FailReason   = ECk_AssetBlockPatch_FailReason::NothingToPatch;
            Result.ErrorMessage = TEXT("No properties were queued for write-back.");
            return Result;
        }

        const auto Location = Find_AssetBlock(InFileContents, InAssetName);
        if (NOT Location.Found)
        {
            Result.FailReason = Location.FailReason;
            Result.ErrorMessage = FString::Printf(
                TEXT("Could not locate the `asset %s of ...` block in the source file."), *InAssetName);
            return Result;
        }

        const auto Blanked = Blank_CommentsAndStrings(InFileContents);
        const auto Existing = detail::Collect_Assignments(Blanked, Location.BodyOpen, Location.BodyClose);
        const auto Terminator = Get_LineTerminator(InFileContents);

        auto InsertIndent = Location.DeclIndent
            + FString::ChrN(detail::DefaultIndentWidth, TEXT(' '));
        for (const auto& Assignment : Existing)
        {
            if (Assignment.OwnsWholeLine)
            { InsertIndent = Assignment.Indent; break; }
        }

        auto Edits       = TArray<detail::FTextEdit>{};
        auto InsertLines = TArray<FString>{};

        for (const auto& Entry : InEntries)
        {
            if (Entry.Delete)
            {
                auto MatchedAny = false;

                for (const auto& Assignment : Existing)
                {
                    if (Assignment.PropertyName != Entry.PropertyName)
                    { continue; }

                    MatchedAny = true;

                    // A statement sharing its line with other code loses only its own span, so the
                    // neighbouring statement survives the delete.
                    const auto DeleteStart = Assignment.OwnsWholeLine ? Assignment.LineStart : Assignment.NameStart;
                    Edits.Add({DeleteStart, Assignment.DeleteEnd, FString{}});

                    auto Line = FCk_AssetBlockLineDiff{};
                    Line.Op         = ECk_AssetBlockPatch_Op::DeleteLine;
                    Line.LineNumber = detail::Get_LineNumberAt(InFileContents, Assignment.NameStart);
                    Line.Before     = InFileContents.Mid(DeleteStart, Assignment.DeleteEnd - DeleteStart).TrimEnd();
                    Result.Diff.Add(MoveTemp(Line));
                }

                if (NOT MatchedAny)
                { Result.UnmatchedDeletes.Add(Entry.PropertyName); }

                continue;
            }

            for (const auto& Assignment : Entry.Assignments)
            {
                // Last write wins: AngelScript executes the body top-down, so patching the final
                // assignment is the only edit that changes what the file actually produces.
                const auto* Target = static_cast<const detail::FParsedAssignment*>(nullptr);
                for (const auto& Candidate : Existing)
                {
                    if (Candidate.PropertyName == Entry.PropertyName && Candidate.SubPath == Assignment.SubPath)
                    { Target = &Candidate; }
                }

                if (Target != nullptr)
                {
                    Edits.Add({Target->ValueStart, Target->SemicolonPos, Assignment.Expression});

                    const auto TailEnd = detail::Get_NextLineStartAt(InFileContents, Target->SemicolonPos);

                    auto Line = FCk_AssetBlockLineDiff{};
                    Line.Op         = ECk_AssetBlockPatch_Op::ReplaceValue;
                    Line.LineNumber = detail::Get_LineNumberAt(InFileContents, Target->NameStart);
                    Line.Before     = InFileContents.Mid(Target->LineStart, TailEnd - Target->LineStart).TrimEnd();
                    Line.After      = (InFileContents.Mid(Target->LineStart, Target->ValueStart - Target->LineStart)
                                    + Assignment.Expression
                                    + InFileContents.Mid(Target->SemicolonPos, TailEnd - Target->SemicolonPos)).TrimEnd();
                    Result.Diff.Add(MoveTemp(Line));
                    continue;
                }

                const auto NewLine = FString::Printf(TEXT("%s%s%s = %s;"),
                    *InsertIndent, *Entry.PropertyName, *Assignment.SubPath, *Assignment.Expression);
                InsertLines.Add(NewLine);
            }
        }

        auto InsertAnchor = int32{INDEX_NONE};
        if (NOT InsertLines.IsEmpty())
        {
            const auto CloseLineStart = detail::Get_LineStartAt(InFileContents, Location.BodyClose);
            const auto CloseIsAlone = Blanked.Mid(CloseLineStart, Location.BodyClose - CloseLineStart)
                                             .TrimStartAndEnd().IsEmpty();

            auto Block = FString{};
            if (CloseIsAlone)
            {
                InsertAnchor = CloseLineStart;
                for (const auto& Line : InsertLines)
                { Block += Line + Terminator; }
            }
            else
            {
                // The closing brace shares its line with code — push the new statements in front of
                // it on their own lines rather than rewriting the author's layout.
                InsertAnchor = Location.BodyClose;
                for (const auto& Line : InsertLines)
                { Block += Terminator + Line; }
                Block += Terminator + Location.DeclIndent;
            }

            Edits.Add({InsertAnchor, InsertAnchor, Block});

            const auto AnchorLine = detail::Get_LineNumberAt(InFileContents, InsertAnchor);
            for (const auto& Line : InsertLines)
            {
                auto Diff = FCk_AssetBlockLineDiff{};
                Diff.Op         = ECk_AssetBlockPatch_Op::InsertLine;
                Diff.LineNumber = AnchorLine;
                Diff.After      = Line;
                Result.Diff.Add(MoveTemp(Diff));
            }
        }

        Edits.Sort([](const detail::FTextEdit& InA, const detail::FTextEdit& InB)
        {
            return InA.Start > InB.Start;
        });

        auto Patched = InFileContents;
        for (const auto& Edit : Edits)
        {
            Patched = Patched.Left(Edit.Start) + Edit.Replacement + Patched.RightChop(Edit.End);
        }

        Result.Diff.Sort([](const FCk_AssetBlockLineDiff& InA, const FCk_AssetBlockLineDiff& InB)
        {
            return InA.LineNumber < InB.LineNumber;
        });

        Result.Success         = true;
        Result.PatchedContents = MoveTemp(Patched);
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetBlockPatcher::
        Get_IsInsideEditorGuard(
            const FString& InFileContents,
            int32          InOffset)
        -> bool
    {
        // Blanked, so a directive inside a comment cannot open a guard that never closes.
        const auto Blanked = Blank_CommentsAndStrings(InFileContents);
        const auto Head    = Blanked.Left(FMath::Clamp(InOffset, 0, Blanked.Len()));

        auto Lines = TArray<FString>{};
        Head.ParseIntoArrayLines(Lines, /*InCullEmpty=*/false);

        // One entry per open `#if`. `IsEditor` is whether the condition names Editor; `InElseArm`
        // tracks which side of an `#else`/`#elif` we are on. The region counts as editor-guarded
        // only on the TRUE side of an editor condition — flipping the flag instead would make the
        // else-arm of a NON-editor `#if` read as editor-guarded, which would let an editor-only
        // accessor through into code that ships.
        struct FGuard
        {
            bool IsEditor  = false;
            bool InElseArm = false;
        };

        auto GuardStack = TArray<FGuard>{};

        for (const auto& Line : Lines)
        {
            const auto Trimmed = Line.TrimStart();
            if (NOT Trimmed.StartsWith(TEXT("#"), ESearchCase::CaseSensitive))
            { continue; }

            const auto Directive = Trimmed.RightChop(1).TrimStart();

            if (Directive.StartsWith(TEXT("endif"), ESearchCase::CaseSensitive))
            {
                if (NOT GuardStack.IsEmpty())
                { GuardStack.Pop(); }
                continue;
            }

            // `#elif` leaves the original condition's arm and is never itself an editor guard here:
            // the emitter only ever writes a plain `#if Editor`.
            if (Directive.StartsWith(TEXT("elif"), ESearchCase::CaseSensitive)
                || Directive.StartsWith(TEXT("else"), ESearchCase::CaseSensitive))
            {
                if (NOT GuardStack.IsEmpty())
                { GuardStack.Last().InElseArm = true; }
                continue;
            }

            if (Directive.StartsWith(TEXT("if"), ESearchCase::CaseSensitive))
            {
                const auto Condition = Directive.RightChop(2).TrimStartAndEnd();
                GuardStack.Add(FGuard{Condition.Equals(TEXT("Editor"), ESearchCase::IgnoreCase), false});
            }
        }

        return GuardStack.ContainsByPredicate([](const FGuard& InGuard)
        {
            return InGuard.IsEditor && NOT InGuard.InElseArm;
        });
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetBlockPatcher::
        Get_LineTerminator(
            const FString& InFileContents)
        -> FString
    {
        return InFileContents.Contains(TEXT("\r\n"), ESearchCase::CaseSensitive)
            ? FString{TEXT("\r\n")}
            : FString{TEXT("\n")};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetBlockPatcher::
        Try_ReadSnapshot(
            const FString&      InPath,
            FCk_AsFileSnapshot& OutSnapshot)
        -> bool
    {
        OutSnapshot = FCk_AsFileSnapshot{};
        OutSnapshot.AbsolutePath = InPath;

        auto Raw = TArray<uint8>{};
        if (NOT FFileHelper::LoadFileToArray(Raw, *InPath))
        {
            OutSnapshot.ErrorMessage = FString::Printf(TEXT("Could not read '%s'."), *InPath);
            return false;
        }

        const auto HasUtf16Bom = Raw.Num() >= 2
            && ((Raw[0] == 0xFF && Raw[1] == 0xFE) || (Raw[0] == 0xFE && Raw[1] == 0xFF));

        if (HasUtf16Bom)
        {
            OutSnapshot.ErrorMessage = FString::Printf(
                TEXT("'%s' is UTF-16 encoded; write-back only round-trips UTF-8 sources."), *InPath);
            return false;
        }

        auto Contents = FString{};
        if (NOT FFileHelper::LoadFileToString(Contents, *InPath))
        {
            OutSnapshot.ErrorMessage = FString::Printf(TEXT("Could not decode '%s'."), *InPath);
            return false;
        }

        OutSnapshot.Loaded         = true;
        OutSnapshot.Contents       = MoveTemp(Contents);
        OutSnapshot.HadUtf8Bom     = Raw.Num() >= 3 && Raw[0] == 0xEF && Raw[1] == 0xBB && Raw[2] == 0xBF;
        OutSnapshot.LineTerminator = Get_LineTerminator(OutSnapshot.Contents);
        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetBlockPatcher::
        Try_AtomicWrite(
            const FCk_AsFileSnapshot& InSnapshot,
            const FString&            InContents)
        -> bool
    {
        const auto TempPath = InSnapshot.AbsolutePath + TEXT(".writebacktmp");

        IFileManager::Get().Delete(*TempPath, /*RequireExists=*/false, /*EvenReadOnly=*/false, /*Quiet=*/true);

        const auto Encoding = InSnapshot.HadUtf8Bom
            ? FFileHelper::EEncodingOptions::ForceUTF8
            : FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM;

        if (NOT FFileHelper::SaveStringToFile(InContents, *TempPath, Encoding))
        { return false; }

        return IFileManager::Get().Move(*InSnapshot.AbsolutePath, *TempPath, /*Replace=*/true);
    }
}

// --------------------------------------------------------------------------------------------------------------------
