#include "CkObjectPooling_Settings.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ObjectPooling_ProjectSettings_UE::
    TryGet_PoolEntry(
        const TSubclassOf<UObject>& InObjectClass)
    -> TOptional<FCk_ObjectPooling_SettingsEntry>
{
    if (ck::Is_NOT_Valid(InObjectClass))
    { return {}; }

    const auto* Settings = GetDefault<UCk_ObjectPooling_ProjectSettings_UE>();

    for (const auto& Entry : Settings->Get_ObjectPools())
    {
        if (Entry.Get_ObjectClass().IsNull())
        { continue; }

        // sync load OK — pool creation is rare
        if (Entry.Get_ObjectClass().LoadSynchronous() == InObjectClass.Get())
        { return Entry; }
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------
