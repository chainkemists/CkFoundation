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
// Tier 3 fallback (UObject for soft accessors, refusal for blocking-loads) is
// preserved as the policy-gated escape hatch when LoadObject can't resolve a
// class — typically a BP whose native parent module loads later than modal-tick.

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
        auto Try_AtomicAppendUtf16(
            const FString& InTargetPath,
            const FString& InAppendedBlock) -> bool
        {
            auto Existing = FString{};
            if (NOT FFileHelper::LoadFileToString(Existing, *InTargetPath))
            { return false; }

            const auto NewContents = Existing + InAppendedBlock;

            const auto TempPath = InTargetPath + TEXT(".asstubtmp");
            IFileManager::Get().Delete(*TempPath, /*RequireExists=*/false, /*EvenReadOnly=*/false, /*Quiet=*/true);

            if (NOT FFileHelper::SaveStringToFile(NewContents, *TempPath,
                FFileHelper::EEncodingOptions::ForceUnicode))
            { return false; }

            return IFileManager::Get().Move(*InTargetPath, *TempPath, /*Replace=*/true);
        }

        // ---- Output-file / discovery-root discovery via file-scan ------------------

        struct FConfigSiteInfo
        {
            FString OutputPath;      // absolute path to the matched <Plugin>_Assets.as file
            FString DiscoveryRoot;   // package-style discovery root, e.g. "/Game/Raw/"
        };

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

        // Parses a generated `*Assets.as` file's header line in the shape:
        //   // Source config: RawAssets (/Game/Raw/RawAssets.as [assets])
        // Extracts (DiscoveryRoot, Namespace). Returns false if the header
        // isn't found within the first few lines.
        //
        // The header isn't load-bearing in normal operation, but is the only
        // ground-truth mapping we can read at modal-tick without AR. The real
        // generator emits it consistently — if a future change breaks the
        // format, the snapshot tests under Test_AssetRegistryStub.cpp will
        // catch it.
        auto Try_ParseAssetsFileHeader(
            const FString& InFilePath,
            FString&       OutDiscoveryRoot,
            FString&       OutNamespace) -> bool
        {
            auto Contents = FString{};
            if (NOT FFileHelper::LoadFileToString(Contents, *InFilePath))
            { return false; }

            // Only look at the prologue — the header is in the first ~5 lines.
            // Pattern: "// Source config: <Name> (<DiscoveryRoot>... [<Namespace>])"
            static const auto Pattern = FRegexPattern{TEXT(
                R"(^//\s*Source config:\s*[^(]*\(([^\s]+)[^\[]*\[([^\]]+)\])")};

            auto Lines = TArray<FString>{};
            Contents.ParseIntoArrayLines(Lines, /*InCullEmpty=*/false);

            for (auto i = 0; i < FMath::Min(Lines.Num(), 10); ++i)
            {
                auto Matcher = FRegexMatcher{Pattern, Lines[i]};
                if (Matcher.FindNext())
                {
                    OutDiscoveryRoot = Matcher.GetCaptureGroup(1);
                    OutNamespace     = Matcher.GetCaptureGroup(2);

                    // Strip a stray ".as" appended to the path token (the
                    // generator writes "(/Game/Raw/RawAssets.as [assets])");
                    // we want "/Game/Raw/" as the discovery root, not the
                    // .as filename. Truncate at the last '/' if there is one.
                    if (OutDiscoveryRoot.EndsWith(TEXT(".as")))
                    {
                        auto LastSlash = int32{INDEX_NONE};
                        OutDiscoveryRoot.FindLastChar(TEXT('/'), LastSlash);
                        if (LastSlash != INDEX_NONE)
                        { OutDiscoveryRoot = OutDiscoveryRoot.Left(LastSlash + 1); }
                    }
                    if (NOT OutDiscoveryRoot.EndsWith(TEXT("/")))
                    { OutDiscoveryRoot += TEXT("/"); }

                    return true;
                }
            }
            return false;
        }

        // Walks the candidate Script/Generated directories, parses each .as
        // file's header, and returns the one whose namespace matches.
        // Returns IsValid=false when no match found.
        auto Find_OutputSite_ByNamespace(
            const FString& InNamespace) -> FConfigSiteInfo
        {
            for (const auto& Dir : Collect_GeneratedScriptDirs())
            {
                if (NOT IFileManager::Get().DirectoryExists(*Dir))
                { continue; }

                auto Files = TArray<FString>{};
                IFileManager::Get().FindFilesRecursive(Files, *Dir, TEXT("*Assets.as"), /*Files=*/true, /*Directories=*/false);

                for (const auto& File : Files)
                {
                    auto Root      = FString{};
                    auto Namespace = FString{};
                    if (Try_ParseAssetsFileHeader(File, Root, Namespace)
                        && Namespace == InNamespace)
                    {
                        auto Site          = FConfigSiteInfo{};
                        Site.OutputPath    = File;
                        Site.DiscoveryRoot = Root;
                        return Site;
                    }
                }
            }
            return FConfigSiteInfo{};
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

        // ---- Tier 3 fallback policy (option C from CTO conversation) ---------------

        // Returns true if the given flavor permits the UObject fallback.
        // Per CTO 2026-05-12: soft refs degrade gracefully (typed assignment
        // produces a follow-up AS error pointing at the right line — better
        // diagnostic than a wedge), blocking loads do not (default-constructing
        // the asset would crash worse than the wedge).
        auto Tier3_IsAllowed(
            ECk_AssetAccessorFlavor InFlavor) -> bool
        {
            switch (InFlavor)
            {
                case ECk_AssetAccessorFlavor::SoftRef:
                case ECk_AssetAccessorFlavor::SoftClass:
                    return true;
                case ECk_AssetAccessorFlavor::BlockingLoad:
                    return false;
            }
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

        // ---- Step 1: locate the output file + discovery root via file-scan ----

        const auto Site = Find_OutputSite_ByNamespace(SoftNamespace);
        if (Site.OutputPath.IsEmpty())
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("No generated *Assets.as file matched namespace '%s' (stripped from '%s'). ")
                TEXT("Searched project + plugin Script/Generated directories. ")
                TEXT("Either the namespace is unknown or the output file was never generated."),
                *SoftNamespace, *InError.TargetNamespace);
            return Result;
        }

        // ---- Step 2: locate the asset .uasset on disk ----

        const auto BaseFunctionName = (Flavor == ECk_AssetAccessorFlavor::SoftClass)
            ? Strip_ClassSuffix(InError.FunctionName)
            : InError.FunctionName;

        const auto AssetPackagePath = Find_AssetPackagePath_OnDisk(Site.DiscoveryRoot, BaseFunctionName);
        if (AssetPackagePath.IsEmpty())
        {
            // We can still attempt Tier 3 fallback with an empty FSoftObjectPath
            // for soft accessors — the stub satisfies AS compile (caller's typed
            // assignment still fails). For blocking loads we refuse.
            if (NOT Tier3_IsAllowed(Flavor))
            {
                Result.ErrorMessage = FString::Printf(
                    TEXT("Asset '%s.uasset' not found under disk-converted root for '%s'. ")
                    TEXT("Tier 3 fallback not permitted for blocking-load accessors. ")
                    TEXT("Manual recovery: force-quit, comment out the failing %s::%s() call site, ")
                    TEXT("relaunch, click 'Generate All Asset Registries', uncomment, save."),
                    *BaseFunctionName, *Site.DiscoveryRoot,
                    *InError.TargetNamespace, *InError.FunctionName);
                return Result;
            }

            Result.UsedTier3Fallback = true;
        }
        Result.ResolvedAssetPath = AssetPackagePath; // empty when Tier 3 with no on-disk hit

        // ---- Step 3: resolve UClass via LoadObject ----

        auto ClassName = FString{};
        if (NOT AssetPackagePath.IsEmpty())
        {
            const auto Resolved = Resolve_AssetClass_ViaLoadObject(AssetPackagePath);
            ClassName = Resolved.ClassName;
        }

        if (ClassName.IsEmpty())
        {
            // LoadObject couldn't resolve (asset not loaded, BP parent module not
            // up yet, etc.) — fall through to Tier 3 policy.
            if (NOT Tier3_IsAllowed(Flavor))
            {
                Result.ErrorMessage = FString::Printf(
                    TEXT("Could not resolve UClass via LoadObject for '%s' (package path '%s'). ")
                    TEXT("Tier 3 fallback not permitted for blocking-load accessors. ")
                    TEXT("Manual recovery: force-quit, comment out the failing %s::%s() call site, ")
                    TEXT("relaunch, click 'Generate All Asset Registries', uncomment, save."),
                    *InError.FunctionName, *AssetPackagePath,
                    *InError.TargetNamespace, *InError.FunctionName);
                return Result;
            }

            ClassName = TEXT("UObject");
            Result.UsedTier3Fallback = true;
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

        // ---- Step 5: UTF-16 LE atomic append to the matched output file ----

        if (NOT Try_AtomicAppendUtf16(Site.OutputPath, StubBlock))
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("UTF-16 atomic append failed for target file '%s'."), *Site.OutputPath);
            return Result;
        }

        Result.Success        = true;
        Result.TargetFilePath = Site.OutputPath;
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
