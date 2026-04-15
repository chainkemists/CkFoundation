# CkProvider

**Purpose:** Data-asset-driven value providers. A Provider is a `UDataAsset`-derived object that computes or returns a value (bool, float, int, tag, struct) given an optional entity handle context. Providers let designers configure "how much does X contribute to Y" in content rather than code.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`.
**Used by:** `CkAttribute`, `CkAnimation`, `CkCamera`, `CkChaos`, `CkCue`, `CkDynamic`, `CkEntityExtension`, `CkEntityTag`, `CkInteraction`, `CkIsmRenderer`, `CkMessaging`, `CkObjective`, `CkPmg`, `CkRaySense`, `CkRelationship`, `CkResolver`, `CkShapes`, `CkStateTree`, `CkTargeting`, `CkTween`, `CkVfx`, and more.

---

## Core types

```cpp
// Base (not directly instantiated)
UCLASS(NotEditInlineNew) class UCk_Provider_PDA : public UCk_DataAsset_PDA { };

// Bool variant (abstract — subclass to implement Get_Value)
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class UCk_Provider_Bool_PDA : public UCk_Provider_PDA
{
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    bool Get_Value(FCk_Handle InOptionalHandle) const;
};

// Built-in literal subclass (returns a constant bool)
UCLASS() class UCk_Provider_Bool_Literal_PDA : public UCk_Provider_Bool_PDA { };
```

The same pattern repeats for `Float`, `Int32`, `FGameplayTag`, `FName`, `FText`, and struct variants. Each has:
- An abstract base (`UCk_Provider_T_PDA`) with `Get_Value(FCk_Handle)`.
- A `_Literal_` subclass for constant values.
- Support for `EditInlineNew` so instances can be nested directly in parent asset properties.

---

## How features use providers

A feature's `_Params` fragment stores a `TObjectPtr<UCk_Provider_Bool_PDA>` (or float/etc.). At runtime, the processor calls `Get_Value(InHandle)` to get the configured value. The designer selects a provider subclass in the Details panel.

```cpp
// In a fragment:
UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess))
TObjectPtr<UCk_Provider_Float_PDA> _DamageScaleProvider;

// In a processor:
const auto Scale = ck::IsValid(InParams.Get_DamageScaleProvider())
    ? InParams.Get_DamageScaleProvider()->Get_Value(InHandle)
    : 1.0f;
```

---

## Creating a custom provider

1. Inherit from the appropriate base (`UCk_Provider_Float_PDA`, etc.).
2. Mark `UCLASS(Blueprintable, BlueprintType, EditInlineNew)`.
3. Override `Get_Value(FCk_Handle InOptionalHandle) const` in C++ or Blueprint.
4. Use `InOptionalHandle` to look up entity state if the value is context-dependent.

```cpp
UCLASS(Blueprintable, BlueprintType, EditInlineNew)
class UMyProvider_DamageByAttributeLevel_PDA : public UCk_Provider_Float_PDA
{
    GENERATED_BODY()

    float Get_Value_Implementation(FCk_Handle InHandle) const override
    {
        // read an attribute fragment and return a scaled value
    }
};
```

---

## Anti-patterns

1. Don't hardcode values that designers should configure. If it's a tunable scalar, the corresponding fragment property should be a `UCk_Provider_Float_PDA*`, not a `float`.
2. Don't skip the null check before calling `Get_Value`. Not every property is guaranteed to be set, and a null crash in a processor is hard to diagnose.
3. Don't store stateful data in a provider asset — they're shared instances (often a CDO or a data asset). The entity handle argument is the only runtime context.

---

## See also
- `CkAttribute/Claude.md` — primary heavy consumer of providers (attribute modifiers use providers for value computation).
- `CkCore/Types/DataAsset` — `UCk_DataAsset_PDA`, the root base type for all PDA assets.
