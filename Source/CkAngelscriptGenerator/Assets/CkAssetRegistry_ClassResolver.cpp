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
    namespace ck_asset_registry_class_resolver
    {
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

        // GetAssetByObjectPath is a point-query and stays safe at modal-tick: it bypasses the
        // IsEngineStartupModuleLoadingComplete gate, and the tag is a string needing no class-load.
        // AS-parented WBPs often cache it as "None" — Resolve_ViaPackageReader covers those.
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

        auto Try_ResolveTag = [&](const FName& InTagName) -> UClass*
        {
            auto TagValue = FString{};
            if (NOT AssetData.GetTagValue<FString>(InTagName, TagValue) || TagValue.IsEmpty() || TagValue == TEXT("None"))
            { return nullptr; }

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

        if (NOT TaggedParent)
        { return Result; }

        TaggedParent = ck_asset_registry_class_resolver::Get_AsResolvableClass(TaggedParent);

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

        // ToImport() ALREADY returns the 0-based array index — do NOT apply `-X - 1` again.
        auto Resolve_ImportPath = [&Imports](FPackageIndex InIndex) -> FString
        {
            if (NOT InIndex.IsImport())
            { return FString{}; }

            auto Names = TArray<FName>{};
            auto Cursor = InIndex;
            while (Cursor.IsImport())
            {
                const auto ImportIdx = Cursor.ToImport();
                if (NOT Imports.IsValidIndex(ImportIdx))
                { return FString{}; }
                const auto& Import = Imports[ImportIdx];
                Names.Insert(Import.ObjectName, 0);
                Cursor = Import.OuterIndex;
            }
            if (Names.Num() < 2)
            { return FString{}; }

            // The package name may be stored with OR without its leading slash.
            auto PackageStr = Names[0].ToString();
            if (NOT PackageStr.StartsWith(TEXT("/")))
            { PackageStr = FString{TEXT("/Script/")} + PackageStr; }

            auto ClassStr = Names[1].ToString();
            for (int32 i = 2; i < Names.Num(); ++i)
            {
                ClassStr += TEXT(".") + Names[i].ToString();
            }
            return PackageStr + TEXT(".") + ClassStr;
        };

        // Fallback for the reload window, where Hazelight has unregistered AS UClasses and even a
        // validly-parsed one is unfindable. AS paths ONLY: the `U` prefix rule holds for AS classes,
        // but a native Actor would need `A` and we would guess wrong.
        auto Derive_AsClassNameFromPath = [](const FString& InPath) -> FString
        {
            static const auto AsPrefix = FString{TEXT("/Script/Angelscript.")};
            if (NOT InPath.StartsWith(AsPrefix))
            { return FString{}; }
            const auto BaseName = InPath.Mid(AsPrefix.Len());
            if (BaseName.IsEmpty())
            { return FString{}; }
            return FString{TEXT("U")} + BaseName;
        };

        // Path A — the generated-class export's SuperIndex. A .uasset saved while its parent was
        // unloadable has SuperIndex=0 and falls through to Path B.
        for (const auto& Export : Exports)
        {
            const auto NameStr = Export.ObjectName.ToString();
            if (NOT NameStr.EndsWith(TEXT("_C")))
            { continue; }
            if (NOT Export.SuperIndex.IsImport())
            { continue; }

            const auto ParentPath = Resolve_ImportPath(Export.SuperIndex);
            if (ParentPath.IsEmpty())
            { continue; }

            if (auto* ParentClass = UClass::TryFindTypeSlow<UClass>(ParentPath))
            {
                ParentClass = ck_asset_registry_class_resolver::Get_AsResolvableClass(ParentClass);

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

        // Path B — the parent's name survives in the import table even when Path A's SuperIndex was
        // zero'd at save time. AS imports carry ClassName "ASClass", not "Class"; any *Class suffix
        // is accepted to stay robust to engine changes.
        const auto AssetBaseName = FPaths::GetBaseFilename(InPackagePath);

        auto Get_OuterPackageName = [&Imports](const FObjectImport& InImport) -> FString
        {
            if (NOT InImport.OuterIndex.IsImport())
            { return FString{}; }
            const auto OuterIdx = InImport.OuterIndex.ToImport();
            if (NOT Imports.IsValidIndex(OuterIdx))
            { return FString{}; }
            return Imports[OuterIdx].ObjectName.ToString();
        };

        auto AsCandidates = TArray<TPair<FString /*path*/, FString /*name*/>>{};
        for (const auto& Import : Imports)
        {
            const auto ClassNameStr = Import.ClassName.ToString();
            const auto IsClassImport = ClassNameStr == TEXT("Class")
                                    || ClassNameStr.EndsWith(TEXT("Class"));
            if (NOT IsClassImport)
            { continue; }

            const auto OuterPkg = Get_OuterPackageName(Import);
            const auto IsAsPkg = OuterPkg == TEXT("/Script/Angelscript")
                              || OuterPkg == TEXT("Angelscript");
            if (NOT IsAsPkg)
            { continue; }

            const auto FullPath = FString{TEXT("/Script/Angelscript.")} + Import.ObjectName.ToString();
            AsCandidates.Emplace(FullPath, Import.ObjectName.ToString());
        }

        auto Try_ResolveAndEmit = [&](const FString& InPath) -> bool
        {
            auto* Cls = UClass::TryFindTypeSlow<UClass>(InPath);
            if (NOT Cls)
            { return false; }
            Result.ResolvedClass = Cls;
            Result.ClassName     = UCkAssetRegistrySubsystem::Get_CorrectClassNameWithPrefix(Cls);
            Result.IsBlueprint   = true;
            ck::angelscriptgenerator::Verbose(TEXT("[ClassResolver][PackageReader][PathB] Resolved '{}' parent via AS-import scan: '{}' -> emit '{}'"),
                InPackagePath, InPath, Result.ClassName);
            return true;
        };

        if (AsCandidates.Num() == 1)
        {
            if (Try_ResolveAndEmit(AsCandidates[0].Key))
            { return Result; }
        }
        else if (AsCandidates.Num() > 1)
        {
            // Tie-break on shared prefix: the WBP naming convention ties the parent AS class name
            // to the asset's own file name.
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
                if (Try_ResolveAndEmit(AsCandidates[BestIdx].Key))
                { return Result; }
            }
        }

        ck::angelscriptgenerator::Warning(TEXT("[ClassResolver][PackageReader] No usable parent found in '{}' ({} exports, {} imports, {} AS-class candidates)"),
            DiskFilename, Exports.Num(), Imports.Num(), AsCandidates.Num());
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
