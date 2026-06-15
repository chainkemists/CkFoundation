#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CkParticles_ScriptDefinition)

// --------------------------------------------------------------------------------------------------------------------

FName
    UCkParticles_ScriptDefinition::
    Get_EffectiveScriptName() const
{
    return _ScriptName.IsNone() ? GetFName() : _ScriptName;
}

// --------------------------------------------------------------------------------------------------------------------
