#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Resolves an object reference to the `assets::` accessor expression that reaches it from AngelScript.
//
// The source of truth is the GENERATED `.as` accessor files, parsed fresh — not the subsystem's
// in-memory `AssetPathToFunctionName` (no namespace per entry, and it is Reset() per config) and not
// a recomputation from discovery (`_DUP{N}` dedup resolves against sorted scan order, so an accessor
// name is not derivable from an asset name). The generated file is the only statement of what
// actually compiles today.
//
// Pure text: no UObject, no editor. Callable headless so the automation tests can drive it directly.

namespace ck::angelscriptgenerator::write_back
{
    enum class ECk_ScriptAccessorKind : uint8
    {
        SoftObject, // TSoftObjectPtr<T>
        HardObject, // T* / TObjectPtr<T>
        SoftClass,  // TSoftClassPtr<T>
        HardClass,  // TSubclassOf<T> / UClass*
    };

    enum class ECk_AccessorResolve_FailReason : uint8
    {
        None,
        // No asset-reference provider is registered at all — the generated files were never produced.
        // Reported separately from NoAccessorFound so the user is told to run generation rather than
        // hunting for a missing asset. See CkAssetReferenceProvider.h's Get_HasAnyProvider() doctrine.
        NoProviderRegistered,
        NoAccessorFound,
        NoClassAccessor,
        EditorOnlyAccessorFromRuntimeBlock,
        CrossFileLiteralAsset,
        UnsupportedPropertyKind,
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_ScriptAccessorEntry
    {
        FString ObjectPath;   // `/CkTests/Characters/.../SK_Mannequin.SK_Mannequin`
        FString Namespace;    // `assets`
        FString FunctionName; // `SK_Mannequin`, already `_DUP{N}`-resolved by the generator
        FString SourceFile;   // the generated .as it was parsed from, for collision diagnostics
        bool    HasClassAccessor = false;
        bool    IsEditorOnly     = false;
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_AccessorResolveResult
    {
        bool    Success = false;
        FString Expression;
        ECk_AccessorResolve_FailReason FailReason = ECk_AccessorResolve_FailReason::None;
        FString ErrorMessage;
    };

    // One generated accessor file, as handed to the resolver. The orchestration layer produces these
    // from UCkAssetRegistrySubsystem::Request_DiscoverAllConfigs(), which keeps this class headless.
    struct CKANGELSCRIPTGENERATOR_API FCk_GeneratedAccessorFile
    {
        FString AbsolutePath;
        FString Contents;
        FString FallbackNamespace; // used only when the file declares none
    };

    class CKANGELSCRIPTGENERATOR_API FCkAsAccessorResolver
    {
    public:
        // Extends the shape `SeedMapsFromGeneratedFiles` already parses
        // (`CkAssetRegistrySubsystem.cpp:1189-1243`) with the three things it throws away: the
        // declaring namespace, whether a `_Class` sibling exists, and whether the entry sits inside
        // an `#if Editor` guard.
        static auto Parse_GeneratedAccessorFile(
            const FCk_GeneratedAccessorFile& InFile) -> TArray<FCk_ScriptAccessorEntry>;

        // First entry wins per object path, so a stale duplicate in a later file cannot displace the
        // live one. Callers pass files in sorted-path order to make that deterministic.
        static auto Build_Index(
            const TArray<FCk_ScriptAccessorEntry>& InEntries) -> TMap<FString, FCk_ScriptAccessorEntry>;

        static auto Resolve(
            const TMap<FString, FCk_ScriptAccessorEntry>& InIndex,
            bool                                          InAnyProviderRegistered,
            const FString&                                InObjectPath,
            ECk_ScriptAccessorKind                        InKind,
            bool                                          InTargetBlockIsEditorOnly) -> FCk_AccessorResolveResult;

        // Blueprint class paths carry a trailing `_C` that the generated soft entry does not.
        static auto Strip_ClassPathSuffix(
            const FString& InObjectPath) -> FString;
    };
}

// --------------------------------------------------------------------------------------------------------------------
