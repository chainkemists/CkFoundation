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
    // Authoring toolbar for ACk_PathNetwork_UE: a button row in the details panel driving the
    // manual + automatic workflows — add a ribbon (then drag its MakeEditWidget point handles in
    // the viewport), run the assigned detector over the detection bounds, promote generated
    // ribbons to authored (so a re-detect can't clobber hand edits), clear generated, and
    // validate ribbon points against the navmesh.
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
