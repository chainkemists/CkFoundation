#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkInput/CkInputButtonMap_Fragment_Data.h"
#include "CkInput/CkInputLayer_Fragment_Data.h"

#include <InputCoreTypes.h>

#include "CkIntentSampler_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKINTENT_API FCk_Handle_IntentSampler : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_IntentSampler); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_IntentSampler);

// --------------------------------------------------------------------------------------------------------------------

/**
 * The eight compass directions of the sampler's axis pair, plus the centre.
 *
 * The order IS the angle order: E is the positive X axis and every following value is another 45 degrees
 * counter-clockwise, so an octant's centre angle is (value - 1) * 45 and the octant for an angle is that
 * arithmetic run backwards. Neutral takes slot zero, which is what the -1 pays for and what makes a
 * default-constructed row read as "no direction" rather than as East.
 *
 * N is the axis pair's POSITIVE Y, not the world's north — on a stick that is up, on a screen-space pair it
 * would be down. The names describe the two axes the sampler was configured with and nothing else.
 */
UENUM(BlueprintType)
enum class ECk_Intent_Octant : uint8
{
    Neutral,
    E,
    NE,
    N,
    NW,
    W,
    SW,
    S,
    SE
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Intent_Octant);

// --------------------------------------------------------------------------------------------------------------------

/**
 * One axis of the SOCD-cleaned cardinal state.
 *
 * Positive and Negative rather than Right/Left and Up/Down because the row already expresses direction as the
 * SIGN of an axis: Positive on the horizontal slot is the direction _ConditionedAxisX reports positive, and on
 * the vertical slot the direction _ConditionedAxisY reports positive. One enum therefore serves both halves and
 * the policy that resolves an opposing pair is written once, instead of twice under two spellings of the same
 * three states.
 */
UENUM(BlueprintType)
enum class ECk_Intent_CleanedAxis : uint8
{
    Neutral,
    Positive,
    Negative
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Intent_CleanedAxis);

// --------------------------------------------------------------------------------------------------------------------

/**
 * One logic frame of input, as a plain value.
 *
 * The row is the unit everything above it reads: matching, phases and the debugger all consume rows rather than
 * live state, which is what makes a frame reproducible from the record alone. It carries no handles to anything
 * whose lifetime it does not control and no pointers at all, so a row copied out of the ring stays meaningful
 * after the entity that produced it is gone.
 *
 * FrameIndex counts the sampler's own logic frames from zero and never resets. A row whose FrameIndex is
 * negative is the "no such frame" answer from the range queries — a real row always carries a real index, so the
 * two can never be confused.
 *
 * Buttons are ButtonIds rather than keys because a key is not an identity: it moves when the player rebinds. A
 * source with no ButtonMap composed therefore records EMPTY button sets — inventing an identity here would be a
 * second minting authority beside the map's Physical tier, and nothing downstream could tell the two apart. The
 * delivery outcomes still carry the raw keys, so nothing is lost, only unnamed.
 *
 * The octant and the two cleaned axes are DERIVED here rather than left to every consumer: both readings depend
 * on state that only survives between rows (the octant a boundary was last cleared into, the order the cardinal
 * buttons went down), and a consumer deriving them from the row's raw values alone would get a different answer
 * than its neighbour. Deriving once, at the moment the state exists, is what makes two consumers of the same row
 * agree.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_FrameRecord
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Intent_FrameRecord);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _FrameIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Input_ButtonId> _Pressed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Input_ButtonId> _Released;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Input_ButtonId> _Held;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ConditionedAxisX = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ConditionedAxisY = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_InputLayer_RoutedEvent> _RoutedEvents;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Intent_Octant _Octant = ECk_Intent_Octant::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Intent_CleanedAxis _CleanedHorizontal = ECk_Intent_CleanedAxis::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Intent_CleanedAxis _CleanedVertical = ECk_Intent_CleanedAxis::Neutral;

public:
    CK_PROPERTY_GET(_FrameIndex);
    CK_PROPERTY(_Pressed);
    CK_PROPERTY(_Released);
    CK_PROPERTY(_Held);
    CK_PROPERTY(_ConditionedAxisX);
    CK_PROPERTY(_ConditionedAxisY);
    CK_PROPERTY(_RoutedEvents);
    CK_PROPERTY(_Octant);
    CK_PROPERTY(_CleanedHorizontal);
    CK_PROPERTY(_CleanedVertical);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Intent_FrameRecord, _FrameIndex);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * What the sampler reports for an axis whose two opposing buttons are both held.
 *
 * Neutral is the default because it is the fighting-game tournament standard and the only policy that cannot
 * produce a reading the hardware it emulates could not: a gated stick has no left-AND-right, so answering
 * neither is the answer that stays inside what the player could physically have asked for. The two priority
 * policies answer with one end of the pair and therefore need press ORDER, which the sampler derives from the
 * press edges it already claims — a button re-pressed while its opposite is down counts as pressed then.
 */
