#pragma once

#include <CoreMinimal.h>
#include <Delegates/Delegate.h>
#include <UObject/SoftObjectPath.h>

// --------------------------------------------------------------------------------------------------------------------

/**
 * One place a reference to an asset lives that the on-disk package dependency graph structurally CANNOT see.
 *
 * The graph only records edges a package serialized. A reference resolved at runtime from text — an AngelScript
 * generated accessor, a config-driven soft path, a string assembled in a Blueprint — leaves no edge, so any tool that
 * asks the graph alone will report the asset as unreferenced and offer it for deletion. That answer is not merely
 * incomplete; it is confidently wrong, and the reader's usual safety net (the engine's delete dialog) derives its
 * referencer list from the SAME graph and so agrees with it.
 *
 * Plain public fields, matching the other non-reflected transport structs in this codebase: nothing here is a fragment
 * or a reflected params shape, and a `CK_PROPERTY_GET` on an unreflected type would emit an AngelScript registration
 * for a type AngelScript has never heard of.
 */
struct CKCORE_API FCk_AssetReference_External
{
    // Which provider found it. Printed beside the count, so it names a thing the reader can go look at
    // ("AngelScript"), never an implementation detail.
    FName SourceId;

    // What holds the reference, in the words the reader needs to go find it — a file path, a config key. Sorted by
    // the provider, because a list that reordered between two identical queries is one nobody can diff.
    TArray<FString> Referencers;
};

// --------------------------------------------------------------------------------------------------------------------

/** Answers "who outside the package graph points at this asset?". Returns the referencer descriptions, or an empty
 *  array when the provider knows of none. Called on the game thread, once per candidate asset during a scan. */
DECLARE_DELEGATE_RetVal_OneParam(
    TArray<FString>,
    FCk_Delegate_AssetReference_Query,
    const FSoftObjectPath& /*InAsset*/);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Process-wide registry of external asset-reference providers.
 *
 * This is an inversion: a module that CREATES references the package graph cannot see (today, `CkAngelscriptGenerator`
 * and its generated `assets::` accessors) declares that capability here, and any tool that reasons about whether an
 * asset is reachable asks the registry. Neither side links the other, which is what lets an editor-only codegen module
 * inform a DeveloperTool auditor without either learning the other exists.
 *
 * **`Has_AnyProvider()` is load-bearing and is not the same question as an empty `Query`.** "Nobody could answer" and
 * "everybody answered no" are different statements, and a consumer that collapsed them would report a project it never
 * asked about as clean — the same defect as printing `0 B` for a size that was never measured. Ask it before drawing a
 * conclusion from silence.
 *
 * Plain C++ plumbing with no reflected surface, matching `FCk_PersistenceHandlerRegistry`: the payload is a C++
 * delegate, which has no meaningful Blueprint or AngelScript spelling. A reader-facing query belongs in a
 * `UCk_Utils_*_UE` wrapper over this, not on the registry itself.
 *
 * Game thread only. Registration happens at module startup and queries happen inside a scan; both are game-thread, so
 * there is no lock and an `IsInGameThread` ensure pins it.
 */
class CKCORE_API FCk_AssetReferenceProviderRegistry
{
public:
    static auto
    Get() -> FCk_AssetReferenceProviderRegistry&;

public:
    /** Register or REPLACE the provider under `InSourceId`. Replacing is deliberate: a module that re-registers after
     *  a reload must end up with one entry, not two that disagree. */
    auto
    Request_Register(
        FName InSourceId,
        FCk_Delegate_AssetReference_Query InQuery) -> void;

    /** Safe to call when nothing is registered under that id — a module's shutdown must not depend on whether its
     *  startup got far enough to register. */
    auto
    Request_Unregister(
        FName InSourceId) -> void;

public:
    /** Every provider that reported at least one referencer, in registration-id order. Providers that know of none are
     *  omitted rather than returned empty: a caller iterating the result is asking "who points at this", and an entry
     *  that answers "nobody" is not an answer to that question. */
    auto
    Get_ExternalReferences(
        const FSoftObjectPath& InAsset) const -> TArray<FCk_AssetReference_External>;

    /** Whether ANY provider is registered. See the class comment — this is the difference between "asked and got
     *  nothing" and "nobody was there to ask". */
    auto
    Get_HasAnyProvider() const -> bool;

private:
    struct FProvider
    {
        FName SourceId;
        FCk_Delegate_AssetReference_Query Query;
    };

private:
    auto
    TryFind_Provider(
        FName InSourceId) -> FProvider*;

private:
    // A sorted array rather than a `TMap`: the provider count is single-digit, and a map's iteration order follows its
    // hash layout, so every projection over it would need re-sorting to be deterministic anyway.
    TArray<FProvider> _Providers;
};

// --------------------------------------------------------------------------------------------------------------------
