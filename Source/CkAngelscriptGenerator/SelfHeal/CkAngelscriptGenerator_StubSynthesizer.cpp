#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_StubSynthesizer.h"

#include "CkCore/Macros/CkMacros.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::self_heal
{
    namespace
    {
        // ---- Argument-list emission ------------------------------------------------

        // Splits an args-list captured from "No matching signatures to '...(X, Y, Z)'"
        // into individual type strings. Tolerates spaces around commas. Does NOT try
        // to handle templates with commas inside (e.g. TMap<int, FString>) — those
        // are not seen in EntitySpawnParams Params() signatures and would require a
        // bracket-aware tokenizer.
        auto Split_ArgTypes(
            const FString& InArgsList) -> TArray<FString>
        {
            auto Out = TArray<FString>{};
            if (InArgsList.IsEmpty())
            { return Out; }

            auto Parts = TArray<FString>{};
            InArgsList.ParseIntoArray(Parts, TEXT(","), /*InCullEmpty=*/true);
            for (auto& Part : Parts)
            { Out.Add(Part.TrimStartAndEnd()); }
            return Out;
        }

        // Strips a leading "const " from a type token. The AS error reports "const T"
        // but the real generator emits parameter declarations without const, matching
        // how the property would appear on the originating UPROPERTY. Match the real
        // generator's shape so stubs look authentic.
        auto Strip_LeadingConst(
            const FString& InTypeToken) -> FString
        {
            const auto Trimmed = InTypeToken.TrimStartAndEnd();
            if (Trimmed.StartsWith(TEXT("const ")))
            { return Trimmed.RightChop(6).TrimStart(); }
            return Trimmed;
        }

        // "FTransform, const UClass" -> "FTransform Arg0, UClass Arg1"
        auto Format_ParameterList(
            const FString& InArgsList) -> FString
        {
            const auto Types = Split_ArgTypes(InArgsList);
            if (Types.Num() == 0)
            { return FString{}; }

            auto Parts = TArray<FString>{};
            for (auto i = 0; i < Types.Num(); ++i)
            {
                Parts.Add(FString::Printf(TEXT("%s Arg%d"), *Strip_LeadingConst(Types[i]), i));
            }
            return FString::Join(Parts, TEXT(", "));
        }

        // ---- File IO helpers -------------------------------------------------------

        // Read full file contents as text. Returns empty string + false if not found.
        auto Try_ReadFile(
            const FString& InPath,
            FString&       OutContents) -> bool
        {
            return FFileHelper::LoadFileToString(OutContents, *InPath);
        }

        // Atomic write: temp + rename. The destination is overwritten only on a
        // successful temp write; partial writes never reach the destination.
        auto Try_AtomicWrite(
            const FString& InPath,
            const FString& InContents) -> bool
        {
            const auto TempPath = InPath + TEXT(".stubtmp");

            // Make sure no leftover from a prior failed write blocks us.
            IFileManager::Get().Delete(*TempPath, /*RequireExists=*/false, /*EvenReadOnly=*/false, /*Quiet=*/true);

            if (NOT FFileHelper::SaveStringToFile(InContents, *TempPath,
                FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
            { return false; }

            return IFileManager::Get().Move(*InPath, *TempPath, /*Replace=*/true);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Get_MarkerComment()
        -> FString
    {
        return FString{TEXT("// CkAngelscriptGenerator: synthesized stub for emergency recovery; will be replaced on next clean compile.")};
    }

    auto
        FCkAsStubSynthesizer::
        Derive_SpawnParamsStructName(
            const FString& InNamespaceName)
        -> FString
    {
        if (InNamespaceName.IsEmpty())
        { return FString{}; }

        // Convention from the real generator: U<X> -> F<X>_SpawnParams.
        // If the leading char isn't 'U', we don't recognize the input as an
        // entity-script class name and refuse to derive (the dispatcher won't
        // route this case to us anyway, but defend against misuse).
        if (NOT InNamespaceName.StartsWith(TEXT("U")))
        { return FString{}; }

        return FString::Printf(TEXT("F%s_SpawnParams"), *InNamespaceName.RightChop(1));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Build_EntityScriptParamsStub(
            const FCk_AsParsedError& InError,
            bool                     InEmitStruct)
        -> FString
    {
        if (InError.Kind != ECk_AsParsedError_Kind::NoMatchingSignatures)
        { return FString{}; }

        const auto StructName = Derive_SpawnParamsStructName(InError.TargetNamespace);
        if (StructName.IsEmpty())
        { return FString{}; }

        const auto ParamList = Format_ParameterList(InError.ArgsList);
        const auto Marker    = Get_MarkerComment();

        auto Out = FString{};
        Out += LINE_TERMINATOR;
        Out += Marker;                                                                            Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("// Target: %s::%s(%s)"),
            *InError.TargetNamespace, *InError.FunctionName, *InError.ArgsList);                  Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("// Triggering site: %s:%d:%d"),
            *InError.FilePath, InError.Line, InError.Column);                                     Out += LINE_TERMINATOR;

        if (InEmitStruct)
        {
            Out += TEXT("USTRUCT()");                                                             Out += LINE_TERMINATOR;
            Out += FString::Printf(TEXT("struct %s"), *StructName);                               Out += LINE_TERMINATOR;
            Out += TEXT("{");                                                                     Out += LINE_TERMINATOR;
            Out += TEXT("}");                                                                     Out += LINE_TERMINATOR;
            Out += LINE_TERMINATOR;
        }

        Out += FString::Printf(TEXT("namespace %s"), *InError.TargetNamespace);                   Out += LINE_TERMINATOR;
        Out += TEXT("{");                                                                         Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("    %s %s(%s)"),
            *StructName, *InError.FunctionName, *ParamList);                                      Out += LINE_TERMINATOR;
        Out += TEXT("    {");                                                                     Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("        return %s();"), *StructName);                        Out += LINE_TERMINATOR;
        Out += TEXT("    }");                                                                     Out += LINE_TERMINATOR;
        Out += TEXT("}");                                                                         Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("// End synthesized stub for %s::%s"),
            *InError.TargetNamespace, *InError.FunctionName);                                     Out += LINE_TERMINATOR;

        return Out;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Has_SpawnParamsStruct(
            const FString& InFileContents,
            const FString& InStructName)
        -> bool
    {
        if (InStructName.IsEmpty())
        { return false; }

        // Look for a definition-style occurrence: "struct <Name>". This avoids
        // false positives on references like "FX_SpawnParams()" calls.
        const auto Needle = FString::Printf(TEXT("struct %s"), *InStructName);
        return InFileContents.Contains(Needle);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Find_TargetFile_ByContent(
            const FString&         InNamespaceName,
            const TArray<FString>& InCandidateFilePaths)
        -> FString
    {
        if (InNamespaceName.IsEmpty() || InCandidateFilePaths.Num() == 0)
        { return FString{}; }

        const auto StructName = Derive_SpawnParamsStructName(InNamespaceName);
        // We accept a match on either the namespace name OR the derived struct
        // name — a corruption that mangled one but not the other still resolves.
        // The class-identifier suffix (without leading 'U' / 'F') is also a
        // strong signal that survives prefix-preserving renames.
        const auto Suffix = InNamespaceName.StartsWith(TEXT("U")) ? InNamespaceName.RightChop(1) : InNamespaceName;

        auto Match = FString{};
        for (const auto& Path : InCandidateFilePaths)
        {
            auto Contents = FString{};
            if (NOT Try_ReadFile(Path, Contents))
            { continue; }

            const auto HasNamespace = Contents.Contains(InNamespaceName);
            const auto HasStruct    = NOT StructName.IsEmpty() && Contents.Contains(StructName);
            const auto HasSuffix    = NOT Suffix.IsEmpty()     && Contents.Contains(Suffix);

            if (HasNamespace || HasStruct || HasSuffix)
            {
                if (NOT Match.IsEmpty())
                {
                    // Ambiguous — the identifier appears in multiple candidates.
                    // Refuse to choose; caller falls back to a default.
                    return FString{};
                }
                Match = Path;
            }
        }
        return Match;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Inject_EntityScriptParamsStub(
            const FCk_AsParsedError& InError,
            const TArray<FString>&   InCandidateFilePaths)
        -> FCk_StubInjectionResult
    {
        auto Result = FCk_StubInjectionResult{};

        if (InError.Kind != ECk_AsParsedError_Kind::NoMatchingSignatures)
        {
            Result.ErrorMessage = TEXT("Stub synthesizer only handles NoMatchingSignatures errors.");
            return Result;
        }

        const auto TargetPath = Find_TargetFile_ByContent(InError.TargetNamespace, InCandidateFilePaths);
        if (TargetPath.IsEmpty())
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Could not locate a candidate file referencing namespace '%s' or its derived struct."),
                *InError.TargetNamespace);
            return Result;
        }

        auto Contents = FString{};
        if (NOT Try_ReadFile(TargetPath, Contents))
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Failed to read target file '%s' (located by content match but unreadable)."),
                *TargetPath);
            return Result;
        }

        const auto StructName  = Derive_SpawnParamsStructName(InError.TargetNamespace);
        const auto EmitStruct  = NOT Has_SpawnParamsStruct(Contents, StructName);
        const auto StubBlock   = Build_EntityScriptParamsStub(InError, EmitStruct);

        if (StubBlock.IsEmpty())
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Build_EntityScriptParamsStub returned empty output for namespace '%s' (unrecognized name shape?)."),
                *InError.TargetNamespace);
            return Result;
        }

        const auto NewContents = Contents + StubBlock;
        if (NOT Try_AtomicWrite(TargetPath, NewContents))
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Atomic write failed for target file '%s'."), *TargetPath);
            return Result;
        }

        Result.Success        = true;
        Result.TargetFilePath = TargetPath;
        Result.InjectedBlock  = StubBlock;
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
