#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AssetRegistryStub.h"

#include "CkAngelscriptGenerator/Assets/CkAssetRegistry_ClassResolver.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/Macros/CkMacros.h"

#include <HAL/FileManager.h>
#include <Interfaces/IPluginManager.h>
#include <Internationalization/Regex.h>
#include <Misc/DateTime.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/Paths.h>

// --------------------------------------------------------------------------------------------------------------------

// AssetRegistry's bulk-scan paths are deliberately NOT used for discovery here:
// this runs mid-startup-module-loading, where they exit early and return nothing.
// Class resolution MUST stay on the same tiers as the canonical generator, or
// canonical-vs-self-heal divergence triggers a synth-cleanup loop.

namespace ck::angelscriptgenerator::self_heal
{
    namespace ck_angelscript_generator_asset_registry_stub
    {
        // `ForceUnicode` (UTF-16 LE + BOM) matches the real generator's encoding,
        // so the sibling looks encoding-identical to hot-reload detection and
        // external tooling. First write prepends the banner; later ones append.
        auto Try_AtomicWriteOrAppend_StubFile_Utf16(
            const FString& InStubPath,
            const FString& InAppendedBlock) -> bool
        {
            auto Existing = FString{};
            const auto FileExists = FFileHelper::LoadFileToString(Existing, *InStubPath);

            // Per-accessor dedup on the block's unique end-marker line: a second
            // append would collide as a duplicate function once AS merges the
            // namespace blocks at compile time.
            if (FileExists)
            {
                static const auto EndMarkerPrefix = FString{TEXT("// End synthesized stub for ")};
                const auto MarkerPos = InAppendedBlock.Find(EndMarkerPrefix);
                if (MarkerPos != INDEX_NONE)
                {
                    const auto LineEndPos = InAppendedBlock.Find(
                        FString{LINE_TERMINATOR}, ESearchCase::CaseSensitive,
                        ESearchDir::FromStart, MarkerPos);
                    const auto MarkerLine = LineEndPos != INDEX_NONE
                        ? InAppendedBlock.Mid(MarkerPos, LineEndPos - MarkerPos)
                        : InAppendedBlock.Mid(MarkerPos);

                    if (Existing.Contains(MarkerLine))
                    { return true; }
                }
            }

            const auto NewContents = FileExists
                ? (Existing + InAppendedBlock)
                : (FCkAsAssetRegistryStubSynthesizer::Get_StubFileHeader() + InAppendedBlock);

            const auto TempPath = InStubPath + TEXT(".asstubtmp");
            IFileManager::Get().Delete(*TempPath, /*RequireExists=*/false, /*EvenReadOnly=*/false, /*Quiet=*/true);

            if (NOT FFileHelper::SaveStringToFile(NewContents, *TempPath,
                FFileHelper::EEncodingOptions::ForceUnicode))
            { return false; }

            return IFileManager::Get().Move(*InStubPath, *TempPath, /*Replace=*/true);
        }

        auto Collect_GeneratedScriptDirs() -> TArray<FString>
        {
            auto Dirs = TArray<FString>{};
            Dirs.Add(FPaths::ProjectDir() / TEXT("Script") / TEXT("Generated"));

            for (const auto& Plugin : IPluginManager::Get().GetEnabledPlugins())
            {
                Dirs.Add(Plugin->GetBaseDir() / TEXT("Script") / TEXT("Generated"));
            }
            return Dirs;
        }

        // FPackageName needs the package name WITHOUT a trailing slash; the
        // slash is stripped here and re-added on the converted side.
        auto Convert_PackageRootToDisk(
            const FString& InPackageRoot) -> FString
        {
            auto LocalPath = FString{};
            if (FPackageName::TryConvertLongPackageNameToFilename(
                InPackageRoot.LeftChop(InPackageRoot.EndsWith(TEXT("/")) ? 1 : 0),
                LocalPath))
            {
                if (NOT LocalPath.EndsWith(TEXT("/")) && NOT LocalPath.EndsWith(TEXT("\\")))
                { LocalPath += TEXT("/"); }
                return LocalPath;
            }
            return FString{};
        }

