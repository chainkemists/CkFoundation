#pragma once

#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------
// Its own small header so foundational code (e.g. CkProcessor.h) can pick the tag up without dragging
// the editor subsystem into every processor translation unit. Stamped on the editor subsystem's
// transient entity and cascaded to descendants; TIgnoreInEditor<> dispatch keys off it.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_EditorOnlyEntity);
}

#define CK_IF_EDITOR_ONLY_ENTITY ck::FTag_EditorOnlyEntity