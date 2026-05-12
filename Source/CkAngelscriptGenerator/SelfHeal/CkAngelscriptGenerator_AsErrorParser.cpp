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

    namespace
    {
        // ---- Line classifiers ------------------------------------------------------

        // A standalone "<path>.as:" line establishes file context for subsequent
        // "(L:C) ..." error lines. Hazelight emits these as a banner before each
        // file's errors.
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
        //
        // Captures the *last* "::" as the namespace/function separator: greedy
        // (.+) eats any nested namespace prefix, and the function-name class
        // [^':(]+ explicitly excludes ':' so the greedy match stops at the
        // correct boundary.
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

            OutError                  = FCk_AsParsedError{};
            OutError.Kind             = ECk_AsParsedError_Kind::NoMatchingSignatures;
            OutError.FilePath         = InCurrentFile;
            OutError.Line             = FCString::Atoi(*Matcher.GetCaptureGroup(1));
            OutError.Column           = FCString::Atoi(*Matcher.GetCaptureGroup(2));
            OutError.TargetNamespace  = Matcher.GetCaptureGroup(3);
            OutError.FunctionName     = Matcher.GetCaptureGroup(4);
            OutError.ArgsList         = Matcher.GetCaptureGroup(5);
            return true;
        }

        // "(L:C) Identifier '<X>' is not a data type [in namespace '<N>' or parent | in global namespace]"
        //
        // The in-clause is optional in the regex so we tolerate the (rare) case
        // where the error has no scope suffix. When present and naming a
        // namespace, group 4 captures the namespace name; "global namespace"
        // matches the second alternation and leaves group 4 empty.
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
            if (Try_MatchFileHeader(Line, NewFile))
            {
                CurrentFile = NewFile;
                continue;
            }

            auto Error = FCk_AsParsedError{};
            if (Try_MatchNoMatchingSignatures(Line, CurrentFile, Error)
                || Try_MatchIdentifierNotADataType(Line, CurrentFile, Error))
            {
                Results.Add(MoveTemp(Error));
            }
            // Unrecognized lines (cascade noise, "Compiling ..." context, candidate
            // signature lists, blank lines, etc.) are intentionally dropped — see
            // header docstring for rationale.
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
            auto Key = FString{};
            switch (Err.Kind)
            {
                case ECk_AsParsedError_Kind::NoMatchingSignatures:
                    Key = FString::Printf(TEXT("nm|%s::%s(%s)"),
                        *Err.TargetNamespace, *Err.FunctionName, *Err.ArgsList);
                    break;
                case ECk_AsParsedError_Kind::IdentifierNotADataType:
                    Key = FString::Printf(TEXT("id|%s"), *Err.MissingIdentifier);
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
