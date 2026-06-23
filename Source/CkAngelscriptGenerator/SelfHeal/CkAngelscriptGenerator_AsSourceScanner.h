#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// AS source scanner for the self-heal dispatcher's SOURCE-DERIVED stub
// synthesis (the wholesale-missing EntitySpawnParams case a gitignored
// generated file creates on every fresh clone).
//
// During a failed AS compile, AS-defined entity-script classes are invisible
// to UObjectIterator/reflection — but their DECLARING .as source files are
// plain files on disk. This scanner finds `class U<X>` in the project +
// plugin Script/ trees and textually parses its `UPROPERTY(ExposeOnSpawn)`
// members (verbatim type text, names, declaration order), walking the base
// chain: AS bases recurse through more source scans; the first C++ base
// switches to reflection (C++ classes ARE registered during a failed
// compile) via UCk_Utils_Reflection_UE::Get_ExposedPropertiesOfClass.
//
// The output mirrors the property set + ordering of the real generator
// (CkAngelscriptEntityScriptParamsGenerator), which walks base -> derived
// with per-class declaration order — positional Params(...) argument order
// depends on this. Defaults are deliberately NOT captured: a stub only has
// to compile; the canonical regen replaces it seconds after the first
// successful compile.
//
// Known textual-parse limitations (full fidelity belongs to the real
// generator): #if blocks inside class bodies are parsed as-is; multicast
// (event) delegate members are not excluded (none exist on ExposeOnSpawn
// entity-script properties in practice — and a shape mismatch self-corrects
// via the per-signature error-text fallback path).

namespace ck::angelscriptgenerator::self_heal
{
    struct CKANGELSCRIPTGENERATOR_API FCk_AsExposedProperty
    {
        FString TypeText; // verbatim AS type as declared, whitespace-collapsed (e.g. "TMap<FGameplayTag, float32>")
        FString Name;
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_AsClassParseResult
    {
        bool    Found = false;
        FString ClassName;
        FString SourceFilePath;  // declaring .as (empty for reflection-resolved C++ classes)
        FString BaseClassName;   // empty when the declaration has no base
        TArray<FCk_AsExposedProperty> ExposedProperties; // this class's own, declaration order
        FString ErrorMessage;    // populated when Found == false
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_AsClassShape
    {
        bool    Found = false;
        FString ClassName;
        FString SourceFilePath;  // root class's declaring .as
        TArray<FCk_AsExposedProperty> FlattenedProperties; // base-first across the whole chain
        FString ErrorMessage;    // populated when Found == false
    };

    // Drain-scoped memoization for Scan_ClassShape (QW1). A self-heal drain
    // resolves one class shape PER DRIFT (and one per class during a quarantine
    // resynthesis) — each call otherwise re-runs the full recursive *.as
    // enumeration AND cold-re-reads files until the class matches, for the same
    // unchanging on-disk tree. Threading one cache through a drain collapses N
    // tree walks + redundant reads into a single walk + read-once-per-file.
    //
    // LIFETIME CONTRACT: the cache MUST live no longer than a single drain. The
    // drain handler owns it as a stack local, so a subsequent compile cycle
    // (whose on-disk source may have changed) re-enumerates from scratch —
    // never reusing a shape that would mismatch what the post-compile generator
    // will later emit (which would defeat the canonical diff-skip and re-trigger
    // a recompile). The cache also assumes a STABLE root set for its lifetime
    // (every call in one drain passes the same Get_DefaultScanRoots() result).
    struct CKANGELSCRIPTGENERATOR_API FCk_AsSourceScanCache
    {
        TArray<FString>        Files;                   // enumerated *.as, built once
        bool                   FilesEnumerated = false; // guards the one-time enumeration
        TMap<FString, FString> ContentsByPath;          // lazily loaded; empty value = load failed / empty file
    };

    class CKANGELSCRIPTGENERATOR_API FCkAsSourceScanner
    {
    public:
        // Project Script/ + every enabled plugin's Script/. Mirrors the
        // dispatcher's candidate collection; results are sorted for
        // deterministic first-match behavior.
        static auto Get_DefaultScanRoots() -> TArray<FString>;

        // Recursively enumerates *.as under the roots, excluding Generated/
        // subtrees and _StubRecovery_* files. Sorted.
        static auto Enumerate_AsSourceFiles(const TArray<FString>& InScanRoots) -> TArray<FString>;

        // Pure-text parse of one file's contents for `class <InClassName>
        // [: Base]` and its depth-1 UPROPERTY(ExposeOnSpawn) members.
        // Comment- and string-literal-aware (contents are blanked before
        // structural scanning). Found == false when the class is not
        // declared in the text.
        static auto Parse_ClassDeclaration(
            const FString& InFileContents,
            const FString& InClassName,
            const FString& InSourcePathForDiagnostics) -> FCk_AsClassParseResult;

        // Full flattened walk for U<X>: source-scan the class, recurse AS
        // bases, switch to reflection at the first base with no .as
        // declaration (C++ boundary; its own supers come flattened from
        // reflection). Found == false when the root class or any link of
        // the chain resolves neither way — callers fall back to the
        // error-text stub path.
        //
        // InCache (optional, drain-scoped): when supplied, the *.as
        // enumeration and per-file contents are shared across every call that
        // passes the same cache, eliminating the per-drift tree re-walk +
        // cold re-reads. nullptr reproduces the original walk-every-time
        // behavior (tests, one-off callers). See FCk_AsSourceScanCache.
        static auto Scan_ClassShape(
            const FString&         InClassName,
            const TArray<FString>& InScanRoots,
            FCk_AsSourceScanCache* InCache = nullptr) -> FCk_AsClassShape;
    };
}

// --------------------------------------------------------------------------------------------------------------------
