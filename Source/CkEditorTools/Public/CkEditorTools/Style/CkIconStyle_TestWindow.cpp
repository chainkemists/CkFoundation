// ====================================================================================================================
// TEMPORARY verification surface for the icon pipeline (Phase 2, step 5) — DELETE after the
// in-editor visual check is confirmed. Not referenced by anything; reachable only via the
// console command below.
//
//   Ck.Icons.ShowTestWindow
//
// Expected: every seed icon renders crisply at 16 and 24, white in the first two columns and
// tinted in the rest. A BLACK glyph means the recolour step failed; a BLANK cell means the
// vendored SVG is missing (an ensure will have fired at registration).
// ====================================================================================================================

#include "CkEditorTools/Style/CkIconStyle.h"

#include <Framework/Application/SlateApplication.h>
#include <HAL/IConsoleManager.h>
#include <Widgets/Images/SImage.h>
#include <Widgets/SBoxPanel.h>
#include <Widgets/SWindow.h>
#include <Widgets/Text/STextBlock.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_icon_style_test_window
{
    static FAutoConsoleCommand ShowTestWindowCommand
    {
        TEXT("Ck.Icons.ShowTestWindow"),
        TEXT("TEMPORARY: opens a window rendering every generated ECk_Icon at 16/24 in several tints"),
        FConsoleCommandDelegate::CreateLambda([]() -> void
        {
            const auto Tints = TArray<FLinearColor>
            {
                FLinearColor::White,
                FLinearColor{0.2f, 0.6f, 1.0f},
                FLinearColor{0.2f, 0.9f, 0.3f},
                FLinearColor{1.0f, 0.3f, 0.2f},
            };

            const auto Rows = SNew(SVerticalBox);

            for (const auto& Entry : ck::icons::Get_GeneratedEntries())
            {
                const auto Row = SNew(SHorizontalBox);

                Row->AddSlot()
                .AutoWidth()
                .Padding(8.0f, 4.0f)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .MinDesiredWidth(90.0f)
                    .Text(FText::FromString(Entry.SemanticName))
                ];

                for (const auto Size : { ECk_Icon_BrushSize::Size_16x16, ECk_Icon_BrushSize::Size_24x24 })
                {
                    for (const auto& Tint : Tints)
                    {
                        Row->AddSlot()
                        .AutoWidth()
                        .Padding(6.0f, 4.0f)
                        .VAlign(VAlign_Center)
                        [
                            SNew(SImage)
                            .Image(FCkIconStyle::Get_Brush(Entry.Icon, Size))
                            .ColorAndOpacity(Tint)
                        ];
                    }
                }

                Rows->AddSlot()
                .AutoHeight()
                [
                    Row
                ];
            }

            const auto Window = SNew(SWindow)
                .Title(FText::FromString(TEXT("CkIconStyle test — TEMPORARY")))
                .ClientSize(FVector2D{560.0f, 640.0f})
                [
                    Rows
                ];

            FSlateApplication::Get().AddWindow(Window);
        })
    };
}

// --------------------------------------------------------------------------------------------------------------------
