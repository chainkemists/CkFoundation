#include "CkWatermark_Panel_Widget.h"

#include "CkCore/Ensure/CkEnsure_Subsystem.h"
#include "CkCore/Engine/CkGameState.h"
#include "CkWatermark/Settings/CkWatermark_Settings.h"
#include "CkWatermark/Generated/CkWatermark_BuildId.h"
#include "CkMemory/CkMemory_Subsystem.h"
#include "CkEcs/Subsystem/CkEcsWorldStats_Subsystem.h"
#include "CkEcs/Settings/CkEcs_Settings.h"
#include "CkWatermark/Stats/CkWatermarkStat_Base_Widget.h"
#include "CkWatermark/CkWatermark_InfoBar_Widget.h"

#include <Fonts/SlateFontInfo.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>
#include <HAL/PlatformMemory.h>
#include <HAL/PlatformMisc.h>
#include <Styling/CoreStyle.h>
#include <Widgets/Layout/SBox.h>

ENGINE_API extern float  GAverageFPS;
extern ENGINE_API uint64 GFrameCounter;

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto GetHAlign(ECk_Watermark_GroupAnchor InAnchor) -> EHorizontalAlignment
    {
        switch (InAnchor)
        {
            case ECk_Watermark_GroupAnchor::TopLeft:
            case ECk_Watermark_GroupAnchor::BottomLeft:   return HAlign_Left;
            case ECk_Watermark_GroupAnchor::TopCenter:
            case ECk_Watermark_GroupAnchor::BottomCenter: return HAlign_Center;
            default:                                       return HAlign_Right;
        }
    }

    auto GetVAlign(ECk_Watermark_GroupAnchor InAnchor) -> EVerticalAlignment
    {
        switch (InAnchor)
        {
            case ECk_Watermark_GroupAnchor::TopLeft:
            case ECk_Watermark_GroupAnchor::TopCenter:
            case ECk_Watermark_GroupAnchor::TopRight: return VAlign_Top;
            default:                                   return VAlign_Bottom;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

UCkWatermark_Panel_UWidget_UE::UCkWatermark_Panel_UWidget_UE()
{
    // Prevent the overlay from intercepting game input; SynchronizeProperties()
    // propagates this UMG-level property to the Slate widget on every sync.
    Visibility = ESlateVisibility::HitTestInvisible;

    _StatsGroupPlacement.Anchor      = ECk_Watermark_GroupAnchor::BottomRight;
    _StatsGroupPlacement.EdgePadding = FMargin(0.f, 0.f, 8.f, 8.f);

    _EcsGroupsPlacement.Anchor      = ECk_Watermark_GroupAnchor::TopRight;
    _EcsGroupsPlacement.EdgePadding = FMargin(0.f, 8.f, 8.f, 0.f);

    _InfoGroupPlacement.Anchor      = ECk_Watermark_GroupAnchor::BottomLeft;
    _InfoGroupPlacement.EdgePadding = FMargin(8.f, 0.f, 0.f, 8.f);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermark_Panel_UWidget_UE::
    Request_SetDisplayPolicy(
        ECk_Watermark_DisplayPolicy InNewPolicy)
    -> void
{
    _CurrentDisplayPolicy = InNewPolicy;
    if (_SlateRoot)
    {
        _SlateRoot->Invalidate(EInvalidateWidgetReason::Visibility);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermark_Panel_UWidget_UE::
    RebuildWidget()
    -> TSharedRef<SWidget>
{
    // ---- Fonts + layout from project settings --------------------------------
    // Font override is read once at rebuild — changing the typeface still needs a
    // rebuild, but outline size/color are re-read live inside each lambda.
    const FSlateFontInfo FontOverride = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_FontOverride();
    const bool           bHasFont     = FontOverride.HasValidFont();

    auto MakeFont = [FontOverride, bHasFont](const char* InFace, int32 InSize) -> TAttribute<FSlateFontInfo>
    {
        return TAttribute<FSlateFontInfo>::CreateLambda([InFace, InSize, FontOverride, bHasFont]() -> FSlateFontInfo
        {
            FSlateFontInfo F = bHasFont
                ? FontOverride
                : FCoreStyle::GetDefaultFontStyle(InFace, InSize);
            F.TypefaceFontName             = FName(InFace);
            F.Size                         = InSize;
            F.OutlineSettings.OutlineSize  = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_TextOutlineSize();
            F.OutlineSettings.OutlineColor = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_TextOutlineColor();
            return F;
        });
    };

    const TAttribute<FSlateFontInfo> ValueFont   = MakeFont("Bold",    UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_ValueFontSize());
    const TAttribute<FSlateFontInfo> LabelFont   = MakeFont("Regular", UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_LabelFontSize());
    const TAttribute<FSlateFontInfo> BracketFont = MakeFont("Bold",    UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BracketFontSize());

    const FText BracketOpen  = FText::FromString(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BracketOpen());
    const FText BracketClose = FText::FromString(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BracketClose());

    const float InnerPad = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Bracket_InnerPadding();
    const TAttribute<FSlateColor> LabelColor = TAttribute<FSlateColor>::CreateLambda([]() -> FSlateColor
    {
        return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_LabelColor());
    });
    const float CellHPad = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_StatCell_HorizontalPadding();
    const float RowVPad  = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_VerticalPadding();

    // ---- Helper: one stat cell (uses standard value + bracket fonts) --------
    auto MakeStat = [ValueFont, LabelFont, BracketFont, BracketOpen, BracketClose,
                     InnerPad, LabelColor](
        TAttribute<FText>       InValue,
        TAttribute<FSlateColor> InColor,
        FText                   InName) -> TSharedRef<SCkWatermarkStat>
    {
        return SNew(SCkWatermarkStat)
            .StatValue(MoveTemp(InValue))
            .ValueColor(MoveTemp(InColor))
            .StatName(InName)
            .ValueFont(ValueFont)
            .LabelFont(LabelFont)
            .BracketFont(BracketFont)
            .BracketOpen(BracketOpen)
            .BracketClose(BracketClose)
            .BracketInnerPadding(InnerPad)
            .LabelColor(LabelColor);
    };

    // Color for stats that have no good/bad meaning — read from Project Settings.
    const auto UncoloredAttr = TAttribute<FSlateColor>::CreateWeakLambda(this, []() -> FSlateColor
    {
        return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_UncoloredStat_Color());
    });

    // ---- Number format helpers ----------------------------------------------
    auto MakeOpts1dp = []() -> FNumberFormattingOptions
    {
        FNumberFormattingOptions Opts;
        Opts.MinimumFractionalDigits = 1;
        Opts.MaximumFractionalDigits = 1;
        Opts.UseGrouping             = false;
        return Opts;
    };

    // ---- Visibility attributes ----------------------------------------------
    const auto VisMain = TAttribute<EVisibility>::CreateWeakLambda(this, [this]() -> EVisibility
    {
        return (_CurrentDisplayPolicy != ECk_Watermark_DisplayPolicy::Hidden)
            ? EVisibility::SelfHitTestInvisible
            : EVisibility::Collapsed;
    });

    const auto VisDetailed = TAttribute<EVisibility>::CreateWeakLambda(this, [this]() -> EVisibility
    {
        return (_CurrentDisplayPolicy == ECk_Watermark_DisplayPolicy::Detailed)
            ? EVisibility::SelfHitTestInvisible
            : EVisibility::Collapsed;
    });

    // ---- ECS group rows (Detailed only, one per tag) ------------------------
    TSharedRef<SVerticalBox> EcsBox = SNew(SVerticalBox);

    for (const FGameplayTag& Tag : _DetailedEcsGroups)
    {
        FString ShortName;
        const FString TagStr = Tag.GetTagName().ToString();
        if (!TagStr.Split(TEXT("."), nullptr, &ShortName,
                          ESearchCase::IgnoreCase, ESearchDir::FromEnd))
        {
            ShortName = TagStr;
        }

        EcsBox->AddSlot()
            .AutoHeight()
            .Padding(0.f, RowVPad)
            [
                MakeStat(
                    TAttribute<FText>::CreateWeakLambda(this, [this, Tag]() -> FText
                    {
                        if (!Tag.IsValid()) { return FText::FromString(TEXT("---")); }
                        if (const UWorld* World = GetWorld())
                        {
                            if (const UCk_EcsWorld_Stats_Subsystem_UE* Sub =
                                    World->GetSubsystem<UCk_EcsWorld_Stats_Subsystem_UE>())
                            {
                                const float Ms = Sub->Get_StatDataForEcsWorldTickingGroup(
                                    Tag, _EcsStatSource);
                                FNumberFormattingOptions Opts;
                                Opts.MinimumFractionalDigits = 2;
                                Opts.MaximumFractionalDigits = 2;
                                Opts.UseGrouping             = false;
                                return FText::Format(FText::FromString(TEXT("{0} ms")),
                                                     FText::AsNumber(Ms, &Opts));
                            }
                        }
                        return FText::FromString(TEXT("---"));
                    }),
                    UncoloredAttr,
                    FText::FromString(ShortName))
            ];
    }

    // ---- Info group (bottom-center, pre-built to avoid MSVC attribute-specifier ambiguity) --
    // Info bar appearance from project settings
    const TAttribute<FSlateFontInfo> IBFont = MakeFont("Regular", UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_InfoBar_FontSize());

    const FText        IBSep    = FText::FromString(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_InfoBar_Separator());
    const FText        IBKVSep  = FText::FromString(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_InfoBar_KeyValueSeparator());
    const TAttribute<FSlateColor> IBKeyCol = TAttribute<FSlateColor>::CreateLambda([]() -> FSlateColor
    {
        return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_InfoBar_KeyColor());
    });
    const TAttribute<FSlateColor> IBValCol = TAttribute<FSlateColor>::CreateLambda([]() -> FSlateColor
    {
        return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_InfoBar_ValueColor());
    });
    const TAttribute<FSlateColor> IBSepCol = TAttribute<FSlateColor>::CreateLambda([]() -> FSlateColor
    {
        return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_InfoBar_SeparatorColor());
    });

    // Helper: visibility attribute from a static bool getter.
    auto MakeInfoVis = [](bool (*Getter)()) -> TAttribute<EVisibility>
    {
        return TAttribute<EVisibility>::CreateLambda([Getter]() -> EVisibility
        {
            return Getter() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
        });
    };

    // Row A — Device Info entries
    TArray<FCkWatermarkInfoBarEntry> InfoRowA;

    InfoRowA.Add({
        FText::FromString(TEXT("CPU")),
        TAttribute<FText>::CreateLambda([]() -> FText
        {
            static const FString Brand = FPlatformMisc::GetCPUBrand().TrimStartAndEnd();
            return FText::FromString(Brand.IsEmpty() ? TEXT("---") : Brand);
        }),
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_CpuBrand)
    });

    InfoRowA.Add({
        FText::FromString(TEXT("OS")),
        TAttribute<FText>::CreateLambda([]() -> FText
        {
            static const FString Ver = FPlatformMisc::GetOSVersion();
            return FText::FromString(Ver.IsEmpty() ? TEXT("---") : Ver);
        }),
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_OsVersion)
    });

    InfoRowA.Add({
        FText::FromString(TEXT("Cores")),
        TAttribute<FText>::CreateLambda([]() -> FText
        {
            static const int32 Physical = FPlatformMisc::NumberOfCores();
            static const int32 Logical  = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
            return FText::FromString(FString::Printf(TEXT("%dc / %dt"), Physical, Logical));
        }),
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_CpuCores)
    });

    InfoRowA.Add({
        FText::FromString(TEXT("Net")),
        TAttribute<FText>::CreateLambda([]() -> FText
        {
            switch (FPlatformMisc::GetNetworkConnectionType())
            {
                case ENetworkConnectionType::None:         return FText::FromString(TEXT("None"));
                case ENetworkConnectionType::AirplaneMode: return FText::FromString(TEXT("Airplane"));
                case ENetworkConnectionType::Cell:         return FText::FromString(TEXT("Cell"));
                case ENetworkConnectionType::WiFi:         return FText::FromString(TEXT("WiFi"));
                case ENetworkConnectionType::WiMAX:        return FText::FromString(TEXT("WiMAX"));
                case ENetworkConnectionType::Bluetooth:    return FText::FromString(TEXT("Bluetooth"));
                case ENetworkConnectionType::Ethernet:     return FText::FromString(TEXT("Ethernet"));
                default:                                    return FText::FromString(TEXT("Unknown"));
            }
        }),
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_NetworkType)
    });

    // Row B — ECS Debug entries
    TArray<FCkWatermarkInfoBarEntry> InfoRowB;

    InfoRowB.Add({
        FText::FromString(TEXT("ECS DBG")),
        TAttribute<FText>::CreateLambda([]() -> FText
        {
            switch (UCk_Utils_Ecs_Settings_UE::Get_HandleDebuggerBehavior())
            {
                case ECk_Ecs_HandleDebuggerBehavior::Disable:                      return FText::FromString(TEXT("Off"));
                case ECk_Ecs_HandleDebuggerBehavior::Enable:                       return FText::FromString(TEXT("On"));
                case ECk_Ecs_HandleDebuggerBehavior::EnableWithBlueprintDebugging: return FText::FromString(TEXT("On (BP)"));
                default:                                                             return FText::FromString(TEXT("---"));
            }
        }),
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_EcsDebugger)
    });

    InfoRowB.Add({
        FText::FromString(TEXT("EntityMap")),
        TAttribute<FText>::CreateLambda([]() -> FText
        {
            switch (UCk_Utils_Ecs_Settings_UE::Get_EntityMapPolicy())
            {
                case ECk_Ecs_EntityMap_Policy::DoNotLog:  return FText::FromString(TEXT("Off"));
                case ECk_Ecs_EntityMap_Policy::AlwaysLog: return FText::FromString(TEXT("Always Log"));
                default:                                   return FText::FromString(TEXT("---"));
            }
        }),
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_EcsEntityMap)
    });

    InfoRowB.Add({
        FText::FromString(TEXT("CS")),
        TAttribute<FText>::CreateLambda([]() -> FText
        {
            const bool bCpp = UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Cpp();
            const bool bBP  = UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Blueprint();
            const bool bAS  = UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Angelscript();
            if (!bCpp && !bBP && !bAS) { return FText::FromString(TEXT("---")); }
            FString Result;
            if (bCpp) { Result += TEXT("C++"); }
            if (bBP)  { Result += Result.IsEmpty() ? TEXT("BP") : TEXT(" BP"); }
            if (bAS)  { Result += Result.IsEmpty() ? TEXT("AS") : TEXT(" AS"); }
            return FText::FromString(Result);
        }),
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_EcsCallstacks)
    });

    // Row-level visibility — collapse each row if all its entries are disabled
    const auto VisInfoRowA = TAttribute<EVisibility>::CreateLambda([]() -> EVisibility
    {
        const bool bAny =
            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_CpuBrand()   ||
            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_OsVersion()   ||
            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_CpuCores()    ||
            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_NetworkType();
        return bAny ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
    });

    const auto VisInfoRowB = TAttribute<EVisibility>::CreateLambda([]() -> EVisibility
    {
        const bool bAny =
            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_EcsDebugger()  ||
            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_EcsEntityMap()  ||
            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_EcsCallstacks();
        return bAny ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
    });

    // ---- Build config label + color (compile-time label, runtime color from Project Settings) -
