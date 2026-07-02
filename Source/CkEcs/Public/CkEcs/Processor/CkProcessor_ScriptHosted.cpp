#include "CkProcessor_ScriptHosted.h"

#include "CkProcessor_Script.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/CkEcsLog.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FProcessor_ScriptHosted::
        FProcessor_ScriptHosted(
            const RegistryType& InRegistry,
            UClass* InScriptClass)
        : _Registry(InRegistry)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InScriptClass),
            TEXT("FProcessor_ScriptHosted constructed with null UClass"))
        { return; }

        // Spawn the hosted instance using GetTransientPackage as outer. The instance is a plain UObject —
        // world access is brokered through registry handles passed into its Tick body rather than GetWorld().
        auto* Instance = NewObject<UCk_Processor_Script_Base_UE>(GetTransientPackage(), InScriptClass);
        _Instance = TStrongObjectPtr<UCk_Processor_Script_Base_UE>{Instance};

        if (ck::IsValid(_Instance.Get()))
        {
            const auto TransientHandle = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InRegistry);
            _Instance->Set_Handle(TransientHandle);
            _Instance->BeginPlay();
        }
    }

    FProcessor_ScriptHosted::
        ~FProcessor_ScriptHosted()
    {
        if (ck::IsValid(_Instance.Get()))
        {
            _Instance->EndPlay();
        }
    }

    auto
        FProcessor_ScriptHosted::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        if (ck::Is_NOT_Valid(_Instance.Get()))
        { return; }

        // During registry teardown (e.g. a snapshot-load level transition) the scheduler keeps ticking for a
        // few frames after the transient entity is destroyed. Every ForEach helper the script body could call
        // would ensure on the dead handle — skip the dispatch entirely, matching how C++ processors see an
        // empty view instead of erroring.
        if (ck::Is_NOT_Valid(_Instance->Get_Handle()))
        { return; }

        _Instance->Tick(InDeltaT);
    }

    auto
        FProcessor_ScriptHosted::
        Pump()
        -> void
    {
        Tick(TimeType::ZeroSecond());
    }
}

// --------------------------------------------------------------------------------------------------------------------