        // FSoftObjectPath-formatted package path of the `<InFunctionName>.uasset`
        // under InDiscoveryRoot. Empty on no match OR on ambiguity.
        auto Find_AssetPackagePath_OnDisk(
            const FString& InDiscoveryRoot,
            const FString& InFunctionName) -> FString
        {
            const auto DiskRoot = Convert_PackageRootToDisk(InDiscoveryRoot);
            if (DiskRoot.IsEmpty() || NOT IFileManager::Get().DirectoryExists(*DiskRoot))
            { return FString{}; }

            const auto FilenameFilter = InFunctionName + TEXT(".uasset");

            auto Found = TArray<FString>{};
            IFileManager::Get().FindFilesRecursive(Found, *DiskRoot, *FilenameFilter,
                /*Files=*/true, /*Directories=*/false);

            if (Found.Num() != 1)
            { return FString{}; }

            auto PackageName = FString{};
            if (NOT FPackageName::TryConvertFilenameToLongPackageName(Found[0], PackageName))
            { return FString{}; }

            return PackageName + TEXT(".") + InFunctionName;
        }

        // A UObject stub for an unresolvable class is refused for EVERY flavor:
        // the typed-conversion error it provokes matches no parser pattern, so
        // the next cycle finds zero roots and wedges. Refusing keeps Hazelight's
        // original, actionable error on screen. Rationale: the module CLAUDE.md.
        auto Tier3_IsAllowed(
            ECk_AssetAccessorFlavor /*InFlavor*/) -> bool
        {
            return false;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Get_MarkerComment()
        -> FString
    {
        return FString{TEXT("// CkAngelscriptGenerator: synthesized AssetRegistry stub for emergency recovery; will be replaced on next clean compile.")};
    }

    auto
        FCkAsAssetRegistryStubSynthesizer::
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

    auto
        FCkAsAssetRegistryStubSynthesizer::
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

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Try_ParseConfigSiteHeader(
            const FString&            InFilePath,
            FCk_AssetConfigSiteInfo&  OutSite,
            FString&                  OutNamespace)
        -> bool
    {
        auto Contents = FString{};
        if (NOT FFileHelper::LoadFileToString(Contents, *InFilePath))
        { return false; }

        // The namespace comes from `// Source config:`'s bracketed token, but
        // that line's PATH token is malformed — the root is read from
        // `// Discovery root:` instead.
        static const auto NamespacePattern = FRegexPattern{TEXT(
            R"(^//\s*Source config:\s*[^\[]*\[([^\]]+)\])")};
        static const auto DiscoveryRootPattern = FRegexPattern{TEXT(
            R"(^//\s*Discovery root:\s*(\S+))")};

        auto Lines = TArray<FString>{};
        Contents.ParseIntoArrayLines(Lines, /*InCullEmpty=*/false);

        auto Namespace = FString{};
        auto Root      = FString{};

        for (auto i = 0; i < FMath::Min(Lines.Num(), 10); ++i)
        {
            if (Namespace.IsEmpty())
            {
                auto M = FRegexMatcher{NamespacePattern, Lines[i]};
                if (M.FindNext())
                { Namespace = M.GetCaptureGroup(1); }
            }
            if (Root.IsEmpty())
            {
                auto M = FRegexMatcher{DiscoveryRootPattern, Lines[i]};
                if (M.FindNext())
                { Root = M.GetCaptureGroup(1); }
            }
            if (NOT Namespace.IsEmpty() && NOT Root.IsEmpty())
            { break; }
        }

        if (Namespace.IsEmpty() || Root.IsEmpty())
        { return false; }

        if (NOT Root.EndsWith(TEXT("/")))
        { Root += TEXT("/"); }

        OutSite.OutputPath    = InFilePath;
        OutSite.DiscoveryRoot = Root;
        OutNamespace          = Namespace;
        return true;
    }

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Collect_MatchingSites(
            const TArray<FString>& InDirs,
            const FString&         InNamespace)
        -> TArray<FCk_AssetConfigSiteInfo>
    {
        auto Out = TArray<FCk_AssetConfigSiteInfo>{};
        for (const auto& Dir : InDirs)
        {
            if (NOT IFileManager::Get().DirectoryExists(*Dir))
            { continue; }

            auto Files = TArray<FString>{};
            IFileManager::Get().FindFilesRecursive(Files, *Dir, TEXT("*Assets.as"),
                /*Files=*/true, /*Directories=*/false);

            // Deterministic order matters: no-prefix-wins falls back to first match.
            Files.Sort();

            for (const auto& File : Files)
            {
                auto Site = FCk_AssetConfigSiteInfo{};
                auto Ns   = FString{};
                if (NOT Try_ParseConfigSiteHeader(File, Site, Ns))
                { continue; }

                if (Ns != InNamespace)
                { continue; }

                Out.Add(Site);
            }
        }
        return Out;
    }

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Pick_BestSite_ByAssetPath(
            const TArray<FCk_AssetConfigSiteInfo>& InCandidates,
            const FString&                         InAssetPackagePath)
        -> int32
    {
        if (InAssetPackagePath.IsEmpty())
        { return INDEX_NONE; }

        auto BestIndex     = int32{INDEX_NONE};
        auto BestRootLen   = int32{0};

        for (auto i = 0; i < InCandidates.Num(); ++i)
        {
            const auto& Root = InCandidates[i].DiscoveryRoot;
            if (Root.IsEmpty())
            { continue; }

            // Root keeps its trailing '/' so "/Game/" can't match "/GameOther/".
            if (NOT InAssetPackagePath.StartsWith(Root, ESearchCase::IgnoreCase))
            { continue; }

            if (Root.Len() > BestRootLen)
            {
                BestRootLen = Root.Len();
                BestIndex   = i;
            }
        }
        return BestIndex;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Classify_AccessorFlavor(
            const FCk_AsParsedError& InError)
        -> ECk_AssetAccessorFlavor
    {
        // The conjunction must be tested FIRST — otherwise the `_Class` strip
        // never fires and the disk walk looks for `<X>_Class.uasset` literally.
        const auto IsLoad  = InError.TargetNamespace.EndsWith(TEXT("::load"));
        const auto IsClass = InError.FunctionName.EndsWith(TEXT("_Class"));

        if (IsLoad && IsClass) { return ECk_AssetAccessorFlavor::BlockingLoadClass; }
        if (IsLoad)            { return ECk_AssetAccessorFlavor::BlockingLoad; }
        if (IsClass)           { return ECk_AssetAccessorFlavor::SoftClass; }
        return ECk_AssetAccessorFlavor::SoftRef;
    }

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Strip_LoadSuffix(
            const FString& InNamespace)
        -> FString
    {
        const auto Suffix = FString{TEXT("::load")};
        if (InNamespace.EndsWith(Suffix))
        { return InNamespace.LeftChop(Suffix.Len()); }
        return InNamespace;
    }

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Strip_ClassSuffix(
            const FString& InFunctionName)
        -> FString
    {
        const auto Suffix = FString{TEXT("_Class")};
        if (InFunctionName.EndsWith(Suffix))
        { return InFunctionName.LeftChop(Suffix.Len()); }
        return InFunctionName;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Build_SoftRefAccessor(
            const FString& InFunctionName,
            const FString& InResolvedClassName,
            const FString& InAssetPath)
        -> FString
    {
        return FString::Printf(
            TEXT("    TSoftObjectPtr<%s> %s() { return TSoftObjectPtr<%s>(FSoftObjectPath(\"%s\")); }"),
            *InResolvedClassName, *InFunctionName, *InResolvedClassName, *InAssetPath);
    }

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Build_SoftClassAccessor(
            const FString& InFunctionName,
            const FString& InResolvedClassName,
            const FString& InAssetPath)
        -> FString
    {
        const auto ClassPath = InAssetPath + TEXT("_C");
        return FString::Printf(
            TEXT("    TSoftClassPtr<%s> %s() { return TSoftClassPtr<%s>(FSoftObjectPath(\"%s\")); }"),
            *InResolvedClassName, *InFunctionName, *InResolvedClassName, *ClassPath);
    }

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Build_BlockingLoadAccessor(
            const FString& InFunctionName,
            const FString& InResolvedClassName,
            const FString& InSoftNamespace)
        -> FString
    {
        auto Out = FString{};
        Out += FString::Printf(TEXT("    %s %s()"), *InResolvedClassName, *InFunctionName);                        Out += LINE_TERMINATOR;
        Out += TEXT("    {");                                                                                       Out += LINE_TERMINATOR;
        Out += TEXT("        if (UCk_Utils_IO_UE::IsEngineSafeForBlockingLoads() == false)");                       Out += LINE_TERMINATOR;
        Out += TEXT("        {");                                                                                   Out += LINE_TERMINATOR;
        Out += FString::Printf(
            TEXT("            ck::EnsureIfNot_PrematureAssetLoad(UCk_Utils_IO_UE::Get_IsRunningCommandlet(), \"%s::load::%s() called before engine init. Use %s::%s() (soft ref) with UCk_DeferredConfig_UE instead.\");"),
            *InSoftNamespace, *InFunctionName, *InSoftNamespace, *InFunctionName);                                  Out += LINE_TERMINATOR;
        Out += TEXT("            return nullptr;");                                                                 Out += LINE_TERMINATOR;
        Out += TEXT("        }");                                                                                   Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("        return System::LoadAsset_Blocking(%s::%s());"),
            *InSoftNamespace, *InFunctionName);                                                                     Out += LINE_TERMINATOR;
        Out += TEXT("    }");
        return Out;
    }

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Build_BlockingLoadClassAccessor(
            const FString& InFunctionName,
            const FString& InResolvedClassName,
            const FString& InSoftNamespace)
        -> FString
    {
        auto Out = FString{};
        Out += FString::Printf(TEXT("    TSubclassOf<%s> %s()"), *InResolvedClassName, *InFunctionName);            Out += LINE_TERMINATOR;
        Out += TEXT("    {");                                                                                       Out += LINE_TERMINATOR;
        Out += TEXT("        if (UCk_Utils_IO_UE::IsEngineSafeForBlockingLoads() == false)");                       Out += LINE_TERMINATOR;
        Out += TEXT("        {");                                                                                   Out += LINE_TERMINATOR;
        Out += FString::Printf(
            TEXT("            ck::EnsureIfNot_PrematureAssetLoad(UCk_Utils_IO_UE::Get_IsRunningCommandlet(), \"%s::load::%s() called before engine init. Use %s::%s() (soft ref) with UCk_DeferredConfig_UE instead.\");"),
            *InSoftNamespace, *InFunctionName, *InSoftNamespace, *InFunctionName);                                  Out += LINE_TERMINATOR;
        Out += TEXT("            return nullptr;");                                                                 Out += LINE_TERMINATOR;
        Out += TEXT("        }");                                                                                   Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("        return System::LoadClassAsset_Blocking(%s::%s());"),
            *InSoftNamespace, *InFunctionName);                                                                     Out += LINE_TERMINATOR;
        Out += TEXT("    }");
        return Out;
    }

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Build_NamespaceBlock(
            const FString&           InNamespace,
            const FString&           InFunctionBody,
            const FCk_AsParsedError& InError)
        -> FString
    {
        auto Out = FString{};
        Out += LINE_TERMINATOR;
        Out += Get_MarkerComment();                                                                                 Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("// Target: %s::%s(%s)"),
            *InError.TargetNamespace, *InError.FunctionName, *InError.ArgsList);                                    Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("// Triggering site: %s:%d:%d"),
            *InError.FilePath, InError.Line, InError.Column);                                                       Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("namespace %s"), *InNamespace);                                                 Out += LINE_TERMINATOR;
        Out += TEXT("{");                                                                                           Out += LINE_TERMINATOR;
        Out += InFunctionBody;                                                                                      Out += LINE_TERMINATOR;
        Out += TEXT("}");                                                                                           Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("// End synthesized stub for %s::%s"),
            *InError.TargetNamespace, *InError.FunctionName);                                                       Out += LINE_TERMINATOR;
        return Out;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Inject_AssetRegistryStub(
            const FCk_AsParsedError& InError)
        -> FCk_AssetStubInjectionResult
    {
        auto Result = FCk_AssetStubInjectionResult{};

        if (InError.Kind != ECk_AsParsedError_Kind::NoMatchingSignatures)
        {
            Result.ErrorMessage = TEXT("AssetRegistry synthesizer only handles NoMatchingSignatures errors.");
            return Result;
        }

        const auto Flavor        = Classify_AccessorFlavor(InError);
        const auto SoftNamespace = Strip_LoadSuffix(InError.TargetNamespace);

        // ---- Step 1: candidates matching the namespace ----

        const auto Candidates = Collect_MatchingSites(ck_angelscript_generator_asset_registry_stub::Collect_GeneratedScriptDirs(), SoftNamespace);
        if (Candidates.Num() == 0)
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("No generated *Assets.as file matched namespace '%s' (stripped from '%s'). ")
                TEXT("Either the namespace is unknown or the output file was never generated."),
                *SoftNamespace, *InError.TargetNamespace);
            return Result;
        }

        // ---- Step 2: locate the asset on disk, under the candidate that owns it ----
        //
        // Several files legitimately share `namespace assets` over disjoint roots.
        // AS merges them, so any file would unwedge — but the stub must land in
        // the OWNING file or the deferred regen has to rewrite both.
        const auto NeedsClassStrip = (Flavor == ECk_AssetAccessorFlavor::SoftClass)
                                  || (Flavor == ECk_AssetAccessorFlavor::BlockingLoadClass);
        const auto BaseFunctionName = NeedsClassStrip
            ? Strip_ClassSuffix(InError.FunctionName)
            : InError.FunctionName;

        auto AssetPackagePath = FString{};
        auto Site             = FCk_AssetConfigSiteInfo{};
        for (const auto& Candidate : Candidates)
        {
            const auto Found = ck_angelscript_generator_asset_registry_stub::Find_AssetPackagePath_OnDisk(Candidate.DiscoveryRoot, BaseFunctionName);
            if (Found.IsEmpty())
            { continue; }

            AssetPackagePath = Found;
            Site             = Candidate;
            break;
        }

        if (Site.OutputPath.IsEmpty())
        { Site = Candidates[0]; }

        if (AssetPackagePath.IsEmpty())
        {
            if (NOT ck_angelscript_generator_asset_registry_stub::Tier3_IsAllowed(Flavor))
            {
                Result.ErrorMessage = FString::Printf(
                    TEXT("Asset '%s.uasset' not found under disk-converted root for '%s'. ")
                    TEXT("Tier 3 UObject fallback is disabled (produces parser-blind typed-conversion error). ")
                    TEXT("Manual recovery: force-quit, comment out the failing %s::%s() call site, ")
                    TEXT("relaunch, click 'Generate All Asset Registries', uncomment, save."),
                    *BaseFunctionName, *Site.DiscoveryRoot,
                    *InError.TargetNamespace, *InError.FunctionName);
                return Result;
            }
        }
        Result.ResolvedAssetPath = AssetPackagePath;

        // ---- Step 3: resolve UClass via LoadObject ----

        auto ClassName = FString{};
        if (NOT AssetPackagePath.IsEmpty())
        {
            const auto Resolved = FCkAssetRegistry_ClassResolver::Resolve_ViaLoadObject(AssetPackagePath);
            ClassName = Resolved.ClassName;
        }

        // Tier 2.5 — usable only when AR's NativeParentClass/ParentClass tags are
        // populated; they cache as "None" if AR scanned while the chain was wedged.
        if (ClassName.IsEmpty() && NOT AssetPackagePath.IsEmpty())
        {
            const auto Resolved = FCkAssetRegistry_ClassResolver::Resolve_ViaAssetDataTag(AssetPackagePath);
            ClassName = Resolved.ClassName;
        }

        // Tier 2.6 — reads the .uasset linker tables directly, bypassing AR's
        // poisoned cache. Resolves AS-parented WBPs, since AS UClasses are
        // registered at parse time even when the link fails.
        if (ClassName.IsEmpty() && NOT AssetPackagePath.IsEmpty())
        {
            const auto Resolved = FCkAssetRegistry_ClassResolver::Resolve_ViaPackageReader(AssetPackagePath);
            ClassName = Resolved.ClassName;
        }

        if (ClassName.IsEmpty())
        {
            if (NOT ck_angelscript_generator_asset_registry_stub::Tier3_IsAllowed(Flavor))
            {
                Result.ResolvedAssetClass = TEXT("UObject");
                Result.ErrorMessage = FString::Printf(
                    TEXT("Could not resolve UClass for '%s' (package path '%s') via LoadObject ")
                    TEXT("(Tier 2), AssetData tags (Tier 2.5), or .uasset linker walk (Tier 2.6). ")
                    TEXT("Tier 3 UObject fallback is disabled (produces parser-blind typed-conversion error). ")
                    TEXT("Manual recovery: force-quit, comment out the failing %s::%s() call site, ")
                    TEXT("relaunch, click 'Generate All Asset Registries', uncomment, save."),
                    *InError.FunctionName, *AssetPackagePath,
                    *InError.TargetNamespace, *InError.FunctionName);
                return Result;
            }
        }
        Result.ResolvedAssetClass = ClassName;

        // ---- Step 4: build function body ----

        auto FunctionBody = FString{};
        switch (Flavor)
        {
            case ECk_AssetAccessorFlavor::SoftRef:
                FunctionBody = Build_SoftRefAccessor(InError.FunctionName, ClassName, AssetPackagePath);
                break;

            case ECk_AssetAccessorFlavor::SoftClass:
                FunctionBody = Build_SoftClassAccessor(InError.FunctionName, ClassName, AssetPackagePath);
                break;

            case ECk_AssetAccessorFlavor::BlockingLoad:
                FunctionBody = Build_BlockingLoadAccessor(InError.FunctionName, ClassName, SoftNamespace);
                break;

            case ECk_AssetAccessorFlavor::BlockingLoadClass:
                FunctionBody = Build_BlockingLoadClassAccessor(InError.FunctionName, ClassName, SoftNamespace);
                break;
        }

        if (FunctionBody.IsEmpty())
        {
            Result.ErrorMessage = TEXT("Internal: function body construction returned empty.");
            return Result;
        }

        const auto StubBlock = Build_NamespaceBlock(InError.TargetNamespace, FunctionBody, InError);

        // ---- Step 5: write to sibling file (canonical untouched) ----

        const auto StubPath = FCkAsAssetRegistryStubSynthesizer::Derive_StubSiblingPath(Site.OutputPath);
        if (StubPath.IsEmpty())
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Failed to derive stub sibling path from canonical '%s'."), *Site.OutputPath);
            return Result;
        }

        if (NOT ck_angelscript_generator_asset_registry_stub::Try_AtomicWriteOrAppend_StubFile_Utf16(StubPath, StubBlock))
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("UTF-16 atomic write/append failed for stub file '%s'."), *StubPath);
            return Result;
        }

        Result.Success        = true;
        Result.TargetFilePath = StubPath;
        Result.InjectedBlock  = StubBlock;

        // ---- Step 6: touch caller mtime to nudge hot-reload thread ----

        if (NOT InError.FilePath.IsEmpty()
            && IFileManager::Get().FileExists(*InError.FilePath))
        {
            IFileManager::Get().SetTimeStamp(*InError.FilePath, FDateTime::UtcNow());
        }

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
