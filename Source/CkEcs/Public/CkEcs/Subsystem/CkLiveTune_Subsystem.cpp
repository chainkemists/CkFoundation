#include "CkLiveTune_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/IO/CkDeferredAssetInit_AngelScript.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_SpawnRecipe.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/LiveTune/CkLiveTune_Fragment.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Persistence/CkPersistenceHydration.h"
#include "CkEcs/Persistence/CkPersistenceHydration_Processor.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <UObject/UObjectGlobals.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_LiveTune_Subsystem_UE::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
#if WITH_EDITOR
    return Super::ShouldCreateSubsystem(InOuter);
#else
    return false;
#endif
}

auto
    UCk_LiveTune_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

#if WITH_EDITOR
    // The stamp-destroy sink below connects to the EcsWorld registry, so that registry must exist before
    // us AND deinitialize after us (reverse-init order) — the connection must never outlive the registry.
    InCollection.InitializeDependency<UCk_EcsWorld_Subsystem_UE>();

    _OnObjectPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(
        this, &UCk_LiveTune_Subsystem_UE::DoOnObjectPropertyChanged);

    _OnAssetsReinitializedHandle = UCk_DeferredAssetInit_UE::Get_OnAssetsReinitialized().AddUObject(
        this, &UCk_LiveTune_Subsystem_UE::DoOnAssetsReinitialized);

    auto* EcsWorldSubsystem = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    const auto EcsWorldSubsystemIsValid = ck::IsValid(EcsWorldSubsystem);
    CK_ENSURE_IF_NOT(EcsWorldSubsystemIsValid,
        TEXT("LiveTune: no EcsWorld subsystem on World [{}] — stamp cleanup cannot connect"), GetWorld())
    {}
    if (NOT EcsWorldSubsystemIsValid)
    { return; }

    auto* Registry = ck::registry_table::Resolve(EcsWorldSubsystem->Get_Registry().Get_RegistryHandle());
    const auto RegistryResolves = Registry != nullptr;
    CK_ENSURE_IF_NOT(RegistryResolves,
        TEXT("LiveTune: the EcsWorld Registry does not resolve — stamp cleanup cannot connect"))
    {}
    if (NOT RegistryResolves)
    { return; }

    _StampDestroyConnection = Registry->on_destroy<ck::FFragment_LiveTune_Stamp>()
        .connect<&UCk_LiveTune_Subsystem_UE::OnStampDestroyed>(*this);
#endif
}

auto
    UCk_LiveTune_Subsystem_UE::
    Deinitialize()
    -> void
{
#if WITH_EDITOR
    FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(_OnObjectPropertyChangedHandle);
    UCk_DeferredAssetInit_UE::Get_OnAssetsReinitialized().Remove(_OnAssetsReinitializedHandle);
    _StampDestroyConnection.release();
    _LinkedEntities.Empty();
    _LastDispatchedValues.Empty();
    _PendingRebuilds.Empty();
#endif

    Super::Deinitialize();
}

