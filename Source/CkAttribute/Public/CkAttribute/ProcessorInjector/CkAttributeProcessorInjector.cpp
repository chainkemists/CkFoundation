#include "CkAttributeProcessorInjector.h"

#include "CkAttribute/ByteAttribute/CkByteAttribute_Processor.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Processor.h"
#include "CkAttribute/MeterAttribute/CkMeterAttribute_Processor.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Attribute_ProcessorInjector_Teardown::DoInjectProcessors(
        EcsWorldType& InWorld)
{
    InWorld.Add<ck::FProcessor_VectorAttributeModifier_Additive_Teardown>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_VectorAttributeModifier_Multiplicative_Teardown>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_FloatAttributeModifier_Additive_Teardown>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttributeModifier_Multiplicative_Teardown>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_ByteAttributeModifier_Additive_Teardown>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_ByteAttributeModifier_Multiplicative_Teardown>(InWorld.Get_Registry());
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

    InWorld.Add<ck::FProcessor_FloatAttribute_RecomputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttributeModifier_ComputeAll>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_VectorAttribute_RecomputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_VectorAttributeModifier_ComputeAll>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Meter_Clamp>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_ByteAttribute_FireSignals>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttribute_FireSignals>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_VectorAttribute_FireSignals>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
