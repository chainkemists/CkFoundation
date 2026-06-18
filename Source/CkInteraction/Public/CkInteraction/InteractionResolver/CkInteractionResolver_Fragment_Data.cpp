#include "CkInteractionResolver_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

// NOTE: TAG_Label_InteractionChannel ("InteractionChannel") is registered by the Interaction core
// (CkInteraction_Fragment_Data.cpp). The duplicate UE_DEFINE_GAMEPLAY_TAG_STATIC that used to live here
// was dead (registration side-effect only, never referenced) and collided with the core definition under
// unity builds (C2374) — removed. The tag is still registered exactly once, module-wide.