#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Asset -> emit-ready AS class-name resolution, shared by the canonical generator and the
// self-heal stub synthesizer. Both MUST resolve through the SAME tiers, in the declared order,
// or every regen flips the accessor type and the synth-cleanup loop never converges.
// Tier rationale and the BPGC walk-past rule: CkAngelscriptGenerator/Claude.md.

namespace ck::angelscriptgenerator
{
    struct CKANGELSCRIPTGENERATOR_API FCk_ResolvedAssetClass
    {
        // Emit-ready AS type name WITH prefix ("AActor"); empty means resolution failed.
        FString ClassName;

        // Null even on SUCCESS: during a failed AS compile Hazelight unregisters AS UClasses, so a
        // name can be derived from the import path alone. Class-level checks must tolerate null.
        UClass* ResolvedClass = nullptr;

        bool    IsBlueprint = false;
    };

    class CKANGELSCRIPTGENERATOR_API FCkAssetRegistry_ClassResolver
    {
    public:
        static auto Resolve_ViaLoadObject(const FString& InPackagePath) -> FCk_ResolvedAssetClass;

        // Fallback for assets LoadObject cannot construct — typically a WBP whose ParentClass is an
        // AS-defined UClass that is unregistered during an AS-compile failure.
        static auto Resolve_ViaAssetDataTag(const FString& InPackagePath) -> FCk_ResolvedAssetClass;

        // Last resort: walks the .uasset linker table directly, bypassing the AssetRegistry.
        // InPackagePath is FSoftObjectPath form — "/Game/X/Y/Asset.Asset".
        static auto Resolve_ViaPackageReader(const FString& InPackagePath) -> FCk_ResolvedAssetClass;
    };
}

// --------------------------------------------------------------------------------------------------------------------
