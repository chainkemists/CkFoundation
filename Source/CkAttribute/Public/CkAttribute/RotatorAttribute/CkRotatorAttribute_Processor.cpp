#include "CkRotatorAttribute_Processor.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/CkAttribute_Processor.inl.h"
#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_RotatorAttribute_RecomputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_RotatorAttributeModifier_ComputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_RotatorAttribute_MinMaxClamp);
CK_REGISTER_PROCESSOR(ck::FProcessor_RotatorAttribute_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_RotatorAttributeModifier_EndPlayAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_RotatorAttribute_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_RotatorAttribute_ReplicateOnRestore);

// --------------------------------------------------------------------------------------------------------------------
