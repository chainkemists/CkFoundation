#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/World/CkEcsWorld.h"

#include "CkEcsEditor_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { CK_DEFINE_ECS_TAG(FTag_EditorOnlyEntity); }

#define CK_IF_EDITOR_HANDLE(_Handle_)\
if (_Handle_.Has<FTag_EditorOnlyEntity>())

#define CK_IF_EDITOR_HANDLE_RETURN_VOID(_Handle_)\
if (_Handle_.Has<FTag_EditorOnlyEntity>() { return ; }

#define CK_IF_EDITOR_HANDLE_RETURN(_Handle_)\
if (_Handle_.Has<FTag_EditorOnlyEntity>() { return {}; }

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_EcsWorld")
class CKECS_API UCk_EcsEditor_Subsystem : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsEditor_Subsystem);

public:
    friend class UCk_EcsWorld_Stats_Subsystem_UE;

public:
    using EcsWorldType = ck::FEcsWorld;

    auto
    Initialize(
        FSubsystemCollectionBase& Collection) -> void override;

public:
    UFUNCTION(BlueprintCallable)
    FCk_Handle
    Request_AddOrGet_EntityForObject(
        UObject* InObject);

public:
    static auto
    Get_EditorWorld() -> ck::FEcsWorld;

private:
    static inline FCk_Handle _TransientEntity;
    TMap<TWeakObjectPtr<UObject>, FCk_Handle> _ObjectToHandle;
};

// --------------------------------------------------------------------------------------------------------------------
