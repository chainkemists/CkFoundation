#include "CkDynamic_Fragment_Data.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"
#include "CkEcs/Snapshot/CkSnapshot_Context.h"   // ck::FSnapshotContext::Snapshot_Handle (full type for the body)

#include "CkEcs/Handle/CkHandle.h"               // FCk_Handle::StaticStruct()

#include "UObject/UnrealType.h"                  // FStructProperty / FArrayProperty / FScriptArrayHelper / TFieldIterator

#include <StructUtils/InstancedStruct.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_dynamic_snapshot
{
    // A handle field is any struct DERIVED from FCk_Handle, not just the base. AngelScript-DYNAMIC handle types
    // (registered via UCkDynamic_HandleDefinition, e.g. FCk_Handle_QuickUseSelector) reflect as an FStructProperty
    // whose Struct IS FCk_Handle::StaticStruct() (see CkDynamic_AngelScriptType's CreateProperty), but C++-NATIVE
    // typed handles (FCk_Handle_IntegerAttribute, FCk_Handle_Inventory, FCk_Handle_Timer, ...) reflect as their own
    // typed UScriptStruct. An exact equality test silently skipped every native typed handle -- nothing was written
    // on save and nothing read on load, so those fields restored as default TOMBSTONES (the player held-item chain
    // dangled this way). IsChildOf catches both; the typed wrapper is layout-identical to the base
    // (static_assert(sizeof(FCk_Handle_TypeSafe) == sizeof(FCk_Handle)) in CkHandle_TypeSafe.h), so remapping
    // through a base pointer at the field address is complete.
    static auto
        Get_IsHandleStruct(
            const UScriptStruct* InStruct)
        -> bool
    {
        return InStruct != nullptr && InStruct->IsChildOf(FCk_Handle::StaticStruct());
    }

    // Route every FCk_Handle-derived field inside InStruct (top-level, inside arrays/sets/maps, and nested one or
    // more structs deep) through FSnapshotContext::Snapshot_Handle. Save writes each handle's entity id; load reads
    // it back through the continuous-loader remap. Save and load MUST visit the same handles in the same order -- a
    // single deterministic TFieldIterator walk guarantees that, and the container element counts/order are already
    // materialized by the preceding `InAr << _StructData` pass before this runs on load (TArray/TSet/TMap property
    // serialization preserves element order). TOptional fields are not walked (no fragment stores optional handles;
    // add FOptionalProperty support here if one ever does).
    static auto
        RemapHandles(
            const UScriptStruct* InStruct,
            void* InMemory,
            FArchive& InAr,
            ck::FSnapshotContext& InCtx)
        -> void
    {
        if (InStruct == nullptr || InMemory == nullptr)
        { return; }

        const auto RemapOrRecurse = [&](const FStructProperty* InStructProp, void* InElementMemory) -> void
        {
            if (Get_IsHandleStruct(InStructProp->Struct))
            { InCtx.Snapshot_Handle(InAr, *static_cast<FCk_Handle*>(InElementMemory)); }
            else
            { RemapHandles(InStructProp->Struct, InElementMemory, InAr, InCtx); }
        };

        const auto ContainsHandles = [](const FStructProperty* InStructProp) -> bool
        {
            // Cheap pre-filter for containers: only walk elements whose struct is (or can nest) a handle. A struct
            // that is not a handle may still nest one, so only a null struct-prop is skippable outright.
            return InStructProp != nullptr;
        };

        for (TFieldIterator<FProperty> PropIt(InStruct); PropIt; ++PropIt)
        {
            const FProperty* Property = *PropIt;

            if (const auto* StructProp = CastField<FStructProperty>(Property))
            {
                RemapOrRecurse(StructProp, StructProp->ContainerPtrToValuePtr<void>(InMemory));
            }
            else if (const auto* ArrayProp = CastField<FArrayProperty>(Property))
            {
                const auto* InnerStruct = CastField<FStructProperty>(ArrayProp->Inner);
                if (NOT ContainsHandles(InnerStruct))
                { continue; } // arrays of non-struct elements never hold handles.

                auto ArrayHelper = FScriptArrayHelper{ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InMemory)};

                for (auto Index = 0; Index < ArrayHelper.Num(); ++Index)
                { RemapOrRecurse(InnerStruct, ArrayHelper.GetRawPtr(Index)); }
            }
            else if (const auto* SetProp = CastField<FSetProperty>(Property))
            {
                const auto* ElemStruct = CastField<FStructProperty>(SetProp->ElementProp);
                if (NOT ContainsHandles(ElemStruct))
                { continue; }

                auto SetHelper = FScriptSetHelper{SetProp, SetProp->ContainerPtrToValuePtr<void>(InMemory)};

                for (auto It = SetHelper.CreateIterator(); It; ++It)
                { RemapOrRecurse(ElemStruct, SetHelper.GetElementPtr(It)); }

                // Load remapped the element ids IN PLACE, invalidating the hash buckets built from the stale ids
                // during the data pass -- rebuild them. Save never mutates, so no rehash is needed there.
                if (InAr.IsLoading())
                { SetHelper.Rehash(); }
            }
            else if (const auto* MapProp = CastField<FMapProperty>(Property))
            {
                const auto* KeyStruct   = CastField<FStructProperty>(MapProp->KeyProp);
                const auto* ValueStruct = CastField<FStructProperty>(MapProp->ValueProp);
                if (NOT ContainsHandles(KeyStruct) && NOT ContainsHandles(ValueStruct))
                { continue; }

                auto MapHelper = FScriptMapHelper{MapProp, MapProp->ContainerPtrToValuePtr<void>(InMemory)};

                for (auto It = MapHelper.CreateIterator(); It; ++It)
                {
                    if (ContainsHandles(KeyStruct))
                    { RemapOrRecurse(KeyStruct, MapHelper.GetKeyPtr(It)); }
                    if (ContainsHandles(ValueStruct))
                    { RemapOrRecurse(ValueStruct, MapHelper.GetValuePtr(It)); }
                }

                // Same in-place key mutation concern as sets: a remapped handle KEY hashes differently -- rehash.
                if (InAr.IsLoading() && ContainsHandles(KeyStruct))
                { MapHelper.Rehash(); }
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_DynamicFragment_Data::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& InCtx)
    -> void
{
    // (1) Serialize the held struct's TYPE + data. The proxy archive has ArIsSaveGame=true, so non-handle fields
    // round-trip via their CPF_SaveGame flag; FCk_Handle fields write ZERO bytes here (FCk_Handle's _Entity /
    // _RegistryHandle are not SaveGame), so there is no double-serialize with step (2). On load this materializes
    // the struct + any arrays first, so (2) only rewrites the handle ids in place.
    _StructData.Serialize(InAr);

    // (2) Remap every FCk_Handle the held struct carries onto the restored entities (a no-op set of stale ids
    // otherwise -- see CkSnapshot_Context.h). GetMutableMemory is valid on both save (read) and load (write).
    ck_dynamic_snapshot::RemapHandles(_StructData.GetScriptStruct(), _StructData.GetMutableMemory(), InAr, InCtx);
}

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_DynamicFragment_Data);
