#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "UObject/SoftObjectPath.h"

#include "CkSnapshot_Header.generated.h"

USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_Header_FragmentManifestEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_Header_FragmentManifestEntry);

private:
    UPROPERTY() FString  _DisplayName;
    UPROPERTY() uint32   _EnttTypeHash = 0;
    // The entt storage id this entry was captured from. For most fragments this equals _EnttTypeHash (the default
    // storage). Dynamic fragments (CkDynamic) keep one NAMED storage per AS struct type, all of value-type
    // FCk_Fragment_DynamicFragment_Data -- so several entries share _EnttTypeHash but carry distinct _StorageId.
    // Restore replays each entry onto ITS storage id via entt's id-parameterized get<T>(archive, id).
    UPROPERTY() uint32   _StorageId    = 0;
    UPROPERTY() int32    _EntityCount  = 0;
    UPROPERTY() int64    _ByteLength   = 0;
    UPROPERTY() int64    _ByteOffset   = 0;

public:
    CK_PROPERTY(_DisplayName);
    CK_PROPERTY(_EnttTypeHash);
    CK_PROPERTY(_StorageId);
    CK_PROPERTY(_EntityCount);
    CK_PROPERTY(_ByteLength);
    CK_PROPERTY(_ByteOffset);

    CK_DEFINE_CONSTRUCTORS(FCk_Snapshot_Header_FragmentManifestEntry, _DisplayName, _EnttTypeHash, _EntityCount, _ByteLength, _ByteOffset);
};

USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_Snapshot_Header
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Snapshot_Header);

private:
    UPROPERTY() uint16              _FormatVersion = 1;
    // Stored as string (FEngineVersion is not a USTRUCT and cannot be a UPROPERTY).
    // Populated via FEngineVersion::Current().ToString() on save.
    UPROPERTY() FString             _EngineVersion;
    UPROPERTY() FGuid               _PluginBuildHash;
    UPROPERTY() FDateTime           _TimestampUTC;
    UPROPERTY() FSoftObjectPath     _WorldAssetPath;
    UPROPERTY() TArray<FCk_Snapshot_Header_FragmentManifestEntry> _Manifest;
    UPROPERTY() int32               _EntityCount = 0;
    // Raw entt id (underlying uint32) of the registry's transient entity at capture time. Consumed only by the
    // live-world restore to adopt the restored transient. 0xFFFFFFFF == entt::null (registry-core captures with
    // no transient ctx leave it at the sentinel; that path never adopts).
    UPROPERTY() uint32              _TransientEntityId = 0xFFFFFFFFu;
    // Byte offset (within the snapshot stream) of the self-describing tag section, appended after all fragment
    // passes. Restore Seeks here rather than trusting the stream position, since the fragment manifest byte-jumps.
    UPROPERTY() int64               _TagSectionByteOffset = 0;

public:
    CK_PROPERTY(_FormatVersion);
    CK_PROPERTY(_EngineVersion);
    CK_PROPERTY(_PluginBuildHash);
    CK_PROPERTY(_TimestampUTC);
    CK_PROPERTY(_WorldAssetPath);
    CK_PROPERTY(_Manifest);
    CK_PROPERTY(_EntityCount);
    CK_PROPERTY(_TransientEntityId);
    CK_PROPERTY(_TagSectionByteOffset);
};
