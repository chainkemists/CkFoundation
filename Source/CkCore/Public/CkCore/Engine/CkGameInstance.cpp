#include "CkGameInstance.h"

#include "CkCore/CkCoreLog.h"

// --------------------------------------------------------------------------------------------------------------------

TWeakObjectPtr<UCk_GameInstance_UE> UCk_GameInstance_UE::_Instance;

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameInstance_UE::
    Init()
    -> void
{
    TRACE_BOOKMARK(TEXT("Game Instance Init"));
    ck::core::Log(TEXT("Game Instance [{}] Init"), this);
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