auto
    UCk_LiveTune_Subsystem_UE::
    Tick(
        float InDeltaTime)
    -> void
{
    Super::Tick(InDeltaTime);

#if WITH_EDITOR
    if (_PendingRebuilds.IsEmpty())
    { return; }

    for (auto Index = _PendingRebuilds.Num() - 1; Index >= 0; --Index)
    {
        auto& Pending = _PendingRebuilds[Index];

        // The dying entity still EXISTS while the destroy pipeline runs; records disconnect during its
        // teardown, so a re-Add before it is fully gone can collide with the same-named dying entry on a
        // DisallowDuplicateNames record. The driver therefore keys on actual destruction, never a tick count.
        if (ck::IsValid(Pending._DyingEntity, ck::IsValid_Policy_IncludePendingKill{}))
        {
            Pending._PendingForSeconds += InDeltaTime;

            if (Pending._PendingForSeconds >= ck::PendingApplyTimeoutSeconds)
            {
                CK_TRIGGER_ENSURE(
                    TEXT("LiveTune: a rebuild waited [{}]s for the old entity [{}] to finish destroying — dropping "
                         "the rebuild (captured state is lost; the destroy pipeline appears stuck)"),
                    Pending._PendingForSeconds, Pending._DyingEntity);

                _PendingRebuilds.RemoveAtSwap(Index);
            }

            continue;
        }

        auto Finished = MoveTemp(Pending);
        _PendingRebuilds.RemoveAtSwap(Index);
        DoFinishRebuild(Finished);
    }
#endif
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR

auto
    UCk_LiveTune_Subsystem_UE::
    Request_RegisterLink(
        FCk_Handle& InHandle,
        const UObject* InTuningAsset,
        FName InMemberName)
    -> void
{
    auto& Stamp = InHandle.AddOrGet<ck::FFragment_LiveTune_Stamp>();
    const auto Entry = ck::FFragment_LiveTune_Stamp::FEntry{FObjectKey{InTuningAsset}, InMemberName};
    if (NOT Stamp._Entries.Contains(Entry))
    { Stamp._Entries.Add(Entry); }

    const auto Key = FStampKey{FObjectKey{InTuningAsset}, InMemberName};
    _LinkedEntities.FindOrAdd(Key).AddUnique(InHandle);

    // Seed so the first dispatch attempt with an unchanged value (e.g. the very next AS full-heal) is
    // recognized as a no-op and suppressed.
    if (NOT _LastDispatchedValues.Contains(Key))
    {
        if (auto SeedValue = DoTryReadMemberValue(InTuningAsset, InMemberName);
            SeedValue.IsValid())
        { _LastDispatchedValues.Add(Key, MoveTemp(SeedValue)); }
    }
}

auto
    UCk_LiveTune_Subsystem_UE::
    Test_SimulatePropertyChange(
        const UObject* InTuningAsset,
        FName InMemberName,
        EPropertyChangeType::Type InChangeType)
    -> void
{
    const auto AssetIsValid = ck::IsValid(InTuningAsset);
    CK_ENSURE_IF_NOT(AssetIsValid,
        TEXT("LiveTune Test_SimulatePropertyChange: invalid Tuning Asset"))
    {}
    if (NOT AssetIsValid)
    { return; }

    auto* Property = InTuningAsset->GetClass()->FindPropertyByName(InMemberName);
    const auto PropertyExists = Property != nullptr;
    CK_ENSURE_IF_NOT(PropertyExists,
        TEXT("LiveTune Test_SimulatePropertyChange: [{}] has no member property named [{}]"),
        InTuningAsset, InMemberName)
    {}
    if (NOT PropertyExists)
    { return; }

    auto Event = FPropertyChangedEvent{Property, InChangeType};
    Event.SetActiveMemberProperty(Property);

    FCoreUObjectDelegates::OnObjectPropertyChanged.Broadcast(const_cast<UObject*>(InTuningAsset), Event);
}

auto
    UCk_LiveTune_Subsystem_UE::
    Test_Get_LinkCount(
        const UObject* InTuningAsset,
        FName InMemberName) const
    -> int32
{
    const auto* Entities = _LinkedEntities.Find(FStampKey{FObjectKey{InTuningAsset}, InMemberName});
    return Entities != nullptr ? Entities->Num() : 0;
}

auto
    UCk_LiveTune_Subsystem_UE::
    Test_Get_PendingRebuildCount() const
    -> int32
{
    return _PendingRebuilds.Num();
}

auto
    UCk_LiveTune_Subsystem_UE::
    DoOnObjectPropertyChanged(
        UObject* InObject,
        FPropertyChangedEvent& InEvent)
    -> void
{
    if (ck::Is_NOT_Valid(InObject))
    { return; }

    if (_LinkedEntities.IsEmpty())
    { return; }

    const auto MemberName = InEvent.MemberProperty != nullptr
        ? InEvent.MemberProperty->GetFName()
        : InEvent.GetPropertyName();

    if (MemberName.IsNone())
    {
        // Undo/redo (and other PostEditChange-style full-object notifications) arrive with NO property:
        // FTransaction::Apply -> PostEditUndo -> PostEditChange builds FPropertyChangedEvent(nullptr).
        // Any linked member of this asset may have changed; the value-diff gate reduces that to the
        // members that actually did.
        DoProcessAllLinkedMembers(InObject, InEvent.ChangeType);
        return;
    }

    DoProcessMemberChange(InObject, MemberName, InEvent.ChangeType);
}

auto
    UCk_LiveTune_Subsystem_UE::
    DoOnAssetsReinitialized(
        TConstArrayView<UObject*> InHealedAssets)
    -> void
{
    if (_LinkedEntities.IsEmpty())
    { return; }

    // The AS heal re-initialized these assets in place (stable identity, no per-member signal) — re-diff
    // every linked member of each healed asset; the value-diff gate reduces that to real changes, at
    // final-commit policy (a script save IS a commit).
    for (auto* Asset : InHealedAssets)
    {
        if (ck::Is_NOT_Valid(Asset))
        { continue; }

        DoProcessAllLinkedMembers(Asset, EPropertyChangeType::ValueSet);
    }
}

auto
    UCk_LiveTune_Subsystem_UE::
    DoProcessAllLinkedMembers(
        const UObject* InAsset,
        EPropertyChangeType::Type InChangeType)
    -> void
{
    const auto AssetKey = FObjectKey{InAsset};
    auto AffectedMembers = TArray<FName>{};
    for (const auto& Kvp : _LinkedEntities)
    {
        if (Kvp.Key._Asset == AssetKey)
        { AffectedMembers.Add(Kvp.Key._Member); }
    }

    for (const auto& Member : AffectedMembers)
    { DoProcessMemberChange(InAsset, Member, InChangeType); }
}

auto
    UCk_LiveTune_Subsystem_UE::
    DoProcessMemberChange(
        const UObject* InAsset,
        FName InMemberName,
        EPropertyChangeType::Type InChangeType)
    -> void
{
    const auto Key = FStampKey{FObjectKey{InAsset}, InMemberName};
    const auto* LinkedEntities = _LinkedEntities.Find(Key);
    if (LinkedEntities == nullptr)
    { return; }

    auto FreshValue = DoTryReadMemberValue(InAsset, InMemberName);
    if (NOT FreshValue.IsValid())
    {
        // The asset's class changed under us (AS/C++ reinstancing churn). Properties re-resolve by name
        // on every dispatch, so tolerate: skip, and let a later edit of the healed class land normally.
        ck::ecs::Verbose(TEXT("LiveTune: [{}].[{}] no longer resolves to a struct property — skipping dispatch"),
            InAsset, InMemberName);
        return;
    }

    if (const auto* LastDispatched = _LastDispatchedValues.Find(Key);
        LastDispatched != nullptr && *LastDispatched == FreshValue)
    { return; }

    const auto* Handler = FCk_LiveTuneHandlerRegistry::Find(FreshValue.GetScriptStruct());
    if (Handler == nullptr)
    {
        ck::ecs::Display(TEXT("LiveTune: no handler registered for [{}] — feature not live-tunable"),
            FreshValue.GetScriptStruct()->GetName());
        return;
    }

    const auto IsInteractiveChange = (InChangeType & EPropertyChangeType::Interactive) != 0;
    if (IsInteractiveChange && Handler->Tier != ECk_LiveTune_ApplyTier::ViaReplace)
    { return; }

    // Iterate a COPY — Apply may destroy entities, which re-enters _LinkedEntities via the stamp-destroy sink.
    const auto EntitiesCopy = *LinkedEntities;
    auto AppliedToAny = false;
    for (auto Entity : EntitiesCopy)
    {
        if (ck::Is_NOT_Valid(Entity))
        { continue; }

        const auto IsClientModeReplicated =
            UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(Entity) &&
            UCk_Utils_Net_UE::Get_EntityReplication(Entity) == ECk_Replication::Replicates;
        if (IsClientModeReplicated)
        { continue; }

        if (Handler->Tier == ECk_LiveTune_ApplyTier::ViaRebuild)
        {
            AppliedToAny |= DoBeginRebuild(Entity, *Handler, FreshValue, Key);
            continue;
        }

        Handler->Apply(Entity, FreshValue);
        AppliedToAny = true;
    }

    // Only a dispatch that actually reached an entity advances the cache: an edit that lands while every
    // linked entity is skipped must stay dispatchable for the rebuild-completion catch-up.
    if (AppliedToAny)
    { _LastDispatchedValues.Add(Key, MoveTemp(FreshValue)); }
}

auto
    UCk_LiveTune_Subsystem_UE::
    DoBeginRebuild(
        FCk_Handle& InEntity,
        const FCk_LiveTuneHandlerRegistry::FHandler& InHandler,
        const FInstancedStruct& InFreshParams,
        const FStampKey& InKey)
    -> bool
{
    const auto AlreadyPending = _PendingRebuilds.ContainsByPredicate(
        [&](const FPendingRebuild& InPending) { return InPending._DyingEntity == InEntity; });
    if (AlreadyPending)
    { return false; }

    auto Pending = FPendingRebuild{};
    Pending._Scope = InHandler.RebuildScope;
    Pending._DyingEntity = InEntity;
    Pending._ReAdd = InHandler.ReAdd;
    Pending._Key = InKey;
    Pending._FreshParams = InFreshParams;
    Pending._PinnedFreshParams = TStrongObjectPtr{NewObject<UCk_PendingHydrationPayloads_UE>(this)};
    Pending._PinnedFreshParams->Add(InFreshParams);

    if (InHandler.RebuildScope == ECk_LiveTune_RebuildScope::Entity)
    {
        const auto HasSpawnRecipe = InEntity.Has<ck::FFragment_SpawnRecipe>();
        CK_ENSURE_IF_NOT(HasSpawnRecipe,
            TEXT("LiveTune: Scope::Entity rebuild refused for [{}] — only RuntimeSpawned entities carry a spawn "
                 "recipe; ConstructSpawned children and level-placed entities have nothing to respawn"),
            InEntity)
        {}
        if (NOT HasSpawnRecipe)
        { return false; }

        Pending._Recipe = InEntity.Get<ck::FFragment_SpawnRecipe>().Get_Recipe();
    }

    auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InEntity);
    const auto OwnerIsValid = ck::IsValid(Owner);
    CK_ENSURE_IF_NOT(OwnerIsValid,
        TEXT("LiveTune: a rebuild of [{}] needs a valid lifetime owner to re-create under — none found"), InEntity)
    {}
    if (NOT OwnerIsValid)
    { return false; }
    Pending._Owner = Owner;

    if (InHandler.CaptureOverride)
    {
        if (auto Payload = InHandler.CaptureOverride(InEntity, InFreshParams);
            Payload.IsSet())
        {
            Pending._PinnedLinkedPayloads = TStrongObjectPtr{NewObject<UCk_PendingHydrationPayloads_UE>(this)};
            Pending._PinnedLinkedPayloads->Add(MoveTemp(*Payload));
        }
    }
    else
    {
        for (const auto* Type : FCk_PersistenceHandlerRegistry::Get_SaveHandlerTypes())
        {
            const auto* PersistenceHandler = FCk_PersistenceHandlerRegistry::Find(Type);
            if (PersistenceHandler == nullptr || NOT PersistenceHandler->Produce)
            { continue; }

            auto Payload = PersistenceHandler->Produce(InEntity);
            if (NOT Payload.IsSet())
            { continue; }

            if (Pending._PinnedLinkedPayloads.Get() == nullptr)
            { Pending._PinnedLinkedPayloads = TStrongObjectPtr{NewObject<UCk_PendingHydrationPayloads_UE>(this)}; }

            Pending._PinnedLinkedPayloads->Add(MoveTemp(*Payload));
        }
    }

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InEntity);
    _PendingRebuilds.Add(MoveTemp(Pending));
    return true;
}

