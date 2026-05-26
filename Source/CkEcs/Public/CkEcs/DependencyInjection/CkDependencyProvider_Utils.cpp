#include "CkDependencyProvider_Utils.h"

#include "CkDependencyProvider_InjectionCache.h"
#include "CkDependencyProvider_World_Subsystem.h"
#include "CkDependencyProvider_GameInstance_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/CkEcsLog.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto
    Get_World(
        const UObject* InWorldContextObject)
        -> UWorld*
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject),
            TEXT("DependencyProvider BPFL called with null WorldContextObject"))
        { return nullptr; }

        if (GEngine == nullptr)
        { return nullptr; }

        return GEngine->GetWorldFromContextObject(
            InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    }

    auto
    Get_WorldSubsystem(
        const UObject* InWorldContextObject)
        -> UCk_DependencyProvider_World_Subsystem_UE*
    {
        auto* World = Get_World(InWorldContextObject);
        if (ck::Is_NOT_Valid(World))
        { return nullptr; }

        return World->GetSubsystem<UCk_DependencyProvider_World_Subsystem_UE>();
    }

    auto
    Get_GameInstanceSubsystem(
        const UObject* InWorldContextObject)
        -> UCk_DependencyProvider_GameInstance_Subsystem_UE*
    {
        auto* World = Get_World(InWorldContextObject);
        if (ck::Is_NOT_Valid(World))
        { return nullptr; }

        auto* GameInstance = World->GetGameInstance();
        if (ck::Is_NOT_Valid(GameInstance))
        { return nullptr; }

        return GameInstance->GetSubsystem<UCk_DependencyProvider_GameInstance_Subsystem_UE>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DependencyProvider_UE::
    Register(
        const UObject* InWorldContextObject,
        ECk_DependencyProvider_Scope InScope,
        const FCk_Request_DependencyProvider_Register& InRequest)
    -> void
{
    switch (InScope)
    {
        case ECk_DependencyProvider_Scope::GameInstance:
        {
            if (auto* Subsystem = Get_GameInstanceSubsystem(InWorldContextObject))
            { Subsystem->Register(InRequest); }
            break;
        }
        case ECk_DependencyProvider_Scope::World:
        default:
        {
            if (auto* Subsystem = Get_WorldSubsystem(InWorldContextObject))
            { Subsystem->Register(InRequest); }
            break;
        }
    }
}

auto
    UCk_Utils_DependencyProvider_UE::
    Unregister(
        const UObject* InWorldContextObject,
        ECk_DependencyProvider_Scope InScope,
        UScriptStruct* InHandleType)
    -> void
{
    switch (InScope)
    {
        case ECk_DependencyProvider_Scope::GameInstance:
        {
            if (auto* Subsystem = Get_GameInstanceSubsystem(InWorldContextObject))
            { Subsystem->Unregister(InHandleType); }
            break;
        }
        case ECk_DependencyProvider_Scope::World:
        default:
        {
            if (auto* Subsystem = Get_WorldSubsystem(InWorldContextObject))
            { Subsystem->Unregister(InHandleType); }
            break;
        }
    }
}

auto
    UCk_Utils_DependencyProvider_UE::
    Resolve(
        const UObject* InWorldContextObject,
        ECk_DependencyProvider_Scope InScope,
        UScriptStruct* InHandleType)
    -> FCk_Handle
{
    switch (InScope)
    {
        case ECk_DependencyProvider_Scope::GameInstance:
        {
            if (auto* Subsystem = Get_GameInstanceSubsystem(InWorldContextObject))
            { return Subsystem->Resolve(InHandleType); }
            break;
        }
        case ECk_DependencyProvider_Scope::World:
        default:
        {
            if (auto* Subsystem = Get_WorldSubsystem(InWorldContextObject))
            { return Subsystem->Resolve(InHandleType); }
            break;
        }
    }
    return {};
}

auto
    UCk_Utils_DependencyProvider_UE::
    Get_PendingCount(
        const UObject* InWorldContextObject,
        ECk_DependencyProvider_Scope InScope,
        UScriptStruct* InHandleType)
    -> int32
{
    switch (InScope)
    {
        case ECk_DependencyProvider_Scope::GameInstance:
        {
            if (auto* Subsystem = Get_GameInstanceSubsystem(InWorldContextObject))
            { return Subsystem->Get_PendingCount_ForType(InHandleType); }
            break;
        }
        case ECk_DependencyProvider_Scope::World:
        default:
        {
            if (auto* Subsystem = Get_WorldSubsystem(InWorldContextObject))
            { return Subsystem->Get_PendingCount_ForType(InHandleType); }
            break;
        }
    }
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DependencyProvider_UE::
    Get_InjectionSiteCount_ForClass(
        UClass* InScriptClass)
    -> int32
{
    return FCk_InjectionCache::GetOrBuild(InScriptClass).Num();
}

auto
    UCk_Utils_DependencyProvider_UE::
    Get_InjectionSitePropertyName_ForClass(
        UClass* InScriptClass,
        int32 InIndex)
    -> FName
{
    const auto& Plan = FCk_InjectionCache::GetOrBuild(InScriptClass);
    if (NOT Plan.IsValidIndex(InIndex))
    { return NAME_None; }

    if (Plan[InIndex]._PropertyOnScript == nullptr)
    { return NAME_None; }

    return Plan[InIndex]._PropertyOnScript->GetFName();
}

auto
    UCk_Utils_DependencyProvider_UE::
    Get_InjectionSiteScope_ForClass(
        UClass* InScriptClass,
        int32 InIndex)
    -> ECk_DependencyProvider_Scope
{
    const auto& Plan = FCk_InjectionCache::GetOrBuild(InScriptClass);
    if (NOT Plan.IsValidIndex(InIndex))
    { return ECk_DependencyProvider_Scope::World; }

    return Plan[InIndex]._Scope;
}

auto
    UCk_Utils_DependencyProvider_UE::
    Get_InjectionSiteHandleType_ForClass(
        UClass* InScriptClass,
        int32 InIndex)
    -> UScriptStruct*
{
    const auto& Plan = FCk_InjectionCache::GetOrBuild(InScriptClass);
    if (NOT Plan.IsValidIndex(InIndex))
    { return nullptr; }

    return Plan[InIndex]._HandleType;
}

auto
    UCk_Utils_DependencyProvider_UE::
    Get_InjectionSiteCppTypeName_ForClass(
        UClass* InScriptClass,
        int32 InIndex)
    -> FName
{
    const auto& Plan = FCk_InjectionCache::GetOrBuild(InScriptClass);
    if (NOT Plan.IsValidIndex(InIndex))
    { return NAME_None; }

    const auto* Prop = Plan[InIndex]._PropertyOnScript;
    if (Prop == nullptr)
    { return NAME_None; }

    // GetCPPType returns the spelled-out C++ type for the property (e.g.
    // "FCk_Handle_DayCycle" for a UPROPERTY of that type). For AS-declared
    // dynamic handles where FProperty::Struct erases to FCk_Handle, this is
    // currently our best candidate identity surface for keying the DI
    // registry on the actual AS-declared type.
    return FName{*Prop->GetCPPType()};
}
