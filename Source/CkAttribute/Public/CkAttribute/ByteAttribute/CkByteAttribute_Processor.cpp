#include "CkByteAttribute_Processor.h"

#include "CkAttribute/CkAttribute_Processor.inl.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttribute_RecomputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttributeModifier_ComputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttribute_MinMaxClamp);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttribute_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttributeModifier_EndPlayAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttribute_Replicate);

// --------------------------------------------------------------------------------------------------------------------
