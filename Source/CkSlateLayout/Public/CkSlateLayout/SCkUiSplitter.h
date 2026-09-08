#pragma once

#include "CkSlateLayout/CkUiWidgetRegistry.h"

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SSplitter;
namespace ck_ui_splitter { class FPreparedUpdate; }

/**
 * Retained native splitter whose authored pane configuration is prepared before
 * publication. Pane identity is the stable authored id.
 */
class CKSLATELAYOUT_API SCkUiSplitter final : public SCompoundWidget
{
public:
    struct FPane
    {
        FString Id;
        TSharedPtr<SWidget> Content;
        float Weight = 1.0f;
        float MinSize = 0.0f;
    };

    SLATE_BEGIN_ARGS(SCkUiSplitter)
        : _Orientation(Orient_Horizontal)
    {
    }
        SLATE_ARGUMENT(EOrientation, Orientation)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    /** Validates a whole pane configuration without modifying the mounted splitter. */
    auto Prepare(TArray<FPane> InPanes, FString& OutFailure) -> TUniquePtr<ICkUiPreparedWidgetUpdate>;
    auto GetSplitter() const -> TSharedPtr<SSplitter>;

private:
    friend class ck_ui_splitter::FPreparedUpdate;

    struct FCommittedPane
    {
        FString Id;
        TSharedPtr<SWidget> Content;
        float AuthoredWeight = 1.0f;
        float MinSize = 0.0f;
    };

    auto Commit(TArray<FPane>&& InPanes) -> void;

    TSharedPtr<SSplitter> _Splitter;
    TArray<FCommittedPane> _Panes;
};
