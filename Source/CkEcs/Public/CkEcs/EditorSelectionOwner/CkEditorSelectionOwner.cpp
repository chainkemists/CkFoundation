#include "CkEditorSelectionOwner.h"

#if WITH_EDITOR
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Subsystem/CkEcsEditor_Subsystem.h"

#include <Engine/World.h>
#include <GameFramework/Actor.h>
#include <UObject/ObjectKey.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_editor_selection_owner
{
    // Owner actor -> editor-world proxy actors hosting its preview visuals. Weak on both sides so
    // entries self-prune on access; bounded by the number of placed preview owners this session.
    auto
        Get_ProxyRegistry()
        -> TMap<FObjectKey, TArray<TWeakObjectPtr<AActor>>>&
    {
        static auto Registry = TMap<FObjectKey, TArray<TWeakObjectPtr<AActor>>>{};
        return Registry;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_EditorSelectionOwner::
        FFragment_EditorSelectionOwner(
            AActor* InOwnerActor)
        : _OwnerActor(InOwnerActor)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::editor_selection_owner
{
    auto
        Set(
            FCk_Handle& InHandle,
            AActor* InOwnerActor)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
            TEXT("Cannot stamp EditorSelectionOwner [{}] on an INVALID Entity"), InOwnerActor)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(InOwnerActor),
            TEXT("Cannot stamp an INVALID EditorSelectionOwner Actor on Entity [{}]"), InHandle)
        { return; }

        InHandle.AddOrGet<ck::FFragment_EditorSelectionOwner>() = ck::FFragment_EditorSelectionOwner{InOwnerActor};
    }

    auto
        TryGet(
            const FCk_Handle& InHandle)
        -> AActor*
    {
        if (ck::Is_NOT_Valid(InHandle))
        { return nullptr; }

        auto EntityWithOwner = UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(InHandle,
            [](const FCk_Handle& InEntityInChain)
            {
                return InEntityInChain.Has<ck::FFragment_EditorSelectionOwner>();
            });

        if (ck::Is_NOT_Valid(EntityWithOwner))
        { return nullptr; }

        return EntityWithOwner.Get<ck::FFragment_EditorSelectionOwner>().Get_OwnerActor().Get();
    }

    auto
        RegisterProxyActor(
            AActor* InOwnerActor,
            AActor* InProxyActor)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InOwnerActor) && ck::IsValid(InProxyActor),
            TEXT("Cannot register EditorSelectionOwner proxy — Owner [{}] or Proxy [{}] is INVALID"),
            InOwnerActor, InProxyActor)
        { return; }

        auto& Proxies = ck_editor_selection_owner::Get_ProxyRegistry().FindOrAdd(FObjectKey{InOwnerActor});

        Proxies.RemoveAll([](const TWeakObjectPtr<AActor>& InProxy)
        {
            return NOT InProxy.IsValid();
        });

        Proxies.AddUnique(InProxyActor);
    }

    auto
        PushOwnerSelectionToProxies(
            const AActor* InOwnerActor)
        -> void
    {
        if (ck::Is_NOT_Valid(InOwnerActor, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        auto& Registry = ck_editor_selection_owner::Get_ProxyRegistry();

        const auto Key = FObjectKey{InOwnerActor};
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
        TryGet_SelectionProxyHostActor(
            const UWorld* InWorld,
            const FCk_Handle& InHandle)
        -> AActor*
    {
        if (ck::Is_NOT_Valid(InWorld) || InWorld->WorldType != EWorldType::Editor)
        { return nullptr; }

        auto* SelectionOwner = TryGet(InHandle);
        if (ck::Is_NOT_Valid(SelectionOwner))
        { return nullptr; }

        auto* EditorSubsystem = InWorld->GetSubsystem<UCk_EditorEcsWorld_Subsystem_UE>();
        if (ck::Is_NOT_Valid(EditorSubsystem))
        { return nullptr; }

        return EditorSubsystem->Get_SelectionProxyHostActor(SelectionOwner);
    }
}
#endif

// --------------------------------------------------------------------------------------------------------------------
