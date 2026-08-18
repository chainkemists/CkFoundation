#include "CkDynamic_Fragment.h"

#include "CkDynamic/CkDynamic_Log.h"
#include "CkDynamic/CkDynamic_FragmentSchema.h"
#include "CkDynamic/CkDynamic_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Payload/CkPayload.h"          // ck::MakePayload
#include "CkCore/Validation/CkIsValid.h"       // ck::IsValid / ck::Is_NOT_Valid

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // RegisterLazyTyped
#include "CkEcs/Snapshot/CkSnapshot_HandleWalk.h"               // ck::snapshot::ForEachHandle

#include <NativeGameplayTags.h>
#include <UObject/UnrealType.h> // TFieldIterator / FProperty (transient-preserving hydration copy)

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_EntityFragment_Root, TEXT("DynamicFragment"));

// --------------------------------------------------------------------------------------------------------------------
namespace ck_dynamic_fragment
{
    auto IsSnapshotTransient(const UScriptStruct* InType) -> bool
    {
        return UCk_Utils_DynamicFragment_UE::Get_IsSnapshotTransient(InType);
    }

    // The saved payload cannot carry a meaningful value for these, but the rebuilt world can:
    // CPF_Transient is never written by the archive, and a delegate's saved form names objects from
    // the torn-down world while the live value holds the subscribers that bound during the rebuild.
    // Stomping either leaves a feature that reads correctly once and then never updates again.
    // Top-level only — a nested delegate has no meaningful "live one" once array lengths differ;
    // those fragments take the whole-struct SnapshotTransient marker instead.
    auto Get_IsLiveSessionField(const FProperty* InProperty) -> bool
    {
        if (InProperty->HasAnyPropertyFlags(CPF_Transient))
        { return true; }

        return InProperty->IsA<FMulticastDelegateProperty>() || InProperty->IsA<FDelegateProperty>();
    }

    auto CopyFragment_PreservingLiveSessionFields(const FInstancedStruct& InSource, FInstancedStruct& InDest) -> void
    {
        const auto* Type = InSource.GetScriptStruct();

        auto AnyPreserved = false;
        for (TFieldIterator<FProperty> It{Type}; It; ++It)
        {
            if (Get_IsLiveSessionField(*It))
            { AnyPreserved = true; break; }
        }

        const auto DestTypeMatches = InDest.GetScriptStruct() == Type;
        if (NOT AnyPreserved || NOT DestTypeMatches)
        {
            InDest = InSource;
            return;
        }

        for (TFieldIterator<FProperty> It{Type}; It; ++It)
        {
            if (Get_IsLiveSessionField(*It))
            { continue; }
            It->CopyCompleteValue_InContainer(InDest.GetMutableMemory(), InSource.GetMemory());
        }
    }

    // Collected BEFORE the hydration copy overwrites them; see Restore_UnresolvedHandles.
    auto Collect_Handles(FInstancedStruct& InFragment) -> TArray<FCk_Handle>
    {
        auto Handles = TArray<FCk_Handle>{};

        if (ck::Is_NOT_Valid(InFragment))
        { return Handles; }

        ck::snapshot::ForEachHandle(InFragment.GetScriptStruct(), InFragment.GetMutableMemory(),
        [&Handles](FCk_Handle& InHandle) -> void
        {
            Handles.Emplace(InHandle);
        });

        return Handles;
    }

