#include "CkParticlesEditor/Generator/CkParticles_GeneratorSubsystem.h"

#include "CkParticlesEditor/Generator/CkParticles_Generator.h"
#include "CkParticlesEditor/Generator/CkParticles_TemplateBuilder.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CkParticles_GeneratorSubsystem)

// --------------------------------------------------------------------------------------------------------------------

void
    UCkParticles_GeneratorSubsystem::
    Create_TemplateSystem()
{
    ck::particles_editor::Build_TemplateSystem();
}

void
    UCkParticles_GeneratorSubsystem::
    Generate_ParticleSystems()
{
    ck::particles_editor::Generate_AllParticleSystems();
}

// --------------------------------------------------------------------------------------------------------------------
