#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_StubSynthesizer.h"

#include "CkCore/Macros/CkMacros.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::self_heal
{
    namespace ck_angelscript_generator_stub_synthesizer
    {
        // ---- Argument-list emission ------------------------------------------------

        // Does NOT handle templates with commas inside (TMap<int, FString>) — those
        // don't appear in EntitySpawnParams Params() signatures.
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

        constexpr auto* kUninferableTypeFallback = TEXT("UObject");

        // Placeholders are always a whole bracketed token; templated types
        // (TArray<int>) start with an identifier character and don't match.
        auto Is_UninferablePlaceholder(
            const FString& InTrimmedToken) -> bool
        {
            return InTrimmedToken.StartsWith(TEXT("<"));
        }

        auto Normalize_TypeToken(
            const FString& InTypeToken) -> FString
        {
            auto Trimmed = InTypeToken.TrimStartAndEnd();
            if (Trimmed.StartsWith(TEXT("const ")))
            { Trimmed = Trimmed.RightChop(6).TrimStart(); }
            if (Trimmed.EndsWith(TEXT("&")))
            { Trimmed = Trimmed.LeftChop(1).TrimEnd(); }
            if (Is_UninferablePlaceholder(Trimmed))
            { return kUninferableTypeFallback; }
            return Trimmed;
        }

        auto Has_UninferableArg(
            const FString& InArgsList) -> bool
        {
            for (const auto& Type : Split_ArgTypes(InArgsList))
            {
                if (Is_UninferablePlaceholder(Type))
                { return true; }
            }
            return false;
        }

        auto Format_ParameterList(
            const FString& InArgsList) -> FString
        {
            const auto Types = Split_ArgTypes(InArgsList);
            if (Types.Num() == 0)
            { return FString{}; }

            auto Parts = TArray<FString>{};
            for (auto i = 0; i < Types.Num(); ++i)
            {
                Parts.Add(FString::Printf(TEXT("%s Arg%d"), *Normalize_TypeToken(Types[i]), i));
            }
            return FString::Join(Parts, TEXT(", "));
        }

        // Scans the sibling's `// End synthesized stub for <NS>::<FUNC>(<args>)`
        // markers. Full-shape blocks emit the same prefix, so their overloads
        // are visible here too.
        auto Collect_ExistingStubArgsLists(
            const FString& InExistingStub,
            const FString& InNamespace,
            const FString& InFunction) -> TArray<FString>
        {
            auto Out = TArray<FString>{};
            const auto Prefix = FString::Printf(TEXT("// End synthesized stub for %s::%s("),
                *InNamespace, *InFunction);

            auto Cursor = 0;
            while (true)
            {
                const auto Pos = InExistingStub.Find(Prefix, ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor);
                if (Pos == INDEX_NONE)
                { break; }

                const auto ArgsStart = Pos + Prefix.Len();
                const auto LineEnd   = InExistingStub.Find(
                    FString{LINE_TERMINATOR}, ESearchCase::CaseSensitive, ESearchDir::FromStart, ArgsStart);
                auto Line = LineEnd != INDEX_NONE
                    ? InExistingStub.Mid(ArgsStart, LineEnd - ArgsStart)
                    : InExistingStub.Mid(ArgsStart);

                const auto CloseParen = Line.Find(TEXT(")"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
                if (CloseParen != INDEX_NONE)
                { Line = Line.Left(CloseParen); }
                Out.Add(Line);

                Cursor = ArgsStart;
            }
            return Out;
        }

        TSet<FString> sFullShapeClassesThisSession;

        // ---- File IO helpers -------------------------------------------------------

        auto Try_ReadFile(
            const FString& InPath,
            FString&       OutContents) -> bool
        {
            return FFileHelper::LoadFileToString(OutContents, *InPath);
        }

        auto Try_AtomicWrite(
            const FString& InPath,
            const FString& InContents) -> bool
        {
            const auto TempPath = InPath + TEXT(".stubtmp");

            // A brand-new plugin/project has no Script/Generated/ yet, and
            // SaveStringToFile fails opaquely into a missing directory.
            const auto ParentDir = FPaths::GetPath(InPath);
            if (NOT ParentDir.IsEmpty())
            { IFileManager::Get().MakeDirectory(*ParentDir, /*Tree=*/true); }

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
        Normalize_ArgsList(
            const FString& InArgsList)
        -> FString
    {
        const auto Types = ck_angelscript_generator_stub_synthesizer::Split_ArgTypes(InArgsList);
        auto Parts = TArray<FString>{};
        for (const auto& Type : Types)
        { Parts.Add(ck_angelscript_generator_stub_synthesizer::Normalize_TypeToken(Type)); }
        return FString::Join(Parts, TEXT(", "));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Get_MarkerComment()
        -> FString
    {
        return FString{TEXT("// CkAngelscriptGenerator: synthesized stub for emergency recovery; will be replaced on next clean compile.")};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Get_StubFileHeader()
        -> FString
    {
        auto Out = FString{};
        Out += TEXT("// ============================================================================");                                                Out += LINE_TERMINATOR;
        Out += TEXT("// CkAngelscriptGenerator: AUTO-GENERATED RECOVERY STUBS");                                                                       Out += LINE_TERMINATOR;
        Out += TEXT("//");                                                                                                                              Out += LINE_TERMINATOR;
        Out += TEXT("// This file is generated by the self-heal dispatcher when AS compile-time");                                                     Out += LINE_TERMINATOR;
        Out += TEXT("// drift is detected at cold-start. It contains MINIMUM-VIABLE stub blocks");                                                     Out += LINE_TERMINATOR;
        Out += TEXT("// that satisfy AS compile so the editor can unwedge from the Hazelight");                                                        Out += LINE_TERMINATOR;
        Out += TEXT("// failure modal.");                                                                                                              Out += LINE_TERMINATOR;
        Out += TEXT("//");                                                                                                                              Out += LINE_TERMINATOR;
        Out += TEXT("// This file is GITIGNORED and self-cleans after a successful AS compile");                                                       Out += LINE_TERMINATOR;
        Out += TEXT("// (the dispatcher deletes it from OnPostCompile). Do not edit by hand.");                                                        Out += LINE_TERMINATOR;
        Out += TEXT("// ============================================================================");                                                Out += LINE_TERMINATOR;
        return Out;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Derive_StubSiblingPath(
            const FString& InCanonicalFilePath)
        -> FString
    {
        if (InCanonicalFilePath.IsEmpty())
        { return FString{}; }

        const auto Dir      = FPaths::GetPath(InCanonicalFilePath);
        const auto BaseName = FPaths::GetCleanFilename(InCanonicalFilePath);
        return Dir / (FString{TEXT("_StubRecovery_")} + BaseName);
    }

    auto
        FCkAsStubSynthesizer::
        Anchor_ByCallerAsPath(
            const FString& InCallerAsFilePath)
        -> FString
    {
        if (InCallerAsFilePath.IsEmpty())
        { return FString{}; }

        auto Current = FPaths::ConvertRelativePathToFull(FPaths::GetPath(InCallerAsFilePath));
        FPaths::NormalizeDirectoryName(Current);

        while (NOT Current.IsEmpty())
        {
            auto PluginManifests = TArray<FString>{};
            IFileManager::Get().FindFiles(PluginManifests, *(Current / TEXT("*.uplugin")), /*Files=*/true, /*Dirs=*/false);
            if (PluginManifests.Num() > 0)
            {
                PluginManifests.Sort();
                const auto PluginName = FPaths::GetBaseFilename(PluginManifests[0]);
                return Current / TEXT("Script/Generated") / (PluginName + FString{TEXT("_EntitySpawnParams.as")});
            }

            auto ProjectManifests = TArray<FString>{};
            IFileManager::Get().FindFiles(ProjectManifests, *(Current / TEXT("*.uproject")), /*Files=*/true, /*Dirs=*/false);
            if (ProjectManifests.Num() > 0)
            {
                ProjectManifests.Sort();
                const auto ProjectName = FPaths::GetBaseFilename(ProjectManifests[0]);
                return Current / TEXT("Script/Generated") / (ProjectName + FString{TEXT("_EntitySpawnParams.as")});
            }

            const auto Parent = FPaths::GetPath(Current);
            if (Parent.IsEmpty() || Parent == Current)
            { return FString{}; }
            Current = Parent;
        }

        return FString{};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Derive_SpawnParamsStructName(
            const FString& InNamespaceName)
        -> FString
    {
        if (InNamespaceName.IsEmpty())
        { return FString{}; }

        if (NOT InNamespaceName.StartsWith(TEXT("U")))
        { return FString{}; }

        return FString::Printf(TEXT("F%s_SpawnParams"), *InNamespaceName.RightChop(1));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Derive_ClassNameFromStructName(
            const FString& InStructName)
        -> FString
    {
        static const auto Suffix = FString{TEXT("_SpawnParams")};

        if (NOT InStructName.StartsWith(TEXT("F")) || NOT InStructName.EndsWith(Suffix))
        { return FString{}; }

        const auto Core = InStructName.Mid(1, InStructName.Len() - 1 - Suffix.Len());
        if (Core.IsEmpty())
        { return FString{}; }

        return FString::Printf(TEXT("U%s"), *Core);
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

        const auto ParamList = ck_angelscript_generator_stub_synthesizer::Format_ParameterList(InError.ArgsList);
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
        // NORMALIZED args: distinct overloads (int vs float) still dedup
        // independently, while argument-category variants of one signature
        // collapse to a single stub. `// Target:` keeps the raw error string.
        Out += FString::Printf(TEXT("// End synthesized stub for %s::%s(%s)"),
            *InError.TargetNamespace, *InError.FunctionName, *Normalize_ArgsList(InError.ArgsList)); Out += LINE_TERMINATOR;

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

        // Definition-style needle — avoids false positives on references like
        // "FX_SpawnParams()" calls.
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
        // OR'd on purpose: a corruption that mangled one of the three identifiers
        // but not the others still resolves.
        const auto Suffix = InNamespaceName.StartsWith(TEXT("U")) ? InNamespaceName.RightChop(1) : InNamespaceName;

        auto Match = FString{};
        for (const auto& Path : InCandidateFilePaths)
        {
            auto Contents = FString{};
            if (NOT ck_angelscript_generator_stub_synthesizer::Try_ReadFile(Path, Contents))
            { continue; }

            const auto HasNamespace = Contents.Contains(InNamespaceName);
            const auto HasStruct    = NOT StructName.IsEmpty() && Contents.Contains(StructName);
            const auto HasSuffix    = NOT Suffix.IsEmpty()     && Contents.Contains(Suffix);

            if (HasNamespace || HasStruct || HasSuffix)
            {
                // Ambiguous — the identifier is in multiple candidates; refuse to
                // choose and let the caller fall back to a default.
                if (NOT Match.IsEmpty())
                { return FString{}; }
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
            Result.FailReason   = ECk_StubInjectFailReason::NotApplicable;
            Result.ErrorMessage = TEXT("Stub synthesizer only handles NoMatchingSignatures errors.");
            return Result;
        }

        auto CanonicalPath = Find_TargetFile_ByContent(InError.TargetNamespace, InCandidateFilePaths);
        if (CanonicalPath.IsEmpty())
        { CanonicalPath = Anchor_ByCallerAsPath(InError.FilePath); }
        if (CanonicalPath.IsEmpty())
        {
            Result.FailReason   = ECk_StubInjectFailReason::AnchorFailed;
            Result.ErrorMessage = FString::Printf(
                TEXT("Could not anchor stub for namespace '%s': no candidate file matched and caller path '%s' has no .uplugin or .uproject ancestor."),
                *InError.TargetNamespace, *InError.FilePath);
            return Result;
        }

        Result.CanonicalFilePath = CanonicalPath;

        const auto StubPath = Derive_StubSiblingPath(CanonicalPath);
        if (StubPath.IsEmpty())
        {
            Result.FailReason   = ECk_StubInjectFailReason::AnchorFailed;
            Result.ErrorMessage = FString::Printf(
                TEXT("Failed to derive stub sibling path from canonical '%s'."), *CanonicalPath);
            return Result;
        }

        auto ExistingStub = FString{};
        const auto StubFileExists = ck_angelscript_generator_stub_synthesizer::Try_ReadFile(StubPath, ExistingStub);

        const auto StructName = Derive_SpawnParamsStructName(InError.TargetNamespace);
        auto CanonicalContents = FString{};
        ck_angelscript_generator_stub_synthesizer::Try_ReadFile(CanonicalPath, CanonicalContents);
        const auto CanonicalHasStruct = Has_SpawnParamsStruct(CanonicalContents, StructName);
        const auto StubHasStruct      = Has_SpawnParamsStruct(ExistingStub,      StructName);
        const auto EmitStruct         = NOT CanonicalHasStruct && NOT StubHasStruct;

        const auto StubBlock = Build_EntityScriptParamsStub(InError, EmitStruct);
        if (StubBlock.IsEmpty())
        {
            Result.FailReason   = ECk_StubInjectFailReason::NotApplicable;
            Result.ErrorMessage = FString::Printf(
                TEXT("Build_EntityScriptParamsStub returned empty output for namespace '%s' (unrecognized name shape?)."),
                *InError.TargetNamespace);
            return Result;
        }

        if (StubFileExists)
        {
            static const auto EndMarkerPrefix = FString{TEXT("// End synthesized stub for ")};
            const auto MarkerPos = StubBlock.Find(EndMarkerPrefix);
            if (MarkerPos != INDEX_NONE)
            {
                const auto LineEndPos = StubBlock.Find(
                    FString{LINE_TERMINATOR}, ESearchCase::CaseSensitive,
                    ESearchDir::FromStart, MarkerPos);
                const auto MarkerLine = LineEndPos != INDEX_NONE
                    ? StubBlock.Mid(MarkerPos, LineEndPos - MarkerPos)
                    : StubBlock.Mid(MarkerPos);

                if (ExistingStub.Contains(MarkerLine))
                {
                    Result.Success        = true;
                    Result.TargetFilePath = StubPath;
                    Result.InjectedBlock  = StubBlock;
                    return Result;
                }
            }

            // Declaration-level backstop for alias classes the marker
            // canonicalization doesn't enumerate: a duplicate emitted overload
            // wedges the merged namespace.
            const auto DeclLine = FString::Printf(TEXT("%s %s(%s)"),
                *StructName, *InError.FunctionName, *ck_angelscript_generator_stub_synthesizer::Format_ParameterList(InError.ArgsList));
            if (ExistingStub.Contains(DeclLine))
            {
                Result.Success        = true;
                Result.TargetFilePath = StubPath;
                Result.InjectedBlock  = StubBlock;
                return Result;
            }
        }

        // Same-arity ambiguity gate: a fallback-typed and a typed stub of the same
        // NS::FUNC + arity are mutually ambiguous at every call site. Fails LOUDLY
        // so the dispatcher escalates to the canonical-quarantine path.
        if (StubFileExists)
        {
            const auto NewArity       = ck_angelscript_generator_stub_synthesizer::Split_ArgTypes(InError.ArgsList).Num();
            const auto NewHasFallback = ck_angelscript_generator_stub_synthesizer::Has_UninferableArg(InError.ArgsList);

            for (const auto& ExistingArgsList : ck_angelscript_generator_stub_synthesizer::Collect_ExistingStubArgsLists(
                ExistingStub, InError.TargetNamespace, InError.FunctionName))
            {
                const auto ExistingArgs = ck_angelscript_generator_stub_synthesizer::Split_ArgTypes(ExistingArgsList);
                if (ExistingArgs.Num() != NewArity)
                { continue; }

                const auto ExistingHasFallback = ExistingArgs.Contains(FString{ck_angelscript_generator_stub_synthesizer::kUninferableTypeFallback});
                if (NewHasFallback || ExistingHasFallback)
                {
                    Result.FailReason     = ECk_StubInjectFailReason::SameArityAmbiguous;
                    Result.TargetFilePath = StubPath;
                    Result.ErrorMessage   = FString::Printf(
                        TEXT("Same-arity ambiguity: an overload of %s::%s with arity %d already exists in the sibling ")
                        TEXT("(fallback-typed on one side) - appending another would be mutually ambiguous at every call ")
                        TEXT("site, and the existing one does not satisfy this caller. Escalation required."),
                        *InError.TargetNamespace, *InError.FunctionName, NewArity);
                    return Result;
                }
            }
        }

        auto NewContents = FString{};
        if (StubFileExists)
        {
            NewContents = ExistingStub + StubBlock;
        }
        else
        {
            NewContents = Get_StubFileHeader() + StubBlock;
        }

        if (NOT ck_angelscript_generator_stub_synthesizer::Try_AtomicWrite(StubPath, NewContents))
        {
            Result.FailReason   = ECk_StubInjectFailReason::WriteFailed;
            Result.ErrorMessage = FString::Printf(
                TEXT("Atomic write failed for stub file '%s'."), *StubPath);
            return Result;
        }

        Result.Success        = true;
        Result.TargetFilePath = StubPath;
        Result.InjectedBlock  = StubBlock;
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Get_FullShapeMarkerLine(
            const FString& InClassName)
        -> FString
    {
        return FString::Printf(TEXT("// End synthesized full-shape stub for %s"), *InClassName);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Build_EntityScriptParamsStub_FullShape(
            const FCk_AsClassShape&  InShape,
            const FCk_AsParsedError& InError)
        -> FString
    {
        const auto StructName = Derive_SpawnParamsStructName(InShape.ClassName);
        if (StructName.IsEmpty())
        { return FString{}; }

        const auto& Props = InShape.FlattenedProperties;

        auto ParamParts = TArray<FString>{};
        auto ArgParts   = TArray<FString>{};
        auto TypeParts  = TArray<FString>{};
        for (const auto& Prop : Props)
        {
            ParamParts.Add(FString::Printf(TEXT("%s In%s"), *Prop.TypeText, *Prop.Name));
            ArgParts.Add(FString::Printf(TEXT("In%s"), *Prop.Name));
            TypeParts.Add(Prop.TypeText);
        }
        const auto ParamList = FString::Join(ParamParts, TEXT(", "));
        const auto ArgList   = FString::Join(ArgParts,   TEXT(", "));

        const auto Target = [&]() -> FString
        {
            switch (InError.Kind)
            {
                case ECk_AsParsedError_Kind::IdentifierNotADataType:
                    return FString::Printf(TEXT("missing type '%s'"), *InError.MissingIdentifier);
                case ECk_AsParsedError_Kind::NotAMemberOfStruct:
                    return FString::Printf(TEXT("deleted field '%s.%s'"),
                        *InError.LookupScope, *InError.MissingIdentifier);
                default:
                    return FString::Printf(TEXT("%s::%s(%s)"),
                        *InError.TargetNamespace, *InError.FunctionName, *InError.ArgsList);
            }
        }();

        auto Out = FString{};
        Out += LINE_TERMINATOR;
        Out += Get_MarkerComment();                                                               Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("// Target: %s"), *Target);                                  Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("// Triggering site: %s:%d:%d"),
            *InError.FilePath, InError.Line, InError.Column);                                     Out += LINE_TERMINATOR;
        if (NOT InShape.SourceFilePath.IsEmpty())
        {
            Out += FString::Printf(TEXT("// Source-derived from: %s"), *InShape.SourceFilePath); Out += LINE_TERMINATOR;
        }

        Out += TEXT("USTRUCT()");                                                                 Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("struct %s"), *StructName);                                   Out += LINE_TERMINATOR;
        Out += TEXT("{");                                                                         Out += LINE_TERMINATOR;
        for (auto Index = 0; Index < Props.Num(); ++Index)
        {
            Out += TEXT("    UPROPERTY()");                                                       Out += LINE_TERMINATOR;
            Out += FString::Printf(TEXT("    %s %s;"),
                *Props[Index].TypeText, *Props[Index].Name);                                      Out += LINE_TERMINATOR;
            if (Index + 1 < Props.Num())
            { Out += LINE_TERMINATOR; }
        }
        if (Props.Num() > 0)
        {
            Out += LINE_TERMINATOR;
            Out += FString::Printf(TEXT("    %s(%s)"), *StructName, *ParamList);                  Out += LINE_TERMINATOR;
            Out += TEXT("    {");                                                                 Out += LINE_TERMINATOR;
            for (const auto& Prop : Props)
            {
                Out += FString::Printf(TEXT("        %s = In%s;"), *Prop.Name, *Prop.Name);       Out += LINE_TERMINATOR;
            }
            Out += TEXT("    }");                                                                 Out += LINE_TERMINATOR;
        }
        Out += TEXT("}");                                                                         Out += LINE_TERMINATOR;
        Out += LINE_TERMINATOR;

        Out += FString::Printf(TEXT("namespace %s"), *InShape.ClassName);                         Out += LINE_TERMINATOR;
        Out += TEXT("{");                                                                         Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("    %s Params()"), *StructName);                             Out += LINE_TERMINATOR;
        Out += TEXT("    {");                                                                     Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("        return %s();"), *StructName);                        Out += LINE_TERMINATOR;
        Out += TEXT("    }");                                                                     Out += LINE_TERMINATOR;
        if (Props.Num() > 0)
        {
            Out += LINE_TERMINATOR;
            Out += FString::Printf(TEXT("    %s Params(%s)"), *StructName, *ParamList);           Out += LINE_TERMINATOR;
            Out += TEXT("    {");                                                                 Out += LINE_TERMINATOR;
            Out += FString::Printf(TEXT("        return %s(%s);"), *StructName, *ArgList);        Out += LINE_TERMINATOR;
            Out += TEXT("    }");                                                                 Out += LINE_TERMINATOR;
        }
        Out += TEXT("}");                                                                         Out += LINE_TERMINATOR;

        // Class-level marker (re-fire dedup for source-derived injects) plus
        // per-overload end-markers carrying the SAME prefix the error-text
        // dedup gate scans — a later error-text inject for either canonical
        // overload must no-op against this block.
        Out += Get_FullShapeMarkerLine(InShape.ClassName);                                        Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("// End synthesized stub for %s::Params()"),
            *InShape.ClassName);                                                                  Out += LINE_TERMINATOR;
        if (Props.Num() > 0)
        {
            Out += FString::Printf(TEXT("// End synthesized stub for %s::Params(%s)"),
                *InShape.ClassName, *Normalize_ArgsList(FString::Join(TypeParts, TEXT(", "))));   Out += LINE_TERMINATOR;
        }

        return Out;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Inject_EntityScriptParamsStub_SourceDerived(
            const FString&           InClassName,
            const FCk_AsParsedError& InError,
            const TArray<FString>&   InCandidateFilePaths,
            const TArray<FString>&   InScanRoots,
            FCk_AsSourceScanCache*   InCache)
        -> FCk_StubInjectionResult
    {
        auto Result = FCk_StubInjectionResult{};

        const auto StructName = Derive_SpawnParamsStructName(InClassName);
        if (StructName.IsEmpty())
        {
            Result.FailReason   = ECk_StubInjectFailReason::NotApplicable;
            Result.ErrorMessage = FString::Printf(
                TEXT("'%s' is not an entity-script class name shape (expected U<X>)."), *InClassName);
            return Result;
        }

        const auto Shape = FCkAsSourceScanner::Scan_ClassShape(InClassName, InScanRoots, InCache);
        if (NOT Shape.Found)
        {
            Result.FailReason   = ECk_StubInjectFailReason::ScanFailed;
            Result.ErrorMessage = FString::Printf(
                TEXT("Source scan failed for '%s': %s"), *InClassName, *Shape.ErrorMessage);
            return Result;
        }

        auto CanonicalPath = Find_TargetFile_ByContent(InClassName, InCandidateFilePaths);
        if (CanonicalPath.IsEmpty() && NOT Shape.SourceFilePath.IsEmpty())
        {
            // The declaring file determines the owning plugin/project bucket — the
            // same attribution the real generator uses.
            CanonicalPath = Anchor_ByCallerAsPath(Shape.SourceFilePath);
        }
        if (CanonicalPath.IsEmpty())
        { CanonicalPath = Anchor_ByCallerAsPath(InError.FilePath); }
        if (CanonicalPath.IsEmpty())
        {
            Result.FailReason   = ECk_StubInjectFailReason::AnchorFailed;
            Result.ErrorMessage = FString::Printf(
                TEXT("Could not anchor full-shape stub for '%s': no candidate matched and neither the class file '%s' nor the caller '%s' has a .uplugin or .uproject ancestor."),
                *InClassName, *Shape.SourceFilePath, *InError.FilePath);
            return Result;
        }

        Result.CanonicalFilePath = CanonicalPath;

        const auto StubPath = Derive_StubSiblingPath(CanonicalPath);
        if (StubPath.IsEmpty())
        {
            Result.FailReason   = ECk_StubInjectFailReason::AnchorFailed;
            Result.ErrorMessage = FString::Printf(
                TEXT("Failed to derive stub sibling path from canonical '%s'."), *CanonicalPath);
            return Result;
        }

        auto ExistingStub = FString{};
        const auto StubFileExists = ck_angelscript_generator_stub_synthesizer::Try_ReadFile(StubPath, ExistingStub);

        const auto FullShapeAlreadyInSibling = StubFileExists
            && ExistingStub.Contains(Get_FullShapeMarkerLine(InClassName));

        if (FullShapeAlreadyInSibling)
        {
            const auto FullShapeWrittenThisProcess =
                ck_angelscript_generator_stub_synthesizer::sFullShapeClassesThisSession.Contains(InClassName);
            const auto ErrorIsStructShaped = InError.Kind != ECk_AsParsedError_Kind::NoMatchingSignatures;
            const auto SignatureNeverCompileTested = ErrorIsStructShaped || FullShapeWrittenThisProcess;

            if (SignatureNeverCompileTested)
            {
                Result.Success        = true;
                Result.TargetFilePath = StubPath;
                return Result;
            }

            Result.FailReason   = ECk_StubInjectFailReason::FullShapeOnDiskStillMismatched;
            Result.ErrorMessage = FString::Printf(
                TEXT("Full shape for '%s' already in the sibling (from a prior compile) but this Params overload is still unmatched - defer to the per-signature path."),
                *InClassName);
            return Result;
        }

        auto CanonicalContents = FString{};
        ck_angelscript_generator_stub_synthesizer::Try_ReadFile(CanonicalPath, CanonicalContents);
        if (Has_SpawnParamsStruct(CanonicalContents, StructName))
        {
            Result.FailReason   = ECk_StubInjectFailReason::StructExistsInCanonical;
            Result.ErrorMessage = FString::Printf(
                TEXT("Struct '%s' already defined in the canonical - stale-canonical drift; quarantine escalation applies."),
                *StructName);
            return Result;
        }
        if (Has_SpawnParamsStruct(ExistingStub, StructName))
        {
            Result.FailReason   = ECk_StubInjectFailReason::StructExistsInSibling;
            Result.ErrorMessage = FString::Printf(
                TEXT("Struct '%s' already defined in the sibling stub - full-shape synthesis defers to the per-signature path."),
                *StructName);
            return Result;
        }

        const auto StubBlock = Build_EntityScriptParamsStub_FullShape(Shape, InError);
        if (StubBlock.IsEmpty())
        {
            Result.FailReason   = ECk_StubInjectFailReason::NotApplicable;
            Result.ErrorMessage = FString::Printf(
                TEXT("Build_EntityScriptParamsStub_FullShape returned empty output for '%s'."), *InClassName);
            return Result;
        }

        const auto NewContents = StubFileExists
            ? ExistingStub + StubBlock
            : Get_StubFileHeader() + StubBlock;

        if (NOT ck_angelscript_generator_stub_synthesizer::Try_AtomicWrite(StubPath, NewContents))
        {
            Result.FailReason   = ECk_StubInjectFailReason::WriteFailed;
            Result.ErrorMessage = FString::Printf(
                TEXT("Atomic write failed for stub file '%s'."), *StubPath);
            return Result;
        }

        ck_angelscript_generator_stub_synthesizer::sFullShapeClassesThisSession.Add(InClassName);

        Result.Success        = true;
        Result.TargetFilePath = StubPath;
        Result.InjectedBlock  = StubBlock;
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Enumerate_EntityScriptNamespaces(
            const FString& InCanonicalContents)
        -> TArray<FString>
    {
        auto Out  = TArray<FString>{};
        auto Seen = TSet<FString>{};

        auto Lines = TArray<FString>{};
        InCanonicalContents.ParseIntoArrayLines(Lines, /*InCullEmpty=*/true);

        static const auto NamespaceKeyword = FString{TEXT("namespace ")};
        for (const auto& RawLine : Lines)
        {
            const auto Line = RawLine.TrimStartAndEnd();
            if (NOT Line.StartsWith(NamespaceKeyword))
            { continue; }

            auto Name = Line.RightChop(NamespaceKeyword.Len()).TrimStartAndEnd();
            const auto BracePos = Name.Find(TEXT("{"));
            if (BracePos != INDEX_NONE)
            { Name = Name.Left(BracePos).TrimEnd(); }

            // Corroborate with the derived struct — guards against unrelated
            // namespaces in a hand-mangled file.
            const auto StructName = Derive_SpawnParamsStructName(Name);
            if (StructName.IsEmpty() || NOT Has_SpawnParamsStruct(InCanonicalContents, StructName))
            { continue; }

            if (Seen.Contains(Name))
            { continue; }
            Seen.Add(Name);
            Out.Add(Name);
        }
        return Out;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Quarantine_And_ResynthesizeFullShapes(
            const FString&           InCanonicalPath,
            const TArray<FString>&   InSeedClassNames,
            const FCk_AsParsedError& InError,
            const TArray<FString>&   InScanRoots,
            FCk_AsSourceScanCache*   InCache)
        -> FCk_StubInjectionResult
    {
        auto Result = FCk_StubInjectionResult{};
        Result.CanonicalFilePath = InCanonicalPath;

        if (InCanonicalPath.IsEmpty())
        {
            Result.FailReason   = ECk_StubInjectFailReason::AnchorFailed;
            Result.ErrorMessage = TEXT("Quarantine requires a resolved canonical path.");
            return Result;
        }

        const auto StubPath = Derive_StubSiblingPath(InCanonicalPath);

        // ---- 1. Class union, gathered BEFORE anything is deleted ----------
        auto ClassUnion = TArray<FString>{};
        auto Seen       = TSet<FString>{};
        const auto AddClass = [&](const FString& InName) -> void
        {
            if (InName.IsEmpty() || Seen.Contains(InName))
            { return; }
            Seen.Add(InName);
            ClassUnion.Add(InName);
        };

        for (const auto& Name : InSeedClassNames)
        { AddClass(Name); }

        auto CanonicalContents = FString{};
        const auto CanonicalExists = ck_angelscript_generator_stub_synthesizer::Try_ReadFile(InCanonicalPath, CanonicalContents);
        if (CanonicalExists)
        {
            for (const auto& Name : Enumerate_EntityScriptNamespaces(CanonicalContents))
            { AddClass(Name); }
        }

        auto SiblingContents = FString{};
        const auto SiblingExists = ck_angelscript_generator_stub_synthesizer::Try_ReadFile(StubPath, SiblingContents);
        if (SiblingExists)
        {
            // Carry already-healed classes into the rebuilt sibling so the retry
            // compile doesn't regress accessors that were already covered.
            auto SiblingLines = TArray<FString>{};
            SiblingContents.ParseIntoArrayLines(SiblingLines, /*InCullEmpty=*/true);

            static const auto FullShapePrefix = FString{TEXT("// End synthesized full-shape stub for ")};
            static const auto ErrorTextPrefix = FString{TEXT("// End synthesized stub for ")};
            for (const auto& RawLine : SiblingLines)
            {
                const auto Line = RawLine.TrimStartAndEnd();
                if (Line.StartsWith(FullShapePrefix))
                {
                    AddClass(Line.RightChop(FullShapePrefix.Len()).TrimStartAndEnd());
                }
                else if (Line.StartsWith(ErrorTextPrefix))
                {
                    const auto Remainder = Line.RightChop(ErrorTextPrefix.Len());
                    const auto ScopePos  = Remainder.Find(TEXT("::"));
                    if (ScopePos != INDEX_NONE)
                    { AddClass(Remainder.Left(ScopePos).TrimStartAndEnd()); }
                }
            }
        }

        // ---- 2. Drop the sibling (may hold the wedged per-call-site stub) -
        if (SiblingExists)
        { IFileManager::Get().Delete(*StubPath, /*RequireExists=*/false, /*EvenReadOnly=*/false, /*Quiet=*/true); }

        // ---- 3. Quarantine the canonical -----------------------------------
        if (CanonicalExists)
        {
            const auto QuarantineDir = FPaths::ProjectSavedDir() / TEXT("CkSelfHeal/Quarantine");
            IFileManager::Get().MakeDirectory(*QuarantineDir, /*Tree=*/true);

            const auto Stamp        = FDateTime::UtcNow().ToString(TEXT("%Y.%m.%d-%H.%M.%S"));
            const auto ForensicPath = QuarantineDir /
                FString::Printf(TEXT("%s.stale_%s"), *FPaths::GetCleanFilename(InCanonicalPath), *Stamp);

            // The forensic copy is a courtesy — its failure must not block the
            // heal, which only needs the delete.
            if (IFileManager::Get().Copy(*ForensicPath, *InCanonicalPath) != COPY_OK)
            {
                Result.ErrorMessage = FString::Printf(
                    TEXT("Forensic copy to '%s' failed (continuing). "), *ForensicPath);
            }

            if (NOT IFileManager::Get().Delete(*InCanonicalPath, /*RequireExists=*/false, /*EvenReadOnly=*/true, /*Quiet=*/true))
            {
                Result.FailReason    = ECk_StubInjectFailReason::WriteFailed;
                Result.ErrorMessage += FString::Printf(
                    TEXT("Could not delete stale canonical '%s' - quarantine aborted (sibling was already dropped; ")
                    TEXT("the next heal cycle re-enters here)."), *InCanonicalPath);
                return Result;
            }
        }

        // ---- 4. Bulk full-shape resynthesis --------------------------------
        // Empty candidate list on purpose: the canonical is gone, so each class
        // anchors via its own declaring file.
        auto SeedFailures = TArray<FString>{};
        auto SkippedCount = 0;
        for (const auto& ClassName : ClassUnion)
        {
            const auto Injected = Inject_EntityScriptParamsStub_SourceDerived(
                ClassName, InError, /*InCandidateFilePaths=*/{}, InScanRoots, InCache);

            if (Injected.Success)
            {
                if (Result.TargetFilePath.IsEmpty())
                { Result.TargetFilePath = Injected.TargetFilePath; }
                continue;
            }

            if (InSeedClassNames.Contains(ClassName))
            {
                SeedFailures.Add(FString::Printf(TEXT("%s: %s"), *ClassName, *Injected.ErrorMessage));
            }
            else
            {
                // Non-seed scan failures are expected for deleted classes — they
                // simply drop out of the regenerated canonical.
                ++SkippedCount;
            }
        }

        if (SeedFailures.Num() > 0)
        {
            Result.FailReason    = ECk_StubInjectFailReason::ScanFailed;
            Result.ErrorMessage += FString::Printf(
                TEXT("Quarantine resynthesis failed for seed class(es): %s"),
                *FString::Join(SeedFailures, TEXT("; ")));
            return Result;
        }

        Result.Success      = true;
        Result.FailReason   = ECk_StubInjectFailReason::None;
        Result.ErrorMessage = SkippedCount > 0
            ? FString::Printf(TEXT("%d non-seed class(es) skipped (no scannable source)."), SkippedCount)
            : FString{};
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsStubSynthesizer::
        Reset_SessionState_ForTests()
        -> void
    {
        ck_angelscript_generator_stub_synthesizer::sFullShapeClassesThisSession.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
