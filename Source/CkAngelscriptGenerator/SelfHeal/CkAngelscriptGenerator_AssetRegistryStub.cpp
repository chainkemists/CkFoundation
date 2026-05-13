#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AssetRegistryStub.h"

#include "CkAngelscriptGenerator/Assets/CkAssetRegistrySubsystem.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/Macros/CkMacros.h"

#include <Engine/Blueprint.h>
#include <HAL/FileManager.h>
#include <Interfaces/IPluginManager.h>
#include <Internationalization/Regex.h>
#include <Misc/App.h>
#include <Misc/DateTime.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/Paths.h>
#include <UObject/Class.h>
#include <UObject/UObjectGlobals.h>

// --------------------------------------------------------------------------------------------------------------------

// AR is deliberately NOT used in this file. At modal-tick during initial-compile-failure
// the engine is mid-startup-module-loading and AR's SearchAllAssets/GetAssetsByClass paths
// exit early on IsEngineStartupModuleLoadingComplete() == FALSE. Empirically reproduced
// 2026-05-12: the dispatcher's first wired AR call returned 0 UCkAssetRegistryConfig
// assets even after Force_FullArScan, leaving the modal wedged.
//
// AR-free strategy:
//   1. Output-file + discovery-root discovery — scan Script/Generated/*.as files
//      across project + every enabled plugin, parse each file's header line
//      `// Source config: <name> (<discovery_root> [<namespace>])`. Match the
//      error's TargetNamespace (load-suffix stripped) to one file's bracketed
//      namespace. The file path IS the output path; the parenthesized root is
//      the discovery root used to convert package path → local disk path.
//
//   2. Asset location on disk — convert discovery_root (e.g. "/Game/Raw/")
//      to a disk path (FPackageName::TryConvertGameRelativePackagePathToLocalPath
//      and friends), then IFileManager::FindFilesRecursive for <FunctionName>.uasset.
//
//   3. Class resolution via LoadObject — convert the on-disk path back to a
//      package path, call LoadObject<UObject>(nullptr, *PackagePath). LoadObject
//      uses the UObject linker directly, not AR, so it works at modal-tick for
//      any asset whose native class module has loaded by that point (Engine +
//      most game-runtime modules). For BPs, walk LoadedAsset->ParentClass via
//      UCkAssetRegistrySubsystem::Get_NonBlueprintParentClass.
//
// Tier 3 fallback (UObject stub when LoadObject can't resolve a class) is
// REFUSED for all flavors as of 2026-05-13 (see Tier3_IsAllowed for full
// rationale — probe_a2.log demonstrated the typed-conversion follow-up
// error is parser-blind and wedges the editor worse than the original).
// Refusal surfaces the original `No matching signatures` error to the user
// via Hazelight's modal; manual recovery is documented in the refusal
// banner.

namespace ck::angelscriptgenerator::self_heal
{
    namespace
    {
        // ---- Encoding-preserving UTF-16 LE atomic append ---------------------------

