#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AssetRegistryStub.h"

#include "CkAngelscriptGenerator/Assets/CkAssetRegistryConfig.h"
#include "CkAngelscriptGenerator/Assets/CkAssetRegistrySubsystem.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/Macros/CkMacros.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <AssetRegistry/IAssetRegistry.h>
#include <Engine/Blueprint.h>
#include <HAL/FileManager.h>
#include <Misc/DateTime.h>
#include <Misc/FileHelper.h>
#include <Misc/Paths.h>

// --------------------------------------------------------------------------------------------------------------------

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

        // ---- Sync AR scan ----------------------------------------------------------

        // Force a full synchronous AR scan. At modal-tick time AR is typically
        // partially populated — calling SearchAllAssets(true) blocks until the
        // index is complete. First call is multi-second on cold cache; subsequent
        // calls are cheap. Same pattern as the DynamicHandle recovery path.
        auto Force_FullArScan() -> void
        {
            auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
            AssetRegistryModule.Get().SearchAllAssets(/*bSynchronousSearch=*/true);
        }

        // ---- Asset lookup by function name ----------------------------------------

        // Iterates assets under InDiscoveryRoot and returns the one whose
        // generator-derived function name (Get_CleanAssetName + dup handling
        // collapsed to first match) equals InTargetFunctionName.
        //
        // Returns an empty FAssetData on no match. Dup-suffixed function names
        // (`Foo_DUP1`, etc.) are not resolved — those are rare and the caller
        // surfaces a manual-intervention message instead of synthesizing
        // potentially-wrong stubs.
        auto Find_AssetByFunctionName(
            const FString& InDiscoveryRoot,
            const FString& InTargetFunctionName) -> FAssetData
        {
            const auto Discovered = UCkAssetRegistrySubsystem::Request_DiscoverAssetsInPath(InDiscoveryRoot);

            for (const auto& AssetData : Discovered)
            {
                const auto CleanName = UCkAssetRegistrySubsystem::Get_CleanAssetName(AssetData.AssetName.ToString());
                if (CleanName == InTargetFunctionName)
                { return AssetData; }
            }
            return FAssetData{};
        }

        // ---- Class resolution (Tier 1 / Tier 2) -----------------------------------

        // Result of class resolution. ClassName carries the prefixed name
        // ("USkeletalMesh", "AMyActor"). IsBlueprint=true means we walked
        // through UBlueprint to find the native parent. Empty ClassName
        // signals "could not resolve" and the caller falls through to Tier 3.
        struct FResolvedAssetClass
        {
            FString ClassName;
            bool    IsBlueprint = false;
        };

        auto Resolve_AssetClass_Tier1_2(
            const FAssetData& InAssetData) -> FResolvedAssetClass
        {
            auto Result = FResolvedAssetClass{};

            // Tier 1: try the assetdata-reported class without loading. If
            // it's a non-Blueprint native class that's already loaded, we
            // don't need to pay the sync-load cost.
            //
            // FAssetData::GetClass() returns nullptr if the class isn't loaded
            // into the UObject system; that's our cue to fall through to Tier 2.
            if (auto* AssetClass = InAssetData.GetClass())
            {
                if (AssetClass != UBlueprint::StaticClass()
                    && NOT AssetClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
                {
                    Result.ClassName = UCkAssetRegistrySubsystem::Get_CorrectClassNameWithPrefix(AssetClass);
                    return Result;
                }
            }

            // Tier 2: sync-load the asset. For Blueprints we then walk the
            // parent chain to find the native parent. For non-Blueprints
            // whose class wasn't loaded earlier, the load will pull it in.
            //
            // GetAsset() does a sync FindObject + LoadObject under the hood.
            // Can return nullptr if the package fails to load — that's Tier 3.
            auto* LoadedAsset = InAssetData.GetAsset();
            if (NOT LoadedAsset)
            { return Result; }

            if (auto* LoadedBlueprint = Cast<UBlueprint>(LoadedAsset))
            {
                Result.IsBlueprint = true;
                // LoadedBlueprint->ParentClass is TSubclassOf<UObject>, which has
                // implicit conversion to UClass* — mirror the existing code in
                // UCkAssetRegistrySubsystem::Get_AssetTypeFromAssetData (no `*`).
                auto ParentClass = LoadedBlueprint->ParentClass;
                if (NOT ParentClass)
                { return Result; }

                auto* NativeParent = UCkAssetRegistrySubsystem::Get_NonBlueprintParentClass(ParentClass);
                if (NOT NativeParent)
                { return Result; }

                Result.ClassName = UCkAssetRegistrySubsystem::Get_CorrectClassNameWithPrefix(NativeParent);
                return Result;
            }

            // Non-Blueprint asset whose class is now loaded (because we
            // loaded the asset). Still walk parent chain — non-BP assets can
            // technically inherit from BP classes in edge cases.
            if (auto* AssetClass = LoadedAsset->GetClass())
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
        // produces a follow-up AS error pointing at the right line), blocking
        // loads do not (default-constructing the asset crashes worse).
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
        // BP class refs use AssetPath + "_C" — the generated class object lives
        // alongside the BP asset in the same package, named <AssetName>_C.
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
        // Body mirrors the real generator's shape from
        // UCkAssetRegistrySubsystem::GenerateAssetRegistryForConfig_Internal —
        // engine-init guard + delegating to the soft-ref accessor + System::LoadAsset_Blocking.
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

        if (NOT (InError.TargetNamespace == TEXT("assets")
                 || InError.TargetNamespace.StartsWith(TEXT("assets::"))
                 || InError.TargetNamespace.EndsWith(TEXT("::load"))))
        {
            // Not an asset-registry-shape error. Caller (dispatcher) should not
            // have routed this here, but defend.
            Result.ErrorMessage = FString::Printf(
                TEXT("Namespace '%s' does not look like an asset-registry accessor namespace."),
                *InError.TargetNamespace);
            return Result;
        }

        const auto Flavor = Classify_AccessorFlavor(InError);

        // ---- Discover matching UCkAssetRegistryConfig ----

        Force_FullArScan();
        const auto AllConfigs = UCkAssetRegistrySubsystem::Request_DiscoverAllConfigs();
        if (AllConfigs.IsEmpty())
        {
            Result.ErrorMessage = TEXT("No UCkAssetRegistryConfig data assets found after sync AR scan.");
            return Result;
        }

        const auto SoftNamespace = Strip_LoadSuffix(InError.TargetNamespace);

        auto* MatchedConfig = static_cast<UCkAssetRegistryConfig*>(nullptr);
        for (auto* Cfg : AllConfigs)
        {
            if (ck::IsValid(Cfg) && Cfg->Namespace == SoftNamespace)
            {
                MatchedConfig = Cfg;
                break;
            }
        }

        if (NOT MatchedConfig)
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("No UCkAssetRegistryConfig matched namespace '%s' (stripped from '%s'). ")
                TEXT("Available namespaces: %d configs discovered."),
                *SoftNamespace, *InError.TargetNamespace, AllConfigs.Num());
            return Result;
        }

        // ---- Find the asset whose function name matches the error ----

        // For _Class variants, search under the base name (the underlying
        // asset is the BP itself; "_Class" is just a different accessor on
        // the same asset).
        const auto BaseFunctionName = (Flavor == ECk_AssetAccessorFlavor::SoftClass)
            ? Strip_ClassSuffix(InError.FunctionName)
            : InError.FunctionName;

        const auto AssetData = Find_AssetByFunctionName(MatchedConfig->AssetDiscoveryRoot, BaseFunctionName);
        if (NOT AssetData.IsValid())
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("No asset matching function name '%s' found under '%s' (config '%s'). ")
                TEXT("The accessor may reference a deleted or renamed asset — manual cleanup required."),
                *BaseFunctionName, *MatchedConfig->AssetDiscoveryRoot, *MatchedConfig->GetDisplayName());
            return Result;
        }

        const auto AssetPath = AssetData.GetSoftObjectPath().ToString();
        Result.ResolvedAssetPath = AssetPath;

        // ---- Resolve the UClass return type (Tier 1/2/3) ----

        const auto Resolved = Resolve_AssetClass_Tier1_2(AssetData);
        auto       ClassName = Resolved.ClassName;

        if (ClassName.IsEmpty())
        {
            // Tier 3 fallback decision.
            if (NOT Tier3_IsAllowed(Flavor))
            {
                Result.ErrorMessage = FString::Printf(
                    TEXT("Could not resolve UClass for '%s' (asset path '%s'). ")
                    TEXT("Tier 3 fallback (UObject) is not permitted for blocking-load accessors — ")
                    TEXT("returning a default-constructed asset would crash worse than the current wedge. ")
                    TEXT("Manual recovery: force-quit editor, comment out the failing assets::load::%s() call site, ")
                    TEXT("relaunch, click 'Generate All Asset Registries', uncomment, save."),
                    *InError.FunctionName, *AssetPath, *InError.FunctionName);
                return Result;
            }

            ClassName = TEXT("UObject");
            Result.UsedTier3Fallback = true;
        }
        Result.ResolvedAssetClass = ClassName;

        // ---- Build the function body for the matched flavor ----

        auto FunctionBody = FString{};
        auto NamespaceToUse = InError.TargetNamespace;

        switch (Flavor)
        {
            case ECk_AssetAccessorFlavor::SoftRef:
                FunctionBody = Build_SoftRefAccessor(InError.FunctionName, ClassName, AssetPath);
                break;

            case ECk_AssetAccessorFlavor::SoftClass:
                FunctionBody = Build_SoftClassAccessor(InError.FunctionName, ClassName, AssetPath);
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

        const auto StubBlock = Build_NamespaceBlock(NamespaceToUse, FunctionBody, InError);

        // ---- Resolve target file path + atomic UTF-16 LE append ----

        const auto OutputDir = UCkAssetRegistrySubsystem::Get_OutputDirectoryForRootPath(MatchedConfig->AssetDiscoveryRoot);
        const auto OutputPath = OutputDir / MatchedConfig->OutputFileName;

        if (NOT IFileManager::Get().FileExists(*OutputPath))
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("Target file '%s' does not exist (expected from config '%s'). ")
                TEXT("Run 'Generate All Asset Registries' first to bootstrap the file."),
                *OutputPath, *MatchedConfig->GetDisplayName());
            return Result;
        }

        if (NOT Try_AtomicAppendUtf16(OutputPath, StubBlock))
        {
            Result.ErrorMessage = FString::Printf(
                TEXT("UTF-16 atomic append failed for target file '%s'."), *OutputPath);
            return Result;
        }

        Result.Success        = true;
        Result.TargetFilePath = OutputPath;
        Result.InjectedBlock  = StubBlock;

        // ---- Touch caller mtime to nudge hot-reload thread ----

        if (NOT InError.FilePath.IsEmpty()
            && IFileManager::Get().FileExists(*InError.FilePath))
        {
            IFileManager::Get().SetTimeStamp(*InError.FilePath, FDateTime::UtcNow());
        }

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
