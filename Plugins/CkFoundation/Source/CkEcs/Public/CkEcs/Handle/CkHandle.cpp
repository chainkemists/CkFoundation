#include "CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle::FCk_Handle(FEntityType InEntity, const FRegistryType& InRegistry)
    : _Entity(InEntity)
    , _Registry(InRegistry)
{
}

auto FCk_Handle::operator*() -> TOptional<FCk_Registry>
{
    return _Registry;
}

auto FCk_Handle::operator*() const -> TOptional<FCk_Registry>
{
    return _Registry;
}

auto FCk_Handle::IsValid() const -> bool
{
    return ck::IsValid(_Registry) && _Registry->IsValid(_Entity);
}

// --------------------------------------------------------------------------------------------------------------------
