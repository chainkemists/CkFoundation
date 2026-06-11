#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Asset -> emit-ready AS class-name resolution, shared by the canonical
// UCkAssetRegistryConfig generator (sync-resolve path) and the self-heal
// dispatcher's AssetRegistry stub synthesizer (Tiers 2/2.5/2.6).
//
// Both consumers MUST resolve through the same tiers — if the canonical
// generator and self-heal disagree on an asset's class, every regen flips the
// accessor type back and forth and the dispatcher's synth-cleanup loop never
// converges.
//
// Tier overview (cheapest viable first; callers pick the tiers they need):
//   * ViaLoadObject    — LoadObject + native-parent walk. Works at modal-tick
//                        for any asset whose native class module is loaded.
//   * ViaAssetDataTag  — AR point-query (GetAssetByObjectPath bypasses the
//                        IsEngineStartupModuleLoadingComplete gate) reading
//                        NativeParentClassPath/ParentClassPath tags. No
//                        class-load needed; fails when AR cached "None".
//   * ViaPackageReader — direct .uasset linker-table walk. AR-free; the only
//                        tier that works when AR's cache is poisoned AND the
//                        asset can't be constructed.
//
// Every tier emits names that resolve as AS identifiers: Blueprint-generated
// classes in the parent chain are walked past (via Get_NonBlueprintParentClass)
// to the nearest native or AS-script ancestor — a BPGC name is not an AS type
// and would make the emitted accessor fail AS compile.

namespace ck::angelscriptgenerator
{
    struct CKANGELSCRIPTGENERATOR_API FCk_ResolvedAssetClass
    {
        // Emit-ready AS type name with prefix (e.g. "AActor", "UUserWidget").
        // Empty when resolution failed.
        FString ClassName;

        // The live UClass the name was derived from. Null when the name was
        // derived WITHOUT a live UClass (AS reload window — Hazelight
        // unregisters AS UClasses during failed compile, so PathA's AS-derive
        // branch produces a name from the import path alone). Callers needing
        // class-level checks (e.g. editor-only detection) must tolerate null.
        UClass* ResolvedClass = nullptr;

        bool    IsBlueprint = false;
    };

    class CKANGELSCRIPTGENERATOR_API FCkAssetRegistry_ClassResolver
    {
    public:
        // LoadObject<UObject>(InPackagePath) + class walk. ClassName empty when
        // LoadObject returns nullptr or the loaded UBlueprint's native parent
        // can't be resolved (parent module not up yet).
        static auto Resolve_ViaLoadObject(const FString& InPackagePath) -> FCk_ResolvedAssetClass;

        // AssetData tag fallback when LoadObject can't construct the asset
        // (most often: WBPs whose ParentClass is an AS-defined UClass that
        // isn't registered during AS-compile failure).
        static auto Resolve_ViaAssetDataTag(const FString& InPackagePath) -> FCk_ResolvedAssetClass;

        // Last-resort linker walk on the .uasset directly. Bypasses AR
        // entirely. Path A reads the generated-class export's SuperIndex;
        // Path B scans the import table for Class-typed AS imports.
        // InPackagePath: "/Game/X/Y/Asset.Asset" (FSoftObjectPath form).
        static auto Resolve_ViaPackageReader(const FString& InPackagePath) -> FCk_ResolvedAssetClass;
    };
}

// --------------------------------------------------------------------------------------------------------------------
