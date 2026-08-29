#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"
#include "CkCore/Time/CkTime.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkVisualLodArbiter_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_IskmAnimCollection_Data;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_VisualLod_PoolExhaustionPolicy : uint8
{
    // A managed entity that cannot get a crowd slot is promoted instead — exactly the pre-LOD cost,
    // never an invisible entity
    PromoteInstead,

    // The entity stays unrendered until a slot frees. An ensure fires on first exhaustion
    Unrendered
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VisualLod_PoolExhaustionPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKVISUALLOD_API FCk_Handle_VisualLodArbiter : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_VisualLodArbiter); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_VisualLodArbiter);

// --------------------------------------------------------------------------------------------------------------------

// One batched crowd the arbiter can park far entities in. Members reference a config by index
// (FCk_Fragment_VisualLod_ParamsData::_CrowdIndex)
USTRUCT(BlueprintType)
struct CKVISUALLOD_API FCk_VisualLod_CrowdConfig
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_VisualLod_CrowdConfig);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCk_IskmAnimCollection_Data> _AnimCollection;

    // Fixed member-pool size — the batched crowd cannot grow after Finalize
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1))
    int32 _PoolSize = 192;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 100))
    float _TileSize = 2000.0f;

    // ---- speed-driven far animation (ECk_VisualLod_FarAnimMode::SpeedDriven) ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Far Animation",
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    int32 _IdleSequenceIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Far Animation",
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    int32 _MoveSequenceIndex = 1;

    // Planar speed (uu/s) above which the move sequence plays instead of idle
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Far Animation",
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _MoveSpeedThreshold = 25.0f;

    // The speed (uu/s) the move sequence was authored at — playback rate = speed / this, clamped
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Far Animation",
              meta = (AllowPrivateAccess = true, ClampMin = 1))
    float _MoveAuthoredSpeed = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Far Animation",
              meta = (AllowPrivateAccess = true))
    FCk_FloatRange _MoveRateClamp = FCk_FloatRange{0.5f, 1.5f};

public:
    CK_PROPERTY_GET(_AnimCollection);
    CK_PROPERTY(_PoolSize);
    CK_PROPERTY(_TileSize);
    CK_PROPERTY(_IdleSequenceIndex);
    CK_PROPERTY(_MoveSequenceIndex);
    CK_PROPERTY(_MoveSpeedThreshold);
    CK_PROPERTY(_MoveAuthoredSpeed);
    CK_PROPERTY(_MoveRateClamp);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_VisualLod_CrowdConfig, _AnimCollection);
};

// --------------------------------------------------------------------------------------------------------------------

// Everything one LOD domain tunes: thresholds, budgets, ranking, fades, and the crowds it may build.
// One asset per arbiter; entities join the domain via the tag
UCLASS(BlueprintType)
class CKVISUALLOD_API UCk_VisualLodArbiter_Data : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_VisualLodArbiter_Data);

private:
    // The domain identity members resolve their _ArbiterTag against. Two live arbiters with the
    // same tag in one world is an error (ensure at resolution)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Domain",
              meta = (AllowPrivateAccess = true, Categories = "VisualLod"))
    FGameplayTag _DomainTag;

    // ---- hysteresis band ----

    // Promote when nearer than this to the view; demote past DemoteDistance — the gap is the
    // hysteresis band so a loitering entity can't flap
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distances",
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _PromoteDistance = 2200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distances",
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _DemoteDistance = 2600.0f;

    // ---- budgets ----

    // Max concurrently promoted entities (full SKMC + AnimBP). Ranked, not first-come
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budgets",
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    int32 _NearBudget = 16;

    // Reserved for lock-driven promotes, independent of the near budget in BOTH directions: a
    // firefight can't starve the near-camera crowd, and a dense crowd can't stop a ragdoll
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budgets",
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    int32 _LockBudget = 8;

    // Ceiling on STARTING a lock-driven promote — past this a locked entity stays batched until
    // the view closes in (its holder keeps the lock; it promotes the moment it becomes eligible)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budgets",
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _LockPromoteMaxDistance = 8000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Budgets",
              meta = (AllowPrivateAccess = true))
    ECk_VisualLod_PoolExhaustionPolicy _ExhaustionPolicy = ECk_VisualLod_PoolExhaustionPolicy::PromoteInstead;

    // ---- ranking ----

    // Widened past the camera FOV so screen-edge entities still count as in-view
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranking",
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _ViewConeMarginDeg = 10.0f;

    // This close, in-view regardless of facing — a body at arm's length reads even off-centre
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranking",
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _AlwaysInViewDistance = 600.0f;

    // A challenger must win by this much before it takes an incumbent's slot — stops two
    // near-equal entities trading the same slot back and forth every tick
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranking",
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _PreemptDistanceMargin = 400.0f;

    // Each preempt costs a visible crossfade at both ends; bound the per-tick churn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ranking",
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    int32 _MaxPreemptsPerTick = 2;

    // ---- crossfade ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fade",
              meta = (AllowPrivateAccess = true))
    FCk_Time _FadeDuration = FCk_Time{0.1};

    // Per-instance custom-data float carrying the member's dither alpha (1 = solid, 0 = dissolved).
    // The crowd's material must read the same slot
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fade",
              meta = (AllowPrivateAccess = true, ClampMin = 2, ClampMax = 15))
    int32 _FadeCustomDataSlot = 13;

    // Custom-primitive-data float on the PROMOTED PROXY's mesh carrying the SAME alpha as the crowd
    // slot above — one value, two complementary dither masks. The near mesh's material must dither
    // itself OUT as the value rises (1 = far crowd member solid, 0 = near mesh solid), so the two
    // masks are complements and the crossfade is seamless. Written through the proxy's custom-data
    // lane, so the member's RendererData must declare _NumCustomDataFloat > this slot (the writes
    // ensure loudly otherwise), and a near material that ignores the slot pops instead of dithering
    // — loud by design, not a silent fallback
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fade",
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 31))
    int32 _FadeNearCustomPrimitiveDataSlot = 0;

    // The near mesh's play anchor leads the crowd clock by this many frame-times: the play request
    // applies one frame after the anchor is read, and the crowd's clock advances once more in
    // between. 1 cancels that latency exactly; tune by eye if a game's frame pacing says otherwise
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fade",
              meta = (AllowPrivateAccess = true))
    float _FadeAnchorLeadFrames = 1.0f;

    // ...and pulls BACK by this many bake intervals: the crowd displays trunc(time x SampleFrequency),
    // lagging its own clock by half an interval on average while the promoted mesh interpolates
    // smoothly. 0.5 centres the mesh on the crowd's displayed frame; tune by eye
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fade",
              meta = (AllowPrivateAccess = true))
    float _FadeAnchorBakeLagIntervals = 0.5f;

    // ---- pool ----

    // Unassigned pool members park here (hidden AND out of sight)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool",
              meta = (AllowPrivateAccess = true))
    float _ParkZ = -100000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool",
              meta = (AllowPrivateAccess = true))
    TArray<FCk_VisualLod_CrowdConfig> _CrowdConfigs;

