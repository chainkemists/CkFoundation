#include "CkEntityPool_Subsystem.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkPool/CkPool_Log.h"
#include "CkPool/EntityPool/CkEntityPool_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityPool_Subsystem_UE::
    Deinitialize()
    -> void
{
    _PoolsByClass.Reset();
    _PoolsByName.Reset();

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityPool_Subsystem_UE::
    DoGetOrCreate_Pool(
        const FCk_Fragment_EntityPool_ParamsData& InParams)
    -> FCk_Handle_EntityPool
{
    const auto& PoolName = InParams.Get_PoolName();

    if (auto Existing = PoolName.IsValid()
            ? DoTryGet_Pool_ByName(PoolName)
            : DoTryGet_Pool_ByClass(InParams.Get_EntityScriptClass());
        ck::IsValid(Existing))
    { return Existing; }

    auto NewPool = UCk_Utils_EntityPool_UE::DoCreate_PoolEntity(this, InParams);

    if (ck::Is_NOT_Valid(NewPool))
    { return {}; }

    if (PoolName.IsValid())
    { _PoolsByName.Add(PoolName, NewPool); }
    else
    { _PoolsByClass.Add(InParams.Get_EntityScriptClass(), NewPool); }

    ck::pool::Verbose(TEXT("Created EntityPool [{}] for EntityScript class [{}]"), NewPool, InParams.Get_EntityScriptClass());

    return NewPool;
}

auto
    UCk_EntityPool_Subsystem_UE::
    DoTryGet_Pool_ByClass(
        const TSubclassOf<UCk_EntityScript_UE>& InEntityScriptClass)
    -> FCk_Handle_EntityPool
{
    if (const auto* Found = _PoolsByClass.Find(InEntityScriptClass))
    {
        if (ck::IsValid(*Found))
        { return *Found; }

        _PoolsByClass.Remove(InEntityScriptClass);
    }

    return {};
}

auto
    UCk_EntityPool_Subsystem_UE::
    DoTryGet_Pool_ByName(
        const FGameplayTag& InPoolName)
    -> FCk_Handle_EntityPool
{
    if (const auto* Found = _PoolsByName.Find(InPoolName))
    {
        if (ck::IsValid(*Found))
        { return *Found; }

        _PoolsByName.Remove(InPoolName);
    }

    return {};
}

auto
    UCk_EntityPool_Subsystem_UE::
    DoForget_Pool(
        const FCk_Handle_EntityPool& InPool)
    -> void
{
    for (auto It = _PoolsByClass.CreateIterator(); It; ++It)
    {
        if (It.Value() == InPool)
        { It.RemoveCurrent(); }
    }

    for (auto It = _PoolsByName.CreateIterator(); It; ++It)
    {
        if (It.Value() == InPool)
        { It.RemoveCurrent(); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
