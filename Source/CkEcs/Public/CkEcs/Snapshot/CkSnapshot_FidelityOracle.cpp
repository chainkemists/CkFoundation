#include "CkEcs/Snapshot/CkSnapshot_FidelityOracle.h"

#if CK_WITH_FIDELITY_ORACLE

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Registry/CkRegistry.h"                   // ck::FCtx_TransientEntity
#include "CkEcs/Handle/CkHandle.h"                        // ck::FTag_DestroyEntity_*
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"   // ck::FFragment_EntityScript_Current
#include "CkEcs/EntityScript/CkEntityScript.h"            // UCk_EntityScript_UE (GetClass)

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
        // One accumulator per entity: fragment names collected as we sweep each storage once (O(fragment instances),
        // not O(entities x storages)).
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

            const auto DisplayName = ck_snapshot_oracle::FragmentDisplayName(TypeHash);

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

        auto Image = FCk_Oracle_StructuralImage{};
        Image._Entities.Reserve(ByEntity.Num());

        for (auto& Pair : ByEntity)
        {
            auto& Acc = Pair.Value;

            if (TransientId.IsSet() && Pair.Key == TransientId.GetValue())
            { continue; }

            if (ck_snapshot_oracle::IsMarkedForDestruction(InRegistry, Acc._Entity))
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

            Image._Entities.Emplace(MoveTemp(Signature));
        }

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
}

#endif // CK_WITH_FIDELITY_ORACLE

// --------------------------------------------------------------------------------------------------------------------