auto
    UCk_LiveTune_Subsystem_UE::
    DoFinishRebuild(
        FPendingRebuild& InPending)
    -> void
{
    const auto OwnerIsValid = ck::IsValid(InPending._Owner);
    if (NOT OwnerIsValid)
    {
        ck::ecs::Verbose(TEXT("LiveTune: the owner died while a rebuild of [{}] was pending — dropped"),
            InPending._FreshParams.GetScriptStruct()->GetName());
        return;
    }

    if (InPending._Scope == ECk_LiveTune_RebuildScope::Entity)
    {
        const auto Pending = UCk_Utils_EntityScript_UE::Request_SpawnEntity(
            InPending._Owner, InPending._Recipe->Get_ScriptClass(), InPending._Recipe->Get_SpawnParams(), {});
        auto Respawned = Pending.Get_EntityUnderConstruction();

        if (InPending._PinnedLinkedPayloads.Get() != nullptr)
        {
            for (auto& Payload : InPending._PinnedLinkedPayloads->Get_Entries())
            { DoEnqueueHydration(Respawned, MoveTemp(Payload)); }
        }

        ck::ecs::Display(
            TEXT("LiveTune: respawned entity [{}] from its recipe after a [{}] edit — links, signal bindings and "
                 "cached handles to the old entity are severed; the script's own construction re-establishes them"),
            Respawned, InPending._FreshParams.GetScriptStruct()->GetName());
        return;
    }

    auto NewHandle = InPending._ReAdd(InPending._Owner, InPending._FreshParams);
    const auto NewHandleIsValid = ck::IsValid(NewHandle);
    CK_ENSURE_IF_NOT(NewHandleIsValid,
        TEXT("LiveTune: ReAdd for [{}] returned an invalid handle — rebuild aborted, captured state dropped"),
        InPending._FreshParams.GetScriptStruct()->GetName())
    {}
    if (NOT NewHandleIsValid)
    { return; }

    if (InPending._PinnedLinkedPayloads.Get() != nullptr)
    {
        for (auto& Payload : InPending._PinnedLinkedPayloads->Get_Entries())
        { DoEnqueueHydration(NewHandle, MoveTemp(Payload)); }
    }

    const auto ReAppliedPayloadCount = InPending._PinnedLinkedPayloads.Get() != nullptr
        ? InPending._PinnedLinkedPayloads->Get_Entries().Num()
        : 0;
    ck::ecs::Display(
        TEXT("LiveTune: rebuilt [{}] under Entity [{}], re-applying [{}] captured payload(s) — the old feature "
             "entity [{}] is gone; signal bindings and cached handles to it are severed and must be "
             "re-established by their owners"),
        InPending._FreshParams.GetScriptStruct()->GetName(), InPending._Owner, ReAppliedPayloadCount,
        InPending._DyingEntity);

    if (auto* Asset = InPending._Key._Asset.ResolveObjectPtr();
        ck::IsValid(Asset))
    {
        Request_RegisterLink(NewHandle, Asset, InPending._Key._Member);

        // The rebuild applied _FreshParams, but an edit may have landed while the old entity was mid-destroy
        // (deliberately left uncached). Record what was ACTUALLY applied, then re-read the asset: the
        // catch-up dispatches a newer value and no-ops via the diff gate when nothing changed.
        _LastDispatchedValues.Add(InPending._Key, InPending._FreshParams);
        DoProcessMemberChange(Asset, InPending._Key._Member, EPropertyChangeType::ValueSet);
    }
}

