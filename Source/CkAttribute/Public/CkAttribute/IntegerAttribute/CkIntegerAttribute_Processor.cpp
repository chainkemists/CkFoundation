#include "CkIntegerAttribute_Processor.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/CkAttribute_Processor.inl.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_RecomputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttributeModifier_ComputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_MinMaxClamp);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttributeModifier_EndPlayAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_ReplicateOnRestore);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_Refill);

// --------------------------------------------------------------------------------------------------------------------
