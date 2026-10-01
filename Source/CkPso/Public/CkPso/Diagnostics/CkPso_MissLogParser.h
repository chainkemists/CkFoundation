#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <Containers/StringView.h>
#include <Containers/UnrealString.h>
#include <Misc/Optional.h>

#include "CkPso_MissLogParser.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Pso_MissKind : uint8
{
    Unknown,
    Graphics,
    Compute
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pso_MissKind);

// --------------------------------------------------------------------------------------------------------------------

/** The engine's own "Type:" token. Compute misses print the literal token "Compute" in the same field. */
UENUM(BlueprintType)
enum class ECk_Pso_MissType : uint8
{
    Unknown,
    ShadersOnly,
    MinimalPSOState,
    FullPSO,
    Compute
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pso_MissType);

// --------------------------------------------------------------------------------------------------------------------

/** The engine's "PSOPrecachingState:" token. Only these three reach the miss log; anything else maps to Unknown. */
UENUM(BlueprintType)
enum class ECk_Pso_MissPrecachingState : uint8
{
    Unknown,
    Missed,
    TooLate,
    Untracked
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pso_MissPrecachingState);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Which collector registry named the pass: a mesh pass ("MeshPassName", and the compute "PassName") or a global
 * PSO collector ("GlobalTypeName"). Global misses carry no material and no vertex factory.
 */
UENUM(BlueprintType)
enum class ECk_Pso_MissCollectorScope : uint8
{
    Unknown,
    MeshPass,
    Global
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pso_MissCollectorScope);

// --------------------------------------------------------------------------------------------------------------------

/** The engine writes a "Frame:" line only while a CSV profiler capture is running. */
UENUM(BlueprintType)
enum class ECk_Pso_MissCsvCapture : uint8
{
    NotCapturing,
    Capturing
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pso_MissCsvCapture);

// --------------------------------------------------------------------------------------------------------------------

/**
 * One "PSO PRECACHING MISS:" block as the engine logs it. Only the block's header is decoded; the free-form
 * diagnosis that may follow it is kept verbatim in _RawText.
 *
 * Strings hold the engine's tokens unchanged, including its placeholders for absent data: Material "Unknown",
 * VertexFactoryType "None", pass "Unknown". A string field is empty only when its line was not in the block.
 * _MaterialName is an asset name, not a path.
 */
USTRUCT(BlueprintType)
struct CKPSO_API FCk_Pso_MissRecord
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Pso_MissRecord);

    friend class FCk_Pso_MissLogParser;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Pso_MissKind _Kind = ECk_Pso_MissKind::Unknown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Pso_MissType _Type = ECk_Pso_MissType::Unknown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Pso_MissPrecachingState _PrecachingState = ECk_Pso_MissPrecachingState::Unknown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FString _MaterialName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FString _VertexFactoryTypeName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Pso_MissCollectorScope _CollectorScope = ECk_Pso_MissCollectorScope::Unknown;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FString _PassName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FString _MdcStatsCategory;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FString _VertexShaderHash;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FString _PixelShaderHash;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FString _GeometryShaderHash;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FString _MeshShaderHash;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FString _AmplificationShaderHash;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FString _ComputeShaderHash;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Pso_MissCsvCapture _CsvCapture = ECk_Pso_MissCsvCapture::NotCapturing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _CsvFrameNumber = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FString _RawText;

public:
    CK_PROPERTY_GET(_Kind);
    CK_PROPERTY_GET(_Type);
    CK_PROPERTY_GET(_PrecachingState);
    CK_PROPERTY_GET(_MaterialName);
    CK_PROPERTY_GET(_VertexFactoryTypeName);
    CK_PROPERTY_GET(_CollectorScope);
    CK_PROPERTY_GET(_PassName);
    CK_PROPERTY_GET(_MdcStatsCategory);
    CK_PROPERTY_GET(_VertexShaderHash);
    CK_PROPERTY_GET(_PixelShaderHash);
    CK_PROPERTY_GET(_GeometryShaderHash);
    CK_PROPERTY_GET(_MeshShaderHash);
    CK_PROPERTY_GET(_AmplificationShaderHash);
    CK_PROPERTY_GET(_ComputeShaderHash);
    CK_PROPERTY_GET(_CsvCapture);
    CK_PROPERTY_GET(_CsvFrameNumber);
    CK_PROPERTY_GET(_RawText);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Pure text -> FCk_Pso_MissRecord for the engine's PSO miss log (LogPSOMissInfo, PSOPrecacheValidation.cpp).
 * No globals, no engine state; safe to call from any thread.
 *
 * Rejection is a result, not an error: almost every line handed to the prefilter is not a miss, and a log file
 * cut mid-write legitimately holds a partial block, so nothing here ensures or logs.
 */
class CKPSO_API FCk_Pso_MissLogParser
{
public:
    /**
     * Cheap, case-sensitive substring check for the block marker. True does not guarantee TryParse succeeds;
     * a caller that sees true followed by an unset TryParse is looking at a truncated block or at a format the
     * engine changed, and should surface that rather than drop it.
     */
    static auto
    Get_IsMissBlock(
        FStringView InText) -> bool;

    /**
     * Decodes ONE miss message: either the bare text an FOutputDevice receives, or the same message as it sits in a
     * log file behind its "[time][frame]LogEngine: " prefix. Unset when the marker is absent or when any line the
     * engine always writes for that miss kind is missing.
     *
     * Only the header (the lines between the marker and the first blank line) is decoded, and the first occurrence
     * of a label wins: the engine's diagnosis sections repeat "Material:", "VertexFactoryType:" and "PassName:"
     * for the closest PRECACHED state, and those must never overwrite the missed one. _RawText runs from the
     * marker to the end of InText, so pass one message, not a whole log.
     */
    static auto
    TryParse(
        FStringView InText) -> TOptional<FCk_Pso_MissRecord>;

    /**
     * Type | Material | VertexFactoryType | Pass, built from the engine's own tokens so it is stable across builds and
     * engine sessions. Misses that differ only in precaching state or shader hashes share a key.
     */
    static auto
    Get_DedupeKey(
        const FCk_Pso_MissRecord& InRecord) -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
