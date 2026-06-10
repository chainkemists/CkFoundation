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
        static auto Scan_ClassShape(
            const FString&         InClassName,
            const TArray<FString>& InScanRoots) -> FCk_AsClassShape;
    };
}

// --------------------------------------------------------------------------------------------------------------------