#if UE_BUILD_SHIPPING
    const FText BuildLabel(FText::FromString(TEXT("SHIPPING")));
    const TAttribute<FSlateColor> BuildColorAttr = TAttribute<FSlateColor>::CreateWeakLambda(this, []() -> FSlateColor
    {
        return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BuildConfig_Shipping_Color());
    });
#elif UE_BUILD_TEST
    const FText BuildLabel(FText::FromString(TEXT("TEST")));
    const TAttribute<FSlateColor> BuildColorAttr = TAttribute<FSlateColor>::CreateWeakLambda(this, []() -> FSlateColor
    {
        return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BuildConfig_Test_Color());
    });
#elif UE_BUILD_DEBUG
    const FText BuildLabel(FText::FromString(TEXT("DEBUG")));
    const TAttribute<FSlateColor> BuildColorAttr = TAttribute<FSlateColor>::CreateWeakLambda(this, []() -> FSlateColor
    {
        return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BuildConfig_Debug_Color());
    });
#else  // UE_BUILD_DEVELOPMENT
    const FText BuildLabel(FText::FromString(TEXT("DEV")));
    const TAttribute<FSlateColor> BuildColorAttr = TAttribute<FSlateColor>::CreateWeakLambda(this, []() -> FSlateColor
    {
        return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BuildConfig_Dev_Color());
    });
