#include "CkEditorSelectionOwner_Utils.h"

#if WITH_EDITOR
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Subsystem/CkEcsEditor_Subsystem.h"

#include <Engine/World.h>
#include <GameFramework/Actor.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
namespace ck_editor_selection_owner_utils
{
    // Owner actor -> editor-world proxy actors hosting its preview visuals. Weak on both sides so
    // entries self-prune on access; bounded by the number of placed preview owners this session.
    // TWeakObjectPtr keys compare by object index + serial, so entries stay addressable after the
    // owner actor is destroyed.
    auto
        Get_ProxyRegistry()
        -> TMap<TWeakObjectPtr<const AActor>, TArray<TWeakObjectPtr<AActor>>>&
    {
        static auto Registry = TMap<TWeakObjectPtr<const AActor>, TArray<TWeakObjectPtr<AActor>>>{};
        return Registry;
    }

    auto
        Get_OnRefreshRequested()
        -> FCk_EditorSelectionOwner_OnRefreshRequested&
    {
        static auto Delegate = FCk_EditorSelectionOwner_OnRefreshRequested{};
        return Delegate;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_EditorSelectionOwner::
        FFragment_EditorSelectionOwner(
            const TWeakObjectPtr<AActor>& InOwnerActor)
        : _OwnerActor(InOwnerActor)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EditorSelectionOwner_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_EditorSelectionOwner>();
}

auto
    UCk_Utils_EditorSelectionOwner_UE::
    TryGet_SelectionOwner(
        const FCk_Handle& InHandle)
    -> AActor*
{
    if (NOT Has(InHandle))
    { return nullptr; }

    return InHandle.Get<ck::FFragment_EditorSelectionOwner>().Get_OwnerActor().Get();
}

auto
    UCk_Utils_EditorSelectionOwner_UE::
    TryGet_SelectionOwnerWeak(
        const FCk_Handle& InHandle)
    -> TWeakObjectPtr<AActor>
{
    if (NOT Has(InHandle))
    { return {}; }

    return InHandle.Get<ck::FFragment_EditorSelectionOwner>().Get_OwnerActor();
}

auto
    UCk_Utils_EditorSelectionOwner_UE::
    Request_SetupEntityWithEditorSelectionOwner(
        FCk_Handle& InNewEntity,
        const TWeakObjectPtr<AActor>& InOwnerActor)
    -> void
{
    if (InNewEntity.Has<ck::FFragment_EditorSelectionOwner>())
    {
        const auto& CurrentOwner = InNewEntity.Get<ck::FFragment_EditorSelectionOwner>().Get_OwnerActor();
        CK_ENSURE
        (
            CurrentOwner == InOwnerActor,
            TEXT("Trying to Setup Entity [{}] with EditorSelectionOwner [{}] but it is already setup with [{}]"),
            InNewEntity,
            InOwnerActor.Get(),
            CurrentOwner.Get()
        );

        return;
    }

    InNewEntity.Add<ck::FFragment_EditorSelectionOwner>(InOwnerActor);
}

auto
    UCk_Utils_EditorSelectionOwner_UE::
    RegisterProxyActor(
        AActor* InOwnerActor,
        AActor* InProxyActor)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwnerActor) && ck::IsValid(InProxyActor),
        TEXT("Cannot register EditorSelectionOwner proxy — Owner [{}] or Proxy [{}] is INVALID"),
        InOwnerActor, InProxyActor)
    { return; }

    auto& Proxies = ck_editor_selection_owner_utils::Get_ProxyRegistry().FindOrAdd(InOwnerActor);

    Proxies.RemoveAll([](const TWeakObjectPtr<AActor>& InProxy)
    {
        return NOT InProxy.IsValid();
    });

    Proxies.AddUnique(InProxyActor);
}

auto
    UCk_Utils_EditorSelectionOwner_UE::
    PushOwnerSelectionToProxies(
        const AActor* InOwnerActor)
    -> void
{
    if (ck::Is_NOT_Valid(InOwnerActor, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    ck_editor_selection_owner_utils::Get_OnRefreshRequested().Broadcast(InOwnerActor);

    auto& Registry = ck_editor_selection_owner_utils::Get_ProxyRegistry();

    const auto Key = TWeakObjectPtr<const AActor>{InOwnerActor};
    auto* Proxies = Registry.Find(Key);

    if (Proxies == nullptr)
    { return; }

    Proxies->RemoveAll([](const TWeakObjectPtr<AActor>& InProxy)
    {
        return NOT InProxy.IsValid();
    });

    if (Proxies->IsEmpty())
    {
        Registry.Remove(Key);
        return;
    }

    for (const auto& Proxy : *Proxies)
    {
        Proxy->PushSelectionToProxies();
    }
}


auto
    UCk_Utils_EditorSelectionOwner_UE::
    Get_OnRefreshRequested()
    -> FCk_EditorSelectionOwner_OnRefreshRequested&
{
    return ck_editor_selection_owner_utils::Get_OnRefreshRequested();
}

auto
    UCk_Utils_EditorSelectionOwner_UE::
    TryGet_SelectionProxyHostActor(
        const UWorld* InWorld,
        const FCk_Handle& InHandle)
    -> AActor*
{
    if (ck::Is_NOT_Valid(InWorld) || InWorld->WorldType != EWorldType::Editor)
    { return nullptr; }

    auto* SelectionOwner = TryGet_SelectionOwner(InHandle);
    if (ck::Is_NOT_Valid(SelectionOwner))
    { return nullptr; }

    auto* EditorSubsystem = InWorld->GetSubsystem<UCk_EditorEcsWorld_Subsystem_UE>();
    if (ck::Is_NOT_Valid(EditorSubsystem))
    { return nullptr; }

    return EditorSubsystem->Get_SelectionProxyHostActor(SelectionOwner);
}
#endif

// --------------------------------------------------------------------------------------------------------------------
