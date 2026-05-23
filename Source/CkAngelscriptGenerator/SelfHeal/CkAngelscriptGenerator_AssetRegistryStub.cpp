#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AssetRegistryStub.h"

#include "CkAngelscriptGenerator/Assets/CkAssetRegistrySubsystem.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/Macros/CkMacros.h"

#include <AssetRegistry/AssetData.h>
#include <AssetRegistry/IAssetRegistry.h>
#include <AssetRegistry/PackageReader.h>
#include <Blueprint/BlueprintSupport.h>
#include <Engine/Blueprint.h>
#include <UObject/ObjectResource.h>
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

// AR is deliberately NOT used in this file. At modal-tick during initial-
// compile-failure the engine is mid-startup-module-loading and AR's
// SearchAllAssets/GetAssetsByClass paths exit early on
// IsEngineStartupModuleLoadingComplete() == FALSE. Empirically caught
// 2026-05-12: the dispatcher's first wired AR call returned 0
// UCkAssetRegistryConfig assets even after Force_FullArScan.
//
// AR-free strategy:
//   1. Output file + discovery root: file-scan Script/Generated/*Assets.as
//      across project + plugins, parse `// Discovery root:` header line.
//      `// Source config:` line is malformed in BB's generator (no slash
//      between root and filename) — Discovery root is the canonical source.
//   2. Asset location on disk: convert package root to disk path via
//      FPackageName::TryConvertLongPackageNameToFilename, then
//      IFileManager::FindFilesRecursive for `<FunctionName>.uasset`.
//   3. Class resolution: LoadObject<UObject>(nullptr, *PackagePath). Uses
//      the UObject linker directly, so it works at modal-tick for any
//      asset whose native class module is loaded. For BPs, walk parent
//      via Get_NonBlueprintParentClass.
//
// Tier 3 fallback (UObject stub when LoadObject can't resolve) is REFUSED
// for all flavors as of 2026-05-13 — see Tier3_IsAllowed for the
// probe_a2.log rationale.

