#pragma once

#include "CkEcs/Tag/CkTag.h"

// --------------------------------------------------------------------------------------------------------------------
// FTag_EditorOnlyEntity is its own small header so foundational code (e.g. CkProcessor.h) can
// pick it up without dragging the editor subsystem (UCk_EditorEcsWorld_Subsystem_UE) into every
// processor translation unit. The tag is stamped on the editor subsystem's transient entity
// and cascades to descendants — see CkEcsEditor_Subsystem.cpp and CkEntityLifetime_Utils.cpp.
//
// Used by:
//   - Editor-only processor view filters via the CK_IF_EDITOR_ONLY_ENTITY shorthand below.
//   - TIgnoreInEditor<> dispatch in TProcessor::DoTick — the runtime view excludes this tag,
//     the editor view requires it.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_EditorOnlyEntity);
}

#define CK_IF_EDITOR_ONLY_ENTITY ck::FTag_EditorOnlyEntity