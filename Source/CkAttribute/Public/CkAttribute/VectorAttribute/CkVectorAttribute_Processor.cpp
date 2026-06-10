#include "CkVectorAttribute_Processor.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/CkAttribute_Processor.inl.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_VectorAttribute_RecomputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_VectorAttributeModifier_ComputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_VectorAttribute_MinMaxClamp);
CK_REGISTER_PROCESSOR(ck::FProcessor_VectorAttribute_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_VectorAttributeModifier_EndPlayAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_VectorAttribute_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_VectorAttribute_ReplicateOnRestore);

// --------------------------------------------------------------------------------------------------------------------
