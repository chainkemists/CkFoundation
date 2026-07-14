# PHASE 4 — Produce symmetry, class (b): fold the child-keyed Produce into the owner container (build 4)

The record-aggregated features keep TWO projections today: an owner-keyed wire aggregation and a
child-keyed `Produce`. This phase derives the wire payload FROM the child-keyed `Produce` — one projection,
two scopes. Highest-risk phase; independently shippable — Adam may stop before it.

## Entry criteria

Phase 3 committed; trees clean; gates at the PROGRESS-recorded baseline (counts AND names).
`TryProduce<T>` exists.

## Step 1 — Attributes (one template edit covers all 5 kinds)

`CkAttribute_Processor.inl.h:221-255`, `TProcessor_Attribute_Replicate::ForEachEntity`. Today: builds ONE
`(name, Base, Final, component)` entry for the single component this processor instance ticks, find-or-
emplaces it into the owner container. New body:

```cpp
    auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

    // One projection: the registered child-keyed Produce emits ALL composed components (Current + Min/Max)
    // of this attribute entity with the same EntryType shape the wire uses. Fold each into the owner
    // container by (name, component) — identical content to the per-component hand-build this replaces;
    // sibling components refresh with their identical live values (equality is nearly-equal, harmless).
    const auto Produced = UCk_Utils_Net_UE::TryProduce<T_RepDataStruct>(InHandle);
    if (Produced.IsSet())
    {
        UCk_Utils_Net_UE::TryUpdateContainerFragment<T_RepDataStruct>(
            LifetimeOwner, [&](T_RepDataStruct& Data)
        {
            using EntryType = typename decltype(Data.Attributes)::ElementType;
            for (const auto& ProducedEntry : Produced->Attributes)
            {
                const auto Found = Data.Attributes.FindByPredicate([&](const EntryType& InElement)
                {
                    return InElement.Get_AttributeName() == ProducedEntry.Get_AttributeName() &&
                           InElement.Get_Component() == ProducedEntry.Get_Component();
                });

                if (ck::Is_NOT_Valid(Found, ck::IsValid_Policy_NullptrOnly{}))
                { Data.Attributes.Emplace(ProducedEntry); }
                else
                { *Found = ProducedEntry; }
            }
        });
    }

    InHandle.template Remove<MarkedDirtyBy>();
```

Notes: the three per-component processor instances (`TProcessor_Attribute_Replicate_All`, inl.h:616-651)
stay — each still ticks on its own dirty marker; the fold is idempotent so redundant refreshes are
harmless. The unused `InAttribute` parameter may remain (signature untouched) — mark `[[maybe_unused]]`.

## Step 2 — EntityCollection

`CkEntityCollection_Processor.cpp:307-333`: runs per CHILD collection entity, writes one entry into the
owner. The registered `Produce` (`CkEntityCollection_Fragment.cpp:103-115`) is OWNER-keyed and rebuilds the
full array. Replace the per-child find-or-emplace with:

```cpp
    auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
    const auto Produced = UCk_Utils_Net_UE::TryProduce<FCk_RepData_EntityCollections>(LifetimeOwner);
    if (Produced.IsSet())
    { UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_EntityCollections>(LifetimeOwner, *Produced); }
```

Full-replace vs today's incremental find-or-emplace: content-identical (Produce reads every collection's
live members via the same record walk). If any EntityCollection Net spec disagrees → it found a real
ordering/content divergence → STOP + Blocker with the diff.

## Step 3 — Inventory Spatial + DataOnly

`CkInventory_Spatial_Processor.cpp:60-90` and `CkInventory_DataOnly_Processor.cpp:32-57`: both already
full-replace the owner container from the inventory record. Their registered `Produce`s
(`CkInventory_Spatial_Fragment.cpp:91-115`, `CkInventory_DataOnly_Fragment.cpp:83-101`) are keyed on the
INVENTORY entity and build the identical arrays. Replace the hand-build with
`TryProduce<FCk_RepData_Inventory_*_Items>(InventoryEntity)` (the entity the processor already keys on)
and write the result to the `LifetimeOwner` container as today.

KNOWN PRE-EXISTING GAP (do not fix, do not regress): two inventories sharing one owner clobber each
other's container entry (full-replace semantics). This phase preserves that behavior exactly; the fix is a
separate Adam-gated item.

## Step 4 — Build + gate (the phase's ONLY build)

1. Build → exit 0.
2. `--test --test-pattern "Ck.Net"` → delta-zero vs the recorded baseline — the attribute/inventory/
   collection Net specs are the byte-parity oracle. A red here = the fold changed wire content; STOP,
   revert that feature's hunk, record the exact failing test + diff hypothesis in Blockers. Do not stack
   fixes on a red fold.
3. `--test --test-pattern "Ck.Snapshot"` → delta-zero vs the recorded baseline (attribute + inventory +
   collection MPReload parities re-exercise the save path against the same Produce).
4. Exit grep: `rg --no-ignore -n "Data.Attributes.Emplace\(ToReplicate\)" Source/CkAttribute` → 0 (the old
   hand-build is gone).

## Commit

`refactor(persistence): class-(b) wire builders fold the registered child-keyed Produce (attributes x5 via shared template, EntityCollection, Inventory Spatial+DataOnly)`

## Fences

- **StateMachine and RenderTarget remain untouched** (class (c) — see PROMPT.md kill reasons).
- Do NOT change any `Produce` body to "make the fold easier" — if a Produce seems wrong, that is a Blocker,
  not an edit (its shape is save-file surface).
- Do NOT fix the multi-inventory clobber gap.
- Do NOT parallelize or reorder the attribute processor registrations.
- The fold must not skip `Remove<MarkedDirtyBy>` — dirty-marker hygiene is what stops per-tick rebuilds.
