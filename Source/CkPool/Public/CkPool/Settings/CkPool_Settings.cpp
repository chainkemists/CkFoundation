#include "CkPool_Settings.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Pool_ProjectSettings_UE::
    TryGet_EntityPoolEntry(
        const TSubclassOf<UCk_EntityScript_UE>& InEntityScriptClass)
    -> TOptional<FCk_EntityPool_SettingsEntry>
{
    if (ck::Is_NOT_Valid(InEntityScriptClass))
    { return {}; }

    const auto& Entries = GetDefault<UCk_Pool_ProjectSettings_UE>()->Get_EntityPools();
    const auto ClassAsSoftPtr = TSoftClassPtr<UCk_EntityScript_UE>{InEntityScriptClass.Get()};

    return ck::algo::FindIf(Entries, [&](const FCk_EntityPool_SettingsEntry& InEntry)
    {
        return InEntry.Get_EntityScriptClass() == ClassAsSoftPtr;
    });
}

auto
    UCk_Pool_ProjectSettings_UE::
    TryGet_ObjectPoolEntry(
        const TSubclassOf<UObject>& InObjectClass)
    -> TOptional<FCk_ObjectPool_SettingsEntry>
{
    if (ck::Is_NOT_Valid(InObjectClass))
    { return {}; }

    const auto& Entries = GetDefault<UCk_Pool_ProjectSettings_UE>()->Get_ObjectPools();
    const auto ClassAsSoftPtr = TSoftClassPtr<UObject>{InObjectClass.Get()};

    return ck::algo::FindIf(Entries, [&](const FCk_ObjectPool_SettingsEntry& InEntry)
    {
        return InEntry.Get_ObjectClass() == ClassAsSoftPtr;
    });
}

// --------------------------------------------------------------------------------------------------------------------
