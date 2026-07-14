#include "CkFloatAttribute_Processor.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/CkAttribute_Processor.inl.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_RecomputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttributeModifier_ComputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_MinMaxClamp);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttributeModifier_EndPlayAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_Refill);

// --------------------------------------------------------------------------------------------------------------------
