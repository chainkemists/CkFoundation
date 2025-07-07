#include "CkEntityScript_Processor.h"

#include "CkEntityScript_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include "Engine/UserDefinedStruct.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_EntityScript_SpawnEntity_HandleRequests::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_RequestSpawnEntity& InRequestFragment)
        -> void
    {
        if (const auto& LifetimeOwner = InRequestFragment.Get_Owner();
            LifetimeOwner.Has_Any<FTag_EntityJustCreated, FTag_EntityScript_ContinueConstruction, FTag_EntityScript_FinishConstruction>())
        { return; }

        DoHandleRequest(InHandle, InRequestFragment);
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
        InHandle.Remove<MarkedDirtyBy>();
    }

    auto
        FProcessor_EntityScript_SpawnEntity_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FCk_Request_EntityScript_SpawnEntity& InRequest)
        -> void
    {
        QUICK_SCOPE_CYCLE_COUNTER(FCk_Request_EntityScript_SpawnEntity)
        const auto EntityScriptClass = InRequest.Get_EntityScriptClass();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScriptClass),
            TEXT("Invalid EntityScript supplied, cannot Spawn Entity"))
        { return; }

        const auto& LifetimeOwner = InRequest.Get_Owner();

        const auto& Outer = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(LifetimeOwner);

        const auto& NewEntityScript = [&]()
        {
            QUICK_SCOPE_CYCLE_COUNTER(FCk_Request_EntityScript_SpawnEntity_CreateObject)

            switch (const auto& EntityScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_EntityScript_UE>(EntityScriptClass);
                    EntityScriptCDO->Get_InstancingPolicy())
            {
                case ECk_EntityScript_InstancingPolicy::NotInstanced:
                {
                    return EntityScriptCDO;
                }
                case ECk_EntityScript_InstancingPolicy::InstancedPerEntity:
                {
                    return UCk_Utils_Object_UE::Request_CreateNewObject<UCk_EntityScript_UE>(Outer,
                        EntityScriptClass, nullptr, nullptr);
                }
                default:
                {
                    CK_INVALID_ENUM(EntityScriptCDO->Get_InstancingPolicy());
                    return EntityScriptCDO;
                }
            }
        }();

        CK_ENSURE_IF_NOT(ck::IsValid(NewEntityScript), TEXT("Failed to Spawn New Entity using EntityScript [{}]"), EntityScriptClass)
        { return; }

        if (const auto& SpawnParams = InRequest.Get_SpawnParams();
            ck::IsValid(SpawnParams))
        {
            if (const auto& SpawnParamsStruct = Cast<UScriptStruct>(SpawnParams.GetScriptStruct());
                ck::IsValid(SpawnParamsStruct))
            {
                QUICK_SCOPE_CYCLE_COUNTER(FCk_Request_EntityScript_SpawnEntity_CopySpawnParams)
                const auto& SpawnParamsData = SpawnParams.GetMemory();

                for (TFieldIterator<FProperty> PropIt(SpawnParamsStruct, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
                {
                    const auto* SpawnParamsProp = *PropIt;

                    const auto* EntityScriptProp = UCk_Utils_Reflection_UE::Get_PropertyBySanitizedName(
                        NewEntityScript, UCk_Utils_Reflection_UE::Get_SanitizedUserDefinedPropertyName(SpawnParamsProp));

                    if (ck::IsValid(EntityScriptProp, ck::IsValid_Policy_NullptrOnly{}))
                    {
                        auto* EntityScriptPropAddr = EntityScriptProp->ContainerPtrToValuePtr<uint8>(NewEntityScript);
                        const auto* SpawnParamsPropAddr = SpawnParamsProp->ContainerPtrToValuePtr<uint8>(SpawnParamsData);

                        EntityScriptProp->CopyCompleteValue(EntityScriptPropAddr, SpawnParamsPropAddr);
                    }
                }
            }
        }

        auto NewEntity = InRequest.Get_NewEntity();

        NewEntityScript->_AssociatedEntity = NewEntity;
        NewEntity.Add<ck::FFragment_EntityScript_Current>(NewEntityScript);

        if (NewEntityScript->Get_Replication() == ECk_Replication::Replicates)
        {
            UCk_Utils_EntityReplicationDriver_UE::Add(NewEntity);

            if (auto ReplicatedOwner = InRequest.Get_Owner(); UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(ReplicatedOwner))
            {
                NewEntity.Add<ck::FRequest_EntityScript_Replicate>(ReplicatedOwner, InRequest.Get_SpawnParams(),
                    NewEntityScript);
            }
        }

        switch (NewEntityScript->Construct(NewEntity, InRequest.Get_SpawnParams()))
        {
            case ECk_EntityScript_ConstructionFlow::Finished:
            {
                const auto& ContinueConstructionFuncName = GET_FUNCTION_NAME_CHECKED(UCk_EntityScript_UE, DoContinueConstruction);
                CK_ENSURE_IF_NOT(NOT NewEntityScript->GetClass()->IsFunctionImplementedInScript(ContinueConstructionFuncName),
                    TEXT("EntityScript [{}] Construction is FINISHED, but the script [{}] implements the [ContinueConstruction] event!\n"
                         "This event will be ignored as it is only invoked for ONGOING construction of EntityScript"),
                NewEntity, NewEntityScript) {}

                NewEntity.Add<FTag_EntityScript_FinishConstruction>();
                break;
            }
            case ECk_EntityScript_ConstructionFlow::Continue:
            {
                const auto& IsBlueprintClass = ck::IsValid(NewEntityScript->GetClass()->ClassGeneratedBy);
                const auto& ContinueConstructionFuncName = GET_FUNCTION_NAME_CHECKED(UCk_EntityScript_UE, DoContinueConstruction);
                CK_ENSURE_IF_NOT(NOT IsBlueprintClass || NewEntityScript->GetClass()->IsFunctionImplementedInScript(ContinueConstructionFuncName),
                    TEXT("EntityScript [{}] Construction is ONGOING, but the script [{}] DOES NOT implement the [ContinueConstruction] event!\n"
                         "Implement this event and ensure that [FinishConstruction] is called to ensure that the script correctly BeginPlay"),
                NewEntity, NewEntityScript) {}

                NewEntity.Add<FTag_EntityScript_ContinueConstruction>();
                break;
            }
        }


        if (InRequest.Get_PostConstruction_Func())
        {
            InRequest.Get_PostConstruction_Func()(NewEntity);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_ContinueConstruction::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        const auto& EntityScript = InCurrent.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScript),
            TEXT("EntityScript is INVALID for [{}] when attempting to invoke ContinueConstruction on it"), InHandle)
        { return; }

        EntityScript->ContinueConstruction(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_Replicate::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FRequest_EntityScript_Replicate& InRequest)
        -> void
    {
        ON_SCOPE_EXIT
        { InHandle.Remove<MarkedDirtyBy>(); };

        const auto& EntityScript = InRequest.Get_Script();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScript),
            TEXT("EntityScript is INVALID for [{}] when attempting to invoke ContinueConstruction on it"), InHandle)
        { return; }

        auto ReplicatedOwner = InRequest.Get_Owner();
        auto SpawnParams = InRequest.Get_SpawnParams();

        const auto& ReplicationDriver = ReplicatedOwner.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();

        CK_ENSURE_IF_NOT(ck::IsValid(ReplicationDriver),
            TEXT("Entity [{}] is missing a ReplicationDriver Fragment!"), ReplicatedOwner)
        { return; }

        if (ReplicatedOwner.Has<FTag_EntityJustCreated>())
        {
            ReplicationDriver->Set_ExpectedNumberOfDependentReplicationDrivers(
                ReplicationDriver->Get_ExpectedNumberOfDependentReplicationDrivers() + 1);
        }

        UCk_Utils_EntityReplicationDriver_UE::Request_Replicate(InHandle, ReplicatedOwner,
            InRequest.Get_Script()->GetClass(), SpawnParams);

        InHandle.Add<FTag_EntityReplicationDriver_FireOnDependentReplicationComplete>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_FinishConstruction::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();
        InHandle.Add<FTag_EntityScript_BeginPlay>();

        UUtils_Signal_OnConstructed::Broadcast(InHandle, ck::MakePayload(InHandle));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_BeginPlay::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        const auto& EntityScript = InCurrent.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScript), TEXT("EntityScript is INVALID for [{}] when attempting to invoke BeginPlay on it"), InHandle)
        { return; }

        EntityScript->BeginPlay();

        InHandle.Add<ck::FTag_EntityScript_HasBegunPlay>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_EntityScript_EndPlay::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            FFragment_EntityScript_Current& InCurrent)
        -> void
    {
        const auto& EntityScript = InCurrent.Get_Script().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(EntityScript), TEXT("EntityScript is INVALID for [{}] when attempting to invoke EndPlay on it"), InHandle)
        { return; }

        EntityScript->EndPlay();
        InHandle.Add<ck::FTag_EntityScript_HasEndedPlay>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

