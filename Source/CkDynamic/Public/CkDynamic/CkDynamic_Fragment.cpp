#include "CkDynamic_Fragment.h"

#include "CkDynamic/CkDynamic_Log.h"
#include "CkDynamic/CkDynamic_FragmentSchema.h"
#include "CkDynamic/CkDynamic_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Payload/CkPayload.h"          // ck::MakePayload
#include "CkCore/Validation/CkIsValid.h"       // ck::IsValid / ck::Is_NOT_Valid

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // RegisterLazyTyped
#include "CkEcs/Persistence/CkPersistenceHydration.h"           // Hydration_OnTypeHydrated
#include "CkEcs/Snapshot/CkSnapshot_Posture.h"                  // ck::Get_FragmentPosture

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_EntityFragment_Root, TEXT("DynamicFragment"));

static struct FCkDynamicFragmentsSaveHandlerRegistrar
{
    FCkDynamicFragmentsSaveHandlerRegistrar()
    {
        FCk_PersistenceHandlerRegistry::Register_SaveOnly<FCk_SaveData_DynamicFragments>({
            .Posture = ECk_Snapshot_Posture::Durable,
            .Produce = [](FCk_Handle& InEntity) -> TOptional<FInstancedStruct>
            {
                if (NOT InEntity.Has<ck::FFragment_DynamicFragment_Data>())
                { return {}; }

                auto Fragments = UCk_Utils_DynamicFragment_UE::Get_AllFragments(InEntity);
                Fragments.RemoveAll([](const FInstancedStruct& InEntry)
                {
                    return ck::Get_FragmentPosture(InEntry.GetScriptStruct()) == ECk_Snapshot_Posture::Session;
                });
                if (Fragments.IsEmpty())
                { return {}; }

                for (const auto& Entry : Fragments)
                {
                    const auto Schema = ck::dynamic::Validate_FragmentSchema(Entry.GetScriptStruct());
                    CK_ENSURE_IF_NOT(Schema.IsSafe,
                        TEXT("Refusing to save unsafe legacy Dynamic Fragment schema [{}] at [{}]: {}"),
                        Entry.GetScriptStruct(), Schema.FailurePath, Schema.FailureReason)
                    { return {}; }
                }

                auto SaveData = FCk_SaveData_DynamicFragments{};
                SaveData.Set_Fragments(MoveTemp(Fragments));
                return FInstancedStruct::Make<FCk_SaveData_DynamicFragments>(MoveTemp(SaveData));
            },
            .HydrationApply = [](FCk_Handle& InEntity, const FInstancedStruct& InNew, const TOptional<FInstancedStruct>& /*InOld*/) -> ECk_Persistence_ApplyResult
            {
                const auto WrapperTypeIsValid = InNew.GetScriptStruct() == FCk_SaveData_DynamicFragments::StaticStruct();
                CK_ENSURE_IF_NOT(WrapperTypeIsValid,
                    TEXT("Dynamic Fragment hydration received the wrong wrapper type [{}]"),
                    InNew.GetScriptStruct())
                { return ECk_Persistence_ApplyResult::Rejected; }

                const auto EntityIsValid = ck::IsValid(InEntity);
                CK_ENSURE_IF_NOT(EntityIsValid,
                    TEXT("Dynamic Fragment hydration received an invalid entity [{}]"), InEntity)
                { return ECk_Persistence_ApplyResult::Rejected; }

                const auto& SaveData = InNew.Get<FCk_SaveData_DynamicFragments>();

                // Validate the complete wrapper before the first write: one unsafe schema rejects the whole save.
                auto ResolvedEntries = TArray<TPair<const FInstancedStruct*, FInstancedStruct*>>{};
                ResolvedEntries.Reserve(SaveData.Get_Fragments().Num());
                for (const auto& Entry : SaveData.Get_Fragments())
                {
                    const auto* Type = Entry.GetScriptStruct();
                    if (ck::Is_NOT_Valid(Type))
                    { continue; } // unresolved content drift retains the warning-and-skip behavior below
                    if (ck::Get_FragmentPosture(Type) == ECk_Snapshot_Posture::Session)
                    { continue; }

                    const auto Schema = ck::dynamic::Validate_FragmentSchema(Type);
                    CK_ENSURE_IF_NOT(Schema.IsSafe,
                        TEXT("Refusing to hydrate unsafe Dynamic Fragment schema [{}] at [{}]: {}"),
                        Type, Schema.FailurePath, Schema.FailureReason)
                    { return ECk_Persistence_ApplyResult::Rejected; }
                }

                for (const auto& Entry : SaveData.Get_Fragments())
                {
                    const auto* Type = Entry.GetScriptStruct();
                    if (ck::Is_NOT_Valid(Type))
                    {
                        // Recoverable content drift, NOT a programmer error: a type deleted since the save was
                        // written must not break loading the whole save.
                        ck::dynamic::Warning(TEXT("v3 hydrate: a saved dynamic fragment on Entity [{}] has an "
                            "unresolved type (deleted since the save was written?) — skipping it; other fragments "
                            "still applied."), InEntity);
                        continue;
                    }
                    // A Session type in the payload predates its declaration. Skipping it BEFORE the add is what
                    // keeps hydration from composing session state the rebuild deliberately did not.
                    if (ck::Get_FragmentPosture(Type) == ECk_Snapshot_Posture::Session)
                    { continue; }

                    auto* Storage = UCk_Utils_DynamicFragment_UE::TryAddOrGet_Fragment_TypeUnsafe(InEntity, Type);
                    if (Storage == nullptr)
                    { return ECk_Persistence_ApplyResult::Rejected; }

                    ResolvedEntries.Emplace(&Entry, Storage);
                }

                // Commit every value before the first notification: a listener may destroy the entity, but it
                // can no longer observe a half-hydrated set.
                //
                // Whatever reaches here is captured whole and restored whole. There is no field-wise copy and no
                // handle backstop any more: a fragment holding live-session content — a delegate at any depth, a
                // construct-rebuilt child handle, in-flight bookkeeping — declares FCk_Snapshot_Session and is
                // skipped on both sides, and a Durable declaration over that content reds as "split the
                // fragment". The DECLARATION is what preserves live state; nothing here rescues an undeclared one.
                for (const auto& Resolved : ResolvedEntries)
                {
                    *Resolved.Value = *Resolved.Key;
                }

                // Hydration is not replication, and this edge used to say it was. A consumer binding
                // OnRepNotify is describing a wire update; a load delivering one told it a different thing in
                // the same words, which is how a bind made for one intent came to serve the other. The net path
                // keeps OnRepNotify; this one names what actually happened.
                for (const auto& Resolved : ResolvedEntries)
                {
                    const auto* Type = Resolved.Key->GetScriptStruct();
                    auto Info = FCk_Hydration_TypeInfo{};
                    Info.HydratedType = const_cast<UScriptStruct*>(Type);
                    ck::UUtils_Signal_Hydration_OnTypeHydrated::Broadcast(InEntity, ck::MakePayload(InEntity, Info));
                }

                // Re-arm the Replicate pass ONCE, never per-entry.
                if (InEntity.Has<ck::FFragment_DynamicFragment_ReplicatedTypes>())
                { InEntity.AddOrGet<ck::FTag_DynamicFragment_MayRequireReplication>(); }

                return ECk_Persistence_ApplyResult::Applied;
            }});
    }
} GCkDynamicFragmentsSaveHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