        // The real AssetRegistry generator writes its output via FFileHelper::SaveStringToFile
        // without an explicit encoding flag, which (for our content shape — pure ASCII with
        // UE auto-detect) lands as UTF-16 LE with BOM. To preserve byte-compatibility with
        // the existing generated files (and not confuse mtime-based hot-reload detection
        // with a mid-file encoding switch), we read the existing file as text and
        // re-write the concatenated content with ForceUnicode (= UTF-16 LE + BOM).
        // UTF-16 LE atomic write for the sibling stub file. Writes the
        // StubFileHeader-prefixed content fresh when the target does not
        // exist yet, and otherwise appends to existing stub contents. Used
        // by the sibling-file write path so that the first stub for a given
        // canonical begins with the recovery banner and subsequent stubs
        // accumulate beneath it.
        //
        // UTF-16 LE matches the real generator's `*Assets.as` encoding
        // (`FFileHelper::EEncodingOptions::ForceUnicode` produces UTF-16 LE
        // with BOM) so the sibling and canonical look encoding-identical to
        // hot-reload mtime detection and external tooling.
        auto Try_AtomicWriteOrAppend_StubFile_Utf16(
            const FString& InStubPath,
            const FString& InAppendedBlock) -> bool
        {
            auto Existing = FString{};
            const auto FileExists = FFileHelper::LoadFileToString(Existing, *InStubPath);

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

        // ---- Output-file / discovery-root discovery via file-scan ------------------

        // Returns the candidate directories to search for generated `*Assets.as`
        // files: project's Script/Generated, plus each enabled plugin's
        // Script/Generated. Mirrors the directories the real generator writes to
        // (see UCkAssetRegistrySubsystem::Get_OutputDirectoryForRootPath).
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

        // ---- Disk walk for the asset's .uasset file --------------------------------

        // Converts a package-style discovery root ("/Game/Raw/", "/Engine/...", or
        // "/<PluginName>/...") to a local disk path that we can hand to
        // IFileManager::FindFilesRecursive.
        auto Convert_PackageRootToDisk(
            const FString& InPackageRoot) -> FString
        {
            // FPackageName operates on package names ending with the asset stem;
            // for root paths we strip the trailing '/' and append a dummy stem,
            // then take the dirname. Or — simpler — use TryConvertGameRelativePackagePathToLocalPath.
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

        // Returns the package path (e.g. "/Game/Raw/SKM/MALE_SKEL_NEW.MALE_SKEL_NEW")
        // of a .uasset whose basename equals InFunctionName, located somewhere under
        // InDiscoveryRoot. Returns empty string on no match or ambiguous (multiple
        // files with same basename) result.
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

            // Convert disk path back to package path.
            auto PackageName = FString{};
            if (NOT FPackageName::TryConvertFilenameToLongPackageName(Found[0], PackageName))
            { return FString{}; }

            // FSoftObjectPath format: "<PackageName>.<AssetName>"
            return PackageName + TEXT(".") + InFunctionName;
        }

        // ---- Class resolution via LoadObject (Tier 1/2 unified) --------------------

        struct FResolvedAssetClass
        {
            FString ClassName;
            bool    IsBlueprint = false;
        };

        // LoadObject<UObject>(InPackagePath) + class walk. AR-free.
        //
        // Returns ClassName empty when:
        //   - LoadObject returns nullptr (package or class can't load at this point)
        //   - The loaded asset is a UBlueprint whose ParentClass / native parent
        //     can't be resolved (parent module not yet loaded)
        //
        // Caller falls through to Tier 3 (UObject) for soft accessors, refuses
        // for blocking-loads.
        auto Resolve_AssetClass_ViaLoadObject(
            const FString& InPackagePath) -> FResolvedAssetClass
        {
            auto Result = FResolvedAssetClass{};

            auto* Loaded = LoadObject<UObject>(nullptr, *InPackagePath);
            if (NOT Loaded)
            { return Result; }

            if (auto* AsBlueprint = Cast<UBlueprint>(Loaded))
            {
                Result.IsBlueprint = true;
                auto ParentClass = AsBlueprint->ParentClass;
                if (NOT ParentClass)
                { return Result; }

                auto* NativeParent = UCkAssetRegistrySubsystem::Get_NonBlueprintParentClass(ParentClass);
                if (NOT NativeParent)
                { return Result; }

                Result.ClassName = UCkAssetRegistrySubsystem::Get_CorrectClassNameWithPrefix(NativeParent);
                return Result;
            }

            if (auto* AssetClass = Loaded->GetClass())
            {
                auto* NativeParent = UCkAssetRegistrySubsystem::Get_NonBlueprintParentClass(AssetClass);
                if (NOT NativeParent)
                { return Result; }
                Result.ClassName = UCkAssetRegistrySubsystem::Get_CorrectClassNameWithPrefix(NativeParent);
            }
            return Result;
        }

        // ---- Tier 3 fallback policy --------------------------------------------------
        //
        // 2026-05-13 revision (probe_a2.log): Tier 3 fallback is now REFUSED
        // for ALL accessor flavors.
        //
        // The original 2026-05-12 policy permitted Tier 3 UObject stubs for
        // SoftRef/SoftClass on the assumption that the caller's typed
        // assignment would produce a "follow-up AS error pointing at the
        // right line — better diagnostic than a wedge". Probe a2 disproved
        // that: the typed-conversion error (`Cannot convert from
        // TSoftObjectPtr<UObject> to TSoftObjectPtr<UWorld>`) does NOT match
        // either of FCkAsErrorParser's two recognized patterns. Cycle 2 of
        // the dispatcher parses zero actionable roots and the editor wedges
        // indefinitely on the terminal banner instead of surfacing the
        // original `No matching signatures` error to the user.
        //
        // Refusing across the board means Hazelight's modal shows the
        // original `No matching signatures` error to the user — actionable,
        // points at the real call site, parser-blind derivatives never
        // appear.
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

    // ----------------------------------------------------------------------------------------------------------------

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

    // ----------------------------------------------------------------------------------------------------------------

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

        // The real generator emits two header lines we care about:
        //
        //   // Source config: <Name> (<...path...>.as [<Namespace>])
        //   // Discovery root: <DiscoveryRoot>
        //
        // The "Source config:" line carries the namespace cleanly inside the
        // brackets, but its path token is malformed in practice — the BB
        // generator emits "(/Game/BusterBlockBusterBlockAssets.as [assets])"
        // (missing slash between root and filename), so trying to recover the
        // root by truncating at the last '/' collapses to "/Game/" and
        // spuriously prefix-matches every other config's assets. The
        // "Discovery root:" line is canonical — use it.
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

    // ----------------------------------------------------------------------------------------------------------------

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

            // FindFilesRecursive ordering is platform-dependent — sort for
            // deterministic candidate order (matters when no asset prefix
            // wins and we fall back to first match).
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

    // ----------------------------------------------------------------------------------------------------------------

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

            // Discovery root is "/Game/Raw/" (or "/PluginName/..."). The
            // package path is "/Game/Raw/SKM/MALE_SKEL_NEW.MALE_SKEL_NEW".
            // Match the root with the trailing '/' included so "/Game/" does
            // NOT spuriously match a path under "/GameOther/".
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
        if (InError.TargetNamespace.EndsWith(TEXT("::load")))
        { return ECk_AssetAccessorFlavor::BlockingLoad; }

        if (InError.FunctionName.EndsWith(TEXT("_Class")))
        { return ECk_AssetAccessorFlavor::SoftClass; }

        return ECk_AssetAccessorFlavor::SoftRef;
    }

