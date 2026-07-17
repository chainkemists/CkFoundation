#include "CkPmg_Fragment.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKPMG_API, pmg_donut, ck::FFragment_Pmg_Donut_UpdateParams);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pmg
{
    auto
        Get_MeshComponentOuter(
            UWorld* InWorld,
            const FCk_Handle& InHandle)
        -> UObject*
    {
#if WITH_EDITOR
        if (ck::IsValid(InWorld) && InWorld->WorldType == EWorldType::Editor)
        {
            // Composite shapes (icons, dashed lines, text glyph runs) render through child
            // entities — honor an opt-in stamped anywhere up the lifetime chain.
            const auto TaggedEntity = UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(InHandle,
                [](const FCk_Handle& InEntityInChain)
                {
                    return InEntityInChain.Has<ck::FTag_Pmg_EditorSelectionHandle>();
                });

            if (ck::IsValid(TaggedEntity))
            {
                if (auto* SelectionProxyHost = ck::editor_selection_owner::TryGet_SelectionProxyHostActor(InWorld, InHandle);
                    ck::IsValid(SelectionProxyHost))
                { return SelectionProxyHost; }
            }
        }
#endif

        return InWorld;
    }
}

// --------------------------------------------------------------------------------------------------------------------
