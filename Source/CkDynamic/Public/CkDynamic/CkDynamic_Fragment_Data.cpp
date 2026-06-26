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
    // Route every FCk_Handle field inside InStruct (top-level, inside arrays, and nested one or more structs deep)
    // through FSnapshotContext::Snapshot_Handle. AngelScript reflects EVERY handle -- even typed ones like
    // FCk_Handle_Inventory -- as an FStructProperty whose Struct is FCk_Handle::StaticStruct() (see
    // CkDynamic_AngelScriptType), so an exact struct-equality test catches them all. Save writes each handle's
    // entity id; load reads it back through the continuous-loader remap. Save and load MUST visit the same handles
    // in the same order -- a single deterministic TFieldIterator walk guarantees that, and the array lengths are
    // already materialized by the preceding `InAr << _StructData` pass before this runs on load.
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

        for (TFieldIterator<FProperty> PropIt(InStruct); PropIt; ++PropIt)
        {
            const FProperty* Property = *PropIt;

            if (const auto* StructProp = CastField<FStructProperty>(Property))
            {
                if (StructProp->Struct == FCk_Handle::StaticStruct())
                {
                    auto* Handle = StructProp->ContainerPtrToValuePtr<FCk_Handle>(InMemory);
                    InCtx.Snapshot_Handle(InAr, *Handle);
                }
                else
                {
                    // Non-handle nested struct: recurse so handles nested inside it are not silently dropped.
                    auto* Nested = StructProp->ContainerPtrToValuePtr<void>(InMemory);
                    RemapHandles(StructProp->Struct, Nested, InAr, InCtx);
                }
            }
            else if (const auto* ArrayProp = CastField<FArrayProperty>(Property))
            {
                const auto* InnerStruct = CastField<FStructProperty>(ArrayProp->Inner);
                if (InnerStruct == nullptr)
                { continue; } // arrays of non-struct elements never hold handles.

                auto* ArrayPtr = ArrayProp->ContainerPtrToValuePtr<void>(InMemory);
                auto ArrayHelper = FScriptArrayHelper{ArrayProp, ArrayPtr};

                for (auto Index = 0; Index < ArrayHelper.Num(); ++Index)
                {
                    auto* ElementMemory = ArrayHelper.GetRawPtr(Index);

                    if (InnerStruct->Struct == FCk_Handle::StaticStruct())
                    { InCtx.Snapshot_Handle(InAr, *reinterpret_cast<FCk_Handle*>(ElementMemory)); }
                    else
                    { RemapHandles(InnerStruct->Struct, ElementMemory, InAr, InCtx); }
                }
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
