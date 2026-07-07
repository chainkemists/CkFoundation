#include "CkUsfEditor/Generator/CkUsf_GeneratorSubsystem.h"

#include "CkUsfEditor/Generator/CkUsf_Generator.h"

// --------------------------------------------------------------------------------------------------------------------

void
    UCkUsf_GeneratorSubsystem::
    Generate_LookMaterials()
{
    ck::usf_editor::Generate_AllLookMaterials();
}

void
    UCkUsf_GeneratorSubsystem::
    Generate_LookMaterial_For(
        UCkUsf_LookDefinition* InLook)
{
    ck::usf_editor::Generate_LookMaterial(InLook);
}

// --------------------------------------------------------------------------------------------------------------------
