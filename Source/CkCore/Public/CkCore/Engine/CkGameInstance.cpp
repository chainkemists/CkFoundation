#include "CkGameInstance.h"

// --------------------------------------------------------------------------------------------------------------------

TWeakObjectPtr<UCk_GameInstance_UE> UCk_GameInstance_UE::_Instance;

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameInstance_UE::
    Init()
    -> void
{
    Super::Init();

    _Instance = this;
}

auto
    UCk_GameInstance_UE::
    Shutdown()
    -> void
{
    _Instance = nullptr;

    Super::Shutdown();
}

auto
    UCk_GameInstance_UE::
    Get_Instance()
    -> ThisType*
{
    return _Instance.Get();
}

// --------------------------------------------------------------------------------------------------------------------
