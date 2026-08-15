#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "Templates/SharedPointer.h"

// --------------------------------------------------------------------------------------------------------------------

/** How a document was sourced. Purely descriptive — the inspection API never re-reads the source. */
enum class ECk_SnapshotInspection_SourceKind : uint8
{
    File,
    Slot,
    Bytes,
    SaveGameObject,
};

// Every failure a malformed / foreign / stale save can produce. None of them is an ensure: unreadable input is
// the expected diet of an offline inspector.
enum class ECk_SnapshotInspection_ReadStatus : uint8
{
    Success,
    FileNotFound,
    IoFailure,
    NotCkSnapshot,
    UnsupportedFormat,
    CorruptEnvelope,
    CorruptTables,
};

enum class ECk_SnapshotInspection_Compatibility : uint8
{
    Current,
    Older,
    Newer,
    Unknown,
};

enum class ECk_SnapshotInspection_DecodeStatus : uint8
{
    NotRequested,
    Decoded,
    TypeUnavailable,
    DeserializeFailed,
    DeclaredTypeMismatch,
    // Reserved for blobs whose decode needs a live object instance — FCk_Snapshot_V3_EntityEntry::_ActorSaveFieldBytes
    // is a class-scoped SerializeScriptProperties capture and has no offline route in v1.
    UnsupportedBlob,
};

// AvailableTypesOnly resolves a blob's struct type with FindObject and never loads a package; AllowTypeLoad lets
// the proxy archive fall through to LoadObject. This flag IS the whole "Try Load Type" mechanism.
enum class ECk_SnapshotInspection_DecodePolicy : uint8
{
    AvailableTypesOnly,
    AllowTypeLoad,
};

enum class ECk_SnapshotInspection_Severity : uint8
{
    Info,
    Warning,
    Error,
};

enum class ECk_SnapshotInspection_DiagnosticCode : uint8
{
    // ---- Intrinsic defects: the file is wrong regardless of the environment reading it ----
    HeaderCensusMismatch,
    DuplicateSavedId,
    // An owner id absent from the entity table is the format's only encoding of "owned by an entity
    // that was never persisted" — most commonly the saving world's transient root. Severity follows
    // the loader's per-provenance consequence: Info where the loader sanctions the state, Warning
    // where it skips or orphans the row.
    OwnerNotPersisted,
    ContextOwnerNotPersisted,
    OwnerCycle,
    DependentBeforeOwner,
    DuplicateSaveKey,
    DuplicateAdoptKey,
    ConstructSpawnedNoLabel,
    RuntimeSpawnedNoRecipe,
    DefinitionBuiltNoOwner,
    DefinitionBuiltNoUsableStep,
    PayloadOwnerMissing,
    PayloadBytesEmpty,
    PayloadTypePathEmpty,
    DuplicateRecipeIdentity,

    // ---- Environment observations: this editor cannot see a type/class the save names ----
    PayloadTypeUnavailable,
    ScriptClassUnavailable,
    ActorClassUnavailable,
    ArchetypeUnavailable,
};

enum class ECk_SnapshotInspection_ValueKind : uint8
{
    Bool,
    Int,
    UInt,
    Float,
    Enum,
    String,
    Name,
    Text,
    ObjectPath,
    SoftObjectPath,
    Struct,
    Array,
    Set,
    Map,
    SavedEntityRef,
    Unsupported,
    Truncated,
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    // Bounds on the projected value tree. A blob is untrusted input: without these a hostile or merely huge
    // payload turns an inspector click into an unbounded walk.
    namespace inspection_limits
    {
        constexpr auto MaxDepth                 = 8;
        constexpr auto MaxChildrenPerCollection = 256;
        constexpr auto MaxTextLen               = 1024;
        constexpr auto MaxTotalNodes            = 10000;
    }

    constexpr auto k_NoSavedEntity = 0xFFFFFFFFu;
}

// --------------------------------------------------------------------------------------------------------------------

class CKSNAPSHOT_API FCk_SnapshotInspection_Diagnostic
{
public:
    CK_GENERATED_BODY(FCk_SnapshotInspection_Diagnostic);

private:
    ECk_SnapshotInspection_Severity       _Severity = ECk_SnapshotInspection_Severity::Info;
    ECk_SnapshotInspection_DiagnosticCode _Code = ECk_SnapshotInspection_DiagnosticCode::HeaderCensusMismatch;
    FString                               _Message;
    TArray<uint32>                        _RelatedSavedIds;
    TArray<int32>                         _RelatedPayloadIndices;

public:
    CK_PROPERTY(_Severity);
    CK_PROPERTY(_Code);
    CK_PROPERTY(_Message);
    CK_PROPERTY(_RelatedSavedIds);
    CK_PROPERTY(_RelatedPayloadIndices);
};

// --------------------------------------------------------------------------------------------------------------------

