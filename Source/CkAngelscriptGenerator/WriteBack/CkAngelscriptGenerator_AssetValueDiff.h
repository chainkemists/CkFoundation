#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AccessorResolver.h"
#include "CkAngelscriptGenerator/WriteBack/CkAngelscriptGenerator_AssetBlockPatcher.h"

#include "UObject/StrongObjectPtr.h"

// --------------------------------------------------------------------------------------------------------------------

// Decides WHAT to write back, and to what expression.
//
// The patch-set predicate is "differs from the value the current file text produces" — NOT "differs
// from the class CDO". A hand-authored accessor line exists precisely because it differs from the
// CDO, and every canonical in-repo asset body populates a container via `.Add()`; CDO-diffing would
// therefore flag both on every write, regenerate lines nobody touched, and abort on the containers
// v1 defers. The baseline is built by re-running `__Init_<Name>` onto a scratch instance, so it is
// by construction exactly what the file text produces.
//
// "Differs from the class CDO" survives as a separate question, asked only to decide whether a
// property that IS in the patch set should have its line deleted rather than rewritten.
//
// Needs UObject reflection and the AngelScript module registry, but no editor UI — the toolbar and
// the dialog live in CkAngelscriptGenerator_AssetWriteBack.

namespace ck::angelscriptgenerator::write_back
{
    // Why one property could not be turned into an AngelScript expression. Every case carries its own
    // message: a partial write is destructive (the watcher's reload resets every non-instanced
    // property from the CDO), so the user has to be told exactly what blocked the whole write.
    struct CKANGELSCRIPTGENERATOR_API FCk_WriteBackUnresolved
    {
        FString PropertyPath; // `_Display._Icon`
        FString Detail;       // object path / container element type, when there is one
        FString Message;
        ECk_AccessorResolve_FailReason FailReason = ECk_AccessorResolve_FailReason::None;
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_AssetValueDiffContext
    {
        const TMap<FString, FCk_ScriptAccessorEntry>* Accessors = nullptr;
        bool          AnyProviderRegistered   = false;
        bool          TargetBlockIsEditorOnly = false;
        // Literal assets declared in the SAME .as file, which a body may reference by bare name.
        TSet<FString> SameFileLiteralAssetNames;
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_AssetValueDiffResult
    {
        // True only when every changed property resolved. A partial write is never offered by
        // default — see the header note above.
        bool Success = false;
        TArray<FCk_AssetBlockPatchEntry> Entries;
        TArray<FCk_WriteBackUnresolved>  Unresolved;
        // An FText in the patch set is emitted as a culture-invariant FText::FromString, losing its
        // localization namespace/key. Accepted for v1, surfaced in the confirmation dialog.
        bool HasLossyText = false;
        // Properties where the live object holds null but the file text produces a real reference.
        // A deliberate clear looks identical to an asset whose initializer failed to resolve its
        // reference at load time (an `__Init_` body that reaches the asset registry before it is
        // scanned leaves the live value null and is never healed). Write-back cannot tell those
        // apart, so it names them and lets the user decide rather than quietly erasing the line.
        TArray<FString> ClearedObjectReferences;
    };

    class CKANGELSCRIPTGENERATOR_API FCkAsAssetValueDiff
    {
    public:
        // A fresh instance of InClass with `__Init_<Name>` executed against it: exactly what the
        // current file text produces, computed at button press so it has no staleness window.
        // Returns null when the AngelScript function cannot be resolved or its execution fails.
        static auto Build_ScratchBaseline(
            UClass*        InClass,
            const FString& InAssetName) -> TStrongObjectPtr<UObject>;

        static auto Compute(
            const UObject*                   InLive,
            const UObject*                   InBaseline,
            const UObject*                   InClassDefaults,
            const FCk_AssetValueDiffContext& InContext) -> FCk_AssetValueDiffResult;

        // True when at least one property differs from the class CDO — the requester's button-enable
        // condition, and deliberately NOT the patch-set predicate.
        static auto Get_DiffersFromClassDefaults(
            const UObject* InLive) -> bool;

        // Skips transient and parameter properties, which are never authored in an asset body.
        static auto Get_IsWriteBackCandidate(
            const FProperty* InProperty) -> bool;
    };
}

// --------------------------------------------------------------------------------------------------------------------