UENUM(BlueprintType)
enum class ECk_Intent_SocdPolicy : uint8
{
    Neutral,
    LastInputPriority,
    FirstInputPriority
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Intent_SocdPolicy);

// --------------------------------------------------------------------------------------------------------------------

/**
 * The four cardinal buttons SOCD cleaning is defined over.
 *
 * All four or none, rejected at composition otherwise: cleaning resolves OPPOSING PAIRS, and three buttons
 * describe an axis with one end — there is no reading to give for it, and half-honouring the declaration would
 * silently clean one axis while leaving the other raw.
 *
 * A button whose name is None is the UNSET state. That mirrors CkInput's own convention, where an invalid FKey
 * is the real, meaningful unset state of a button association: a named button always carries a real name, so
 * "unset" and "some button" can never be confused and no separate flag can go stale against the value.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Intent_SocdQuad
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Intent_SocdQuad);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Input_ButtonId _Up;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Input_ButtonId _Down;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Input_ButtonId _Left;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Input_ButtonId _Right;

public:
    CK_PROPERTY_GET(_Up);
    CK_PROPERTY_GET(_Down);
    CK_PROPERTY_GET(_Left);
    CK_PROPERTY_GET(_Right);

public:
    auto Get_IsFullyNamed() const -> bool
    {
        return _Up.Get_Name()   != NAME_None && _Down.Get_Name()  != NAME_None &&
               _Left.Get_Name() != NAME_None && _Right.Get_Name() != NAME_None;
    }

    auto Get_IsUnnamed() const -> bool
    {
        return _Up.Get_Name()   == NAME_None && _Down.Get_Name()  == NAME_None &&
               _Left.Get_Name() == NAME_None && _Right.Get_Name() == NAME_None;
    }

    auto Get_HasDistinctButtons() const -> bool
    {
        return _Up   != _Down && _Up   != _Left  && _Up   != _Right &&
               _Down != _Left && _Down != _Right && _Left != _Right;
    }

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Intent_SocdQuad, _Up, _Down, _Left, _Right);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * What one sampler records and how much of it it keeps.
 *
 * Capacity is fixed at composition rather than retunable, because the ring's whole value is that its cost is
 * known up front; a resize mid-session would either strand rows or reallocate under a consumer holding offsets.
 * 120 rows is two seconds at the sampler's 60 Hz cadence — long enough for any input pattern a player can
 * perform deliberately.
 *
 * The axis pair is the one stick a direction is derived from. Two separate keys rather than a vector because the
 * source stream is per-axis: a gamepad reports X and Y as independent analog events, and pretending otherwise
 * would require the sampler to invent a pairing rule the device never expressed.
 *
 * The neutral radius is a fraction of full deflection, and it is about direction CONFIDENCE rather than noise:
 * a stick a hair off centre has a perfectly real magnitude and a wildly unstable angle, so below the radius the
 * record says Neutral instead of naming whichever octant the last micro-volt fell in. The hysteresis margin is
 * how far past a boundary a NEW octant has to be entered before the reading moves; both are documented with
 * their default rationale in the module's Claude.md.
 */
USTRUCT(BlueprintType)
struct CKINTENT_API FCk_Fragment_IntentSampler_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_IntentSampler_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _RingCapacity = 120;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FKey _AxisKeyX = EKeys::Gamepad_LeftX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FKey _AxisKeyY = EKeys::Gamepad_LeftY;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _OctantNeutralRadius = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _OctantHysteresisMarginDegrees = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Intent_SocdQuad _SocdQuad;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Intent_SocdPolicy _SocdPolicy = ECk_Intent_SocdPolicy::Neutral;

public:
    CK_PROPERTY_GET(_RingCapacity);
    CK_PROPERTY(_AxisKeyX);
    CK_PROPERTY(_AxisKeyY);
    CK_PROPERTY(_OctantNeutralRadius);
    CK_PROPERTY(_OctantHysteresisMarginDegrees);
    CK_PROPERTY(_SocdQuad);
    CK_PROPERTY(_SocdPolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_IntentSampler_ParamsData, _RingCapacity);
};

// --------------------------------------------------------------------------------------------------------------------