namespace ck::angelscriptgenerator::self_heal
{
    namespace
    {
        // UTF-16 LE atomic write for the sibling stub file. Matches the real
        // generator's encoding (`ForceUnicode` = UTF-16 LE + BOM) so the
        // sibling looks encoding-identical to hot-reload mtime detection and
        // external tooling. First write to a previously-missing path prepends
        // the StubFileHeader banner; subsequent writes accumulate.
        auto Try_AtomicWriteOrAppend_StubFile_Utf16(
            const FString& InStubPath,
            const FString& InAppendedBlock) -> bool
        {
            auto Existing = FString{};
            const auto FileExists = FFileHelper::LoadFileToString(Existing, *InStubPath);

            // Per-accessor dedup. Each appended block ends with a unique
            // "// End synthesized stub for <NS>::<FUNC>" line. If the existing
            // sibling already carries that exact marker, the accessor is covered
            // — a second append would produce a duplicate-function collision
            // when AS merges the namespace blocks at compile time.
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

        // Package-style root ("/Game/Raw/", "/Engine/...", "/<Plugin>/...") →
        // local disk path. FPackageName needs the package name without trailing
        // slash; we strip and re-add it on the converted side.
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

        // Returns the FSoftObjectPath-formatted package path of a `.uasset`
        // whose basename equals InFunctionName, located under InDiscoveryRoot.
        // Empty string on no match or ambiguity.
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

        struct FResolvedAssetClass
        {
            FString ClassName;
            bool    IsBlueprint = false;
        };

        // LoadObject<UObject>(InPackagePath) + class walk. AR-free.
        // ClassName empty when LoadObject returns nullptr or the loaded
        // UBlueprint's native parent can't be resolved (parent module not
        // up yet). Caller falls through to Tier3_IsAllowed (currently REFUSE).
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

        // Tier 2.5 — AssetData NativeParentClass tag fallback for the
        // chicken-and-egg case where LoadObject can't construct the asset.
        //
        // The canonical case this fixes: a WidgetBlueprint whose ParentClass
        // is itself an AS-defined UClass. During AS-compile failure (which is
        // when self-heal runs), the AS parent isn't registered, so the engine
        // refuses to construct the WBP's WidgetBlueprintGeneratedClass and
        // LoadObject<UObject>(PackagePath) returns nullptr. Same chain breaks
        // for any BP-derived asset whose parent BP is itself unloadable —
        // not unique to widgets.
        //
        // The asset header carries `FBlueprintTags::NativeParentClass` as a
        // STRING — no class-load required to read it. AR's
        // GetAssetByObjectPath is a point-query that tries FindObject first
        // and falls back to a state read under a lock; both paths bypass the
        // IsEngineStartupModuleLoadingComplete gate that gates SearchAllAssets
        // / GetAssetsByClass (engine: AssetRegistry.cpp ~3179). Safe at
        // modal-tick. The resolved string points at a native (C++) UClass,
        // always in-memory at modal-tick because it's linked at startup, so
        // UClass::TryFindTypeSlow returns a usable pointer for handoff to
        // Get_CorrectClassNameWithPrefix.
        //
        // Result.IsBlueprint = true is set unconditionally — this path only
        // produces a meaningful answer for assets that carry the BP tag, and
        // the downstream caller treats the flag the same way it does for the
        // existing LoadObject BP branch.
        auto Resolve_AssetClass_ViaAssetDataTag(
            const FString& InPackagePath) -> FResolvedAssetClass
        {
            auto Result = FResolvedAssetClass{};

            auto* AssetRegistry = IAssetRegistry::Get();
            if (NOT AssetRegistry)
            {
                ck::angelscriptgenerator::Verbose(TEXT("[SelfHeal][Tier2.5] IAssetRegistry::Get() returned null for '{}'"), InPackagePath);
                return Result;
            }

            const auto AssetData = AssetRegistry->GetAssetByObjectPath(FSoftObjectPath{InPackagePath});
            if (NOT AssetData.IsValid())
            {
                ck::angelscriptgenerator::Verbose(TEXT("[SelfHeal][Tier2.5] AR.GetAssetByObjectPath returned invalid AssetData for '{}' (AR cache not populated at modal-tick?)"), InPackagePath);
                return Result;
            }

            // Try NativeParentClassPath first (first found native parent). For
            // AS-parented WBPs whose AR scan happened while AS was wedged,
            // this often returns "None" because the walk couldn't reach a
            // native class. Fall back to ParentClassPath (immediate parent)
            // — that's typically the AS class path string verbatim, which we
            // can resolve via TryFindTypeSlow as long as the AS class itself
            // was successfully parsed (Hazelight registers the UClass at
            // parse-time even when other parts of the compile fail at link).
            auto Try_ResolveTag = [&](const FName& InTagName) -> UClass*
            {
                auto TagValue = FString{};
                if (NOT AssetData.GetTagValue<FString>(InTagName, TagValue) || TagValue.IsEmpty() || TagValue == TEXT("None"))
                { return nullptr; }

                // `Class'/Script/Module.ClassName'` -> `/Script/Module.ClassName`
                const auto Unwrapped = FPackageName::ExportTextPathToObjectPath(TagValue);
                auto* Resolved = UClass::TryFindTypeSlow<UClass>(Unwrapped);
                if (NOT Resolved)
                {
                    ck::angelscriptgenerator::Verbose(TEXT("[SelfHeal][Tier2.5] Tag '{}' on '{}' resolved to path '{}' but TryFindTypeSlow returned null"),
                        InTagName.ToString(), InPackagePath, Unwrapped);
                }
                return Resolved;
            };

            auto* TaggedParent = Try_ResolveTag(FBlueprintTags::NativeParentClassPath);
            if (NOT TaggedParent)
            { TaggedParent = Try_ResolveTag(FBlueprintTags::ParentClassPath); }

            if (NOT TaggedParent)
            {
                // Diagnostic: dump available tag keys + a few values so we can
                // see what AR actually has on this asset. Triggers when both
                // tag-walk attempts fail.
                auto TagSummary = FString{};
                AssetData.EnumerateTags([&TagSummary](const TPair<FName, FAssetTagValueRef>& InPair)
                {
                    if (NOT TagSummary.IsEmpty()) { TagSummary += TEXT(","); }
                    TagSummary += InPair.Key.ToString();
                    // Inline a few load-bearing tag values for quick triage.
                    const auto Key = InPair.Key;
                    if (Key == FBlueprintTags::NativeParentClassPath
                        || Key == FBlueprintTags::ParentClassPath
                        || Key == FBlueprintTags::GeneratedClassPath)
                    {
                        TagSummary += FString::Printf(TEXT("=%s"), *InPair.Value.AsString());
                    }
                });
                ck::angelscriptgenerator::Verbose(TEXT("[SelfHeal][Tier2.5] Both NativeParentClassPath and ParentClassPath unresolvable for '{}'. AR tags: [{}]"),
                    InPackagePath, TagSummary);
                return Result;
            }

            // Don't walk Get_NonBlueprintParentClass. NativeParentClassPath is
            // already native by definition; ParentClassPath gives us the
            // *immediate* parent which is what the caller's typed slot
            // expects (caller writes `TSoftClassPtr<UBb_LootableInventory_PanelWidget>`,
            // not `TSoftClassPtr<UUserWidget>`). Walking past an AS class
            // would defeat the whole point of the ParentClassPath fallback.
            Result.ClassName   = UCkAssetRegistrySubsystem::Get_CorrectClassNameWithPrefix(TaggedParent);
            Result.IsBlueprint = true;
            return Result;
        }

        // Tier 2.6 — read the .uasset linker tables directly via FPackageReader.
        //
        // Triggered when both LoadObject (Tier 2) and AR tag lookup (Tier 2.5)
        // fail. The canonical case: AS-parented WBPs where AR was scanned
        // while the AS class was unloadable, so all three class-path tags
        // (NativeParentClass/ParentClass/GeneratedClass) were cached as
        // "None". The .uasset file itself is the on-disk truth — its
        // export's SuperIndex points at the parent-class import regardless
        // of any class-walk state.
        //
        // Algorithm:
        //   1. Convert package path to disk filename via FPackageName.
        //   2. FPackageReader::OpenPackageFile + GetExports/GetImports.
        //   3. Find the generated-class export (name ends "_C" for BPs); its
        //      SuperIndex points at the parent class import.
        //   4. Walk the import's OuterIndex chain to construct
        //      `/Script/Module.ClassName`.
        //   5. UClass::TryFindTypeSlow → use Get_CorrectClassNameWithPrefix.
        //
        // For AS-parented WBPs the resolved path is e.g.
        // `/Script/Angelscript.Bb_LootableInventory_PanelWidget`. The AS
        // class IS in the UObject registry at modal-tick (Hazelight
        // registers UClass at parse-time, even when other parts of the
        // compile fail at link), so TryFindTypeSlow succeeds.
        auto Resolve_AssetClass_ViaPackageReader(
            const FString& InPackagePath) -> FResolvedAssetClass
        {
            auto Result = FResolvedAssetClass{};

            // /Game/X/Y/Z.Z -> /Game/X/Y/Z
            auto LongPackageName = InPackagePath;
            const auto DotIdx = LongPackageName.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
            if (DotIdx != INDEX_NONE)
            { LongPackageName = LongPackageName.Left(DotIdx); }

            auto DiskFilename = FString{};
            if (NOT FPackageName::TryConvertLongPackageNameToFilename(LongPackageName, DiskFilename, FPackageName::GetAssetPackageExtension()))
            {
                ck::angelscriptgenerator::Verbose(TEXT("[SelfHeal][Tier2.6] Could not convert '{}' -> disk path"), LongPackageName);
                return Result;
            }

            auto Reader = FPackageReader{};
            if (NOT Reader.OpenPackageFile(DiskFilename))
            {
                ck::angelscriptgenerator::Verbose(TEXT("[SelfHeal][Tier2.6] FPackageReader::OpenPackageFile failed for '{}'"), DiskFilename);
                return Result;
            }

            auto Imports = TArray<FObjectImport>{};
            auto Exports = TArray<FObjectExport>{};
            if (NOT Reader.GetImports(Imports) || NOT Reader.GetExports(Exports))
            { return Result; }

            // Helper: resolve an FPackageIndex to a flat path string by
            // walking its OuterIndex chain through ImportMap.
            auto Resolve_ImportPath = [&Imports](FPackageIndex InIndex) -> FString
            {
                if (NOT InIndex.IsImport()) { return FString{}; }

                auto Names = TArray<FName>{};
                auto Cursor = InIndex;
                while (Cursor.IsImport())
                {
                    const auto ImportIdx = Cursor.ToImport();
                    if (NOT Imports.IsValidIndex(ImportIdx)) { return FString{}; }
                    const auto& Import = Imports[ImportIdx];
                    Names.Insert(Import.ObjectName, 0);
                    Cursor = Import.OuterIndex;
                }
                // Names is now [/Script/Module, ClassName] (or longer for nested).
                if (Names.Num() < 2) { return FString{}; }

                // Top-level FName is the package, e.g. /Script/Angelscript or
                // /Script/UMG. Stored variants: with leading slash, without,
                // or as bare module name. Normalize to leading slash.
                auto PackageStr = Names[0].ToString();
                if (NOT PackageStr.StartsWith(TEXT("/"))) { PackageStr = FString{TEXT("/Script/")} + PackageStr; }

                auto ClassStr = Names[1].ToString();
                for (int32 i = 2; i < Names.Num(); ++i)
                {
                    ClassStr += TEXT(".") + Names[i].ToString();
                }
                return PackageStr + TEXT(".") + ClassStr;
            };

            // Path A — Generated-class export's SuperIndex (canonical).
            // For a healthy WBP: `<X>_WBP_C` export's SuperIndex points at
            // the parent class import. Fails when the .uasset was saved
            // while the parent was unloadable — serializer wrote
            // SuperIndex=0 because it couldn't reference what wasn't there.
            // Helper: when TryFindTypeSlow fails on an AS-class path (Hazelight
            // unregisters AS UClasses during the reload window — modal-tick is
            // mid-unload), derive the AS-side type name directly from the
            // path string. AS-defined classes that parent a WBP are always
            // UUserWidget-derived (that's what WBP _IS_), so the `U` prefix
            // is correct. The path `/Script/Angelscript.Bb_X` -> `UBb_X`.
            // Non-AS class paths (e.g. /Script/UMG.UserWidget) MUST resolve
            // via TryFindTypeSlow — non-AS classes don't follow the `U`+name
            // rule (Actor classes use `A`, etc.) and we'd guess wrong.
            auto Derive_AsClassNameFromPath = [](const FString& InPath) -> FString
            {
                static const auto AsPrefix = FString{TEXT("/Script/Angelscript.")};
                if (NOT InPath.StartsWith(AsPrefix)) { return FString{}; }
                const auto BaseName = InPath.Mid(AsPrefix.Len());
                if (BaseName.IsEmpty()) { return FString{}; }
                return FString{TEXT("U")} + BaseName;
            };

            for (const auto& Export : Exports)
            {
                const auto NameStr = Export.ObjectName.ToString();
                if (NOT NameStr.EndsWith(TEXT("_C"))) { continue; }
                if (NOT Export.SuperIndex.IsImport()) { continue; }

                const auto ParentPath = Resolve_ImportPath(Export.SuperIndex);
                if (ParentPath.IsEmpty()) { continue; }

                if (auto* ParentClass = UClass::TryFindTypeSlow<UClass>(ParentPath))
                {
                    Result.ClassName   = UCkAssetRegistrySubsystem::Get_CorrectClassNameWithPrefix(ParentClass);
                    Result.IsBlueprint = true;
                    ck::angelscriptgenerator::Verbose(TEXT("[SelfHeal][Tier2.6][PathA] Resolved '{}' parent via _C SuperIndex: '{}' -> emit '{}'"),
                        InPackagePath, ParentPath, Result.ClassName);
                    return Result;
                }

                // AS-class chicken-and-egg fallback: derive `UBb_X` from path.
                const auto Derived = Derive_AsClassNameFromPath(ParentPath);
                if (NOT Derived.IsEmpty())
                {
                    Result.ClassName   = Derived;
                    Result.IsBlueprint = true;
                    ck::angelscriptgenerator::Verbose(TEXT("[SelfHeal][Tier2.6][PathA+ASDerive] '{}' parent '{}' not in UObject registry (AS reload window); derived AS-side name '{}'"),
                        InPackagePath, ParentPath, Result.ClassName);
                    return Result;
                }

                ck::angelscriptgenerator::Verbose(TEXT("[SelfHeal][Tier2.6][PathA] Export '{}' SuperIndex resolved to '{}' but neither TryFindTypeSlow nor AS-derivation produced a class name"),
                    NameStr, ParentPath);
            }

            // Path B — scan imports for a Class-typed AS import. When the
            // _C export's SuperIndex is 0 (parent was unloadable at save),
            // the parent name often STILL survives as a Class import in
            // the import table because the BP's serialized graph references
            // the type by name. Filter to imports whose outer chain ends in
            // `/Script/Angelscript` and whose ClassName is `Class`.
            //
            // Heuristic: if exactly one AS class import exists, use it.
            // If multiple, prefer the one whose ObjectName has the longest
            // shared prefix with the asset's basename (WBP naming convention
            // ties the parent AS class name to the WBP filename).
            const auto AssetBaseName = FPaths::GetBaseFilename(InPackagePath);

            // Get the outer package name of a class import (one hop, no
            // path concatenation — package imports have a single FName).
            auto Get_OuterPackageName = [&Imports](const FObjectImport& InImport) -> FString
            {
                if (NOT InImport.OuterIndex.IsImport()) { return FString{}; }
                const auto OuterIdx = InImport.OuterIndex.ToImport();
                if (NOT Imports.IsValidIndex(OuterIdx)) { return FString{}; }
                return Imports[OuterIdx].ObjectName.ToString();
            };

            auto AsCandidates = TArray<TPair<FString /*path*/, FString /*name*/>>{};
            for (const auto& Import : Imports)
            {
                // The import's CLASS metaclass — for a UClass import, this is
                // typically "Class" (UClass itself). AS-generated classes may
                // use ScriptClass or similar — accept multiple shapes.
                const auto ClassNameStr = Import.ClassName.ToString();
                const auto IsClassImport = ClassNameStr == TEXT("Class")
                                        || ClassNameStr.EndsWith(TEXT("Class"));
                if (NOT IsClassImport) { continue; }

                const auto OuterPkg = Get_OuterPackageName(Import);
                // AS classes live under /Script/Angelscript. The package
                // import's ObjectName is typically "/Script/Angelscript"
                // (with leading slash) but can also be just "Angelscript".
                const auto IsAsPkg = OuterPkg == TEXT("/Script/Angelscript")
                                  || OuterPkg == TEXT("Angelscript");
                if (NOT IsAsPkg) { continue; }

                const auto FullPath = FString{TEXT("/Script/Angelscript.")} + Import.ObjectName.ToString();
                AsCandidates.Emplace(FullPath, Import.ObjectName.ToString());
            }

            auto Try_ResolveAndEmit = [&](const FString& InPath) -> bool
            {
                auto* Cls = UClass::TryFindTypeSlow<UClass>(InPath);
                if (NOT Cls) { return false; }
                Result.ClassName   = UCkAssetRegistrySubsystem::Get_CorrectClassNameWithPrefix(Cls);
                Result.IsBlueprint = true;
                ck::angelscriptgenerator::Verbose(TEXT("[SelfHeal][Tier2.6][PathB] Resolved '{}' parent via AS-import scan: '{}' -> emit '{}'"),
                    InPackagePath, InPath, Result.ClassName);
                return true;
            };

            if (AsCandidates.Num() == 1)
            {
                if (Try_ResolveAndEmit(AsCandidates[0].Key)) { return Result; }
            }
            else if (AsCandidates.Num() > 1)
            {
                // Multiple AS imports: rank by longest shared prefix with the
                // asset basename.
                auto BestIdx       = int32{INDEX_NONE};
                auto BestPrefixLen = int32{-1};
                for (auto i = 0; i < AsCandidates.Num(); ++i)
                {
                    const auto& Name = AsCandidates[i].Value;
                    auto Shared = int32{0};
                    const auto MaxLen = FMath::Min(Name.Len(), AssetBaseName.Len());
                    while (Shared < MaxLen
                        && FChar::ToLower(Name[Shared]) == FChar::ToLower(AssetBaseName[Shared]))
                    { ++Shared; }
                    if (Shared > BestPrefixLen)
                    {
                        BestPrefixLen = Shared;
                        BestIdx       = i;
                    }
                }
                if (AsCandidates.IsValidIndex(BestIdx))
                {
                    if (Try_ResolveAndEmit(AsCandidates[BestIdx].Key)) { return Result; }
                }
            }

            // Diagnostic dump — full export list (just _C-suffixed ones) +
            // all imports' class+name+outer shape so we can see what AR sees.
            auto ExportSummary = FString{};
            for (const auto& E : Exports)
            {
                const auto N = E.ObjectName.ToString();
                if (NOT N.EndsWith(TEXT("_C")) && NOT N.Contains(TEXT("WBP"))) { continue; }
                if (NOT ExportSummary.IsEmpty()) { ExportSummary += TEXT(","); }
                ExportSummary += FString::Printf(TEXT("%s[super=%d,class=%d]"),
                    *N, E.SuperIndex.ForDebugging(), E.ClassIndex.ForDebugging());
            }
            auto ImportSummary = FString{};
            for (auto i = 0; i < FMath::Min(Imports.Num(), 50); ++i)
            {
                const auto& Imp = Imports[i];
                if (NOT ImportSummary.IsEmpty()) { ImportSummary += TEXT(","); }
                ImportSummary += FString::Printf(TEXT("%s:%s[outer=%d]"),
                    *Imp.ClassName.ToString(), *Imp.ObjectName.ToString(),
                    Imp.OuterIndex.ForDebugging());
            }
            ck::angelscriptgenerator::Warning(TEXT("[SelfHeal][Tier2.6] No usable parent found in '{}'. _C/WBP exports: [{}]"),
                DiskFilename, ExportSummary);
            ck::angelscriptgenerator::Warning(TEXT("[SelfHeal][Tier2.6] All imports: [{}]"), ImportSummary);
            return Result;
        }

        // Tier 3 (UObject stub when LoadObject fails) is REFUSED for all
        // flavors as of 2026-05-13 (probe_a2.log).
        //
        // The original policy permitted Tier 3 for SoftRef/SoftClass on the
        // assumption that the caller's typed-conversion error would be "a
        // follow-up AS error pointing at the right line — better diagnostic
        // than a wedge." Probe a2 disproved that: the typed-conversion error
        // (`Cannot convert from TSoftObjectPtr<UObject> to
        // TSoftObjectPtr<UWorld>`) does NOT match either of
        // FCkAsErrorParser's two recognized patterns. Cycle 2 parses zero
        // actionable roots and the editor wedges on the terminal banner
        // instead of surfacing the original `No matching signatures` error.
        //
        // Refusing across the board means Hazelight's modal keeps showing
        // the original `No matching signatures` error — actionable, points
        // at the real call site, parser-blind derivatives never appear.
        auto Tier3_IsAllowed(
            ECk_AssetAccessorFlavor /*InFlavor*/) -> bool
        {
            return false;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAsAssetRegistryStubSynthesizer::
        Resolve_ClassName_FromPackageReader_OnDisk(
            const FString& InPackagePath)
        -> FString
    {
        // Wraps the anon-namespace helper for callers outside this TU.
        // Lookup finds the anon function via the enclosing namespace.
        const auto Resolved = Resolve_AssetClass_ViaPackageReader(InPackagePath);
        return Resolved.ClassName;
    }

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

        // The real generator emits two header lines we care about:
        //   // Source config: <Name> (<...path...>.as [<Namespace>])
        //   // Discovery root: <DiscoveryRoot>
        //
        // We parse the namespace from `Source config:`'s bracketed token. The
        // path token in that same line is malformed in BB's generator (missing
        // slash between root and filename), so we read the canonical root from
        // the `Discovery root:` line instead.
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

            // Sort for deterministic candidate order (matters when no asset
            // prefix wins and we fall back to first match).
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

            // Match with trailing '/' included so "/Game/" doesn't spuriously
            // match a path under "/GameOther/".
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
        // Conjunction is checked first so `assets::load::FOO_Class()` resolves
        // to BlockingLoadClass instead of plain BlockingLoad. Without this the
        // downstream `_Class` strip never fires and the disk walk searches for
        // `FOO_Class.uasset` literally (no such file).
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
        Out += FString::Printf(
            TEXT("        if (ck::EnsureIfNot(UCk_Utils_IO_UE::IsEngineSafeForBlockingLoads(), \"%s::load::%s() called before engine init. Use %s::%s() (soft ref) with UCk_DeferredConfig_UE instead.\"))"),
            *InSoftNamespace, *InFunctionName, *InSoftNamespace, *InFunctionName);                                  Out += LINE_TERMINATOR;
        Out += TEXT("        { return nullptr; }");                                                                 Out += LINE_TERMINATOR;
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
        // Mirrors the canonical generator's BP blocking-class shape
        // (CkAssetRegistrySubsystem.cpp:530-540): returns TSubclassOf<Class>,
        // delegates to LoadClassAsset_Blocking via the soft-class accessor.
        // InFunctionName carries the `_Class` suffix already.
        auto Out = FString{};
        Out += FString::Printf(TEXT("    TSubclassOf<%s> %s()"), *InResolvedClassName, *InFunctionName);            Out += LINE_TERMINATOR;
        Out += TEXT("    {");                                                                                       Out += LINE_TERMINATOR;
        Out += FString::Printf(
            TEXT("        if (ck::EnsureIfNot(UCk_Utils_IO_UE::IsEngineSafeForBlockingLoads(), \"%s::load::%s() called before engine init. Use %s::%s() (soft ref) with UCk_DeferredConfig_UE instead.\"))"),
            *InSoftNamespace, *InFunctionName, *InSoftNamespace, *InFunctionName);                                  Out += LINE_TERMINATOR;
        Out += TEXT("        { return nullptr; }");                                                                 Out += LINE_TERMINATOR;
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

        const auto Candidates = Collect_MatchingSites(Collect_GeneratedScriptDirs(), SoftNamespace);
        if (Candidates.Num() == 0)
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("No generated *Assets.as file matched namespace '%s' (stripped from '%s'). ")
                TEXT("Either the namespace is unknown or the output file was never generated."),
                *SoftNamespace, *InError.TargetNamespace);
            return Result;
        }

