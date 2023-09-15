#include "CkEnsure_Subsystem.h"

#include "CkEnsure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Ensure_Subsystem_UE::Initialize(FSubsystemCollectionBase& InCollection) -> void
{
    Super::Initialize(InCollection);

    OnInitialize();
}

auto UCk_Ensure_Subsystem_UE::Deinitialize() -> void
{
    OnDeinitialize();

    UCk_Utils_Ensure_UE::Request_ClearAllIgnoredEnsures();

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------
