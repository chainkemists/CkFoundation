#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Textual .as class scanner backing SOURCE-DERIVED stub synthesis: during a
// failed AS compile the classes are invisible to reflection, their declaring
// source files are not. The property set and base-first ordering MUST mirror
// the real generator (CkAngelscriptEntityScriptParamsGenerator) — positional
// Params(...) argument order depends on it. See CkAngelscriptGenerator/CLAUDE.md.

namespace ck::angelscriptgenerator::self_heal
{
    struct CKANGELSCRIPTGENERATOR_API FCk_AsExposedProperty
    {
        FString TypeText; // verbatim as declared, whitespace-collapsed (e.g. "TMap<FGameplayTag, float32>")
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

    // Drain-scoped memoization for Scan_ClassShape.
    //
    // LIFETIME CONTRACT: MUST live no longer than a single drain (the drain
    // handler owns it as a stack local). A shape cached across compile cycles
    // could mismatch what the post-compile generator later emits, defeating the
    // canonical diff-skip. Also assumes a STABLE root set for its lifetime.
    struct CKANGELSCRIPTGENERATOR_API FCk_AsSourceScanCache
    {
        TArray<FString>        Files;
        bool                   FilesEnumerated = false;
        TMap<FString, FString> ContentsByPath;          // lazily loaded; empty value = load failed / empty file
    };

    class CKANGELSCRIPTGENERATOR_API FCkAsSourceScanner
    {
    public:
        // Project Script/ + every enabled plugin's Script/, sorted for
        // deterministic first-match behavior.
        static auto Get_DefaultScanRoots() -> TArray<FString>;

        // Excludes Generated/ subtrees and _StubRecovery_* files. Sorted.
        static auto Enumerate_AsSourceFiles(const TArray<FString>& InScanRoots) -> TArray<FString>;

        // Comment- and string-literal-aware (contents are blanked before
        // structural scanning). Found == false when the class is not declared
        // in the text.
        static auto Parse_ClassDeclaration(
            const FString& InFileContents,
            const FString& InClassName,
            const FString& InSourcePathForDiagnostics) -> FCk_AsClassParseResult;

        // Recurses AS bases, switching to reflection at the first base with no
        // .as declaration (the C++ boundary — its own supers come flattened
        // from reflection). Found == false when the root class or any link of
        // the chain resolves neither way; callers fall back to the error-text
        // stub path. InCache is optional: nullptr walks the tree every call.
        static auto Scan_ClassShape(
            const FString&         InClassName,
            const TArray<FString>& InScanRoots,
            FCk_AsSourceScanCache* InCache = nullptr) -> FCk_AsClassShape;
    };
}

// --------------------------------------------------------------------------------------------------------------------
