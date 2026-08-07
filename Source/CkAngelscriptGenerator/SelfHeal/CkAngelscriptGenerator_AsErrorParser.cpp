#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

#include "CkCore/Macros/CkMacros.h"

#include "Internationalization/Regex.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::self_heal
{
    auto
        FCk_AsParsedError::
        operator==(
            const FCk_AsParsedError& Other) const
        -> bool
    {
        return Kind              == Other.Kind
            && FilePath          == Other.FilePath
            && Line              == Other.Line
            && Column            == Other.Column
            && TargetNamespace   == Other.TargetNamespace
            && FunctionName      == Other.FunctionName
            && ArgsList          == Other.ArgsList
            && MissingIdentifier == Other.MissingIdentifier
            && LookupScope       == Other.LookupScope;
    }

    // ----------------------------------------------------------------------------------------------------------------

    namespace ck_angelscript_generator_as_error_parser
    {
        // ---- Line classifiers ------------------------------------------------------

        // Hazelight banners each file's errors with a standalone "<path>.as:" line, which
        // establishes the file context for every following "(L:C) ..." line.
        auto Try_MatchFileHeader(
            const FString& InLine,
            FString&       OutFile) -> bool
        {
            const auto Trimmed = InLine.TrimStartAndEnd();
            if (NOT Trimmed.EndsWith(TEXT(".as:")))
            { return false; }

            OutFile = Trimmed.LeftChop(1);  // drop trailing ":"
            return true;
        }

        // "(L:C) No matching signatures to '<NS>::<func>(<args>)'"
        // The LAST "::" is the separator: greedy (.+) eats any nested namespace prefix, and the
        // function-name class excludes ':' so the greedy match stops at the right boundary.
        auto Try_MatchNoMatchingSignatures(
            const FString&    InLine,
            const FString&    InCurrentFile,
            FCk_AsParsedError& OutError) -> bool
        {
            static const auto Pattern = FRegexPattern{TEXT(
                R"(^\((\d+):(\d+)\) No matching signatures to '(.+)::([^':(]+)\(([^)]*)\)'$)")};

            auto Matcher = FRegexMatcher{Pattern, InLine.TrimStartAndEnd()};
            if (NOT Matcher.FindNext())
            { return false; }

            const auto ArgsList = Matcher.GetCaptureGroup(5);

            // Cascade-only match: AS prints "Unknown" verbatim for a type an EARLIER error left
            // unresolved, so this call site is symptomatic, not a root. Real signatures never
            // contain the token.
            if (ArgsList.Contains(TEXT("Unknown")))
            { return false; }

            OutError                  = FCk_AsParsedError{};
            OutError.Kind             = ECk_AsParsedError_Kind::NoMatchingSignatures;
            OutError.FilePath         = InCurrentFile;
            OutError.Line             = FCString::Atoi(*Matcher.GetCaptureGroup(1));
            OutError.Column           = FCString::Atoi(*Matcher.GetCaptureGroup(2));
            OutError.TargetNamespace  = Matcher.GetCaptureGroup(3);
            OutError.FunctionName     = Matcher.GetCaptureGroup(4);
            OutError.ArgsList         = ArgsList;
            return true;
        }

        // "(L:C) No matching signatures to '<Ident>(<args>)'" — no `::`; a constructor-style call
        // on a type AS could not register. The identifier class forbids ':', so this can never
        // shadow the namespace-qualified form above and matcher order is belt-and-braces.
        auto Try_MatchBareCtorNoMatchingSignatures(
            const FString&    InLine,
            const FString&    InCurrentFile,
            FCk_AsParsedError& OutError) -> bool
        {
            static const auto Pattern = FRegexPattern{TEXT(
                R"(^\((\d+):(\d+)\) No matching signatures to '([A-Za-z_][A-Za-z0-9_]*)\(([^)]*)\)'$)")};

            auto Matcher = FRegexMatcher{Pattern, InLine.TrimStartAndEnd()};
            if (NOT Matcher.FindNext())
            { return false; }

            const auto ArgsList = Matcher.GetCaptureGroup(4);

            // Same cascade filter as the namespace-qualified form.
            if (ArgsList.Contains(TEXT("Unknown")))
            { return false; }

            OutError                   = FCk_AsParsedError{};
            OutError.Kind              = ECk_AsParsedError_Kind::BareCtorNoMatchingSignatures;
            OutError.FilePath          = InCurrentFile;
            OutError.Line              = FCString::Atoi(*Matcher.GetCaptureGroup(1));
            OutError.Column            = FCString::Atoi(*Matcher.GetCaptureGroup(2));
            OutError.MissingIdentifier = Matcher.GetCaptureGroup(3);
            OutError.ArgsList          = ArgsList;
            return true;
        }

        // "(L:C) Identifier '<X>' is not a data type [in namespace '<N>' or parent | in global namespace]"
        // The in-clause is optional so a suffix-less error still matches; "global namespace" hits
        // the second alternation and leaves the scope capture empty.
        auto Try_MatchIdentifierNotADataType(
            const FString&    InLine,
            const FString&    InCurrentFile,
            FCk_AsParsedError& OutError) -> bool
        {
            static const auto Pattern = FRegexPattern{TEXT(
                R"(^\((\d+):(\d+)\) Identifier '([^']+)' is not a data type(?: in namespace '([^']+)' or parent| in global namespace)?$)")};

            auto Matcher = FRegexMatcher{Pattern, InLine.TrimStartAndEnd()};
            if (NOT Matcher.FindNext())
            { return false; }

            OutError                   = FCk_AsParsedError{};
            OutError.Kind              = ECk_AsParsedError_Kind::IdentifierNotADataType;
            OutError.FilePath          = InCurrentFile;
            OutError.Line              = FCString::Atoi(*Matcher.GetCaptureGroup(1));
            OutError.Column            = FCString::Atoi(*Matcher.GetCaptureGroup(2));
            OutError.MissingIdentifier = Matcher.GetCaptureGroup(3);
            OutError.LookupScope       = Matcher.GetCaptureGroup(4);  // empty when "global namespace" or absent
            return true;
        }

        // "(L:C) '<Member>' is not a member of '<Struct>'" — a field access against a struct that
        // no longer declares it. Inside a generated ESP canonical this is the deleted-FIELD twin of
        // the deleted-TYPE case Try_MatchIdentifierNotADataType already covers; the dispatcher keys
        // the two on the same location predicate. Format captured verbatim from FormatDiagnostics
        // during the 2026-08-07 OpenSign repro, NOT from the modal's rendering (which differs).
        auto Try_MatchNotAMemberOfStruct(
            const FString&    InLine,
            const FString&    InCurrentFile,
            FCk_AsParsedError& OutError) -> bool
        {
            static const auto Pattern = FRegexPattern{TEXT(
                R"(^\((\d+):(\d+)\) '([A-Za-z_][A-Za-z0-9_]*)' is not a member of '([A-Za-z_][A-Za-z0-9_]*)'$)")};

            auto Matcher = FRegexMatcher{Pattern, InLine.TrimStartAndEnd()};
            if (NOT Matcher.FindNext())
            { return false; }

            const auto OwningStruct = Matcher.GetCaptureGroup(4);

            // Cascade-only match, and load-bearing: AS prints "Unknown" for a type an EARLIER error
            // left unresolved, so every field access on it reports here. Without this guard the
            // matcher PROMOTES that noise to a root — today it dies only by matching nothing — and
            // a root routed to quarantine DELETES a canonical.
            if (OwningStruct == TEXT("Unknown"))
            { return false; }

            OutError                   = FCk_AsParsedError{};
            OutError.Kind              = ECk_AsParsedError_Kind::NotAMemberOfStruct;
            OutError.FilePath          = InCurrentFile;
            OutError.Line              = FCString::Atoi(*Matcher.GetCaptureGroup(1));
            OutError.Column            = FCString::Atoi(*Matcher.GetCaptureGroup(2));
            OutError.MissingIdentifier = Matcher.GetCaptureGroup(3);
            OutError.LookupScope       = OwningStruct;
            return true;
        }

        // Hazelight emits adjacent-string-literal errors as a TWO-line pair:
        //   (L:C) Expected ')' or ','          <- generic, fires for many parse errors
        //   (L:C) Instead found '<string constant>'   <- unique to the splice case
        // Only the second line is matched; the first would be ambiguous.
        auto Try_MatchAdjacentStringLiteral(
            const FString&    InLine,
            const FString&    InCurrentFile,
            FCk_AsParsedError& OutError) -> bool
        {
            static const auto Pattern = FRegexPattern{TEXT(
                R"(^\((\d+):(\d+)\) Instead found '<string constant>'$)")};

            auto Matcher = FRegexMatcher{Pattern, InLine.TrimStartAndEnd()};
            if (NOT Matcher.FindNext())
            { return false; }

            OutError          = FCk_AsParsedError{};
            OutError.Kind     = ECk_AsParsedError_Kind::AdjacentStringLiteral;
            OutError.FilePath = InCurrentFile;
            OutError.Line     = FCString::Atoi(*Matcher.GetCaptureGroup(1));
            OutError.Column   = FCString::Atoi(*Matcher.GetCaptureGroup(2));
            return true;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsErrorParser::
        ParseErrors(
            const FString& InRawErrorOutput)
        -> TArray<FCk_AsParsedError>
    {
        auto Results    = TArray<FCk_AsParsedError>{};
        auto CurrentFile = FString{};

        auto Lines = TArray<FString>{};
        InRawErrorOutput.ParseIntoArrayLines(Lines, /*InCullEmpty=*/false);

        for (const auto& Line : Lines)
        {
            // File-context first — affects every subsequent "(L:C) ..." line.
            auto NewFile = FString{};
            if (ck_angelscript_generator_as_error_parser::Try_MatchFileHeader(Line, NewFile))
            {
                CurrentFile = NewFile;
                continue;
            }

            auto Error = FCk_AsParsedError{};
            if (ck_angelscript_generator_as_error_parser::Try_MatchNoMatchingSignatures(Line, CurrentFile, Error)
                || ck_angelscript_generator_as_error_parser::Try_MatchBareCtorNoMatchingSignatures(Line, CurrentFile, Error)
                || ck_angelscript_generator_as_error_parser::Try_MatchIdentifierNotADataType(Line, CurrentFile, Error)
                || ck_angelscript_generator_as_error_parser::Try_MatchNotAMemberOfStruct(Line, CurrentFile, Error)
                || ck_angelscript_generator_as_error_parser::Try_MatchAdjacentStringLiteral(Line, CurrentFile, Error))
            {
                Results.Add(MoveTemp(Error));
            }
            // Unrecognized lines (cascade noise, "Compiling ..." context, candidate signature
            // lists, blanks) are intentionally dropped — see the header docstring.
        }

        return Results;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsErrorParser::
        DeduplicateRoots(
            const TArray<FCk_AsParsedError>& InErrors)
        -> TArray<FCk_AsParsedError>
    {
        auto Out  = TArray<FCk_AsParsedError>{};
        auto Seen = TSet<FString>{};

        for (const auto& Err : InErrors)
        {
            // Seeded, not empty: the switch below has no `default` (deliberately — a new kind
            // should be a compile-time prompt to think about its key). A kind that slips through
            // anyway then degrades to per-SITE dedup, which over-attempts and is capped, instead
            // of collapsing every root onto "" and silently dropping all but the first.
            auto Key = FString::Printf(TEXT("kind%d|%s:%d:%d"),
                static_cast<int32>(Err.Kind), *Err.FilePath, Err.Line, Err.Column);

            switch (Err.Kind)
            {
                case ECk_AsParsedError_Kind::NoMatchingSignatures:
                    Key = FString::Printf(TEXT("nm|%s::%s(%s)"),
                        *Err.TargetNamespace, *Err.FunctionName, *Err.ArgsList);
                    break;
                case ECk_AsParsedError_Kind::IdentifierNotADataType:
                    Key = FString::Printf(TEXT("id|%s"), *Err.MissingIdentifier);
                    break;
                case ECk_AsParsedError_Kind::AdjacentStringLiteral:
                    // No qualified identifier — dedup by location: two splices on different lines
                    // are distinct diagnostics, a re-emit of the same line is one.
                    Key = FString::Printf(TEXT("adj|%s:%d:%d"),
                        *Err.FilePath, Err.Line, Err.Column);
                    break;
                case ECk_AsParsedError_Kind::BareCtorNoMatchingSignatures:
                    Key = FString::Printf(TEXT("ctor|%s(%s)"),
                        *Err.MissingIdentifier, *Err.ArgsList);
                    break;
                case ECk_AsParsedError_Kind::NotAMemberOfStruct:
                    // FilePath is part of the key because the recovery this feeds is per-CANONICAL:
                    // two stale canonicals reporting the same dead field are two roots needing two
                    // quarantines, and collapsing them would heal one per cycle against the cap.
                    Key = FString::Printf(TEXT("member|%s|%s.%s"),
                        *Err.FilePath, *Err.LookupScope, *Err.MissingIdentifier);
                    break;
            }

            if (Seen.Contains(Key))
            { continue; }
            Seen.Add(Key);
            Out.Add(Err);
        }

        return Out;
    }
}

// --------------------------------------------------------------------------------------------------------------------