class CKSNAPSHOT_API FCk_SnapshotInspection_ValueNode
{
public:
    CK_GENERATED_BODY(FCk_SnapshotInspection_ValueNode);

private:
    ECk_SnapshotInspection_ValueKind _Kind = ECk_SnapshotInspection_ValueKind::Unsupported;
    FString                          _FieldName;
    FString                          _TypeName;
    FString                          _ValueText;
    // Enum kind only — the reflected value behind the name, so a consumer can sort/filter without re-parsing.
    int64                            _EnumUnderlyingValue = 0;
    // SavedEntityRef kind only. Raw entt id as written by the save; ck::snapshot::k_NoSavedEntity when unset.
    uint32                           _SavedId = ck::snapshot::k_NoSavedEntity;
    TArray<TSharedPtr<FCk_SnapshotInspection_ValueNode>> _Children;

public:
    CK_PROPERTY(_Kind);
    CK_PROPERTY(_FieldName);
    CK_PROPERTY(_TypeName);
    CK_PROPERTY(_ValueText);
    CK_PROPERTY(_EnumUnderlyingValue);
    CK_PROPERTY(_SavedId);
    CK_PROPERTY(_Children);
};

// --------------------------------------------------------------------------------------------------------------------

class CKSNAPSHOT_API FCk_SnapshotInspection_DecodeResult
{
public:
    CK_GENERATED_BODY(FCk_SnapshotInspection_DecodeResult);

private:
    ECk_SnapshotInspection_DecodeStatus _Status = ECk_SnapshotInspection_DecodeStatus::NotRequested;
    // What the save CLAIMED the blob holds (payload rows only — spawn params carry no declared path).
    FString                             _DeclaredTypePath;
    // What the stream actually produced. Both are reported on DeclaredTypeMismatch.
    FString                             _DecodedTypePath;
    FString                             _ErrorText;
    TSharedPtr<FCk_SnapshotInspection_ValueNode> _ValueTreeRoot;

public:
    CK_PROPERTY(_Status);
    CK_PROPERTY(_DeclaredTypePath);
    CK_PROPERTY(_DecodedTypePath);
    CK_PROPERTY(_ErrorText);
    CK_PROPERTY(_ValueTreeRoot);
};

// --------------------------------------------------------------------------------------------------------------------

class CKSNAPSHOT_API FCk_SnapshotInspection_EntitySummary
{
public:
    CK_GENERATED_BODY(FCk_SnapshotInspection_EntitySummary);

private:
    // Verbatim copy of the save's row — the decode entry points read _SpawnParamsBytes back off it, so the
    // document is self-sufficient and nothing has to re-open the source.
    FCk_Snapshot_V3_EntityEntry _Entry;
    int32   _TableIndex = INDEX_NONE;
    int32   _SpawnParamsByteCount = 0;
    int32   _ActorSaveFieldByteCount = 0;
    int32   _BuildStepCount = 0;
    FString _IdentityText;
    // True when any Error or Warning diagnostic names this row's saved id.
    bool    _HasProblems = false;

public:
    CK_PROPERTY(_Entry);
    CK_PROPERTY(_TableIndex);
    CK_PROPERTY(_SpawnParamsByteCount);
    CK_PROPERTY(_ActorSaveFieldByteCount);
    CK_PROPERTY(_BuildStepCount);
    CK_PROPERTY(_IdentityText);
    CK_PROPERTY(_HasProblems);
};

// --------------------------------------------------------------------------------------------------------------------

class CKSNAPSHOT_API FCk_SnapshotInspection_PayloadSummary
{
public:
    CK_GENERATED_BODY(FCk_SnapshotInspection_PayloadSummary);

private:
    FCk_Snapshot_V3_PayloadEntry _Entry;
    int32   _TableIndex = INDEX_NONE;
    int32   _ByteCount = 0;
    FString _PayloadHashHex;
    // FindObject probe at document-build time — never a LoadObject, so this is "visible to this editor now".
    bool    _TypeAvailable = false;
    bool    _HasProblems = false;

public:
    CK_PROPERTY(_Entry);
    CK_PROPERTY(_TableIndex);
    CK_PROPERTY(_ByteCount);
    CK_PROPERTY(_PayloadHashHex);
    CK_PROPERTY(_TypeAvailable);
    CK_PROPERTY(_HasProblems);
};

// --------------------------------------------------------------------------------------------------------------------

/** Counts computed from the parsed tables — the thing the header's own census is checked against. */
class CKSNAPSHOT_API FCk_SnapshotInspection_Census
{
public:
    CK_GENERATED_BODY(FCk_SnapshotInspection_Census);

private:
    int32 _EntityCount = 0;
    int32 _EngineOwnedCount = 0;
    int32 _ConstructSpawnedCount = 0;
    int32 _RuntimeSpawnedCount = 0;
    int32 _DefinitionBuiltCount = 0;
    int32 _PayloadCount = 0;

public:
    CK_PROPERTY(_EntityCount);
    CK_PROPERTY(_EngineOwnedCount);
    CK_PROPERTY(_ConstructSpawnedCount);
    CK_PROPERTY(_RuntimeSpawnedCount);
    CK_PROPERTY(_DefinitionBuiltCount);
    CK_PROPERTY(_PayloadCount);
};

// --------------------------------------------------------------------------------------------------------------------

