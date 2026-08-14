#include "CkJoltStaticWorld_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkJolt/StaticWorld/CkJoltStaticWorld_Subsystem.h"

#include <Components/PrimitiveComponent.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_static_world_utils
{
    static auto Get_Subsystem(const UObject* InWorldContextObject) -> UCk_JoltStaticWorld_Subsystem_UE*
    {
        if (ck::Is_NOT_Valid(InWorldContextObject))
        { return nullptr; }

        const auto* World = InWorldContextObject->GetWorld();
        if (ck::Is_NOT_Valid(World))
        { return nullptr; }

        return World->GetSubsystem<UCk_JoltStaticWorld_Subsystem_UE>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltStaticWorld_UE::
    Request_BakeActor(
        AActor* InActor)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Request_BakeActor called with an INVALID Actor"))
    { return 0; }

    auto* Subsystem = ck_jolt_static_world_utils::Get_Subsystem(InActor);

    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem),
        TEXT("No JoltStaticWorld subsystem for Actor [{}] — game worlds only"), InActor->GetName())
    { return 0; }

    return Subsystem->Request_BakeActor(*InActor);
}

auto
    UCk_Utils_JoltStaticWorld_UE::
    Request_RemoveActor(
        AActor* InActor)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Request_RemoveActor called with an INVALID Actor"))
    { return; }

    auto* Subsystem = ck_jolt_static_world_utils::Get_Subsystem(InActor);
    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->Request_RemoveActor(*InActor);
}

auto
    UCk_Utils_JoltStaticWorld_UE::
    Request_BakeComponent(
        UPrimitiveComponent* InComponent)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InComponent), TEXT("Request_BakeComponent called with an INVALID Component"))
    { return 0; }

    auto* Subsystem = ck_jolt_static_world_utils::Get_Subsystem(InComponent);

    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem),
        TEXT("No JoltStaticWorld subsystem for Component [{}] — game worlds only"), InComponent->GetName())
    { return 0; }

    return Subsystem->Request_BakeComponent(*InComponent);
}

auto
    UCk_Utils_JoltStaticWorld_UE::
    Request_RemoveComponent(
        UPrimitiveComponent* InComponent)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InComponent), TEXT("Request_RemoveComponent called with an INVALID Component"))
    { return; }

    auto* Subsystem = ck_jolt_static_world_utils::Get_Subsystem(InComponent);
    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->Request_RemoveComponent(*InComponent);
}

auto
    UCk_Utils_JoltStaticWorld_UE::
    Get_NumStaticBodies(
        const UObject* InWorldContextObject)
    -> int32
{
    const auto* Subsystem = ck_jolt_static_world_utils::Get_Subsystem(InWorldContextObject);
    if (ck::Is_NOT_Valid(Subsystem))
    { return 0; }

    return Subsystem->Get_NumStaticBodies();
}

auto
    UCk_Utils_JoltStaticWorld_UE::
    Get_NumUniqueShapes(
        const UObject* InWorldContextObject)
    -> int32
{
    const auto* Subsystem = ck_jolt_static_world_utils::Get_Subsystem(InWorldContextObject);
    if (ck::Is_NOT_Valid(Subsystem))
    { return 0; }

    return Subsystem->Get_NumUniqueShapes();
}

auto
    UCk_Utils_JoltStaticWorld_UE::
    Get_RayCastStaticWorld(
        const UObject* InWorldContextObject,
        FVector InStart,
        FVector InEnd)
    -> FCk_Jolt_StaticWorldRayHit_Result
{
    auto Result = FCk_Jolt_StaticWorldRayHit_Result{};

    const auto* Subsystem = ck_jolt_static_world_utils::Get_Subsystem(InWorldContextObject);
    if (ck::Is_NOT_Valid(Subsystem))
    { return Result; }

    const auto Hit = Subsystem->Get_RayCastStaticWorld(InStart, InEnd);

    Result.Set_HasHit(Hit._HasHit);
    Result.Set_Position(Hit._Position);
    Result.Set_Entity(Hit._Entity);

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