auto
    UCk_LiveTune_Subsystem_UE::
    DoEnqueueHydration(
        FCk_Handle& InTarget,
        FInstancedStruct InPayload)
    -> void
{
    InTarget.AddOrGet<ck::FFragment_PendingHydration>().Enqueue(GetWorld(), MoveTemp(InPayload));
    if (NOT InTarget.Has<ck::FTag_Hydration_PendingApply>())
    { InTarget.Add<ck::FTag_Hydration_PendingApply>(); }
}

auto
    UCk_LiveTune_Subsystem_UE::
    OnStampDestroyed(
        ck::registry_table::EnttRegistryType& InRegistry,
        FCk_Entity::IdType InEntity)
    -> void
{
    // EnTT invokes on_destroy BEFORE the fragment is removed, so the stamp is still readable here.
    const auto& Stamp = InRegistry.get<ck::FFragment_LiveTune_Stamp>(InEntity);

    for (const auto& Entry : Stamp.Get_Entries())
    {
        const auto Key = FStampKey{Entry._Asset, Entry._Member};
        auto* Entities = _LinkedEntities.Find(Key);
        if (Entities == nullptr)
        { continue; }

        Entities->RemoveAll([&](const FCk_Handle& InHandle)
        {
            return InHandle.Get_Entity().Get_ID() == InEntity;
        });

        if (Entities->IsEmpty())
        {
            _LinkedEntities.Remove(Key);
            _LastDispatchedValues.Remove(Key);
        }
    }
}

auto
    UCk_LiveTune_Subsystem_UE::
    DoTryReadMemberValue(
        const UObject* InAsset,
        FName InMemberName)
    -> FInstancedStruct
{
    const auto* Property = InAsset->GetClass()->FindPropertyByName(InMemberName);
    const auto* StructProperty = CastField<FStructProperty>(Property);
    if (StructProperty == nullptr)
    { return {}; }

    auto Value = FInstancedStruct{};
    Value.InitializeAs(StructProperty->Struct, StructProperty->ContainerPtrToValuePtr<uint8>(InAsset));
    return Value;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
