#include "CkEcs/Snapshot/CkSnapshot_FidelityOracle.h"

#if CK_WITH_FIDELITY_ORACLE

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Registry/CkRegistry.h"                   // ck::FCtx_TransientEntity, FCk_Registry
#include "CkEcs/Handle/CkHandle.h"                        // ck::FTag_DestroyEntity_*
#include "CkEcs/Handle/CkHandle_Utils.h"                  // ck::MakeHandle (Tier-2 per-entity handle)
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"   // ck::FFragment_EntityScript_Current
#include "CkEcs/EntityScript/CkEntityScript.h"            // UCk_EntityScript_UE (GetClass)
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h" // handler registry (Produce)

#include "Algo/Sort.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::snapshot_oracle
{
    namespace ck_snapshot_oracle
    {
        // A registered snapshotable type surfaces its clean display name; anything else is keyed by its entt
        // type-hash so the signature stays stable without touching the (non-null-terminated) entt name view.
        auto FragmentDisplayName(uint32 InTypeHash) -> FString
        {
            const auto* Registered = ck::FCk_Snapshot_FragmentRegistry::Get().Find_ByEnttHash(InTypeHash);
            if (Registered != nullptr)
            { return Registered->_DisplayName; }

            return FString::Printf(TEXT("EnttType_0x%08X"), InTypeHash);
        }

        auto IsMarkedForDestruction(ck::SnapshotRegistryType& InRegistry, entt::entity InEntity) -> bool
        {
            return InRegistry.any_of<
                ck::FTag_DestroyEntity_Initiate,
                ck::FTag_DestroyEntity_EndPlay,
                ck::FTag_DestroyEntity_Teardown,
                ck::FTag_DestroyEntity_Await,
                ck::FTag_DestroyEntity_Finalize>(InEntity);
        }

        // One real (non-transient, non-destroying) entity + its structural signature. Shared by both tiers.
        struct FEntityWithSignature
        {
            entt::entity               _Entity = entt::null;
            FCk_Oracle_EntitySignature _Signature;
        };

        // Sweep every storage once (O(fragment instances), not O(entities x storages)), accumulate each entity's
        // fragment/tag display names + EntityScript class path, and drop the transient + destruction-marked entities.
        // This is the single source of the Tier-1 signature; both Capture_Structural and Capture_Payloads key off it.
        auto BuildEntitySignatures(
            ck::SnapshotRegistryType& InRegistry,
            const TSet<uint32>* InRegisteredEnttHashes)
            -> TArray<FEntityWithSignature>
        {
            struct FAccum
            {
                entt::entity    _Entity = entt::null;
                TArray<FString> _FragmentTypeNames;
            };
            auto ByEntity = TMap<uint32, FAccum>{};

            const auto EntityStorageHash = static_cast<uint32>(entt::type_hash<ck::SnapshotEntityType>::value());

            for (auto&& StoragePair : InRegistry.storage())
            {
                auto& Storage = StoragePair.second;
                const auto TypeHash = static_cast<uint32>(Storage.info().hash());

                // The entity storage itself is present in storage() — it "contains" every entity and is not a fragment.
                if (TypeHash == EntityStorageHash)
                { continue; }

                if (InRegisteredEnttHashes != nullptr && NOT InRegisteredEnttHashes->Contains(TypeHash))
                { continue; }

                const auto DisplayName = FragmentDisplayName(TypeHash);

                for (const auto Entity : Storage)
                {
                    auto& Acc = ByEntity.FindOrAdd(static_cast<uint32>(Entity));
                    Acc._Entity = Entity;
                    Acc._FragmentTypeNames.Add(DisplayName);
                }
            }

            // Resolve the transient entity so it can be excluded (bookkeeping, not world state).
            auto TransientId = TOptional<uint32>{};
            if (const auto* TransientCtx = InRegistry.ctx().find<const ck::FCtx_TransientEntity>())
            { TransientId = static_cast<uint32>(TransientCtx->Entity.Get_ID()); }

            auto Result = TArray<FEntityWithSignature>{};
            Result.Reserve(ByEntity.Num());

            for (auto& Pair : ByEntity)
            {
                auto& Acc = Pair.Value;

                if (TransientId.IsSet() && Pair.Key == TransientId.GetValue())
                { continue; }

                if (IsMarkedForDestruction(InRegistry, Acc._Entity))
                { continue; }

                auto Signature = FCk_Oracle_EntitySignature{};

                Signature._FragmentTypeNames = MoveTemp(Acc._FragmentTypeNames);
                Signature._FragmentTypeNames.Sort();

                // _LabelPath stays empty: CkEcs cannot reach CkLabel (tier direction). See the header note.

                if (const auto* ScriptFrag = InRegistry.try_get<ck::FFragment_EntityScript_Current>(Acc._Entity))
                {
                    if (const auto* Script = ScriptFrag->Get_Script().Get())
                    { Signature._ScriptClassPath = Script->GetClass()->GetPathName(); }
                }

                Result.Emplace(FEntityWithSignature{Acc._Entity, MoveTemp(Signature)});
            }

            return Result;
        }

        // Export an FInstancedStruct's value to a stable text representation for a payload diff (spec §5: ExportText).
        auto ExportPayload(const FInstancedStruct& InPayload) -> FString
        {
            const auto* Struct = InPayload.GetScriptStruct();
            if (Struct == nullptr)
            { return TEXT("<empty>"); }

            auto Out = FString{};
            Struct->ExportText(Out, InPayload.GetMemory(), /*Delta=*/nullptr, /*OwnerObject=*/nullptr,
                PPF_None, /*ExportRootScope=*/nullptr);
            return Out;
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Oracle_EntitySignature::
        ToString() const
        -> FString
    {
        return FString::Printf(TEXT("L=[%s]|S=[%s]|F=[%s]|T=[%s]"),
            *_LabelPath,
            *_ScriptClassPath,
            *FString::Join(_FragmentTypeNames, TEXT(",")),
            *FString::Join(_TagTypeNames, TEXT(",")));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        Capture_Structural(
            ck::SnapshotRegistryType& InRegistry,
            const TSet<uint32>* InRegisteredEnttHashes)
        -> FCk_Oracle_StructuralImage
    {
        auto Image = FCk_Oracle_StructuralImage{};

        auto Signatures = ck_snapshot_oracle::BuildEntitySignatures(InRegistry, InRegisteredEnttHashes);
        Image._Entities.Reserve(Signatures.Num());
        for (auto& Entry : Signatures)
        { Image._Entities.Emplace(MoveTemp(Entry._Signature)); }

        Algo::Sort(Image._Entities, [](const FCk_Oracle_EntitySignature& InA, const FCk_Oracle_EntitySignature& InB)
        {
            return InA.ToString() < InB.ToString();
        });

        return Image;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        Diff_Structural(
            const FCk_Oracle_StructuralImage& InBefore,
            const FCk_Oracle_StructuralImage& InAfter,
            const TArray<FString>& InAllowlist,
            TArray<FString>& OutAnnotated)
        -> TArray<FString>
    {
        // Multiset difference keyed by signature string: before decrements, after increments. A non-zero net count
        // is an added (+) or removed (-) signature.
        auto Counts = TMap<FString, int32>{};
        for (const auto& Entity : InBefore._Entities)
        { Counts.FindOrAdd(Entity.ToString())--; }
        for (const auto& Entity : InAfter._Entities)
        { Counts.FindOrAdd(Entity.ToString())++; }

        auto Keys = TArray<FString>{};
        Counts.GetKeys(Keys);
        Keys.Sort();

        auto Result = TArray<FString>{};

        for (const auto& Key : Keys)
        {
            const auto Delta = Counts[Key];
            if (Delta == 0)
            { continue; }

            const auto* Sign = Delta > 0 ? TEXT("+") : TEXT("-");
            const auto bAnnotated = InAllowlist.ContainsByPredicate(
                [&Key](const FString& InPrefix) { return Key.StartsWith(InPrefix); });

            for (auto Index = 0; Index < FMath::Abs(Delta); ++Index)
            {
                auto Line = FString::Printf(TEXT("%s %s"), Sign, *Key);
                if (bAnnotated)
                { OutAnnotated.Add(MoveTemp(Line)); }
                else
                { Result.Add(MoveTemp(Line)); }
            }
        }

        return Result;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        Capture_Payloads(
            ck::SnapshotRegistryType& InRegistry,
            FCk_RegistryHandle InRegistryHandle,
            const TSet<uint32>* InRegisteredEnttHashes)
        -> FCk_Oracle_PayloadImage
    {
        auto Image = FCk_Oracle_PayloadImage{};

        const auto ProduceTypes = FCk_ReplicatedFragmentHandlerRegistry::Get_ProduceHandlerTypes();
        if (ProduceTypes.IsEmpty())
        { return Image; }

        auto CkRegistry = FCk_Registry{InRegistryHandle};

        for (auto& Entry : ck_snapshot_oracle::BuildEntitySignatures(InRegistry, InRegisteredEnttHashes))
        {
            auto Handle = ck::MakeHandle(FCk_Entity{Entry._Entity}, CkRegistry);
            if (ck::Is_NOT_Valid(Handle))
            { continue; }

            auto& Payloads = Image.FindOrAdd(Entry._Signature.ToString());

            for (const auto* Type : ProduceTypes)
            {
                const auto* Handler = FCk_ReplicatedFragmentHandlerRegistry::Resolve(Type);
                if (Handler == nullptr || NOT Handler->Produce)
                { continue; }

                auto Payload = Handler->Produce(Handle);
                if (NOT Payload.IsSet())
                { continue; } // feature absent on this entity (a SET-but-empty payload is still captured)

                Payloads.Emplace(Type->GetName(), MoveTemp(Payload.GetValue()));
            }
        }

        return Image;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        Diff_Payloads(
            const FCk_Oracle_PayloadImage& InBefore,
            const FCk_Oracle_PayloadImage& InAfter)
        -> TArray<FString>
    {
        // Group one image's (type -> payload) pairs under a signature key into type -> sorted ExportText strings, so a
        // (signature, type) with multiple structurally-identical entities compares as one composite unit.
        const auto GroupByType = [](const TArray<TPair<FString, FInstancedStruct>>* InEntries) -> TMap<FString, FString>
        {
            auto ByType = TMap<FString, TArray<FString>>{};
            if (InEntries != nullptr)
            {
                for (const auto& Pair : *InEntries)
                { ByType.FindOrAdd(Pair.Key).Add(ck_snapshot_oracle::ExportPayload(Pair.Value)); }
            }

            auto Composite = TMap<FString, FString>{};
            for (auto& Pair : ByType)
            {
                Pair.Value.Sort();
                Composite.Add(Pair.Key, FString::Join(Pair.Value, TEXT(";")));
            }
            return Composite;
        };

        auto Keys = TSet<FString>{};
        for (const auto& Pair : InBefore)
        { Keys.Add(Pair.Key); }
        for (const auto& Pair : InAfter)
        { Keys.Add(Pair.Key); }

        auto SortedKeys = Keys.Array();
        SortedKeys.Sort();

        auto Result = TArray<FString>{};

        for (const auto& Key : SortedKeys)
        {
            const auto Before = GroupByType(InBefore.Find(Key));
            const auto After  = GroupByType(InAfter.Find(Key));

            auto Types = TSet<FString>{};
            for (const auto& Pair : Before)
            { Types.Add(Pair.Key); }
            for (const auto& Pair : After)
            { Types.Add(Pair.Key); }

            auto SortedTypes = Types.Array();
            SortedTypes.Sort();

            for (const auto& Type : SortedTypes)
            {
                const auto* BeforeVal = Before.Find(Type);
                const auto* AfterVal  = After.Find(Type);

                if (BeforeVal != nullptr && AfterVal == nullptr)
                { Result.Add(FString::Printf(TEXT("- %s | %s"), *Key, *Type)); }
                else if (BeforeVal == nullptr && AfterVal != nullptr)
                { Result.Add(FString::Printf(TEXT("+ %s | %s"), *Key, *Type)); }
                else if (BeforeVal != nullptr && AfterVal != nullptr && *BeforeVal != *AfterVal)
                { Result.Add(FString::Printf(TEXT("~ %s | %s"), *Key, *Type)); }
            }
        }

        return Result;
    }
}

#endif // CK_WITH_FIDELITY_ORACLE

// --------------------------------------------------------------------------------------------------------------------
