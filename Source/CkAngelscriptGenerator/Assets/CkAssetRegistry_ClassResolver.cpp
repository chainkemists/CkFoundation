#include "CkAssetRegistry_ClassResolver.h"

#include "CkAngelscriptGenerator/Assets/CkAssetRegistrySubsystem.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include <AssetRegistry/AssetData.h>
#include <AssetRegistry/IAssetRegistry.h>
#include <AssetRegistry/PackageReader.h>
#include <Blueprint/BlueprintSupport.h>
#include <Engine/Blueprint.h>
#include <Misc/PackageName.h>
#include <Misc/Paths.h>
#include <UObject/Class.h>
#include <UObject/ObjectResource.h>
#include <UObject/UObjectGlobals.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator
{
    namespace
    {
        // Walks past Blueprint-generated classes in the parent chain — a BPGC
        // name is not an AS identifier and must never be emitted. Native and
        // AS-script classes return unchanged.
        auto Get_AsResolvableClass(
            UClass* InClass) -> UClass*
        {
            if (auto* AsResolvable = UCkAssetRegistrySubsystem::Get_NonBlueprintParentClass(InClass))
            { return AsResolvable; }
            return InClass;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAssetRegistry_ClassResolver::
        Resolve_ViaLoadObject(
            const FString& InPackagePath)
        -> FCk_ResolvedAssetClass
    {
        auto Result = FCk_ResolvedAssetClass{};

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

            Result.ResolvedClass = NativeParent;
            Result.ClassName     = UCkAssetRegistrySubsystem::Get_CorrectClassNameWithPrefix(NativeParent);
            return Result;
        }

        if (auto* AssetClass = Loaded->GetClass())
        {
            auto* NativeParent = UCkAssetRegistrySubsystem::Get_NonBlueprintParentClass(AssetClass);
            if (NOT NativeParent)
            { return Result; }

            Result.ResolvedClass = NativeParent;
            Result.ClassName     = UCkAssetRegistrySubsystem::Get_CorrectClassNameWithPrefix(NativeParent);
        }
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAssetRegistry_ClassResolver::
        Resolve_ViaAssetDataTag(
            const FString& InPackagePath)
        -> FCk_ResolvedAssetClass
    {
        auto Result = FCk_ResolvedAssetClass{};

        // AR's GetAssetByObjectPath is safe at modal-tick — point-query that
        // bypasses the IsEngineStartupModuleLoadingComplete gate (engine:
        // AssetRegistry.cpp ~3179). The tag value is a STRING; no class-load
        // needed to read it. For AS-parented WBPs the tag often caches as
        // "None" because the AR-scan-time walk couldn't reach a native parent
        // — that's the case the PackageReader walk below picks up.
        auto* AssetRegistry = IAssetRegistry::Get();
        if (NOT AssetRegistry)
        {
            ck::angelscriptgenerator::Verbose(TEXT("[ClassResolver][AssetDataTag] IAssetRegistry::Get() returned null for '{}'"), InPackagePath);
            return Result;
        }

        const auto AssetData = AssetRegistry->GetAssetByObjectPath(FSoftObjectPath{InPackagePath});
        if (NOT AssetData.IsValid())
        {
            ck::angelscriptgenerator::Verbose(TEXT("[ClassResolver][AssetDataTag] AR.GetAssetByObjectPath returned invalid AssetData for '{}' (AR cache not populated at modal-tick?)"), InPackagePath);
            return Result;
        }

        // NativeParentClassPath first; fall back to ParentClassPath
        // (immediate parent — may be the AS class itself, which resolves
        // when Hazelight has its UClass registered).
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
                ck::angelscriptgenerator::Verbose(TEXT("[ClassResolver][AssetDataTag] Tag '{}' on '{}' resolved to path '{}' but TryFindTypeSlow returned null"),
                    InTagName.ToString(), InPackagePath, Unwrapped);
            }
            return Resolved;
        };

        auto* TaggedParent = Try_ResolveTag(FBlueprintTags::NativeParentClassPath);
        if (NOT TaggedParent)
        { TaggedParent = Try_ResolveTag(FBlueprintTags::ParentClassPath); }

        if (NOT TaggedParent) { return Result; }

        // ParentClassPath's immediate-parent is either an AS class (what the
        // caller's typed slot expects — keep it) or another Blueprint's
        // generated class (not an AS identifier — must not be emitted).
        // Get_AsResolvableClass distinguishes the two: AS script classes
        // return unchanged, BPGCs walk up to the nearest native/AS ancestor.
        TaggedParent = Get_AsResolvableClass(TaggedParent);

        Result.ResolvedClass = TaggedParent;
        Result.ClassName     = UCkAssetRegistrySubsystem::Get_CorrectClassNameWithPrefix(TaggedParent);
        Result.IsBlueprint   = true;
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCkAssetRegistry_ClassResolver::
        Resolve_ViaPackageReader(
            const FString& InPackagePath)
        -> FCk_ResolvedAssetClass
    {
        auto Result = FCk_ResolvedAssetClass{};

        // /Game/X/Y/Z.Z -> /Game/X/Y/Z
        auto LongPackageName = InPackagePath;
        const auto DotIdx = LongPackageName.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
        if (DotIdx != INDEX_NONE)
        { LongPackageName = LongPackageName.Left(DotIdx); }

        auto DiskFilename = FString{};
        if (NOT FPackageName::TryConvertLongPackageNameToFilename(LongPackageName, DiskFilename, FPackageName::GetAssetPackageExtension()))
        {
            ck::angelscriptgenerator::Verbose(TEXT("[ClassResolver][PackageReader] Could not convert '{}' -> disk path"), LongPackageName);
            return Result;
        }

        auto Reader = FPackageReader{};
        if (NOT Reader.OpenPackageFile(DiskFilename))
        {
            ck::angelscriptgenerator::Verbose(TEXT("[ClassResolver][PackageReader] FPackageReader::OpenPackageFile failed for '{}'"), DiskFilename);
            return Result;
        }

        auto Imports = TArray<FObjectImport>{};
        auto Exports = TArray<FObjectExport>{};
        if (NOT Reader.GetImports(Imports) || NOT Reader.GetExports(Exports))
        { return Result; }

        // Walk an FPackageIndex up the OuterIndex chain to a flat path
        // like `/Script/Module.ClassName`. NOTE: `ToImport()` already
        // returns the 0-based array index — DO NOT apply `-X - 1` again.
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
            if (Names.Num() < 2) { return FString{}; }

            // Package name may be stored with or without leading slash;
            // normalize to `/Script/<Module>`.
            auto PackageStr = Names[0].ToString();
            if (NOT PackageStr.StartsWith(TEXT("/"))) { PackageStr = FString{TEXT("/Script/")} + PackageStr; }

            auto ClassStr = Names[1].ToString();
            for (int32 i = 2; i < Names.Num(); ++i)
            {
                ClassStr += TEXT(".") + Names[i].ToString();
            }
            return PackageStr + TEXT(".") + ClassStr;
        };

        // AS-class fallback when TryFindTypeSlow fails: derive the AS
        // type name from the path. Hazelight unregisters AS UClasses
        // during the reload window, so even validly-parsed AS classes
        // can be unfindable at modal-tick. Restricted to AS paths only
        // because the `U` prefix rule only holds for UObject-derived AS
        // classes — native Actor classes use `A`, and we'd guess wrong.
        auto Derive_AsClassNameFromPath = [](const FString& InPath) -> FString
        {
            static const auto AsPrefix = FString{TEXT("/Script/Angelscript.")};
            if (NOT InPath.StartsWith(AsPrefix)) { return FString{}; }
            const auto BaseName = InPath.Mid(AsPrefix.Len());
            if (BaseName.IsEmpty()) { return FString{}; }
            return FString{TEXT("U")} + BaseName;
        };

        // Path A — generated-class export's SuperIndex. Fails when the
        // .uasset was saved while the parent was unloadable (serializer
        // wrote SuperIndex=0 because it couldn't reference what wasn't
        // there); Path B below picks up that case.
        for (const auto& Export : Exports)
        {
            const auto NameStr = Export.ObjectName.ToString();
            if (NOT NameStr.EndsWith(TEXT("_C"))) { continue; }
            if (NOT Export.SuperIndex.IsImport()) { continue; }

            const auto ParentPath = Resolve_ImportPath(Export.SuperIndex);
            if (ParentPath.IsEmpty()) { continue; }

            if (auto* ParentClass = UClass::TryFindTypeSlow<UClass>(ParentPath))
            {
                // The SuperIndex import can be another Blueprint's generated
                // class (BP-of-BP chains) — walk to the nearest AS-resolvable
                // ancestor before emitting.
                ParentClass = Get_AsResolvableClass(ParentClass);

                Result.ResolvedClass = ParentClass;
                Result.ClassName     = UCkAssetRegistrySubsystem::Get_CorrectClassNameWithPrefix(ParentClass);
                Result.IsBlueprint   = true;
                ck::angelscriptgenerator::Verbose(TEXT("[ClassResolver][PackageReader][PathA] Resolved '{}' parent via _C SuperIndex: '{}' -> emit '{}'"),
                    InPackagePath, ParentPath, Result.ClassName);
                return Result;
            }

            const auto Derived = Derive_AsClassNameFromPath(ParentPath);
            if (NOT Derived.IsEmpty())
            {
                Result.ClassName   = Derived;
                Result.IsBlueprint = true;
                ck::angelscriptgenerator::Verbose(TEXT("[ClassResolver][PackageReader][PathA+ASDerive] '{}' parent '{}' not in UObject registry (AS reload window); derived AS-side name '{}'"),
                    InPackagePath, ParentPath, Result.ClassName);
                return Result;
            }
        }

        // Path B — scan the import table directly for Class-typed AS
        // imports. The parent's name survives there even when Path A's
        // SuperIndex was zero'd at save time. AS classes use ClassName
        // "ASClass" specifically (not "Class") — accept any *Class
        // suffix to be robust to engine changes.
        const auto AssetBaseName = FPaths::GetBaseFilename(InPackagePath);

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
            const auto ClassNameStr = Import.ClassName.ToString();
            const auto IsClassImport = ClassNameStr == TEXT("Class")
                                    || ClassNameStr.EndsWith(TEXT("Class"));
            if (NOT IsClassImport) { continue; }

            // Package name normalisation: with or without leading slash.
            const auto OuterPkg = Get_OuterPackageName(Import);
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
            Result.ResolvedClass = Cls;
            Result.ClassName     = UCkAssetRegistrySubsystem::Get_CorrectClassNameWithPrefix(Cls);
            Result.IsBlueprint   = true;
            ck::angelscriptgenerator::Verbose(TEXT("[ClassResolver][PackageReader][PathB] Resolved '{}' parent via AS-import scan: '{}' -> emit '{}'"),
                InPackagePath, InPath, Result.ClassName);
            return true;
        };

        if (AsCandidates.Num() == 1)
        {
            if (Try_ResolveAndEmit(AsCandidates[0].Key)) { return Result; }
        }
        else if (AsCandidates.Num() > 1)
        {
            // Tie-break by longest shared prefix with asset basename
            // (WBP naming convention ties parent AS class name to file).
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

        ck::angelscriptgenerator::Warning(TEXT("[ClassResolver][PackageReader] No usable parent found in '{}' ({} exports, {} imports, {} AS-class candidates)"),
            DiskFilename, Exports.Num(), Imports.Num(), AsCandidates.Num());
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