        // ---- Step 2: locate asset on disk, picking the candidate whose
        //              DiscoveryRoot owns it ----
        //
        // Multiple files can legitimately share `namespace assets` while
        // covering disjoint roots (e.g. BusterBlockAssets on "/Game/BusterBlock/"
        // + RawAssets on "/Game/Raw/"). AS merges the namespace at compile time,
        // so any file would unwedge — but the stub must land in the owning file
        // or the deferred regen has to rewrite both.
        const auto NeedsClassStrip = (Flavor == ECk_AssetAccessorFlavor::SoftClass)
                                  || (Flavor == ECk_AssetAccessorFlavor::BlockingLoadClass);
        const auto BaseFunctionName = NeedsClassStrip
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
        { Site = Candidates[0]; }

        if (AssetPackagePath.IsEmpty())
        {
            // Tier 3 refused — surface actionable banner. Hazelight's modal
            // keeps the original `No matching signatures` error visible to
            // the user, which is exactly what they need to see.
            if (NOT Tier3_IsAllowed(Flavor))
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
            const auto Resolved = Resolve_AssetClass_ViaLoadObject(AssetPackagePath);
            ClassName = Resolved.ClassName;
        }

        // Tier 2.5 fallback — Resolve_AssetClass_ViaAssetDataTag works for
        // assets where AR has usable NativeParentClass/ParentClass tags.
        // Doesn't help when AR scanned the asset while the class chain was
        // wedged (all tags cached as "None").
        if (ClassName.IsEmpty() && NOT AssetPackagePath.IsEmpty())
        {
            const auto Resolved = Resolve_AssetClass_ViaAssetDataTag(AssetPackagePath);
            ClassName = Resolved.ClassName;
        }

        // Tier 2.6 fallback — read the .uasset linker tables directly via
        // FPackageReader. Bypasses AR's poisoned cache entirely. Closes the
        // loop for AS-parented WBPs (the live BB case): the WBP's generated
        // class export carries a SuperIndex pointing at the AS parent class
        // import, which TryFindTypeSlow resolves (AS UClasses are registered
        // at parse-time even when link fails).
        if (ClassName.IsEmpty() && NOT AssetPackagePath.IsEmpty())
        {
            const auto Resolved = Resolve_AssetClass_ViaPackageReader(AssetPackagePath);
            ClassName = Resolved.ClassName;
        }

        if (ClassName.IsEmpty())
        {
            if (NOT Tier3_IsAllowed(Flavor))
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
