#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsSourceScanner.h"

#include "CkAngelscriptGenerator/CkAngelscriptEntityScriptParamsGenerator.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_SharedUtils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::self_heal
{
    namespace ck_angelscript_generator_as_source_scanner
    {
        auto Is_IdentChar(
            TCHAR InChar) -> bool
        {
            return FChar::IsAlnum(InChar) || InChar == TEXT('_');
        }

        // Returns a same-length copy with comment text and string-literal
        // CONTENTS blanked to spaces (newlines preserved), so structural
        // scanning (brace/paren matching, token searches) can't be fooled by
        // braces in comments or semicolons in strings. Identifiers and types
        // are never inside comments/strings, so extraction from the blanked
        // text is lossless for our purposes. Handles //, /* */, "..." and
        // '...' with backslash escapes (the f"..."/n"..." prefixes need no
        // special-casing — blanking starts at the quote).
        auto Blank_CommentsAndStrings(
            const FString& InText) -> FString
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

        // Single space between tokens, no leading/trailing whitespace —
        // canonicalizes multi-line declarations into one comparable line.
        auto Collapse_Whitespace(
            const FString& InText) -> FString
        {
            auto Out = FString{};
            Out.Reserve(InText.Len());
            auto PrevWasSpace = true;
            for (const auto Char : InText)
            {
                if (FChar::IsWhitespace(Char))
                {
                    if (NOT PrevWasSpace)
                    { Out.AppendChar(TEXT(' ')); PrevWasSpace = true; }
                }
                else
                {
                    Out.AppendChar(Char);
                    PrevWasSpace = false;
                }
            }
            return Out.TrimEnd();
        }

        // Locates `class <InClassName>` (token-boundary exact) in blanked
        // text; outputs the index of the body's opening brace and the base
        // class name (empty if the declaration has none).
        auto Find_ClassDecl(
            const FString& InBlanked,
            const FString& InClassName,
            int32&         OutBodyOpenBrace,
            FString&       OutBaseName) -> bool
        {
            static const auto Keyword = FString{TEXT("class")};
            const auto N = InBlanked.Len();
            auto SearchFrom = 0;

            while (true)
            {
                const auto KwPos = InBlanked.Find(Keyword, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
                if (KwPos == INDEX_NONE)
                { return false; }
                SearchFrom = KwPos + Keyword.Len();

                if (KwPos > 0 && Is_IdentChar(InBlanked[KwPos - 1]))
                { continue; }
                if (KwPos + Keyword.Len() >= N || NOT FChar::IsWhitespace(InBlanked[KwPos + Keyword.Len()]))
                { continue; }

                auto Pos = KwPos + Keyword.Len();
                while (Pos < N && FChar::IsWhitespace(InBlanked[Pos])) { ++Pos; }

                const auto NameStart = Pos;
                while (Pos < N && Is_IdentChar(InBlanked[Pos])) { ++Pos; }
                if (InBlanked.Mid(NameStart, Pos - NameStart) != InClassName)
                { continue; }

                while (Pos < N && FChar::IsWhitespace(InBlanked[Pos])) { ++Pos; }

                auto BaseName = FString{};
                if (Pos < N && InBlanked[Pos] == TEXT(':'))
                {
                    ++Pos;
                    while (Pos < N && FChar::IsWhitespace(InBlanked[Pos])) { ++Pos; }
                    const auto BaseStart = Pos;
                    while (Pos < N && Is_IdentChar(InBlanked[Pos])) { ++Pos; }
                    BaseName = InBlanked.Mid(BaseStart, Pos - BaseStart);
                }

                const auto BracePos = InBlanked.Find(TEXT("{"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Pos);
                if (BracePos == INDEX_NONE)
                { continue; }

                OutBodyOpenBrace = BracePos;
                OutBaseName      = BaseName;
                return true;
            }
        }

        auto Matches_Token(
            const FString& InText,
            int32          InPos,
            const TCHAR*   InToken) -> bool
        {
            const auto TokenLen = FCString::Strlen(InToken);
            if (InPos + TokenLen > InText.Len())
            { return false; }
            for (auto Offset = 0; Offset < TokenLen; ++Offset)
            {
                if (InText[InPos + Offset] != InToken[Offset])
                { return false; }
            }
            if (InPos > 0 && Is_IdentChar(InText[InPos - 1]))
            { return false; }
            if (InPos + TokenLen < InText.Len() && Is_IdentChar(InText[InPos + TokenLen]))
            { return false; }
            return true;
        }

        // Walks the class body (from its opening brace) collecting depth-1
        // UPROPERTY(...ExposeOnSpawn...) member declarations in order.
        // Returns false on unbalanced braces (malformed/truncated source).
        auto Parse_ExposedMembers(
            const FString&                 InBlanked,
            int32                          InBodyOpenBrace,
            TArray<FCk_AsExposedProperty>& OutProps) -> bool
        {
            const auto N = InBlanked.Len();
            auto Depth = 0;
            auto Index = InBodyOpenBrace;

            while (Index < N)
            {
                const auto Char = InBlanked[Index];

                if (Char == TEXT('{'))
                { ++Depth; ++Index; continue; }

                if (Char == TEXT('}'))
                {
                    --Depth;
                    if (Depth == 0)
                    { return true; }
                    ++Index;
                    continue;
                }

                if (Depth == 1 && Char == TEXT('U') && Matches_Token(InBlanked, Index, TEXT("UPROPERTY")))
                {
                    auto Pos = Index + 9; // after "UPROPERTY"
                    while (Pos < N && FChar::IsWhitespace(InBlanked[Pos])) { ++Pos; }
                    if (Pos >= N || InBlanked[Pos] != TEXT('('))
                    { Index = Pos; continue; }

                    // Match the specifier list's parens (meta = (...) nests).
                    auto ParenDepth = 0;
                    auto SpecEnd    = Pos;
                    while (SpecEnd < N)
                    {
                        if (InBlanked[SpecEnd] == TEXT('('))      { ++ParenDepth; }
                        else if (InBlanked[SpecEnd] == TEXT(')')) { --ParenDepth; if (ParenDepth == 0) { break; } }
                        ++SpecEnd;
                    }
                    if (SpecEnd >= N)
                    { return false; }

                    const auto Specifiers = InBlanked.Mid(Pos + 1, SpecEnd - Pos - 1);

                    // The member declaration runs to the next top-level ';'.
                    const auto DeclStart = SpecEnd + 1;
                    auto DeclEnd   = DeclStart;
                    auto InnerParen = 0;
                    while (DeclEnd < N)
                    {
                        const auto DeclChar = InBlanked[DeclEnd];
                        if (DeclChar == TEXT('('))      { ++InnerParen; }
                        else if (DeclChar == TEXT(')')) { --InnerParen; }
                        else if (DeclChar == TEXT(';') && InnerParen == 0) { break; }
                        else if (DeclChar == TEXT('{') || DeclChar == TEXT('}')) { break; }
                        ++DeclEnd;
                    }

                    if (DeclEnd < N && InBlanked[DeclEnd] == TEXT(';')
                        && Specifiers.Contains(TEXT("ExposeOnSpawn")))
                    {
                        auto Decl = InBlanked.Mid(DeclStart, DeclEnd - DeclStart);

                        // Strip the default initializer — a stub only has to
                        // compile; the canonical regen restores real defaults.
                        const auto EqPos = Decl.Find(TEXT("="));
                        if (EqPos != INDEX_NONE)
                        { Decl = Decl.Left(EqPos); }

                        const auto Collapsed = Collapse_Whitespace(Decl);
                        auto NameStart = Collapsed.Len();
                        while (NameStart > 0 && Is_IdentChar(Collapsed[NameStart - 1]))
                        { --NameStart; }

                        const auto Name = Collapsed.Mid(NameStart);
                        const auto Type = Collapsed.Left(NameStart).TrimStartAndEnd();

                        if (NOT Name.IsEmpty() && NOT Type.IsEmpty())
                        { OutProps.Add(FCk_AsExposedProperty{Type, Name}); }
                    }

                    Index = (DeclEnd < N) ? DeclEnd + 1 : N;
                    continue;
                }

                ++Index;
            }

            return false;
        }

        // C++ boundary: AS source writes prefixed names (UCk_Foo_UE) but
        // UClass::GetName() is prefix-less. C++ classes ARE registered during
        // a failed AS compile, so reflection works here — and
        // Get_ExposedPropertiesOfClass returns the class's WHOLE chain
        // flattened base-first, so the walk stops at this link.
        auto Try_ResolveCppClassShape(
            const FString&                 InClassName,
            TArray<FCk_AsExposedProperty>& OutProps) -> bool
        {
            if (NOT InClassName.StartsWith(TEXT("U")))
            { return false; }

            auto* Class = FindFirstObject<UClass>(*InClassName.RightChop(1), EFindFirstObjectOptions::None);
            if (ck::Is_NOT_Valid(Class, ck::IsValid_Policy_NullptrOnly{}))
            { return false; }

            const auto Props = UCk_Utils_Reflection_UE::Get_ExposedPropertiesOfClass(Class);
            for (auto* Prop : Props)
            {
                if (ck::Is_NOT_Valid(Prop, ck::IsValid_Policy_NullptrOnly{}))
                { continue; }

                auto TypeText = FCkAngelscriptEntityScriptParamsGenerator::Get_RetainedPropertyType(Prop);
                // Mirror the real generator's const handling (Format_PropertyLine).
                if (Prop->HasAnyPropertyFlags(CPF_ConstParm | CPF_BlueprintReadOnly)
                    && NOT TypeText.StartsWith(TEXT("const ")))
                { TypeText = TEXT("const ") + TypeText; }

                OutProps.Add(FCk_AsExposedProperty{MoveTemp(TypeText), Prop->GetName()});
            }
            return true;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsSourceScanner::
        Get_DefaultScanRoots()
        -> TArray<FString>
    {
        auto Roots = TArray<FString>{};
        Roots.Add(FPaths::ProjectDir() / TEXT("Script"));

        for (const auto& Plugin : IPluginManager::Get().GetEnabledPlugins())
        { Roots.Add(Plugin->GetBaseDir() / TEXT("Script")); }

        Roots.RemoveAll([](const FString& Dir)
        { return NOT IFileManager::Get().DirectoryExists(*Dir); });

        Roots.Sort();
        return Roots;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsSourceScanner::
        Enumerate_AsSourceFiles(
            const TArray<FString>& InScanRoots)
        -> TArray<FString>
    {
        auto Out = TArray<FString>{};
        for (const auto& Root : InScanRoots)
        {
            auto Files = TArray<FString>{};
            IFileManager::Get().FindFilesRecursive(Files, *Root, TEXT("*.as"), /*Files=*/true, /*Directories=*/false);
            Out.Append(Files);
        }

        Out.RemoveAll([](const FString& Path)
        {
            auto Normalized = Path;
            FPaths::NormalizeFilename(Normalized);
            if (Normalized.Contains(TEXT("/Generated/")))
            { return true; }
            return FPaths::GetCleanFilename(Normalized).StartsWith(TEXT("_StubRecovery_"));
        });

        Out.Sort();
        return Out;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsSourceScanner::
        Parse_ClassDeclaration(
            const FString& InFileContents,
            const FString& InClassName,
            const FString& InSourcePathForDiagnostics)
        -> FCk_AsClassParseResult
    {
        auto Result = FCk_AsClassParseResult{};
        Result.ClassName      = InClassName;
        Result.SourceFilePath = InSourcePathForDiagnostics;

        if (InFileContents.IsEmpty() || InClassName.IsEmpty())
        {
            Result.ErrorMessage = TEXT("Empty contents or class name.");
            return Result;
        }

        const auto Blanked = ck_angelscript_generator_as_source_scanner::Blank_CommentsAndStrings(InFileContents);

        auto BodyOpenBrace = int32{INDEX_NONE};
        auto BaseName      = FString{};
        if (NOT ck_angelscript_generator_as_source_scanner::Find_ClassDecl(Blanked, InClassName, BodyOpenBrace, BaseName))
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("No `class %s` declaration in '%s'."), *InClassName, *InSourcePathForDiagnostics);
            return Result;
        }

        auto Props = TArray<FCk_AsExposedProperty>{};
        if (NOT ck_angelscript_generator_as_source_scanner::Parse_ExposedMembers(Blanked, BodyOpenBrace, Props))
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Unbalanced braces parsing `class %s` body in '%s'."), *InClassName, *InSourcePathForDiagnostics);
            return Result;
        }

        Result.Found             = true;
        Result.BaseClassName     = BaseName;
        Result.ExposedProperties = MoveTemp(Props);
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsSourceScanner::
        Scan_ClassShape(
            const FString&         InClassName,
            const TArray<FString>& InScanRoots,
            FCk_AsSourceScanCache* InCache)
        -> FCk_AsClassShape
    {
        auto Result = FCk_AsClassShape{};
        Result.ClassName = InClassName;

        if (InClassName.IsEmpty())
        {
            Result.ErrorMessage = TEXT("Empty class name.");
            return Result;
        }

        // Enumerate once. With a drain-scoped cache the recursive *.as walk is
        // shared across every class resolved in this drain; without it the walk
        // runs per call (original behavior — tests, one-off callers).
        auto       LocalFiles = TArray<FString>{};
        const auto FilesFor   = [&]() -> const TArray<FString>&
        {
            if (InCache == nullptr)
            {
                LocalFiles = Enumerate_AsSourceFiles(InScanRoots);
                return LocalFiles;
            }
            if (NOT InCache->FilesEnumerated)
            {
                InCache->Files           = Enumerate_AsSourceFiles(InScanRoots);
                InCache->FilesEnumerated = true;
            }
            return InCache->Files;
        };
        const auto& Files = FilesFor();

        const auto FindAndParse = [&Files, InCache](const FString& ClassName) -> FCk_AsClassParseResult
        {
            for (const auto& Path : Files)
            {
                // Read once per file: a cache hit avoids re-reading a base-class
                // file shared by multiple derived classes (and across drifts).
                // An empty cached value means a prior load failed / empty file —
                // Contains() then no-matches it exactly as a fresh failed read.
                auto              LocalContents = FString{};
                const FString*    ContentsPtr   = nullptr;
                if (InCache != nullptr)
                {
                    if (auto* Cached = InCache->ContentsByPath.Find(Path))
                    {
                        ContentsPtr = Cached;
                    }
                    else
                    {
                        FFileHelper::LoadFileToString(LocalContents, *Path);
                        ContentsPtr = &InCache->ContentsByPath.Add(Path, MoveTemp(LocalContents));
                    }
                }
                else
                {
                    if (NOT FFileHelper::LoadFileToString(LocalContents, *Path))
                    { continue; }
                    ContentsPtr = &LocalContents;
                }

                if (NOT ContentsPtr->Contains(ClassName))
                { continue; }

                auto Parsed = FCkAsSourceScanner::Parse_ClassDeclaration(*ContentsPtr, ClassName, Path);
                if (Parsed.Found)
                { return Parsed; }
            }

            auto NotFound = FCk_AsClassParseResult{};
            NotFound.ClassName = ClassName;
            return NotFound;
        };

        // Derived-first while walking; reversed into base-first at the end.
        auto PerClassLists = TArray<TArray<FCk_AsExposedProperty>>{};
        auto Visited       = TSet<FString>{};
        auto Current       = InClassName;
        auto IsRoot        = true;

        while (true)
        {
            if (Visited.Contains(Current))
            {
                Result.ErrorMessage = FString::Printf(
                    TEXT("Inheritance cycle at '%s' while scanning '%s'."), *Current, *InClassName);
                return Result;
            }
            Visited.Add(Current);

            if (const auto Parsed = FindAndParse(Current);
                Parsed.Found)
            {
                if (IsRoot)
                { Result.SourceFilePath = Parsed.SourceFilePath; }

                PerClassLists.Add(Parsed.ExposedProperties);

                if (Parsed.BaseClassName.IsEmpty())
                { break; }

                Current = Parsed.BaseClassName;
                IsRoot  = false;
                continue;
            }

            // Not declared in any scanned .as — the C++ boundary (its own
            // supers come flattened from reflection), or unresolvable.
            auto CppProps = TArray<FCk_AsExposedProperty>{};
            if (ck_angelscript_generator_as_source_scanner::Try_ResolveCppClassShape(Current, CppProps))
            {
                PerClassLists.Add(MoveTemp(CppProps));
                break;
            }

            Result.ErrorMessage = IsRoot
                ? FString::Printf(
                    TEXT("Class '%s' not declared in any scanned .as and not resolvable via reflection."), *Current)
                : FString::Printf(
                    TEXT("Base class '%s' of '%s' resolves neither by .as source nor reflection."), *Current, *InClassName);
            return Result;
        }

        for (auto Index = PerClassLists.Num() - 1; Index >= 0; --Index)
        { Result.FlattenedProperties.Append(MoveTemp(PerClassLists[Index])); }

        Result.Found = true;
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