public:
    CK_PROPERTY_GET(_DomainTag);
    CK_PROPERTY_GET(_PromoteDistance);
    CK_PROPERTY_GET(_DemoteDistance);
    CK_PROPERTY_GET(_NearBudget);
    CK_PROPERTY_GET(_LockBudget);
    CK_PROPERTY_GET(_LockPromoteMaxDistance);
    CK_PROPERTY_GET(_ExhaustionPolicy);
    CK_PROPERTY_GET(_ViewConeMarginDeg);
    CK_PROPERTY_GET(_AlwaysInViewDistance);
    CK_PROPERTY_GET(_PreemptDistanceMargin);
    CK_PROPERTY_GET(_MaxPreemptsPerTick);
    CK_PROPERTY_GET(_FadeDuration);
    CK_PROPERTY_GET(_FadeCustomDataSlot);
    CK_PROPERTY_GET(_FadeNearCustomPrimitiveDataSlot);
    CK_PROPERTY_GET(_FadeAnchorLeadFrames);
    CK_PROPERTY_GET(_FadeAnchorBakeLagIntervals);
    CK_PROPERTY_GET(_ParkZ);
    CK_PROPERTY_GET(_CrowdConfigs);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISUALLOD_API FCk_Fragment_VisualLodArbiter_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_VisualLodArbiter_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCk_VisualLodArbiter_Data> _Config;

public:
    CK_PROPERTY_GET(_Config);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VisualLodArbiter_ParamsData, _Config);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISUALLOD_API FCk_Request_VisualLodArbiter_SetObserver : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VisualLodArbiter_SetObserver);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VisualLodArbiter_SetObserver);

private:
    // The entity whose camera view drives ranking. Unset/invalid observer at update time falls
    // back to local-view discovery (TryGet_LocalViewInfo)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Observer;

public:
    CK_PROPERTY_GET(_Observer);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VisualLodArbiter_SetObserver, _Observer);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISUALLOD_API FCk_Request_VisualLodArbiter_ClearObserver : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VisualLodArbiter_ClearObserver);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VisualLodArbiter_ClearObserver);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISUALLOD_API FCk_Request_VisualLodArbiter_SetFrozen : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VisualLodArbiter_SetFrozen);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VisualLodArbiter_SetFrozen);

private:
    // Enable holds the arbiter on its current decisions (in-flight fades still finish); Disable
    // resumes ordinary arbitration on the next update
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Frozen = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_Frozen);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VisualLodArbiter_SetFrozen, _Frozen);
};

// --------------------------------------------------------------------------------------------------------------------

// Fired once per crowd, right after Finalize — the game's window to push slot override materials
// and default custom data onto the crowd actor (Get_Crowd resolves it)
DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_VisualLodArbiter_CrowdCreated,
    FCk_Handle_VisualLodArbiter, InHandle,
    int32, InCrowdIndex);

// --------------------------------------------------------------------------------------------------------------------
