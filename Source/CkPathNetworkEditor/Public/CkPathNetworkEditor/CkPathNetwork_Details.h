#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <IDetailCustomization.h>
#include <UObject/WeakObjectPtrTemplates.h>

// --------------------------------------------------------------------------------------------------------------------

class ACk_PathNetwork_UE;
class IDetailLayoutBuilder;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    // Details-panel authoring button row for ACk_PathNetwork_UE. Added ribbons are then edited by dragging
    // their MakeEditWidget point handles in the viewport; promoting generated ribbons to authored is what
    // stops a later re-detect from clobbering hand edits.
    class FCk_PathNetwork_Details : public IDetailCustomization
    {
    public:
        CK_GENERATED_BODY(FCk_PathNetwork_Details);

    public:
        static auto MakeInstance() -> TSharedRef<IDetailCustomization>;

    public:
        auto CustomizeDetails(
            IDetailLayoutBuilder& DetailBuilder) -> void override;

    private:
        auto DoRunDetector() -> FReply;
        auto DoPromoteGenerated() -> FReply;
        auto DoClearGenerated() -> FReply;
        auto DoAddRibbon() -> FReply;
        auto DoValidateAgainstNavmesh() -> FReply;

    private:
        TArray<TWeakObjectPtr<ACk_PathNetwork_UE>> _CustomizedActors;
    };
}

// --------------------------------------------------------------------------------------------------------------------