#endif

    // Row C — Build Config (always visible while watermark is not Hidden)
    TArray<FCkWatermarkInfoBarEntry> InfoRowC;
    FCkWatermarkInfoBarEntry BuildEntry;
    BuildEntry.Key   = FText::FromString(TEXT("Build"));
    BuildEntry.Value = TAttribute<FText>(BuildLabel);
    BuildEntry.ValueColorOverride = BuildColorAttr;
    InfoRowC.Add(MoveTemp(BuildEntry));

    // Build ID entries — one per reference branch baked in at compile time.
    // Entries where the merge-base equals HEAD are shown with the active color
    // (we are at that branch's tip). Diverged entries use the inactive color.
    // If HEAD does not match any branch a final "HEAD" entry is appended.
    {
        static const FString BakedHead(UTF8_TO_TCHAR(CkWatermarkBuildId::HeadHash));
        const auto BuildIdVis = MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_BuildId);

        const TArray<FString>& EnabledBranches =
            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BuildId_EnabledBranches();

        bool bHeadMatchesAny = false;
        for (int32 i = 0; i < CkWatermarkBuildId::BranchCount; ++i)
        {
            const FString BranchName(UTF8_TO_TCHAR(CkWatermarkBuildId::BranchNames[i]));
            if (!EnabledBranches.Contains(BranchName))
            {
                continue;
            }

            const bool bActive =
                BakedHead == FString(UTF8_TO_TCHAR(CkWatermarkBuildId::MergeBaseHashes[i]));
            if (bActive) { bHeadMatchesAny = true; }

            FCkWatermarkInfoBarEntry BranchEntry;
            BranchEntry.Key        = FText::FromString(BranchName);
            BranchEntry.Value      = TAttribute<FText>(FText::FromString(
                                         FString(UTF8_TO_TCHAR(CkWatermarkBuildId::MergeBaseHashes[i]))));
            BranchEntry.Visibility = BuildIdVis;
            BranchEntry.ValueColorOverride = TAttribute<FSlateColor>::CreateLambda([bActive]() -> FSlateColor
            {
                return FSlateColor(bActive
                    ? UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BuildId_Active_Color()
                    : UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BuildId_Inactive_Color());
            });
            InfoRowC.Add(MoveTemp(BranchEntry));
        }

        // On a feature branch, HEAD is ahead of all reference merge-bases —
        // show it explicitly so the current commit is always visible.
        if (!bHeadMatchesAny)
        {
            FCkWatermarkInfoBarEntry HeadEntry;
            HeadEntry.Key        = FText::FromString(TEXT("HEAD"));
            HeadEntry.Value      = TAttribute<FText>(FText::FromString(BakedHead));
            HeadEntry.Visibility = BuildIdVis;
            HeadEntry.ValueColorOverride = TAttribute<FSlateColor>::CreateLambda([]() -> FSlateColor
            {
                return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BuildId_Active_Color());
            });
            InfoRowC.Add(MoveTemp(HeadEntry));
        }
    }

    TSharedRef<SVerticalBox> InfoGroupBox = SNew(SVerticalBox)

        // Row A — Device Info
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, RowVPad)
        [
            SNew(SCkWatermarkInfoBar)
            .Entries(MoveTemp(InfoRowA))
            .Font(IBFont)
            .Separator(IBSep)
            .KeyValueSeparator(IBKVSep)
            .KeyColor(IBKeyCol)
            .ValueColor(IBValCol)
            .SeparatorColor(IBSepCol)
            .Visibility(VisInfoRowA)
        ]

        // Row B — ECS Debug
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, RowVPad)
        [
            SNew(SCkWatermarkInfoBar)
            .Entries(MoveTemp(InfoRowB))
            .Font(IBFont)
            .Separator(IBSep)
            .KeyValueSeparator(IBKVSep)
            .KeyColor(IBKeyCol)
            .ValueColor(IBValCol)
            .SeparatorColor(IBSepCol)
            .Visibility(VisInfoRowB)
        ]

        // Row C — Build Config
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, RowVPad)
        [
            SNew(SCkWatermarkInfoBar)
            .Entries(MoveTemp(InfoRowC))
            .Font(IBFont)
            .Separator(IBSep)
            .KeyValueSeparator(IBKVSep)
            .KeyColor(IBKeyCol)
            .ValueColor(IBValCol)
            .SeparatorColor(IBSepCol)
        ];

    // ---- Dynamic stats group (rows determined by Project Settings) ----------
    struct FStatEntry
    {
        TFunction<TSharedRef<SCkWatermarkStat>()> Factory;
        int32                                      Row;
        TOptional<TAttribute<EVisibility>>         VisOverride;
    };

    TArray<FStatEntry> StatEntries;
    StatEntries.Reserve(10);

    // Ensures
    StatEntries.Add({
        [&]() -> TSharedRef<SCkWatermarkStat>
        {
            return MakeStat(
                TAttribute<FText>::CreateWeakLambda(this, [this]() -> FText
                {
                    if (const UCk_Ensure_Subsystem_UE* Sub = GEngine
                            ? GEngine->GetEngineSubsystem<UCk_Ensure_Subsystem_UE>()
                            : nullptr)
                    {
                        return FText::AsNumber(Sub->Get_EnsureCount(),
                                               &FNumberFormattingOptions::DefaultNoGrouping());
                    }
                    return FText::FromString(TEXT("---"));
                }),
                TAttribute<FSlateColor>::CreateWeakLambda(this, [this]() -> FSlateColor
                {
                    if (const UCk_Ensure_Subsystem_UE* Sub = GEngine
                            ? GEngine->GetEngineSubsystem<UCk_Ensure_Subsystem_UE>()
                            : nullptr)
                    {
                        return FSlateColor(
                            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_EnsureCount_ColorBands()
                            .GetColorForValue(static_cast<float>(Sub->Get_EnsureCount())));
                    }
                    return FSlateColor(FLinearColor::White);
                }),
                FText::FromString(TEXT("Ensures")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_Ensures(),
        {}
    });

    // Unique Ensures
    StatEntries.Add({
        [&]() -> TSharedRef<SCkWatermarkStat>
        {
            return MakeStat(
                TAttribute<FText>::CreateWeakLambda(this, [this]() -> FText
                {
                    if (const UCk_Ensure_Subsystem_UE* Sub = GEngine
                            ? GEngine->GetEngineSubsystem<UCk_Ensure_Subsystem_UE>()
                            : nullptr)
                    {
                        return FText::AsNumber(Sub->Get_UniqueEnsureCount(),
                                               &FNumberFormattingOptions::DefaultNoGrouping());
                    }
                    return FText::FromString(TEXT("---"));
                }),
                TAttribute<FSlateColor>::CreateWeakLambda(this, [this]() -> FSlateColor
                {
                    if (const UCk_Ensure_Subsystem_UE* Sub = GEngine
                            ? GEngine->GetEngineSubsystem<UCk_Ensure_Subsystem_UE>()
                            : nullptr)
                    {
                        return FSlateColor(
                            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_EnsureCount_ColorBands()
                            .GetColorForValue(static_cast<float>(Sub->Get_UniqueEnsureCount())));
                    }
                    return FSlateColor(FLinearColor::White);
                }),
                FText::FromString(TEXT("Unique Ensures")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_UniqueEnsures(),
        {}
    });

    // RAM
    StatEntries.Add({
        [&]() -> TSharedRef<SCkWatermarkStat>
        {
            return MakeStat(
                TAttribute<FText>::CreateWeakLambda(this, [this, MakeOpts1dp]() -> FText
                {
                    const FCk_Utils_Memory_MemoryCountSnapshot_Result Snap =
                        UCk_Stats_Subsystem_UE::Get_MemoryCountSnapshot(this);
                    const FNumberFormattingOptions Opts = MakeOpts1dp();
                    return FText::Format(
                        FText::FromString(TEXT("{0}/{1} GB")),
                        FText::AsNumber(Snap.Get_PhysicalMemoryUsed(),  &Opts),
                        FText::AsNumber(Snap.Get_PhysicalMemoryTotal(), &Opts));
                }),
                UncoloredAttr,
                FText::FromString(TEXT("RAM")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_Ram(),
        {}
    });

    // VRAM
    StatEntries.Add({
        [&]() -> TSharedRef<SCkWatermarkStat>
        {
            return MakeStat(
                TAttribute<FText>::CreateWeakLambda(this, [this, MakeOpts1dp]() -> FText
                {
                    const float UsedGB =
                        UCk_Stats_Subsystem_UE::Get_MemoryCountSnapshot(this).Get_RHIMemoryUsed();
                    const FNumberFormattingOptions Opts = MakeOpts1dp();
                    return FText::Format(FText::FromString(TEXT("{0} GB")),
                                         FText::AsNumber(UsedGB, &Opts));
                }),
                UncoloredAttr,
                FText::FromString(TEXT("VRAM")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_Vram(),
        {}
    });

    // Memory Pressure (with SBox visibility toggle)
    StatEntries.Add({
        [&]() -> TSharedRef<SCkWatermarkStat>
        {
            return MakeStat(
                TAttribute<FText>::CreateWeakLambda(this, []() -> FText
                {
                    switch (FPlatformMemory::GetStats().GetMemoryPressureStatus())
                    {
                        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Nominal:  return FText::FromString(TEXT("Nominal"));
                        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Warning:  return FText::FromString(TEXT("Warning"));
                        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Critical: return FText::FromString(TEXT("Critical"));
                        default:                              return FText::FromString(TEXT("Unknown"));
                    }
                }),
                TAttribute<FSlateColor>::CreateWeakLambda(this, []() -> FSlateColor
                {
                    switch (FPlatformMemory::GetStats().GetMemoryPressureStatus())
                    {
                        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Nominal:  return FSlateColor(FLinearColor(0.2f, 1.0f, 0.3f, 1.f));
                        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Warning:  return FSlateColor(FLinearColor(1.0f, 0.85f, 0.0f, 1.f));
                        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Critical: return FSlateColor(FLinearColor(1.0f, 0.15f, 0.1f, 1.f));
                        default:                              return FSlateColor(FLinearColor(0.55f, 0.55f, 0.55f, 1.f));
                    }
                }),
                FText::FromString(TEXT("Mem Pressure")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_MemPressure(),
        TAttribute<EVisibility>::CreateLambda([]() -> EVisibility
        {
            return UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Show_MemoryPressure()
                ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
        })
    });

    // Ping
    StatEntries.Add({
        [&]() -> TSharedRef<SCkWatermarkStat>
        {
            return MakeStat(
                TAttribute<FText>::CreateWeakLambda(this, [this]() -> FText
                {
                    if (const UWorld* World = GetWorld())
                    {
                        if (const APlayerController* PC = World->GetFirstPlayerController())
                        {
                            if (PC->IsLocalController())
                            {
                                if (const APlayerState* PS = PC->GetPlayerState<APlayerState>())
                                {
                                    return FText::Format(
                                        FText::FromString(TEXT("{0} ms")),
                                        FText::AsNumber(
                                            static_cast<int32>(PS->GetPingInMilliseconds()),
                                            &FNumberFormattingOptions::DefaultNoGrouping()));
                                }
                            }
                        }
                    }
                    return FText::FromString(TEXT("---"));
                }),
                TAttribute<FSlateColor>::CreateWeakLambda(this, [this]() -> FSlateColor
                {
                    if (const UWorld* World = GetWorld())
                    {
                        if (const APlayerController* PC = World->GetFirstPlayerController())
                        {
                            if (PC->IsLocalController())
                            {
                                if (const APlayerState* PS = PC->GetPlayerState<APlayerState>())
                                {
                                    return FSlateColor(
                                        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Ping_ColorBands()
                                        .GetColorForValue(PS->GetPingInMilliseconds()));
                                }
                            }
                        }
                    }
                    return FSlateColor(FLinearColor::White);
                }),
                FText::FromString(TEXT("Ping")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_Ping(),
        {}
    });

    // Server FPS
    StatEntries.Add({
        [&]() -> TSharedRef<SCkWatermarkStat>
        {
            return MakeStat(
                TAttribute<FText>::CreateWeakLambda(this, [this]() -> FText
                {
                    if (const UWorld* World = GetWorld())
                    {
                        if (const ACk_GameState_UE* GS = World->GetGameState<ACk_GameState_UE>())
                        {
                            return FText::AsNumber(static_cast<int32>(GS->Get_ServerFPS()),
                                                   &FNumberFormattingOptions::DefaultNoGrouping());
                        }
                    }
                    return FText::FromString(TEXT("---"));
                }),
                TAttribute<FSlateColor>::CreateWeakLambda(this, [this]() -> FSlateColor
                {
                    if (const UWorld* World = GetWorld())
                    {
                        if (const ACk_GameState_UE* GS = World->GetGameState<ACk_GameState_UE>())
                        {
                            return FSlateColor(
                                UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_ServerFPS_ColorBands()
                                .GetColorForValue(GS->Get_ServerFPS()));
                        }
                    }
                    return FSlateColor(FLinearColor::White);
                }),
                FText::FromString(TEXT("Server FPS")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_ServerFps(),
        {}
    });

    // FPS
    StatEntries.Add({
        [&]() -> TSharedRef<SCkWatermarkStat>
        {
            return MakeStat(
                TAttribute<FText>::CreateWeakLambda(this, [this]() -> FText
                {
                    return FText::AsNumber(static_cast<int32>(GAverageFPS),
                                           &FNumberFormattingOptions::DefaultNoGrouping());
                }),
                TAttribute<FSlateColor>::CreateWeakLambda(this, []() -> FSlateColor
                {
                    return FSlateColor(
                        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_FPS_ColorBands()
                        .GetColorForValue(GAverageFPS));
                }),
                FText::FromString(TEXT("FPS")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_Fps(),
        {}
    });

    // Time
    StatEntries.Add({
        [&]() -> TSharedRef<SCkWatermarkStat>
        {
            return MakeStat(
                TAttribute<FText>::CreateWeakLambda(this, [this]() -> FText
                {
                    float TotalSec = 0.f;
                    if (const UWorld* World = GetWorld())
                    {
                        TotalSec = World->GetRealTimeSeconds();
                    }
                    const int32 T = FMath::FloorToInt(TotalSec);
                    const int32 H = T / 3600;
                    const int32 M = (T % 3600) / 60;
                    const int32 S = T % 60;
                    FString Str;
                    if (H > 0)
                        Str = FString::Printf(TEXT("%dh%dm%02ds"), H, M, S);
                    else if (M > 0)
                        Str = FString::Printf(TEXT("%dm%02ds"), M, S);
                    else
                        Str = FString::Printf(TEXT("%ds"), S);
                    return FText::FromString(Str);
                }),
                UncoloredAttr,
                FText::FromString(TEXT("Time")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_Time(),
        {}
    });

    // Frame
    StatEntries.Add({
        [&]() -> TSharedRef<SCkWatermarkStat>
        {
            return MakeStat(
                TAttribute<FText>::CreateWeakLambda(this, [this]() -> FText
                {
                    return FText::AsNumber(static_cast<int64>(GFrameCounter));
                }),
                UncoloredAttr,
                FText::FromString(TEXT("Frame")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_Frame(),
        {}
    });

    // Collect unique row indices, sort ascending, then emit one SHorizontalBox per row.
    TArray<int32> RowIndices;
    for (const FStatEntry& E : StatEntries)
    {
        RowIndices.AddUnique(E.Row);
    }
    RowIndices.Sort();

    TSharedRef<SVerticalBox> StatsGroupBox = SNew(SVerticalBox).Visibility(VisMain);
    for (const int32 RowIdx : RowIndices)
    {
        TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox);
        for (const FStatEntry& E : StatEntries)
        {
            if (E.Row != RowIdx) { continue; }
            if (E.VisOverride.IsSet())
            {
                HBox->AddSlot()
                    .AutoWidth()
                    .Padding(CellHPad, 0.f)
                    .HAlign(HAlign_Fill)
                    [
                        SNew(SBox)
                        .Visibility(E.VisOverride.GetValue())
                        [ E.Factory() ]
                    ];
            }
            else
            {
                HBox->AddSlot()
                    .AutoWidth()
                    .Padding(CellHPad, 0.f)
                    [ E.Factory() ];
            }
        }
        StatsGroupBox->AddSlot()
            .AutoHeight()
            .Padding(0.f, RowVPad)
            .HAlign(HAlign_Right)
            [ HBox ];
    }

    // ---- Full layout --------------------------------------------------------
    TSharedRef<SOverlay> Panel = SNew(SOverlay)
        // The watermark is a non-interactive debug overlay — never steal input.
        .Visibility(EVisibility::HitTestInvisible)

        // ── Stats group (bottom-right by default) ───────────────────────────
        + SOverlay::Slot()
        .HAlign(GetHAlign(_StatsGroupPlacement.Anchor))
        .VAlign(GetVAlign(_StatsGroupPlacement.Anchor))
        .Padding(_StatsGroupPlacement.EdgePadding)
        [
            StatsGroupBox
        ]

        // ── Info group (bottom-left by default, Regular + Detailed) ─────────
        + SOverlay::Slot()
        .HAlign(GetHAlign(_InfoGroupPlacement.Anchor))
        .VAlign(GetVAlign(_InfoGroupPlacement.Anchor))
        .Padding(_InfoGroupPlacement.EdgePadding)
        [
            SNew(SBox)
            .Visibility(VisMain)
            [
                InfoGroupBox
            ]
        ]

        // ── ECS groups (top-right by default, Detailed only) ────────────────
        + SOverlay::Slot()
        .HAlign(GetHAlign(_EcsGroupsPlacement.Anchor))
        .VAlign(GetVAlign(_EcsGroupsPlacement.Anchor))
        .Padding(_EcsGroupsPlacement.EdgePadding)
        [
            SNew(SBox)
            .Visibility(VisDetailed)
            [
                EcsBox
            ]
        ];

    _SlateRoot = Panel;
    return Panel;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermark_Panel_UWidget_UE::
    ReleaseSlateResources(bool bReleaseChildren)
    -> void
{
    Super::ReleaseSlateResources(bReleaseChildren);
    _SlateRoot.Reset();
}

auto
    UCkWatermark_Panel_UWidget_UE::
    SynchronizeProperties()
    -> void
{
    Super::SynchronizeProperties();
    // Placement, font, and ECS tag changes all affect the static Slate layout —
    // force a full rebuild so the new values take effect.
    TakeWidget();
}

#if WITH_EDITOR
auto
    UCkWatermark_Panel_UWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return FText::FromString(TEXT("CkFoundation|Watermark"));
}
#endif

// --------------------------------------------------------------------------------------------------------------------
