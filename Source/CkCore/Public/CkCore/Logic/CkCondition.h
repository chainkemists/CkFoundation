#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Object/CkWorldContextObject.h"
#include "CkCore/Time/CkTime.h"

#include "CkCondition.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ConditionResult : uint8
{
	Pass,
	Fail
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Abstract base class for a reusable, stateful condition predicate.
 *
 * Conditions follow a lifecycle: Enter → Evaluate (called repeatedly) → Exit.
 * Systems call the public C++ API; C++ and Blueprint subclasses override the Do* functions.
 *
 * Inherits from UCk_GameWorldContextObject_UE for reliable GetWorld() access
 * via the outer chain, enabling BP nodes that require world context.
 */
UCLASS(Abstract, EditInlineNew, Blueprintable, BlueprintType, DefaultToInstanced)
class CKCORE_API UCk_Condition : public UCk_GameWorldContextObject_UE
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_Condition);

public:
	/** Called once when the condition becomes active. */
	auto
	OnEnter() -> void;

	/** Called repeatedly to evaluate the condition. Returns Pass/Fail (respects Negate). */
	auto
	Evaluate(FCk_Time InDeltaT) -> ECk_ConditionResult;

	/** Called once when the condition is deactivated. */
	auto
	OnExit() -> void;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Ck|Condition",
			  meta = (DisplayName = "OnEnter"))
	void DoOnEnter();
	virtual void DoOnEnter_Implementation() {}

	UFUNCTION(BlueprintNativeEvent, Category = "Ck|Condition",
			  meta = (DisplayName = "Evaluate"))
	ECk_ConditionResult DoEvaluate(FCk_Time InDeltaT);
	virtual ECk_ConditionResult DoEvaluate_Implementation(FCk_Time InDeltaT) { return ECk_ConditionResult::Pass; }

	UFUNCTION(BlueprintNativeEvent, Category = "Ck|Condition",
			  meta = (DisplayName = "OnExit"))
	void DoOnExit();
	virtual void DoOnExit_Implementation() {}

private:
	/** When true, the result of DoEvaluate is inverted (Pass becomes Fail and vice versa). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition",
			  meta = (AllowPrivateAccess = true))
	bool _Negate = false;

public:
	CK_PROPERTY_GET(_Negate);
};

// --------------------------------------------------------------------------------------------------------------------
