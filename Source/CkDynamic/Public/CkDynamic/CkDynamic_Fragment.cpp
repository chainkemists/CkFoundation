#include "CkDynamic_Fragment.h"

#include "CkDynamic/CkDynamic_Log.h"
#include "CkDynamic/CkDynamic_Utils.h"

#include "CkCore/Payload/CkPayload.h"          // ck::MakePayload
#include "CkCore/Validation/CkIsValid.h"       // ck::IsValid / ck::Is_NOT_Valid

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // RegisterLazyTyped

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_EntityFragment_Root, TEXT("DynamicFragment"));

// --------------------------------------------------------------------------------------------------------------------
// G2 dynamic-fragment persistence handler.
//
// The runtime-typed FALLBACK (CkDynamic_Module.cpp) owns net receive for individual dynamic-fragment types and is
// structurally invisible to the save census (Get_SaveHandlerTypes enumerates per-type handlers only) — so this wrapper
// is the save participant. FCk_SaveData_DynamicFragments never rides a replicated container, so this handler has NO net
// Apply/Remove; it only flows save -> load. Produce copies EVERY stored dynamic fragment (Model-A parity: replicated or
// not); HydrationApply re-composes each one onto the freshly-rebuilt entity and mirrors the fallback's RepNotify so a
// bound OnRepNotify handler re-runs against the restored value.

static struct FCkDynamicFragmentsSaveHandlerRegistrar
{
    FCkDynamicFragmentsSaveHandlerRegistrar()
    {
        FCk_PersistenceHandlerRegistry::Register_SaveOnly<FCk_SaveData_DynamicFragments>({
            // Save capture: emit every dynamic fragment on the entity, or UNSET when it holds none. READ-ONLY —
            // Get_AllFragments only reads the per-type named storages (mirrors what the fallback would replicate,
            // extended to the non-replicated fragments too, for Model-A parity).
            .Produce = [](FCk_Handle& InEntity) -> TOptional<FInstancedStruct>
            {
                if (NOT InEntity.Has<ck::FFragment_DynamicFragment_Data>())
                { return {}; }

                auto SaveData = FCk_SaveData_DynamicFragments{};
                SaveData.Set_Fragments(UCk_Utils_DynamicFragment_UE::Get_AllFragments(InEntity));
                return FInstancedStruct::Make(SaveData);
            },
            // Authority-side load: rebuild every saved dynamic fragment on the (already re-created) entity. Dynamic
            // fragments have no structural composition step — they exist iff they hold data — so nothing to wait on:
            // always Applied, no NotReady. HydrationApply-only (never the net Apply slot): applying on a client would
            // race construct-time composition.
            .HydrationApply = [](FCk_Handle& InEntity, const FInstancedStruct& InNew, const TOptional<FInstancedStruct>& /*InOld*/) -> ECk_Persistence_ApplyResult
            {
                if (InNew.GetScriptStruct() != FCk_SaveData_DynamicFragments::StaticStruct())
                { return ECk_Persistence_ApplyResult::Applied; }

                const auto& SaveData = InNew.Get<FCk_SaveData_DynamicFragments>();

                for (const auto& Entry : SaveData.Get_Fragments())
                {
                    const auto* Type = Entry.GetScriptStruct();
                    if (ck::Is_NOT_Valid(Type))
                    {
                        // Content drift: the dynamic-fragment type stored in the save no longer resolves (e.g. an
                        // AngelScript struct deleted since the save was written). Recoverable data drift, NOT a
                        // programmer error — WARN and skip this entry, keep applying the rest. Deliberately not an
                        // ensure (a missing content type must not break loading the whole save).
                        ck::dynamic::Warning(TEXT("v3 hydrate: a saved dynamic fragment on Entity [{}] has an "
                            "unresolved type (deleted since the save was written?) — skipping it; other fragments "
                            "still applied."), InEntity);
                        continue;
                    }

                    // AddOrGet re-creates the named storage if the rebuild did not compose it, else overwrites it in
                    // place. Idempotent under a double-apply — re-applying yields the saved value, never a stacked one.
                    auto& Storage = UCk_Utils_DynamicFragment_UE::AddOrGet_Fragment_TypeUnsafe(InEntity, Type);
                    Storage = Entry;

                    // Mirror the net fallback's RepNotify (CkDynamic_Module.cpp) so any bound OnRepNotify handler
                    // re-runs against the restored value. The payload carries only WHICH type changed — handlers read
                    // the new value via Get_Fragment(ChangedType), avoiding the ProcessEvent frame-buffer staleness.
                    auto Info = FCk_DynamicFragment_RepNotifyInfo{};
                    Info.ChangedType = const_cast<UScriptStruct*>(Type);
                    ck::UUtils_Signal_DynamicFragment_OnRepNotify::Broadcast(InEntity, ck::MakePayload(InEntity, Info));
                }

                // Re-arm the Replicate pass ONCE (never per-entry) so post-load clients converge on any dynamic
                // fragment that construct re-registered as replicated. Inert when the entity has no replicated types:
                // this payload does not carry the per-type replication flag, so a fragment absent from the rebuild is
                // restored as local-only — the Replicate processor keys off FFragment_DynamicFragment_ReplicatedTypes.
                if (InEntity.Has<ck::FFragment_DynamicFragment_ReplicatedTypes>())
                { InEntity.AddOrGet<ck::FTag_DynamicFragment_MayRequireReplication>(); }

                return ECk_Persistence_ApplyResult::Applied;
            }});
    }
} GCkDynamicFragmentsSaveHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
