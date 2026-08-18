#include "CkEntityScript_SaveFields.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies

#include <Serialization/MemoryReader.h>
#include <Serialization/MemoryWriter.h>
#include <Serialization/ObjectAndNameAsStringProxyArchive.h>
#include <UObject/UnrealType.h>

// --------------------------------------------------------------------------------------------------------------------
// Framework Save-transport handler for EntityScript SaveGame-tagged UPROPERTYs. Registered once by CkEcs rather than
// per-script — the reflect walk over the script class's CPF_SaveGame FProperties is generic. Save-only: the payload
// never rides the wire, so the load-path hydration dispatcher is its sole caller.
// --------------------------------------------------------------------------------------------------------------------

namespace ck_entity_script_save_fields
{
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
            FCk_PersistenceHandlerRegistry::Register_SaveOnly<FCk_SaveData_EntityScriptFields>({
                    .Posture = ECk_Snapshot_Posture::Durable,
                    // UNSET — not an empty payload — when there is no script, it is a shared CDO, or the class
                    // declares no SaveGame field: those cases have nothing to persist.
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
                    .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                    {
                        if (NOT Entity.Has<ck::FFragment_EntityScript_Current>())
                        { return ECk_Persistence_ApplyResult::NotReady; }

                        auto* Script = Entity.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get();
                        if (ck::Is_NOT_Valid(Script))
                        { return ECk_Persistence_ApplyResult::NotReady; }

                        if (Script->Get_InstancingPolicy() == ECk_EntityScript_InstancingPolicy::NotInstanced)
                        {
                            ck::ecs::Warning(TEXT("EntityScript SaveGame-field hydration skipped for [{}] — script is "
                                "NotInstanced (shared CDO); writing SaveGame fields would corrupt every entity sharing it"), Entity);
                            return ECk_Persistence_ApplyResult::Applied;
                        }

                        const auto& Payload = New.Get<FCk_SaveData_EntityScriptFields>();

                        const auto CurrentClassPath = Script->GetClass()->GetPathName();
                        if (Payload.Get_ScriptClassPath() != CurrentClassPath)
                        {
                            // Tagged-property replay is name-based and layout-tolerant, so a drifted class still
                            // applies the fields it recognizes — warn but proceed.
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

                        return ECk_Persistence_ApplyResult::Applied;
                    }});
        }
    };

    // Filename-derived namespace + descriptive instance name → unity-build-safe (no anonymous-namespace collision).
    const FRegistrar GCkEntityScriptSaveFieldsRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------
