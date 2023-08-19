#include "CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle::FCk_Handle(FEntityType InEntity, const FRegistryType& InRegistry)
    : _Entity(InEntity)
    , _Registry(InRegistry)
#if WITH_EDITORONLY_DATA
    , _EntityID(static_cast<int32>(_Entity.Get_ID()))
    , _EntityNumber(_Entity.Get_EntityNumber())
    , _EntityVersion(_Entity.Get_VersionNumber())
#endif
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
