#include "CkAttribute_ProcessorInjector.h"

#include "CkAttribute/ByteAttribute/CkByteAttribute_Processor.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Processor.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Processor.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Attribute_ProcessorInjector_Teardown::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_ByteAttributeModifier_EndPlayAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_IntegerAttributeModifier_EndPlayAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttributeModifier_EndPlayAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_VectorAttributeModifier_EndPlayAll>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Attribute_ProcessorInjector_Refill::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_FloatAttribute_Refill>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_IntegerAttribute_Refill>(InWorld.Get_Registry());
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

    InWorld.Add<ck::FProcessor_IntegerAttribute_RecomputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_IntegerAttributeModifier_ComputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_IntegerAttribute_MinMaxClamp>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_FloatAttribute_RecomputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttributeModifier_ComputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttribute_MinMaxClamp>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_VectorAttribute_RecomputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_VectorAttributeModifier_ComputeAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_VectorAttribute_MinMaxClamp>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_ByteAttribute_FireSignals>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_IntegerAttribute_FireSignals>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttribute_FireSignals>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_VectorAttribute_FireSignals>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Attribute_ProcessorInjector_Replicate::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_ByteAttribute_Replicate>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_IntegerAttribute_Replicate>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_FloatAttribute_Replicate>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_VectorAttribute_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
