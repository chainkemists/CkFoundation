#include "CkEcs/Snapshot/CkSnapshot_HandleWalk.h"

#include "CkEcs/Handle/CkHandle.h" // FCk_Handle::StaticStruct()
#include "CkEcs/Snapshot/CkSnapshot_Posture.h" // ck::Get_FragmentPosture — the audit walk's descent gate

#include "UObject/UnrealType.h"    // FStructProperty / FArrayProperty / FScriptArrayHelper / TFieldIterator
#include "UObject/SoftObjectPtr.h" // FSoftObjectPtr::Get — the object-ref audit resolves at capture

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

        struct FWalkOptions
        {
            bool RehashAfterKeyVisit = false;

            // Field paths cost a string concatenation per node, so only the audit — which runs once per save and
            // must NAME what it found — asks for them. The remap walk runs on every payload on save AND load.
            bool BuildFieldPaths = false;

            // Descend into an FInstancedStruct only when its payload is DURABLE. Audit-only, and deliberately NOT
            // how the remap walk behaves: the save's handle-id stream is positional, so the remap must visit every
            // handle regardless of posture or the two sides stop agreeing on slot i.
            bool DurableOnlyInstancedDescent = false;
        };

        // TOptional fields are deliberately not walked — no fragment/params struct stores optional handles;
        // add FOptionalProperty support here if one ever does.
        auto
            WalkHandles(
                const UScriptStruct* InStruct,
                void* InMemory,
                const TFunctionRef<void(FCk_Handle&, const FString&)>& InVisit,
                const FWalkOptions& InOptions,
                const FString& InPath)
            -> void
        {
            if (InStruct == nullptr || InMemory == nullptr)
            { return; }

            const auto MakePath = [&](const FString& InLeaf) -> FString
            {
                if (NOT InOptions.BuildFieldPaths)
                { return {}; }

                return InPath.IsEmpty() ? InLeaf : InPath + TEXT(".") + InLeaf;
            };

            const auto VisitOrRecurse = [&](const FStructProperty* InStructProp, void* InElementMemory,
                                            const FString& InElementPath) -> void
            {
                // An FInstancedStruct is opaque to TFieldIterator — its payload's handles live behind
                // GetScriptStruct()/GetMutableMemory(). This test must stay AHEAD of the handle-struct test
                // and the later checks must not be reordered: the save-file handle-id stream is positional.
                if (InStructProp->Struct == FInstancedStruct::StaticStruct())
                {
                    auto& Instanced = *static_cast<FInstancedStruct*>(InElementMemory);
                    if (NOT Instanced.IsValid())
                    { return; }

                    if (InOptions.DurableOnlyInstancedDescent &&
                        ck::Get_FragmentPosture(Instanced.GetScriptStruct()) != ECk_Snapshot_Posture::Durable)
                    { return; }

                    WalkHandles(Instanced.GetScriptStruct(), Instanced.GetMutableMemory(), InVisit, InOptions,
                        InOptions.BuildFieldPaths && Instanced.GetScriptStruct() != nullptr
                            ? InElementPath + TEXT("<") + Instanced.GetScriptStruct()->GetName() + TEXT(">")
                            : InElementPath);
                    return;
                }

                if (Get_IsHandleStruct(InStructProp->Struct))
                { InVisit(*static_cast<FCk_Handle*>(InElementMemory), InElementPath); }
                else
                { WalkHandles(InStructProp->Struct, InElementMemory, InVisit, InOptions, InElementPath); }
            };

            const auto MayContainHandles = [](const FStructProperty* InStructProp) -> bool
            {
                return InStructProp != nullptr;
            };

            for (TFieldIterator<FProperty> PropIt(InStruct); PropIt; ++PropIt)
            {
                const FProperty* Property = *PropIt;

                // CPF_Transient = "never persisted": a Transient handle field is a LIVE-SESSION injection (e.g. a
                // spawn-param handle to boot infrastructure like a DayCycle) that the owner re-acquires after a load.
                // Skipping it here excludes it from the save's handle stream AND from capture's persisted-target
                // validation — symmetric on save and load (the walk is layout-driven on both sides), so the
                // positional stream stays consistent. Without this skip, a spawn-param ref to a non-persisted
                // entity makes the whole recipe unresolvable and the entity is ORPHANED at load (unresolved-other).
                if (Property->HasAnyPropertyFlags(CPF_Transient))
                { continue; }

                if (const auto* StructProp = CastField<FStructProperty>(Property))
                {
                    VisitOrRecurse(StructProp, StructProp->ContainerPtrToValuePtr<void>(InMemory),
                        MakePath(StructProp->GetName()));
                }
                else if (const auto* ArrayProp = CastField<FArrayProperty>(Property))
                {
                    const auto* InnerStruct = CastField<FStructProperty>(ArrayProp->Inner);
                    if (NOT MayContainHandles(InnerStruct))
                    { continue; } // arrays of non-struct elements never hold handles.

                    auto ArrayHelper = FScriptArrayHelper{ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InMemory)};

                    const auto ArrayPath = MakePath(ArrayProp->GetName());
                    for (auto Index = 0; Index < ArrayHelper.Num(); ++Index)
                    {
                        VisitOrRecurse(InnerStruct, ArrayHelper.GetRawPtr(Index),
                            InOptions.BuildFieldPaths ? FString::Printf(TEXT("%s[%d]"), *ArrayPath, Index) : ArrayPath);
                    }
                }
                else if (const auto* SetProp = CastField<FSetProperty>(Property))
                {
                    const auto* ElemStruct = CastField<FStructProperty>(SetProp->ElementProp);
                    if (NOT MayContainHandles(ElemStruct))
                    { continue; }

                    auto SetHelper = FScriptSetHelper{SetProp, SetProp->ContainerPtrToValuePtr<void>(InMemory)};

                    const auto SetPath = MakePath(SetProp->GetName());
                    for (auto It = SetHelper.CreateIterator(); It; ++It)
                    { VisitOrRecurse(ElemStruct, SetHelper.GetElementPtr(It), SetPath); }

                    // A visited element id mutated in place invalidates the hash buckets built during the data pass.
                    if (InOptions.RehashAfterKeyVisit)
                    { SetHelper.Rehash(); }
                }
                else if (const auto* MapProp = CastField<FMapProperty>(Property))
                {
                    const auto* KeyStruct   = CastField<FStructProperty>(MapProp->KeyProp);
                    const auto* ValueStruct = CastField<FStructProperty>(MapProp->ValueProp);
                    if (NOT MayContainHandles(KeyStruct) && NOT MayContainHandles(ValueStruct))
                    { continue; }

                    auto MapHelper = FScriptMapHelper{MapProp, MapProp->ContainerPtrToValuePtr<void>(InMemory)};

                    const auto MapPath = MakePath(MapProp->GetName());
                    for (auto It = MapHelper.CreateIterator(); It; ++It)
                    {
                        if (MayContainHandles(KeyStruct))
                        { VisitOrRecurse(KeyStruct, MapHelper.GetKeyPtr(It), MapPath + TEXT("<key>")); }
                        if (MayContainHandles(ValueStruct))
                        { VisitOrRecurse(ValueStruct, MapHelper.GetValuePtr(It), MapPath + TEXT("<value>")); }
                    }

                    if (InOptions.RehashAfterKeyVisit && MayContainHandles(KeyStruct))
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
        const auto Options = ck_snapshot_handlewalk::FWalkOptions{.RehashAfterKeyVisit = InAr.IsLoading()};
        ck_snapshot_handlewalk::WalkHandles(InStruct, InMemory,
            [&](FCk_Handle& InOutHandle, const FString& /*InFieldPath*/) -> void
            {
                InCtx.Snapshot_Handle(InAr, InOutHandle);
            },
            Options, FString{});
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        ForEachHandle(
            const UScriptStruct* InStruct,
            void* InMemory,
            const TFunctionRef<void(FCk_Handle&)>& InVisitor)
        -> void
    {
        // read-only visit, no in-place mutation
        ck_snapshot_handlewalk::WalkHandles(InStruct, InMemory,
            [&](FCk_Handle& InOutHandle, const FString& /*InFieldPath*/) -> void
            {
                InVisitor(InOutHandle);
            },
            ck_snapshot_handlewalk::FWalkOptions{}, FString{});
    }

    // ----------------------------------------------------------------------------------------------------------------

    namespace ck_snapshot_objectrefwalk
    {
        // Mirrors WalkHandles' descent (transient skip, arrays, sets, maps, durable-only instanced payloads) but
        // visits object properties instead of handle structs. The duplication is deliberate: see the header on why
        // this is not a flag on the shared walk.
        auto
            WalkObjectRefs(
                const UScriptStruct* InStruct,
                void* InMemory,
                const TFunctionRef<void(const UObject&, const FString&)>& InVisit,
                const FString& InPath)
            -> void
        {
            if (InStruct == nullptr || InMemory == nullptr)
            { return; }

            const auto MakePath = [&](const FString& InLeaf) -> FString
            {
                return InPath.IsEmpty() ? InLeaf : InPath + TEXT(".") + InLeaf;
            };

            // Resolves without loading. A soft ref to an asset that is merely not in memory yields null and is
            // skipped -- that path is valid and resolves after a load, so reporting it would be a false positive.
            const auto VisitObject = [&](const UObject* InObject, const FString& InFieldPath) -> void
            {
                if (InObject == nullptr)
                { return; }

                InVisit(*InObject, InFieldPath);
            };

            const auto VisitLeaf = [&](const FProperty* InProperty, void* InValueMemory,
                                       const FString& InLeafPath) -> void
            {
                if (const auto* SoftProp = CastField<FSoftObjectProperty>(InProperty))
                {
                    const auto& Soft = *static_cast<const FSoftObjectPtr*>(InValueMemory);
                    if (Soft.IsNull())
                    { return; }

                    VisitObject(Soft.Get(), InLeafPath);
                }
                else if (const auto* ObjectProp = CastField<FObjectProperty>(InProperty))
                { VisitObject(ObjectProp->GetObjectPropertyValue(InValueMemory), InLeafPath); }
                else if (const auto* StructProp = CastField<FStructProperty>(InProperty))
                {
                    if (StructProp->Struct == FInstancedStruct::StaticStruct())
                    {
                        auto& Instanced = *static_cast<FInstancedStruct*>(InValueMemory);
                        if (NOT Instanced.IsValid())
                        { return; }

                        // Same descent gate as the handle audit: a SESSION payload is rebuilt by its owner's setup,
                        // so whatever it names is not this save's problem.
                        if (ck::Get_FragmentPosture(Instanced.GetScriptStruct()) != ECk_Snapshot_Posture::Durable)
                        { return; }

                        WalkObjectRefs(Instanced.GetScriptStruct(), Instanced.GetMutableMemory(), InVisit,
                            Instanced.GetScriptStruct() != nullptr
                                ? InLeafPath + TEXT("<") + Instanced.GetScriptStruct()->GetName() + TEXT(">")
                                : InLeafPath);
                        return;
                    }

                    WalkObjectRefs(StructProp->Struct, InValueMemory, InVisit, InLeafPath);
                }
            };

            for (TFieldIterator<FProperty> PropIt(InStruct); PropIt; ++PropIt)
            {
                const FProperty* Property = *PropIt;

                // CPF_Transient is the author saying "never persisted" -- the same exclusion the handle walk makes,
                // and for the same reason: the field is a live-session injection, not part of the saved world.
                if (Property->HasAnyPropertyFlags(CPF_Transient))
                { continue; }

                if (const auto* ArrayProp = CastField<FArrayProperty>(Property))
                {
                    auto ArrayHelper = FScriptArrayHelper{ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(InMemory)};

                    const auto ArrayPath = MakePath(ArrayProp->GetName());
                    for (auto Index = 0; Index < ArrayHelper.Num(); ++Index)
                    {
                        VisitLeaf(ArrayProp->Inner, ArrayHelper.GetRawPtr(Index),
                            FString::Printf(TEXT("%s[%d]"), *ArrayPath, Index));
                    }
                }
                else if (const auto* SetProp = CastField<FSetProperty>(Property))
                {
                    auto SetHelper = FScriptSetHelper{SetProp, SetProp->ContainerPtrToValuePtr<void>(InMemory)};

                    const auto SetPath = MakePath(SetProp->GetName());
                    for (auto It = SetHelper.CreateIterator(); It; ++It)
                    { VisitLeaf(SetProp->ElementProp, SetHelper.GetElementPtr(It), SetPath); }
                }
                else if (const auto* MapProp = CastField<FMapProperty>(Property))
                {
                    auto MapHelper = FScriptMapHelper{MapProp, MapProp->ContainerPtrToValuePtr<void>(InMemory)};

                    const auto MapPath = MakePath(MapProp->GetName());
                    for (auto It = MapHelper.CreateIterator(); It; ++It)
                    {
                        VisitLeaf(MapProp->KeyProp,   MapHelper.GetKeyPtr(It),   MapPath + TEXT("<key>"));
                        VisitLeaf(MapProp->ValueProp, MapHelper.GetValuePtr(It), MapPath + TEXT("<value>"));
                    }
                }
                else
                { VisitLeaf(Property, Property->ContainerPtrToValuePtr<void>(InMemory), MakePath(Property->GetName())); }
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        ForEachDurableObjectRef(
            const UScriptStruct* InStruct,
            void* InMemory,
            const TFunctionRef<void(const UObject&, const FString&)>& InVisitor)
        -> void
    {
        ck_snapshot_objectrefwalk::WalkObjectRefs(InStruct, InMemory, InVisitor, FString{});
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        ForEachDurableHandle(
            const UScriptStruct* InStruct,
            void* InMemory,
            const TFunctionRef<void(FCk_Handle&, const FString&)>& InVisitor)
        -> void
    {
        const auto Options = ck_snapshot_handlewalk::FWalkOptions{
            .RehashAfterKeyVisit = false,
            .BuildFieldPaths = true,
            .DurableOnlyInstancedDescent = true};

        ck_snapshot_handlewalk::WalkHandles(InStruct, InMemory, InVisitor, Options, FString{});
    }
}

// --------------------------------------------------------------------------------------------------------------------
