#include "CkEcs/Snapshot/CkSnapshot_HandleWalk.h"

#include "CkEcs/Handle/CkHandle.h" // FCk_Handle::StaticStruct()

#include "UObject/UnrealType.h"    // FStructProperty / FArrayProperty / FScriptArrayHelper / TFieldIterator

#include <StructUtils/InstancedStruct.h> // FInstancedStruct (nested-payload recursion)

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    namespace ck_snapshot_handlewalk
    {
        // A handle field is any struct DERIVED from FCk_Handle, not just the base. AngelScript-DYNAMIC handle types
        // reflect as an FStructProperty whose Struct IS FCk_Handle::StaticStruct(), but C++-NATIVE typed handles
        // (FCk_Handle_Item, FCk_Handle_Timer, ...) reflect as their own typed UScriptStruct. An exact equality test
        // silently skips every native typed handle (nothing written on save, restored as default TOMBSTONES).
        // IsChildOf catches both; the typed wrapper is layout-identical to the base
        // (static_assert(sizeof(FCk_Handle_TypeSafe) == sizeof(FCk_Handle)) in CkHandle_TypeSafe.h), so remapping
        // through a base pointer at the field address is complete.
        auto
            Get_IsHandleStruct(
                const UScriptStruct* InStruct)
            -> bool
        {
            return InStruct != nullptr && InStruct->IsChildOf(FCk_Handle::StaticStruct());
        }

        // Single deterministic walk shared by RemapHandles (visitor = Snapshot_Handle, rehash on load) and
        // ForEachHandle (visitor = caller's, no mutation ⇒ no rehash). InRehashAfterKeyVisit rebuilds set/map hash
        // buckets after their keys were visited IN PLACE (a remapped key hashes differently). TOptional fields are
        // not walked (no fragment/params struct stores optional handles; add FOptionalProperty support if one does).
        auto
            WalkHandles(
                const UScriptStruct* InStruct,
                void* InMemory,
                const TFunctionRef<void(FCk_Handle&)>& InVisit,
                bool InRehashAfterKeyVisit)
            -> void
        {
            if (InStruct == nullptr || InMemory == nullptr)
            { return; }

            const auto VisitOrRecurse = [&](const FStructProperty* InStructProp, void* InElementMemory) -> void
            {
                // An FInstancedStruct is opaque to TFieldIterator: its stored struct's properties (and any handle
                // fields inside them) live behind GetScriptStruct()/GetMutableMemory(), not among the wrapper's own
                // reflected fields (which carry none). Recurse into the payload so a handle nested in a dynamic
                // fragment / spawn-params instanced struct is remapped on save and rehashed on load. Appended AHEAD
                // of the handle-struct test — never reordering the existing checks — so every pre-existing handle is
                // still visited in its original position (the save-file handle-id stream is positional).
                if (InStructProp->Struct == FInstancedStruct::StaticStruct())
                {
                    auto& Instanced = *static_cast<FInstancedStruct*>(InElementMemory);
                    if (Instanced.IsValid())
                    { WalkHandles(Instanced.GetScriptStruct(), Instanced.GetMutableMemory(), InVisit, InRehashAfterKeyVisit); }
                    return;
                }

                if (Get_IsHandleStruct(InStructProp->Struct))
                { InVisit(*static_cast<FCk_Handle*>(InElementMemory)); }
                else
                { WalkHandles(InStructProp->Struct, InElementMemory, InVisit, InRehashAfterKeyVisit); }
            };

            const auto ContainsHandles = [](const FStructProperty* InStructProp) -> bool
            {
                // Cheap pre-filter for containers: a struct that is not a handle may still nest one, so only a null
                // struct-prop is skippable outright.
                return InStructProp != nullptr;
            };

            for (TFieldIterator<FProperty> PropIt(InStruct); PropIt; ++PropIt)
            {
                const FProperty* Property = *PropIt;

                if (const auto* StructProp = CastField<FStructProperty>(Property))
                {
                    VisitOrRecurse(StructProp, StructProp->ContainerPtrToValuePtr<void>(InMemory));
                }
                else if (const auto* ArrayProp = CastField<FArrayProperty>(Property))
                {
                    const auto* InnerStruct = CastField<FStructProperty>(ArrayProp->Inner);
                    if (NOT ContainsHandles(InnerStruct))
                    { continue; } // arrays of non-struct elements never hold handles.

                    auto ArrayHelper = FScriptArrayHelper{ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InMemory)};

                    for (auto Index = 0; Index < ArrayHelper.Num(); ++Index)
                    { VisitOrRecurse(InnerStruct, ArrayHelper.GetRawPtr(Index)); }
                }
                else if (const auto* SetProp = CastField<FSetProperty>(Property))
                {
                    const auto* ElemStruct = CastField<FStructProperty>(SetProp->ElementProp);
                    if (NOT ContainsHandles(ElemStruct))
                    { continue; }

                    auto SetHelper = FScriptSetHelper{SetProp, SetProp->ContainerPtrToValuePtr<void>(InMemory)};

                    for (auto It = SetHelper.CreateIterator(); It; ++It)
                    { VisitOrRecurse(ElemStruct, SetHelper.GetElementPtr(It)); }

                    // A visited element id mutated in place invalidates the hash buckets built during the data pass.
                    if (InRehashAfterKeyVisit)
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
                        { VisitOrRecurse(KeyStruct, MapHelper.GetKeyPtr(It)); }
                        if (ContainsHandles(ValueStruct))
                        { VisitOrRecurse(ValueStruct, MapHelper.GetValuePtr(It)); }
                    }

                    // Same in-place key mutation concern as sets: a remapped handle KEY hashes differently.
                    if (InRehashAfterKeyVisit && ContainsHandles(KeyStruct))
                    { MapHelper.Rehash(); }
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        RemapHandles(
            const UScriptStruct* InStruct,
            void* InMemory,
            FArchive& InAr,
            ck::FSnapshotContext& InCtx)
        -> void
    {
        const auto RehashAfterKeyVisit = InAr.IsLoading(); // save never mutates, so no rehash there
        ck_snapshot_handlewalk::WalkHandles(InStruct, InMemory,
            [&](FCk_Handle& InOutHandle) -> void
            {
                InCtx.Snapshot_Handle(InAr, InOutHandle);
            },
            RehashAfterKeyVisit);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        ForEachHandle(
            const UScriptStruct* InStruct,
            void* InMemory,
            const TFunctionRef<void(FCk_Handle&)>& InVisitor)
        -> void
    {
        constexpr auto RehashAfterKeyVisit = false; // read-only visit, no in-place mutation
        ck_snapshot_handlewalk::WalkHandles(InStruct, InMemory, InVisitor, RehashAfterKeyVisit);
    }
}

// --------------------------------------------------------------------------------------------------------------------
