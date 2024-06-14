#include "CkAttributeProcessorInjector.h"

#include "CkAttribute/ByteAttribute/CkByteAttribute_Processor.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Processor.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Attribute_ProcessorInjector_Teardown::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_ByteAttributeModifier_TeardownAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttributeModifier_TeardownAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_VectorAttributeModifier_TeardownAll>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Attribute_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_ByteAttribute_RecomputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_ByteAttributeModifier_ComputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_ByteAttribute_MinMaxClamp>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_FloatAttribute_RecomputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttributeModifier_ComputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttribute_MinMaxClamp>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_VectorAttribute_RecomputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_VectorAttributeModifier_ComputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_VectorAttribute_MinMaxClamp>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_ByteAttribute_FireSignals>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttribute_FireSignals>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_VectorAttribute_FireSignals>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_FloatAttribute_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
