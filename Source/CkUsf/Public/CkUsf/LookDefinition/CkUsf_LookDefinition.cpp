#include "CkUsf/LookDefinition/CkUsf_LookDefinition.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_LookDefinition::
    Get_EffectiveLookName() const
    -> FName
{
    return _LookName.IsNone() ? GetFName() : _LookName;
}

// --------------------------------------------------------------------------------------------------------------------