    // ----------------------------------------------------------------------------------------------------------------

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
        Out += FString::Printf(
            TEXT("        if (ck::EnsureIfNot(UCk_Utils_IO_UE::IsEngineSafeForBlockingLoads(), \"%s::load::%s() called before engine init. Use %s::%s() (soft ref) with UCk_DeferredConfig_UE instead.\"))"),
            *InSoftNamespace, *InFunctionName, *InSoftNamespace, *InFunctionName);                                  Out += LINE_TERMINATOR;
        Out += TEXT("        { return nullptr; }");                                                                 Out += LINE_TERMINATOR;
        Out += FString::Printf(TEXT("        return System::LoadAsset_Blocking(%s::%s());"),
            *InSoftNamespace, *InFunctionName);                                                                     Out += LINE_TERMINATOR;
        Out += TEXT("    }");
        return Out;
    }

    // ----------------------------------------------------------------------------------------------------------------

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

        // ---- Step 1: locate candidate output files matching the namespace ----

        const auto Candidates = Collect_MatchingSites(Collect_GeneratedScriptDirs(), SoftNamespace);
        if (Candidates.Num() == 0)
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("No generated *Assets.as file matched namespace '%s' (stripped from '%s'). ")
                TEXT("Searched project + plugin Script/Generated directories. ")
                TEXT("Either the namespace is unknown or the output file was never generated."),
                *SoftNamespace, *InError.TargetNamespace);
            return Result;
        }

        // ---- Step 2: locate the asset .uasset on disk, picking the candidate
        //              whose DiscoveryRoot owns it ----
        //
        // Multiple files can legitimately share `namespace assets` while
        // covering disjoint discovery roots (e.g. BusterBlockAssets.as on
        // "/Game/BusterBlock/" + RawAssets.as on "/Game/Raw/"). AS merges the
        // namespace at compile time, so any of the files would *unwedge* the
        // editor — but the stub must land in the file the asset actually
        // belongs to, otherwise the deferred regen has to rewrite both files
        // to clean up. Try each candidate's root; the one that finds the asset
        // on disk is the owner. Fall back to the first candidate if no root
        // owns it (asset can't be located on disk — Tier 3 territory below).
        const auto BaseFunctionName = (Flavor == ECk_AssetAccessorFlavor::SoftClass)
            ? Strip_ClassSuffix(InError.FunctionName)
            : InError.FunctionName;

        auto AssetPackagePath = FString{};
        auto Site             = FCk_AssetConfigSiteInfo{};
        for (const auto& Candidate : Candidates)
        {
            const auto Found = Find_AssetPackagePath_OnDisk(Candidate.DiscoveryRoot, BaseFunctionName);
            if (Found.IsEmpty())
            { continue; }

            AssetPackagePath = Found;
            Site             = Candidate;
            break;
        }

        if (Site.OutputPath.IsEmpty())
        {
            // No candidate's root contained the asset on disk. Fall back to
            // the first candidate so the stub still lands somewhere — AS
            // namespace-merge means compile will succeed regardless, and the
            // deferred regen will fix the location. Tier 3 fallback below
            // decides whether to emit a UObject stub or refuse.
            Site = Candidates[0];
        }
        if (AssetPackagePath.IsEmpty())
        {
            // Tier 3 fallback refused for all flavors (see Tier3_IsAllowed
            // policy comment for rationale). Surface the actionable banner so
            // the user can run a manual AR regen; Hazelight's modal will keep
            // displaying the original `No matching signatures` error which is
            // exactly what they need to see.
            if (NOT Tier3_IsAllowed(Flavor))
            {
                Result.ErrorMessage = FString::Printf(
                    TEXT("Asset '%s.uasset' not found under disk-converted root for '%s'. ")
                    TEXT("Tier 3 UObject fallback is disabled (would produce a parser-blind ")
                    TEXT("typed-conversion error and wedge the editor). ")
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
            const auto Resolved = Resolve_AssetClass_ViaLoadObject(AssetPackagePath);
            ClassName = Resolved.ClassName;
        }

        if (ClassName.IsEmpty())
        {
            // LoadObject couldn't resolve (asset not loaded, BP parent module
            // not up yet, etc.). Tier 3 fallback is refused for all flavors
            // (see Tier3_IsAllowed) — surface the actionable banner so the
            // original `No matching signatures` error remains visible to the
            // user instead of being replaced by a parser-blind
            // typed-conversion error.
            if (NOT Tier3_IsAllowed(Flavor))
            {
                Result.ResolvedAssetClass = TEXT("UObject");
                Result.ErrorMessage = FString::Printf(
                    TEXT("Could not resolve UClass via LoadObject for '%s' (package path '%s'). ")
                    TEXT("Tier 3 UObject fallback is disabled (would produce a parser-blind ")
                    TEXT("typed-conversion error and wedge the editor). ")
                    TEXT("Manual recovery: force-quit, comment out the failing %s::%s() call site, ")
                    TEXT("relaunch, click 'Generate All Asset Registries', uncomment, save."),
                    *InError.FunctionName, *AssetPackagePath,
                    *InError.TargetNamespace, *InError.FunctionName);
                return Result;
            }
        }
        Result.ResolvedAssetClass = ClassName;

        // ---- Step 4: build the function body for the matched flavor ----

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
        }

        if (FunctionBody.IsEmpty())
        {
            Result.ErrorMessage = TEXT("Internal: function body construction returned empty.");
            return Result;
        }

        const auto StubBlock = Build_NamespaceBlock(InError.TargetNamespace, FunctionBody, InError);

        // ---- Step 5: write the stub to a sibling file (not the canonical) ----

        const auto StubPath = FCkAsAssetRegistryStubSynthesizer::Derive_StubSiblingPath(Site.OutputPath);
        if (StubPath.IsEmpty())
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Failed to derive stub sibling path from canonical '%s'."), *Site.OutputPath);
            return Result;
        }

        if (NOT Try_AtomicWriteOrAppend_StubFile_Utf16(StubPath, StubBlock))
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
