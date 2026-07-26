#include "CkEcs/Snapshot/CkSnapshot_HandleWalk.h"

#include "CkEcs/Handle/CkHandle.h" // FCk_Handle::StaticStruct()

#include "UObject/UnrealType.h"    // FStructProperty / FArrayProperty / FScriptArrayHelper / TFieldIterator

#include <StructUtils/InstancedStruct.h> // FInstancedStruct (nested-payload recursion)

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot
{
    namespace ck_snapshot_handlewalk
    {
        // IsChildOf, never equality: native typed handles reflect as their own UScriptStruct, so an exact
        // test silently skips every one of them (nothing saved, restored as default TOMBSTONES). The typed
        // wrapper is layout-identical to the base, so remapping through a base pointer is complete.
        auto
            Get_IsHandleStruct(
                const UScriptStruct* InStruct)
            -> bool
        {
            return InStruct != nullptr && InStruct->IsChildOf(FCk_Handle::StaticStruct());
        }

        // TOptional fields are deliberately not walked — no fragment/params struct stores optional handles;
        // add FOptionalProperty support here if one ever does.
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
                // An FInstancedStruct is opaque to TFieldIterator — its payload's handles live behind
                // GetScriptStruct()/GetMutableMemory(). This test must stay AHEAD of the handle-struct test
                // and the later checks must not be reordered: the save-file handle-id stream is positional.
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

            const auto MayContainHandles = [](const FStructProperty* InStructProp) -> bool
            {
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
                    if (NOT MayContainHandles(InnerStruct))
                    { continue; } // arrays of non-struct elements never hold handles.

                    auto ArrayHelper = FScriptArrayHelper{ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InMemory)};

                    for (auto Index = 0; Index < ArrayHelper.Num(); ++Index)
                    { VisitOrRecurse(InnerStruct, ArrayHelper.GetRawPtr(Index)); }
                }
                else if (const auto* SetProp = CastField<FSetProperty>(Property))
                {
                    const auto* ElemStruct = CastField<FStructProperty>(SetProp->ElementProp);
                    if (NOT MayContainHandles(ElemStruct))
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
                    if (NOT MayContainHandles(KeyStruct) && NOT MayContainHandles(ValueStruct))
                    { continue; }

                    auto MapHelper = FScriptMapHelper{MapProp, MapProp->ContainerPtrToValuePtr<void>(InMemory)};

                    for (auto It = MapHelper.CreateIterator(); It; ++It)
                    {
                        if (MayContainHandles(KeyStruct))
                        { VisitOrRecurse(KeyStruct, MapHelper.GetKeyPtr(It)); }
                        if (MayContainHandles(ValueStruct))
                        { VisitOrRecurse(ValueStruct, MapHelper.GetValuePtr(It)); }
                    }

                    if (InRehashAfterKeyVisit && MayContainHandles(KeyStruct))
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
