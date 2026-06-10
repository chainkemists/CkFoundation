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

        // Canonicalizes a type token to the value-typed shape the real generator
        // emits: strips a leading "const " and a trailing "&". The AS error reports
        // the CALL SITE's argument category, not the function's declared parameter
        // — the same Params() called with a literal vs an lvalue yields "const int"
        // vs "int&" in two otherwise-identical error records. Stubs must collapse
        // those to one value-typed overload: value params accept every argument
        // category, whereas emitting one stub per reported variant produces
        // overloads that are mutually ambiguous at every call site ("Multiple
        // matching signatures" — unhealable, since the dispatcher only recognizes
        // "No matching signatures").
        auto Normalize_TypeToken(
            const FString& InTypeToken) -> FString
        {
            auto Trimmed = InTypeToken.TrimStartAndEnd();
            if (Trimmed.StartsWith(TEXT("const ")))
            { Trimmed = Trimmed.RightChop(6).TrimStart(); }
            if (Trimmed.EndsWith(TEXT("&")))
            { Trimmed = Trimmed.LeftChop(1).TrimEnd(); }
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
                Parts.Add(FString::Printf(TEXT("%s Arg%d"), *Normalize_TypeToken(Types[i]), i));
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

            // Ensure the parent directory exists. For the caller-path anchor
            // fallback (brand-new plugin / project where Script/Generated/
            // hasn't been created yet) the dir may be missing — without this,
            // SaveStringToFile fails opaquely.
            const auto ParentDir = FPaths::GetPath(InPath);
            if (NOT ParentDir.IsEmpty())
            { IFileManager::Get().MakeDirectory(*ParentDir, /*Tree=*/true); }

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
        Normalize_ArgsList(
            const FString& InArgsList)
        -> FString
    {
        const auto Types = Split_ArgTypes(InArgsList);
        auto Parts = TArray<FString>{};
        for (const auto& Type : Types)
        { Parts.Add(Normalize_TypeToken(Type)); }
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
        // Include the NORMALIZED args list in the marker so distinct overloads
        // of the same NS::FUNC name dedup independently (int vs float stay
        // separate), while call-site argument-category variants of the SAME
        // logical signature collapse to one stub: literal vs lvalue
        // ("const int" vs "int&" — the 2026-06-10 CheckoutSettle ambiguity
        // wedge) AND const-qualified duplicates that emit the same declaration
        // ("const FTransform" vs "FTransform" — the fresh-clone bulk-synthesis
        // duplicate-overload wedge). The `// Target:` line above keeps the raw
        // error string for forensics.
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

        auto CanonicalPath = Find_TargetFile_ByContent(InError.TargetNamespace, InCandidateFilePaths);
        if (CanonicalPath.IsEmpty())
        {
            // Brand-new namespace fallback: no existing file references it yet
            // (e.g. an entity-script class just authored mid-session, hot-reload
            // failing on its first Params() call site). Anchor deterministically
            // by walking up the caller's .as file path to the owning plugin or
            // project root.
            CanonicalPath = Anchor_ByCallerAsPath(InError.FilePath);
        }
        if (CanonicalPath.IsEmpty())
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Could not anchor stub for namespace '%s': no candidate file matched and caller path '%s' has no .uplugin or .uproject ancestor."),
                *InError.TargetNamespace, *InError.FilePath);
            return Result;
        }

        const auto StubPath = Derive_StubSiblingPath(CanonicalPath);
        if (StubPath.IsEmpty())
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Failed to derive stub sibling path from canonical '%s'."), *CanonicalPath);
            return Result;
        }

        // Read prior stub-file contents if any (accumulating-append). When no
        // stub file exists yet, we start with the recovery header banner.
        auto ExistingStub = FString{};
        const auto StubFileExists = Try_ReadFile(StubPath, ExistingStub);

        const auto StructName = Derive_SpawnParamsStructName(InError.TargetNamespace);
        // Emit the struct only if neither the canonical nor the in-progress
        // stub file already defines it. Accumulating drift on the same plugin
        // must not redefine the struct multiple times.
        auto CanonicalContents = FString{};
        Try_ReadFile(CanonicalPath, CanonicalContents);
        const auto CanonicalHasStruct = Has_SpawnParamsStruct(CanonicalContents, StructName);
        const auto StubHasStruct      = Has_SpawnParamsStruct(ExistingStub,      StructName);
        const auto EmitStruct         = NOT CanonicalHasStruct && NOT StubHasStruct;

        const auto StubBlock = Build_EntityScriptParamsStub(InError, EmitStruct);
        if (StubBlock.IsEmpty())
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Build_EntityScriptParamsStub returned empty output for namespace '%s' (unrecognized name shape?)."),
                *InError.TargetNamespace);
            return Result;
        }

        // Per-accessor dedup. Each appended block ends with a unique
        // "// End synthesized stub for <NS>::<FUNC>" line. If the existing
        // sibling already carries that marker, the accessor is covered — a
        // second append would produce a duplicate-function collision when AS
        // merges the namespace blocks at compile time.
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

            // Declaration-level backstop: even when the marker key differs,
            // appending a block whose EMITTED overload declaration already
            // exists in the sibling wedges the merged namespace ("A function
            // with the same name and parameters already exists"). The marker
            // canonicalization covers const-aliasing; this catches any alias
            // class it doesn't (e.g. engine type aliases). A decl line
            // embeds the struct name, so it uniquely identifies
            // (namespace, signature) within the sibling.
            const auto DeclLine = FString::Printf(TEXT("%s %s(%s)"),
                *StructName, *InError.FunctionName, *Format_ParameterList(InError.ArgsList));
            if (ExistingStub.Contains(DeclLine))
            {
                Result.Success        = true;
                Result.TargetFilePath = StubPath;
                Result.InjectedBlock  = StubBlock;
                return Result;
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

        if (NOT Try_AtomicWrite(StubPath, NewContents))
        {
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

        const auto Target = InError.Kind == ECk_AsParsedError_Kind::IdentifierNotADataType
            ? FString::Printf(TEXT("missing type '%s'"), *InError.MissingIdentifier)
            : FString::Printf(TEXT("%s::%s(%s)"),
                *InError.TargetNamespace, *InError.FunctionName, *InError.ArgsList);

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
            const TArray<FString>&   InScanRoots)
        -> FCk_StubInjectionResult
    {
        auto Result = FCk_StubInjectionResult{};

        const auto StructName = Derive_SpawnParamsStructName(InClassName);
        if (StructName.IsEmpty())
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("'%s' is not an entity-script class name shape (expected U<X>)."), *InClassName);
            return Result;
        }

        const auto Shape = FCkAsSourceScanner::Scan_ClassShape(InClassName, InScanRoots);
        if (NOT Shape.Found)
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Source scan failed for '%s': %s"), *InClassName, *Shape.ErrorMessage);
            return Result;
        }

        auto CanonicalPath = Find_TargetFile_ByContent(InClassName, InCandidateFilePaths);
        if (CanonicalPath.IsEmpty() && NOT Shape.SourceFilePath.IsEmpty())
        {
            // The class's declaring file determines the owning plugin/project
            // bucket — the same attribution the real generator uses.
            CanonicalPath = Anchor_ByCallerAsPath(Shape.SourceFilePath);
        }
        if (CanonicalPath.IsEmpty())
        { CanonicalPath = Anchor_ByCallerAsPath(InError.FilePath); }
        if (CanonicalPath.IsEmpty())
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Could not anchor full-shape stub for '%s': no candidate matched and neither the class file '%s' nor the caller '%s' has a .uplugin or .uproject ancestor."),
                *InClassName, *Shape.SourceFilePath, *InError.FilePath);
            return Result;
        }

        const auto StubPath = Derive_StubSiblingPath(CanonicalPath);
        if (StubPath.IsEmpty())
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Failed to derive stub sibling path from canonical '%s'."), *CanonicalPath);
            return Result;
        }

        auto ExistingStub = FString{};
        const auto StubFileExists = Try_ReadFile(StubPath, ExistingStub);

        // Re-fire for a class whose full shape already landed: no-op success.
        if (StubFileExists && ExistingStub.Contains(Get_FullShapeMarkerLine(InClassName)))
        {
            Result.Success        = true;
            Result.TargetFilePath = StubPath;
            return Result;
        }

        // A struct defined in the canonical (or by an earlier error-text
        // stub) means this is incremental drift, not wholesale-missing —
        // appending a second struct definition would itself wedge the
        // compile. The per-signature error-text path owns that case.
        auto CanonicalContents = FString{};
        Try_ReadFile(CanonicalPath, CanonicalContents);
        if (Has_SpawnParamsStruct(CanonicalContents, StructName) ||
            Has_SpawnParamsStruct(ExistingStub, StructName))
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Struct '%s' already defined in canonical or sibling — full-shape synthesis defers to the per-signature path."),
                *StructName);
            return Result;
        }

        const auto StubBlock = Build_EntityScriptParamsStub_FullShape(Shape, InError);
        if (StubBlock.IsEmpty())
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Build_EntityScriptParamsStub_FullShape returned empty output for '%s'."), *InClassName);
            return Result;
        }

        const auto NewContents = StubFileExists
            ? ExistingStub + StubBlock
            : Get_StubFileHeader() + StubBlock;

        if (NOT Try_AtomicWrite(StubPath, NewContents))
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Atomic write failed for stub file '%s'."), *StubPath);
            return Result;
        }

        Result.Success        = true;
        Result.TargetFilePath = StubPath;
        Result.InjectedBlock  = StubBlock;
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
