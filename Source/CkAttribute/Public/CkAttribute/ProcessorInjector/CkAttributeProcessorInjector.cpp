#include "CkAttributeProcessorInjector.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Attribute_ProcessorInjector_Teardown::DoInjectProcessors(
        EcsWorldType& InWorld)
{
    InWorld.Add<ck::FProcessor_FloatAttributeModifier_TeardownAll>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Attribute_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_FloatAttribute_RecomputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttributeModifier_ComputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttribute_MinMaxClamp>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_FloatAttribute_FireSignals>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
