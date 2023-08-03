#include "CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle::FCk_Handle(FEntityType InEntity, const FRegistryType& InRegistry)
    : _Entity(InEntity)
    , _Registry(InRegistry)
{
}

auto FCk_Handle::IsValid() -> bool
{
    return _Registry->IsValid(_Entity);
}

// --------------------------------------------------------------------------------------------------------------------
