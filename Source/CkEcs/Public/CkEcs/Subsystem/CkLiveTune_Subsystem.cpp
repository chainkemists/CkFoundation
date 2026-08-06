#include "CkLiveTune_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/LiveTune/CkLiveTune_Fragment.h"
#include "CkEcs/LiveTune/CkLiveTune_HandlerRegistry.h"
#include "CkEcs/Net/CkNet_Utils.h"
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
    _StampDestroyConnection.release();
    _LinkedEntities.Empty();
    _LastDispatchedValues.Empty();
#endif

    Super::Deinitialize();
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
        const auto AssetKey = FObjectKey{InObject};
        auto AffectedMembers = TArray<FName>{};
        for (const auto& Kvp : _LinkedEntities)
        {
            if (Kvp.Key._Asset == AssetKey)
            { AffectedMembers.Add(Kvp.Key._Member); }
        }

        for (const auto& Member : AffectedMembers)
        { DoProcessMemberChange(InObject, Member, InEvent.ChangeType); }

        return;
    }

    DoProcessMemberChange(InObject, MemberName, InEvent.ChangeType);
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

    if (Handler->Tier == ECk_LiveTune_ApplyTier::ViaRebuild)
    {
        CK_TRIGGER_ENSURE(
            TEXT("LiveTune: [{}] is registered ViaRebuild, but no rebuild driver exists yet — the edit was NOT applied"),
            FreshValue.GetScriptStruct()->GetName());
        return;
    }

    // Iterate a COPY — Apply may destroy entities, which re-enters _LinkedEntities via the stamp-destroy sink.
    const auto EntitiesCopy = *LinkedEntities;
    for (auto Entity : EntitiesCopy)
    {
        if (ck::Is_NOT_Valid(Entity))
        { continue; }

        const auto IsClientModeReplicated =
            UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(Entity) &&
            UCk_Utils_Net_UE::Get_EntityReplication(Entity) == ECk_Replication::Replicates;
        if (IsClientModeReplicated)
        { continue; }

        Handler->Apply(Entity, FreshValue);
    }

    _LastDispatchedValues.Add(Key, MoveTemp(FreshValue));
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
