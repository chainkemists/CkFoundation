#include "CkEntityTag_Fragment.h"

#include "CkEntityTag/CkEntityTag_Utils.h" // UCk_Utils_EntityTag_UE (Request_RestoreSet)

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies
#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_EntityTag_Root, TEXT("EntityTag"));

// --------------------------------------------------------------------------------------------------------------------
//
// Save-only persistence handler for the FName EntityTag set (v3 rebuild+hydrate, Phase 3.1). EntityTag is UNREPLICATED
// (no Replicate processor / RepData / MayRequireReplication), so this handler has NO net Apply — only HydrationApply +
// Produce (the load-path applier is its sole caller).
//
// RECONSTITUTE-BY-REQUEST (a lazily-composed, data-defined feature — CkSnapshot/Claude.md §5): EntityTag's Current
// fragment is composed LAZILY by Add and auto-REMOVED when it reaches zero tags, so it has no "composed-but-empty"
// state and exists only with >= 1 tag. The v3 load rebuilds entities from spawn recipes (Construct) and does NOT
// restore raw fragments — a generic entity's Construct does not re-compose EntityTag — so the usual "gate on
// Has<feature> -> NotReady" applier shape would spin to the hydration timeout and drop the payload LOUDLY. Instead the
// feature reconstitutes through its OWN public deferred request: HydrationApply enqueues exactly ONE composite
// FCk_Request_EntityTag_RestoreSet and returns Applied. Its drain-time handler (DoApply_RestoreSet, in
// FProcessor_EntityTag_HandleRequests) is what actually composes Current, rebuilds the per-tag EnTT storage presence,
// and fires the Added/Removed signals — the request itself only ENQUEUES.
//
// WHY A COMPOSITE REQUEST, NOT read-live-then-clear-then-Add: a rebuilt entity's Construct/BeginPlay may seed EntityTag
// tags through the same deferred Add requests, but FProcessor_EntityTag_HandleRequests is GatedDuringLoad, so at
// HydrationApply time those seeds are enqueued and INVISIBLE to any Has<>/Get_ read of the "current" set. Reading the
// live set here and clearing it would therefore MERGE the construct-seeds under the saved adds (monotonic count
// inflation each save/load cycle). The RestoreSet request instead rides the SAME FIFO request array AFTER those seeds,
// so its handler runs once they have materialised and diffs against the TRUE live set — reconstituting EXACTLY the
// saved {name -> count} map regardless of pump ordering. Idempotent under double-apply.
//
// NOT the forbidden "compose in the net Apply" race: this is the HydrationApply slot only (never assigned to a net
// Apply), and it runs authority-side AFTER construction (the dispatcher skips FTag_EntityScript_ConstructedThisFrame).
//
// RE-ARM: none. Unreplicated ⇒ there is no Replicate pass to re-arm — the local RestoreSet IS the whole restore.
//
// ABSENCE IS AMBIGUOUS: Produce returns UNSET both when the entity never had EntityTag and when every tag was removed
// pre-save (Current auto-removed at zero). An UNSET payload emits nothing to hydrate, so a Construct-seeded default
// tag RESURRECTS on load (Construct re-adds it; no RestoreSet arrives to strip it). Documented + pinned as chosen
// behavior (CkSnapshot/Claude.md §5, and the EntityTag parity gate's resurrection scenario).
//
// --------------------------------------------------------------------------------------------------------------------

static struct FCkEntityTagSaveHandlerRegistrar
{
    FCkEntityTagSaveHandlerRegistrar()
    {
        FCk_PersistenceHandlerRegistry::Register_SaveOnly<FCk_SaveData_EntityTags>({
            // Emit the entity's current FName tag set, or UNSET when EntityTag is absent (nothing to persist). UNSET is
            // ambiguous between "never had tags" and "all removed pre-save" — see the block comment's ABSENCE note.
            .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
            {
                if (NOT Entity.Has<ck::FFragment_EntityTag_Current>())
                { return {}; }

                const auto& Tags = Entity.Get<ck::FFragment_EntityTag_Current>().Get_Tags();
                if (Tags.IsEmpty())
                { return {}; } // Current present but tagless should not occur (auto-removed at zero) — nothing to save

                auto Payload = FCk_SaveData_EntityTags{};
                Payload.Get_TagNames().Reserve(Tags.Num());
                Payload.Get_Counts().Reserve(Tags.Num());
                for (const auto& TagCount : Tags)
                {
                    Payload.Get_TagNames().Add(TagCount._Name);
                    Payload.Get_Counts().Add(TagCount._Count);
                }
                return FInstancedStruct::Make(Payload);
            },
            // Load-path applier (no net Apply — EntityTag never rides the wire). REPLACE semantics achieved by a single
            // composite restore-set request that diffs at DRAIN time, never a read-live-then-clear here (see the block
            // comment above): reading the "current" set at HydrationApply time is a lie because GatedDuringLoad construct
            // seeds are still enqueued-but-undrained. Enqueue one request, return Applied.
            .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
            {
                CK_ENSURE_IF_NOT(ck::IsValid(Entity),
                    TEXT("EntityTag hydration skipped — invalid entity [{}]"), Entity)
                { return ECk_Persistence_ApplyResult::Applied; }

                const auto& Payload     = New.Get<FCk_SaveData_EntityTags>();
                const auto& SavedNames  = Payload.Get_TagNames();
                const auto& SavedCounts = Payload.Get_Counts();

                CK_ENSURE_IF_NOT(SavedNames.Num() == SavedCounts.Num(),
                    TEXT("EntityTag hydration payload malformed on [{}] — [{}] names vs [{}] counts; restoring the common prefix"),
                    Entity, SavedNames.Num(), SavedCounts.Num())
                { /* fall through: DoApply_RestoreSet clamps to FMath::Min(names, counts) rather than restoring nothing */ }

                // ONE composite request — the drain-time handler (DoApply_RestoreSet) SETs the live set to EXACTLY the
                // saved counted set, diffing against whatever construct-seeds have materialised by then. No live read here.
                UCk_Utils_EntityTag_UE::Request_RestoreSet(Entity, SavedNames, SavedCounts);

                return ECk_Persistence_ApplyResult::Applied;
            }});
    }
} GCkEntityTagSaveHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
