#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Time/CkTime.h"

#include "CkCore/Format/CkFormat.h"

#include "CkChrono.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class FArchive;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Chrono_ConsumeState : uint8
{
    CouldNotConsume,
    Consumed,
    FullyConsumed,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Chrono_ConsumeState);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Chrono_TickState : uint8
{
    Ticking,
    Done
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Chrono_TickState);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Chrono_OverflowPolicy : uint8
{
    // Clamp at the goal and latch Done until Reset — the one-shot countdown.
    Clamp,
    // On reaching the goal, wrap past it carrying the remainder and report Done that tick — a recurring cadence.
    Wrap
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Chrono_OverflowPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeBreak = "/Script/CkCore.Ck_Utils_Chrono_UE:Break_Chrono"))
struct CKCORE_API FCk_Chrono
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Chrono);

    using TimeType       = FCk_Time;
    using TickStateType = ECk_Chrono_TickState;
    using ConsumeStateType = ECk_Chrono_ConsumeState;
    using NormalizationPolicyType =  ECk_NormalizationPolicy;

public:
    FCk_Chrono() = default;
    explicit FCk_Chrono(TimeType InGoalValue);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

public:
    /** Advances by InDeltaT toward the goal. Returns Done on the tick the goal is reached. InOverflow
     *  selects boundary behavior: Clamp latches at the goal (one-shot; Done every tick thereafter until
     *  Reset); Wrap rolls over carrying the remainder for a recurring cadence, so `Tick(dt, Wrap) == Done`
     *  is the "fire this tick" gate. A goal <= 0 reports Done every tick. */
    auto Tick(const TimeType& InDeltaT, ECk_Chrono_OverflowPolicy InOverflow = ECk_Chrono_OverflowPolicy::Clamp) -> TickStateType;
    auto Consume(const TimeType& InDeltaT) -> ConsumeStateType;
    auto Complete() -> ThisType&;
    auto Reset() -> ThisType&;
    auto Get_IsDone() const -> bool;
    auto Get_IsDepleted() const -> bool;
    auto Get_HasStarted() const -> bool;

    auto Get_TimeRemaining(NormalizationPolicyType InNormalizationPolicy = NormalizationPolicyType::None) const -> FCk_Time;
    auto Get_TimeElapsed(NormalizationPolicyType InNormalizationPolicy = NormalizationPolicyType::None) const -> FCk_Time;

public:
    static const FCk_Chrono Zero;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _GoalValue;

    UPROPERTY(Transient)
    FCk_Time _CurrentValue;

public:
    CK_PROPERTY_GET(_GoalValue);
};

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_Chrono, [](const FCk_Chrono& InObj)
{
    return ck::Format
    (
        TEXT("{} out of {}"),
        InObj.Get_TimeElapsed(),
        InObj.Get_GoalValue()
    );
});

// --------------------------------------------------------------------------------------------------------------------
