#include "CkProcessorGroups.h"
#include "CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_GROUP(ck::FGroup_PreDestruction);
CK_REGISTER_GROUP(ck::FGroup_DestructionPipeline);
CK_REGISTER_GROUP(ck::FGroup_Gameplay_TimeDelta);
CK_REGISTER_GROUP(ck::FGroup_Gameplay);
CK_REGISTER_GROUP(ck::FGroup_Gameplay_AI);
CK_REGISTER_GROUP(ck::FGroup_Gameplay_Audio);
CK_REGISTER_GROUP(ck::FGroup_Gameplay_Rendering);
CK_REGISTER_GROUP(ck::FGroup_Gameplay_Script);
CK_REGISTER_GROUP(ck::FGroup_Gameplay_Chaos);
CK_REGISTER_GROUP(ck::FGroup_Physics);
CK_REGISTER_GROUP(ck::FGroup_Transform_SyncFrom);
CK_REGISTER_GROUP(ck::FGroup_Transform);
CK_REGISTER_GROUP(ck::FGroup_Transform_Finalize);
CK_REGISTER_GROUP(ck::FGroup_PostTransform);
CK_REGISTER_GROUP(ck::FGroup_Replication);
CK_REGISTER_GROUP(ck::FGroup_EntityLifecycle);
CK_REGISTER_GROUP(ck::FGroup_Overlap);

// --------------------------------------------------------------------------------------------------------------------
