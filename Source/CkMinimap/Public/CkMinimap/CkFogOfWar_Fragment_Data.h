#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkMinimap/CkMinimap_WorldBounds.h"

#include "CkFogOfWar_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Persistence payload for a FogOfWar grid (v3 rebuild+hydrate: Produce on save, HydrationApply on load).
// NAMING DEVIATION (deliberate): save-only payloads are normally FCk_SaveData_<Feature>, but persistence handlers
// key payloads by type path — renaming the struct later breaks old saves' payload mapping. Phase 4 commits this
// type to the net wire as well (co-op shared exploration via Register_NetAndSave_*), so it carries the
// FCk_RepData_ name from day one.
//
// BlueprintType (unlike FCk_RepData_Poi): this payload doubles as the PULL-SEED for fog painters
// (Get_ExploredData) and as the FCk_Request_FogOfWar_SetExplored payload, so BP/AS must be able to hold it.
USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_RepData_FogOfWar
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_RepData_FogOfWar);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _CellCountX = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _CellCountY = 0;

    // Bit-packed explored cells, LSB-first: cell index N lives in byte N/8, bit N%8.
    // Cell index = CellY * _CellCountX + CellX (row-major, X fastest).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<uint8> _PackedCells;

public:
    CK_PROPERTY(_CellCountX);
    CK_PROPERTY(_CellCountY);
    CK_PROPERTY(_PackedCells);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKMINIMAP_API FCk_Handle_FogOfWar : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_FogOfWar); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_FogOfWar);

// --------------------------------------------------------------------------------------------------------------------

// Batched payload for the OnCellsRevealed signal — one broadcast per fog entity per update, never per cell.
// A USTRUCT wrapper (not a bare TArray delegate param) because struct payloads are the proven delegate shape.
// Cell index = CellY * CellCountX + CellX; painters map indices to UVs via ck::minimap::Get_CellUVRect.
USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_FogOfWar_RevealedCells
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_FogOfWar_RevealedCells);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<int32> _CellIndices;

public:
    CK_PROPERTY_GET(_CellIndices);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_FogOfWar_RevealedCells, _CellIndices);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Fragment_FogOfWar_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_FogOfWar_ParamsData);

private:
    // World area the grid covers. The grid's max edges may overshoot the bounds by up to one cell
    // (cell counts are ceil(extent / cell size)) — Get_CellUVRect normalizes over the BOUNDS, not the grid.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Minimap_WorldBounds _Bounds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _CellSize = 500.0f;

    // World cm revealed around each revealer's position every update
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _RevealRadius = 2000.0f;

    // Time between revealer sampling passes. Zero = every frame.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _UpdateInterval = FCk_Time{0.25f};

public:
    CK_PROPERTY_GET(_Bounds);
    CK_PROPERTY(_CellSize);
    CK_PROPERTY(_RevealRadius);
    CK_PROPERTY(_UpdateInterval);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_FogOfWar_ParamsData, _Bounds);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Request_FogOfWar_AddRevealer : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_FogOfWar_AddRevealer);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_FogOfWar_AddRevealer);

private:
    // Entity whose Transform position reveals cells every update
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Revealer;

public:
    CK_PROPERTY_GET(_Revealer);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_FogOfWar_AddRevealer, _Revealer);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Request_FogOfWar_RemoveRevealer : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_FogOfWar_RemoveRevealer);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_FogOfWar_RemoveRevealer);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Revealer;

public:
    CK_PROPERTY_GET(_Revealer);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_FogOfWar_RemoveRevealer, _Revealer);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Request_FogOfWar_RevealLocation : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_FogOfWar_RevealLocation);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_FogOfWar_RevealLocation);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Location = FVector::ZeroVector;

    // 0 = use the fog's params RevealRadius
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _Radius = 0.0f;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY(_Radius);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_FogOfWar_RevealLocation, _Location);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Request_FogOfWar_RevealAll : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_FogOfWar_RevealAll);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_FogOfWar_RevealAll);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Request_FogOfWar_Reset : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_FogOfWar_Reset);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_FogOfWar_Reset);
};

// --------------------------------------------------------------------------------------------------------------------

// Bulk restore vehicle (hydration + tests). UNION semantics: ORs the payload's set cells into the grid — it never
// un-explores. Exact restore = Request_Reset first. Monotonic-union keeps hydration idempotent and preserves cells
// a replayed Construct already revealed (restored state rides the request FIFO AFTER Construct's seeds).
USTRUCT(BlueprintType)
struct CKMINIMAP_API FCk_Request_FogOfWar_SetExplored : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_FogOfWar_SetExplored);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_FogOfWar_SetExplored);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_RepData_FogOfWar _ExploredData;

public:
    CK_PROPERTY_GET(_ExploredData);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_FogOfWar_SetExplored, _ExploredData);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_FogOfWar_CellsRevealed,
    FCk_Handle_FogOfWar, InFogOfWar,
    FCk_FogOfWar_RevealedCells, InRevealedCells);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_FogOfWar_Reset,
    FCk_Handle_FogOfWar, InFogOfWar);

// --------------------------------------------------------------------------------------------------------------------