    // Hydration must never DOWNGRADE a live handle to a dead one: a construct-spawned child is
    // unlabeled, so capture rule 3 never writes a row for it and its saved id remaps to a tombstone,
    // leaving the feature structurally complete but inert. UPROPERTY(Transient) is the precise
    // opt-out; this is the backstop for fields nobody has audited.
    // Trade: a deliberately CLEARED field is indistinguishable from an unresolved one (both arrive
    // invalid) and keeps its fresh value, so load-bearing emptiness must be persisted explicitly.
    auto Restore_UnresolvedHandles(
        const TArray<FCk_Handle>& InPreCopyHandles,
        FInstancedStruct& InDest,
        const UScriptStruct* InType,
        const FCk_Handle& InOwner) -> void
    {
        if (InPreCopyHandles.IsEmpty() || InDest.GetScriptStruct() != InType)
        { return; }

        auto DestHandles = TArray<FCk_Handle*>{};
        ck::snapshot::ForEachHandle(InType, InDest.GetMutableMemory(),
            [&DestHandles](FCk_Handle& InHandle) -> void
            {
                DestHandles.Emplace(&InHandle);
            });

        // The walk is positional, so a differing slot count means a saved array of another length
        // shifted every later field and slot i is no longer the same field on both sides.
        if (DestHandles.Num() != InPreCopyHandles.Num())
        {
            ck::dynamic::Verbose(TEXT("v3 hydrate: skipping the unresolved-handle backstop on Entity [{}] fragment [{}] "
                "— the saved layout holds [{}] handle slots against [{}] freshly constructed"),
                InOwner, InType, DestHandles.Num(), InPreCopyHandles.Num());
            return;
        }

        for (auto Index = 0; Index < DestHandles.Num(); ++Index)
        {
            const auto& PreCopyHandle = InPreCopyHandles[Index];

            if (ck::IsValid(*DestHandles[Index]) || ck::Is_NOT_Valid(PreCopyHandle))
            { continue; }

            ck::dynamic::Verbose(TEXT("v3 hydrate: keeping the construction-fresh handle [{}] on Entity [{}] "
                "fragment [{}] — the saved value did not resolve"), PreCopyHandle, InOwner, InType);

            *DestHandles[Index] = PreCopyHandle;
        }
    }
}

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
                    return ck_dynamic_fragment::IsSnapshotTransient(InEntry.GetScriptStruct());
                });
                if (Fragments.IsEmpty())
                { return {}; }

                for (const auto& Entry : Fragments)
                {
                    const auto Schema = ck::dynamic::Validate_FragmentSchema(Entry.GetScriptStruct());
                    CK_ENSURE_IF_NOT(Schema.IsSafe,
                        TEXT("Refusing to save unsafe legacy Dynamic Fragment schema [{}] at [{}]: {}"),
                        Entry.GetScriptStruct(), Schema.FailurePath, Schema.FailureReason)
                    {}
                    if (NOT Schema.IsSafe)
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
                    if (ck_dynamic_fragment::IsSnapshotTransient(Type))
                    { continue; }

                    const auto Schema = ck::dynamic::Validate_FragmentSchema(Type);
                    CK_ENSURE_IF_NOT(Schema.IsSafe,
                        TEXT("Refusing to hydrate unsafe Dynamic Fragment schema [{}] at [{}]: {}"),
                        Type, Schema.FailurePath, Schema.FailureReason)
                    {}
                    if (NOT Schema.IsSafe)
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
                    if (ck_dynamic_fragment::IsSnapshotTransient(Type))
                    { continue; }

                    auto* Storage = UCk_Utils_DynamicFragment_UE::TryAddOrGet_Fragment_TypeUnsafe(InEntity, Type);
                    if (Storage == nullptr)
                    { return ECk_Persistence_ApplyResult::Rejected; }

                    ResolvedEntries.Emplace(&Entry, Storage);
                }

                // Commit every value before the first notification: a listener may destroy the entity, but it
                // can no longer observe a half-hydrated set.
                for (const auto& Resolved : ResolvedEntries)
                {
                    // Read first — the copy is what destroys them.
                    const auto PreCopyHandles = ck_dynamic_fragment::Collect_Handles(*Resolved.Value);

                    ck_dynamic_fragment::CopyFragment_PreservingLiveSessionFields(*Resolved.Key, *Resolved.Value);

                    ck_dynamic_fragment::Restore_UnresolvedHandles(
                        PreCopyHandles, *Resolved.Value, Resolved.Key->GetScriptStruct(), InEntity);
                }

                for (const auto& Resolved : ResolvedEntries)
                {
                    const auto* Type = Resolved.Key->GetScriptStruct();
                    auto Info = FCk_DynamicFragment_RepNotifyInfo{};
                    Info.ChangedType = const_cast<UScriptStruct*>(Type);
                    ck::UUtils_Signal_DynamicFragment_OnRepNotify::Broadcast(InEntity, ck::MakePayload(InEntity, Info));
                }

                // Re-arm the Replicate pass ONCE, never per-entry.
                if (InEntity.Has<ck::FFragment_DynamicFragment_ReplicatedTypes>())
                { InEntity.AddOrGet<ck::FTag_DynamicFragment_MayRequireReplication>(); }

                return ECk_Persistence_ApplyResult::Applied;
            }});
    }
} GCkDynamicFragmentsSaveHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
