# CkCore / GameplayTag

`FGameplayTag` / `FGameplayTagContainer` utilities — requirement checks, container intersection, tag stacks. Exposed to C++, Blueprints, and AngelScript via a UE script-mixin.

**Key files:** `CkGameplayTag_Utils.h`, `CkGameplayTagStack.h`

## Primary class

`UCk_Utils_GameplayTag_UE` — `BlueprintFunctionLibrary` with `ScriptMixin = "FGameplayTagRequirements FGameplayTagContainer"`. So every method appears as a member-style call on tag containers/requirements from BP.

Representative methods (see header for full list):

```cpp
Get_AreTagRequirementsMet(const FGameplayTagRequirements&, const FGameplayTagContainer&)   // bool
Get_DoContainersIntersect(const FGameplayTagContainer&, FGameplayTagContainer)             // bool
Get_DoContainersIntersect_Exact(const FGameplayTagContainer&, FGameplayTagContainer)       // bool
Get_AllIntersectingTags(const FGameplayTagContainer&, FGameplayTagContainer)               // FGameplayTagContainer
```

## Tag stacks

`CkGameplayTagStack.h` defines a count-based tag container: each tag has an associated integer. Use for "how many buffs of type X are on this entity" scenarios where a plain container loses the count.

## Pitfalls

1. `FGameplayTagContainer::HasAll` / `HasAny` do **hierarchical** matching (parent tag matches child). The `_Exact` variants in this module skip hierarchy. Pick the one that matches your semantic intent; bugs from using the wrong one are subtle.
2. The second `FGameplayTagContainer` arg is passed **by value** in several methods — that's intentional because the implementation mutates it internally. Don't "fix" the signature.
3. Tags themselves are defined via `UCk_GameplayTags` data assets (see `Types/DataAsset`) or `.ini`. Prefer the AngelScript `asset ... of UCk_GameplayTags { GameplayTags.Add(n"…"); }` pattern from the root CLAUDE.md.

## Depends on
`GameplayTags` UE module, `Enums/`, `Macros/`.

## Used by
`CkRelationship`, `CkInteraction`, `CkInventory`, `CkResolver`, `CkObjective`, `CkTagSet`, state machines, condition systems — basically any gameplay feature that tag-filters.

## See also
- `CkTagSet/` — entity-scoped tag containers with signal support.
- `/Source/CLAUDE.md` section 15 "Angelscript asset creation" — how to declare tags from AS.
