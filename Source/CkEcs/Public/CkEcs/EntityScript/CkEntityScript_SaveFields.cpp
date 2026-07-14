#include "CkEntityScript_SaveFields.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h" // RegisterLazyTyped<T> body

#include <Serialization/MemoryReader.h>
#include <Serialization/MemoryWriter.h>
#include <Serialization/ObjectAndNameAsStringProxyArchive.h>
#include <UObject/UnrealType.h>

// --------------------------------------------------------------------------------------------------------------------
//
// Framework Save-transport handler for EntityScript SaveGame-tagged UPROPERTYs (spec §4B.3).
//
// Registered once by CkEcs (this file's static registrar) rather than per-script — the reflect walk over the script
// class's CPF_SaveGame FProperties is generic. Save-only handler (HydrationApply + Produce, no Apply): the payload
// type is never placed in a replicated container, so it stays off the wire and the load-path hydration dispatcher
// (FProcessor_Hydration_Dispatch) is its sole caller.
//
// --------------------------------------------------------------------------------------------------------------------

namespace ck_entity_script_save_fields
{
    // True iff the script class declares at least one UPROPERTY(SaveGame) — the "no-op for scripts without SaveGame
    // fields" gate. Reflect-walks every FProperty (own + inherited) checking CPF_SaveGame.
    auto
        Has_AnySaveGameProperty(
            const UClass* InClass)
        -> bool
    {
        if (InClass == nullptr)
        { return false; }

        for (TFieldIterator<FProperty> PropIt{InClass}; PropIt; ++PropIt)
        {
            if (PropIt->HasAnyPropertyFlags(CPF_SaveGame))
            { return true; }
        }
        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------

    struct FRegistrar
    {
        FRegistrar()
        {
            FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<FCk_SaveData_EntityScriptFields>(
                {
                    // Save-only: HydrationApply is the load-path applier — the only path this type ever takes. It is
                    // never placed in a replicated container, so it has no net Apply.
                    .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                    {
                        if (NOT Entity.Has<ck::FFragment_EntityScript_Current>())
                        { return ECk_RepFragment_ApplyResult::NotReady; }

                        auto* Script = Entity.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get();
                        if (ck::Is_NOT_Valid(Script))
                        { return ECk_RepFragment_ApplyResult::NotReady; }

                        if (Script->Get_InstancingPolicy() == ECk_EntityScript_InstancingPolicy::NotInstanced)
                        {
                            ck::ecs::Warning(TEXT("EntityScript SaveGame-field hydration skipped for [{}] — script is "
                                "NotInstanced (shared CDO); writing SaveGame fields would corrupt every entity sharing it"), Entity);
                            return ECk_RepFragment_ApplyResult::Applied;
                        }

                        const auto& Payload = New.Get<FCk_SaveData_EntityScriptFields>();

                        const auto CurrentClassPath = Script->GetClass()->GetPathName();
                        if (Payload.Get_ScriptClassPath() != CurrentClassPath)
                        {
                            // Tagged-property replay is name-based and layout-tolerant, so a drifted class still applies
                            // the fields it recognizes — warn (script re-Constructed as a different class than saved)
                            // but proceed.
                            ck::ecs::Warning(TEXT("EntityScript SaveGame-field hydration for [{}] — saved script class "
                                "[{}] != current [{}]; applying recognized fields by name"), Entity, Payload.Get_ScriptClassPath(), CurrentClassPath);
                        }

                        auto Reader = FMemoryReader{Payload.Get_FieldBytes(), /*bIsPersistent=*/true};
                        constexpr auto LoadIfFindFails = true;
                        auto Proxy = FObjectAndNameAsStringProxyArchive{Reader, LoadIfFindFails};
                        Proxy.ArIsSaveGame = true;      // restore ONLY the CPF_SaveGame fields (symmetric with Produce)
                        Proxy.SetIsPersistent(true);

                        Script->SerializeScriptProperties(Proxy);

                        ck::ecs::VeryVerbose(TEXT("EntityScript SaveGame-field hydration applied for [{}] ([{}] bytes)"),
                            Entity, Payload.Get_FieldBytes().Num());

                        return ECk_RepFragment_ApplyResult::Applied;
                    },
                    // Capture the script instance's CPF_SaveGame fields as a Save-only payload. UNSET when there is no
                    // script, it is a shared CDO (NotInstanced), or the class declares no SaveGame field — those cases
                    // have nothing to persist and must not emit a (misleading) empty payload.
                    .Produce = [](FCk_Handle& Entity) -> TOptional<FInstancedStruct>
                    {
                        if (NOT Entity.Has<ck::FFragment_EntityScript_Current>())
                        { return {}; }

                        auto* Script = Entity.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get();
                        if (ck::Is_NOT_Valid(Script))
                        { return {}; }

                        if (Script->Get_InstancingPolicy() == ECk_EntityScript_InstancingPolicy::NotInstanced)
                        { return {}; }

                        if (NOT ck_entity_script_save_fields::Has_AnySaveGameProperty(Script->GetClass()))
                        { return {}; }

                        auto Blob = TArray<uint8>{};
                        auto Writer = FMemoryWriter{Blob, /*bIsPersistent=*/true};
                        constexpr auto LoadIfFindFails = true;
                        auto Proxy = FObjectAndNameAsStringProxyArchive{Writer, LoadIfFindFails};
                        Proxy.ArIsSaveGame = true;      // capture ONLY the CPF_SaveGame fields — the whole point of 4B.3
                        Proxy.SetIsPersistent(true);

                        Script->SerializeScriptProperties(Proxy);

                        auto Payload = FCk_SaveData_EntityScriptFields{};
                        Payload.Set_ScriptClassPath(Script->GetClass()->GetPathName());
                        Payload.Set_FieldBytes(MoveTemp(Blob));
                        return FInstancedStruct::Make(Payload);
                    },
                });
        }
    };

    // Filename-derived namespace + descriptive instance name → unity-build-safe (no anonymous-namespace collision).
    const FRegistrar GCkEntityScriptSaveFieldsRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
