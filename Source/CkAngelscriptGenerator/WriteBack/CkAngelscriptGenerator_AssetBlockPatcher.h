#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Surgical text patcher for AngelScript literal-asset blocks (`asset <Name> of <Type> { ... }`).
//
// The block body is arbitrary AS code — locals, `.Add()` loops, comments — that the preprocessor
// lifts verbatim into `void __Init_<Name>(<Type>)`. Whole-body regeneration would therefore destroy
// user code, so every write is a per-statement splice: replace one RHS, insert one line, or delete
// one statement. Everything unrecognised is left byte-identical.
//
// Pure text: no UObject, no editor. Callable headless so the automation tests can drive it directly.

namespace ck::angelscriptgenerator::write_back
{
    enum class ECk_AssetBlockPatch_FailReason : uint8
    {
        None,
        DeclarationNotFound,   // no `asset <Name> of <Type>` outside comments/strings
        BodyOpenNotFound,      // declaration matched but no `{` follows it
        BodyUnterminated,      // `{` found, matching `}` never reached
        NothingToPatch,        // caller passed an empty entry list
    };

    enum class ECk_AssetBlockPatch_Op : uint8
    {
        // Splice a new RHS between an existing statement's `=` and `;`.
        ReplaceValue,
        // Add a whole new statement line before the body's closing brace.
        InsertLine,
        // Drop an entire statement (all lines it spans, plus its terminator).
        DeleteLine,
    };

    // One emitted assignment. `SubPath` is empty for `_Prop = <Expression>;` and dotted for a struct
    // leaf (`_Prop._Field = <Expression>;`) — the shape AngelScript forces on any struct carrying a
    // UObject field, which rejects the positional constructor.
    struct CKANGELSCRIPTGENERATOR_API FCk_AssetBlockAssignment
    {
        FString SubPath;
        FString Expression; // RHS only, no trailing `;`
    };

    // What to do with every statement in the body whose left-hand root token is `PropertyName`.
    //
    // `Assignments` is replace-or-insert and NEVER authoritative: a sub-path absent from the list is
    // left untouched, because the diff only reports leaves the user actually changed. Removing a
    // value is the separate `Delete` flavour.
    struct CKANGELSCRIPTGENERATOR_API FCk_AssetBlockPatchEntry
    {
        FString PropertyName;
        TArray<FCk_AssetBlockAssignment> Assignments;
        bool    Delete = false;

        static auto Make_Assign(
            const FString&                          InPropertyName,
            const TArray<FCk_AssetBlockAssignment>& InAssignments) -> FCk_AssetBlockPatchEntry;

        static auto Make_Delete(
            const FString& InPropertyName) -> FCk_AssetBlockPatchEntry;
    };

    // One user-visible line change, for the confirmation dialog. Line numbers are 1-based against the
    // ORIGINAL file; an InsertLine reports the line it will be pushed in front of.
    struct CKANGELSCRIPTGENERATOR_API FCk_AssetBlockLineDiff
    {
        ECk_AssetBlockPatch_Op Op = ECk_AssetBlockPatch_Op::ReplaceValue;
        int32   LineNumber = 0;
        FString Before;
        FString After;
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_AssetBlockLocation
    {
        bool    Found = false;
        FString TypeName;
        int32   DeclStart = INDEX_NONE; // offset of the `a` in `asset`
        int32   BodyOpen  = INDEX_NONE; // offset of `{`
        int32   BodyClose = INDEX_NONE; // offset of the matching `}`
        int32   DeclLine  = 0;          // 1-based
        FString DeclIndent;             // leading whitespace of the declaration line
        ECk_AssetBlockPatch_FailReason FailReason = ECk_AssetBlockPatch_FailReason::None;
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_AssetBlockPatchResult
    {
        bool    Success = false;
        ECk_AssetBlockPatch_FailReason FailReason = ECk_AssetBlockPatch_FailReason::None;
        FString PatchedContents; // whole-file text, ready to write (empty on failure)
        TArray<FCk_AssetBlockLineDiff> Diff;
        FString ErrorMessage;
        // Properties whose value the caller wanted REMOVED but for which no statement exists at body
        // top level — because the value is produced somewhere the patcher deliberately does not
        // touch (inside an `if`, a loop, a helper call). Deleting nothing here is not harmless: the
        // caller would write an unchanged file, the watcher would reload it, and the initializer
        // would put the old value straight back over the user's revert. The caller must abort.
        TArray<FString> UnmatchedDeletes;
    };

    // Byte-for-byte identity of a file as read, so a patch computed at button press can be proven
    // still current at confirm time, and re-encoded without churning the whole file.
    struct CKANGELSCRIPTGENERATOR_API FCk_AsFileSnapshot
    {
        bool    Loaded = false;
        FString AbsolutePath;
        FString Contents;
        bool    HadUtf8Bom = false;
        FString LineTerminator = TEXT("\n");
        FString ErrorMessage; // populated when Loaded == false
    };

    class CKANGELSCRIPTGENERATOR_API FCkAsAssetBlockPatcher
    {
    public:
        // SAME-LENGTH copy with newlines preserved, so every index into the result still addresses
        // the original text. Mirrors the self-heal scanner's helper; duplicated rather than shared
        // because that one is file-local and this module must stay independently testable.
        static auto Blank_CommentsAndStrings(
            const FString& InText) -> FString;

        // Matches the preprocessor's own pattern (`AngelscriptPreprocessor.cpp:3953`), which requires
        // the declaration to end its line — the `{` is always on the next one. Comment and string
        // matches are impossible because the search runs over the blanked copy.
        static auto Find_AssetBlock(
            const FString& InFileContents,
            const FString& InAssetName) -> FCk_AssetBlockLocation;

        // Every literal asset declared in this file, in declaration order. These are the only ones a
        // body may reference by bare name — cross-file references need a hand-authored namespace
        // wrapper, which is not machine-derivable.
        static auto Find_AllAssetDeclarations(
            const FString& InFileContents) -> TArray<FString>;

        static auto Apply_Patch(
            const FString&                          InFileContents,
            const FString&                          InAssetName,
            const TArray<FCk_AssetBlockPatchEntry>& InEntries) -> FCk_AssetBlockPatchResult;

        // Whether the offset sits inside an `#if EDITOR` region. Gates whether an editor-only
        // accessor may be emitted into this block — writing one into unguarded source breaks the
        // cooked build.
        static auto Get_IsInsideEditorGuard(
            const FString& InFileContents,
            int32          InOffset) -> bool;

        // Dominant terminator across the file; LF only when no CRLF appears anywhere.
        static auto Get_LineTerminator(
            const FString& InFileContents) -> FString;

        static auto Try_ReadSnapshot(
            const FString&      InPath,
            FCk_AsFileSnapshot& OutSnapshot) -> bool;

        // Temp + move, mirroring the self-heal synthesizer. Re-emits the BOM only when the snapshot
        // carried one — ForceUTF8WithoutBOM on a BOM'd file would rewrite every byte of it.
        static auto Try_AtomicWrite(
            const FCk_AsFileSnapshot& InSnapshot,
            const FString&            InContents) -> bool;
    };
}

// --------------------------------------------------------------------------------------------------------------------