/** The menu sidecar that sits beside a save, projected for offline reading.
 *
 *  The sidecar is a SEPARATE file (`<Slot>.meta`) — deliberately, so a menu listing N slots does not deserialize
 *  N world blobs to draw N rows — which means a reader that only opens the `.sav` never sees it. The Inspect_*
 *  file and slot routes resolve it automatically; the bytes and object routes cannot, because neither carries a
 *  location to resolve FROM.
 *
 *  A save with no sidecar is ORDINARY, not damaged: a bare Request_Save writes none, and neither did any build
 *  predating it. `_Found` false means "nothing to show", never "something is wrong". */
class CKSNAPSHOT_API FCk_SnapshotInspection_SlotMeta
{
public:
    CK_GENERATED_BODY(FCk_SnapshotInspection_SlotMeta);

private:
    bool    _Found = false;
    // Where the sidecar was resolved from — a sibling path or a slot name. Empty when none was looked for.
    FString _SourceDescription;

    FText     _Title;
    FDateTime _TimestampUTC = FDateTime{};
    FString   _WorldAssetPath;
    TMap<FName, FString> _CustomFields;

    // Raw PNG. Carried whole rather than as a decoded texture: this layer creates no UObjects, so the
    // presentation layer decodes it (ck::snapshot::slot_meta::Decode_PngAsTexture) and owns the result.
    TArray<uint8> _ScreenshotPng;
    int64   _ScreenshotByteCount = 0;
    FString _ScreenshotHashHex;

public:
    CK_PROPERTY(_Found);
    CK_PROPERTY(_SourceDescription);
    CK_PROPERTY(_Title);
    CK_PROPERTY(_TimestampUTC);
    CK_PROPERTY(_WorldAssetPath);
    CK_PROPERTY(_CustomFields);
    CK_PROPERTY(_ScreenshotPng);
    CK_PROPERTY(_ScreenshotByteCount);
    CK_PROPERTY(_ScreenshotHashHex);
};

// --------------------------------------------------------------------------------------------------------------------

/** Everything an offline reader can learn about one save without instantiating any of it.
 *
 *  Built once by an Inspect_* entry point and thereafter read-only by convention: decode results are returned
 *  separately and stored by the caller, so a document is safe to share. It holds no world, registry, subsystem
 *  or live handle — every entity reference in it is a raw saved id. */
class CKSNAPSHOT_API FCk_SnapshotInspection_Document
{
public:
    CK_GENERATED_BODY(FCk_SnapshotInspection_Document);

private:
    ECk_SnapshotInspection_SourceKind _SourceKind = ECk_SnapshotInspection_SourceKind::Bytes;
    FString _SourceDescription;
    // Zero for the slot / SaveGame-object routes: those never see the envelope bytes.
    int64   _SourceByteCount = 0;
    FString _SourceHashHex;
    // Set when the envelope loaded, even if the class was not ours — it is the only clue a foreign save offers.
    FString _SaveGameClassPath;

    bool                                _HeaderRecovered = false;
    FCk_Snapshot_HeaderV3               _Header;
    ECk_SnapshotInspection_ReadStatus   _ReadStatus = ECk_SnapshotInspection_ReadStatus::IoFailure;
    ECk_SnapshotInspection_Compatibility _Compatibility = ECk_SnapshotInspection_Compatibility::Unknown;

    int64   _SnapshotByteCount = 0;
    FString _SnapshotHashHex;
    bool    _TableParseAttempted = false;
    bool    _TableParseSucceeded = false;

    TArray<FCk_SnapshotInspection_EntitySummary>  _Entities;
    TArray<FCk_SnapshotInspection_PayloadSummary> _Payloads;
    TArray<FCk_SnapshotInspection_Diagnostic>     _Diagnostics;
    FCk_SnapshotInspection_Census                 _Census;
    FCk_SnapshotInspection_SlotMeta               _SlotMeta;

    TMap<uint32, int32> _SavedIdToEntityIndex;

public:
    CK_PROPERTY(_SourceKind);
    CK_PROPERTY(_SourceDescription);
    CK_PROPERTY(_SourceByteCount);
    CK_PROPERTY(_SourceHashHex);
    CK_PROPERTY(_SaveGameClassPath);
    CK_PROPERTY(_HeaderRecovered);
    CK_PROPERTY(_Header);
    CK_PROPERTY(_ReadStatus);
    CK_PROPERTY(_Compatibility);
    CK_PROPERTY(_SnapshotByteCount);
    CK_PROPERTY(_SnapshotHashHex);
    CK_PROPERTY(_TableParseAttempted);
    CK_PROPERTY(_TableParseSucceeded);
    CK_PROPERTY(_Entities);
    CK_PROPERTY(_Payloads);
    CK_PROPERTY(_Diagnostics);
    CK_PROPERTY(_Census);
    CK_PROPERTY(_SlotMeta);
    CK_PROPERTY(_SavedIdToEntityIndex);

public:
    auto Get_ErrorCount() const -> int32;
    auto Get_WarningCount() const -> int32;

    /** Index into Get_Entities(), or INDEX_NONE when the saved id is absent from the entity table. */
    auto TryGet_EntityIndexForSavedId(uint32 InSavedId) const -> int32;
};

// --------------------------------------------------------------------------------------------------------------------
