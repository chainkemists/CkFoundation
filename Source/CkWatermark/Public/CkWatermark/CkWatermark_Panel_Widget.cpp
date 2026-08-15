#include "CkWatermark_Panel_Widget.h"

#include "CkCore/Ensure/CkEnsure_Subsystem.h"
#include "CkCore/Engine/CkGameState.h"
#include "CkCore/Net/CkNetVersionSubsystem.h"
#include "CkCore/Net/CkNetVersionReport.h"
#include "CkCore/BuildId/CkBuildId.h"
#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Format/CkFormat.h"
#include "CkWatermark/Settings/CkWatermark_Settings.h"
#include "CkCore/Generated/CkCore_BuildId.h"
#include "CkWatermark/Subsystem/CkWatermark_Subsystem.h"
#include "CkMemory/CkMemory_Subsystem.h"
#include "CkEcs/Subsystem/CkEcsWorldStats_Subsystem.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcs/Settings/CkEcs_Settings.h"
#include "CkJolt/Subsystem/CkJolt_Subsystem.h"
#include "CkWatermark/Stats/CkWatermarkStat_Base_Widget.h"
#include "CkWatermark/CkWatermark_InfoBar_Widget.h"

#include <Engine/LocalPlayer.h>
#include <Fonts/SlateFontInfo.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>
#include <HAL/PlatformMemory.h>
#include <HAL/PlatformMisc.h>
#include <HAL/PlatformTime.h>
#include <Styling/CoreStyle.h>
#include <Widgets/Layout/SBox.h>
#include <Widgets/Text/STextBlock.h>

ENGINE_API extern float  GAverageFPS;
extern ENGINE_API uint64 GFrameCounter;

// ---- Replication CVAR externs (defined in CkWatermark_Subsystem.cpp) --------
namespace ck_watermark { namespace cvar {
    extern int32 ReplicationEnabled;
    extern float ReplicationFrequency;
} }

// --------------------------------------------------------------------------------------------------------------------

namespace ck_watermark_panel_widget
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
    // SynchronizeProperties() re-propagates this UMG-level property to Slate on every sync.
    SetVisibility(ESlateVisibility::HitTestInvisible);

    _StatsGroupPlacement.Anchor      = ECk_Watermark_GroupAnchor::BottomRight;
    _StatsGroupPlacement.EdgePadding = FMargin(0.f, 0.f, 8.f, 8.f);

    _EcsGroupsPlacement.Anchor      = ECk_Watermark_GroupAnchor::TopRight;
    _EcsGroupsPlacement.EdgePadding = FMargin(0.f, 8.f, 8.f, 0.f);

    _InfoGroupPlacement.Anchor      = ECk_Watermark_GroupAnchor::BottomLeft;
    _InfoGroupPlacement.EdgePadding = FMargin(8.f, 0.f, 0.f, 8.f);

    _CenterGroupPlacement.Anchor      = ECk_Watermark_GroupAnchor::BottomCenter;
    _CenterGroupPlacement.EdgePadding = FMargin(0.f, 0.f, 0.f, 8.f);
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
    // Typeface is read once at rebuild; outline size/color are re-read live inside each lambda.
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
    const TAttribute<FVector2D> ShadowOffsetAttr = TAttribute<FVector2D>::CreateLambda([]() -> FVector2D
    {
        return UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_TextShadowOffset();
    });
    const TAttribute<FLinearColor> ShadowColorAttr = TAttribute<FLinearColor>::CreateLambda([]() -> FLinearColor
    {
        return UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_TextShadowColor();
    });
    const float CellHPad = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_StatCell_HorizontalPadding();
    const float RowVPad  = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_VerticalPadding();

    // ---- Helper: one stat cell (uses standard value + bracket fonts) --------
    auto MakeStat = [ValueFont, LabelFont, BracketFont, BracketOpen, BracketClose,
                     InnerPad, LabelColor, ShadowOffsetAttr, ShadowColorAttr](
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
            .LabelColor(LabelColor)
            .ShadowOffset(ShadowOffsetAttr)
            .ShadowColorAndOpacity(ShadowColorAttr);
    };

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
    auto MakeVisForMinPolicy = [this](ECk_Watermark_DisplayPolicy InMinPolicy) -> TAttribute<EVisibility>
    {
        return TAttribute<EVisibility>::CreateWeakLambda(this, [this, InMinPolicy]() -> EVisibility
        {
            return (InMinPolicy != ECk_Watermark_DisplayPolicy::Hidden
                    && _CurrentDisplayPolicy >= InMinPolicy)
                ? EVisibility::SelfHitTestInvisible
                : EVisibility::Collapsed;
        });
    };

    const auto VisMinimal  = MakeVisForMinPolicy(ECk_Watermark_DisplayPolicy::Minimal);
    const auto VisMain     = MakeVisForMinPolicy(ECk_Watermark_DisplayPolicy::Regular);
    const auto VisDetailed = MakeVisForMinPolicy(ECk_Watermark_DisplayPolicy::Detailed);

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
                        if (!Tag.IsValid())
                        { return FText::FromString(TEXT("---")); }
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

    auto MakeInfoVis = [this](ECk_Watermark_DisplayPolicy (*Getter)()) -> TAttribute<EVisibility>
    {
        return TAttribute<EVisibility>::CreateWeakLambda(this, [this, Getter]() -> EVisibility
        {
            const auto MinPolicy = Getter();
            return (MinPolicy != ECk_Watermark_DisplayPolicy::Hidden
                    && _CurrentDisplayPolicy >= MinPolicy)
                ? EVisibility::SelfHitTestInvisible
                : EVisibility::Collapsed;
        });
    };

    TArray<FCkWatermarkInfoBarEntry> DeviceInfoRow;

    DeviceInfoRow.Add({
        FText::FromString(TEXT("CPU")),
        TAttribute<FText>::CreateLambda([]() -> FText
        {
            static const FString Brand = FPlatformMisc::GetCPUBrand().TrimStartAndEnd();
            return FText::FromString(Brand.IsEmpty() ? TEXT("---") : Brand);
        }),
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_CpuBrand)
    });

    DeviceInfoRow.Add({
        FText::FromString(TEXT("OS")),
        TAttribute<FText>::CreateLambda([]() -> FText
        {
            static const FString Ver = FPlatformMisc::GetOSVersion();
            return FText::FromString(Ver.IsEmpty() ? TEXT("---") : Ver);
        }),
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_OsVersion)
    });

    DeviceInfoRow.Add({
        FText::FromString(TEXT("Cores")),
        TAttribute<FText>::CreateLambda([]() -> FText
        {
            static const int32 Physical = FPlatformMisc::NumberOfCores();
            static const int32 Logical  = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
            return FText::FromString(FString::Printf(TEXT("%dc / %dt"), Physical, Logical));
        }),
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_CpuCores)
    });

    DeviceInfoRow.Add({
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
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_NetworkType)
    });

    TArray<FCkWatermarkInfoBarEntry> RoleEcsAndJoltRow;
    {
        FCkWatermarkInfoBarEntry RoleEntry;
        RoleEntry.Key = FText::FromString(TEXT("Role"));
        RoleEntry.Value = TAttribute<FText>::CreateWeakLambda(this, [this]() -> FText
        {
            if (const UWorld* World = GetWorld())
            {
                switch (World->GetNetMode())
                {
                    case NM_Standalone:      return FText::FromString(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_NetMode_Label_SinglePlayer());
                    case NM_DedicatedServer: return FText::FromString(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_NetMode_Label_Server());
                    case NM_ListenServer:    return FText::FromString(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_NetMode_Label_ListenServer());
                    case NM_Client:          return FText::FromString(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_NetMode_Label_Client());
                    default:                 return FText::FromString(TEXT("---"));
                }
            }
            return FText::FromString(TEXT("---"));
        });
        RoleEntry.ValueColorOverride = TAttribute<FSlateColor>::CreateWeakLambda(this, [this]() -> FSlateColor
        {
            if (const UWorld* World = GetWorld())
            {
                switch (World->GetNetMode())
                {
                    case NM_Standalone:      return FSlateColor(FLinearColor(0.2f, 1.0f, 0.4f, 1.f)); // Green
                    case NM_DedicatedServer: return FSlateColor(FLinearColor(1.0f, 0.2f, 0.2f, 1.f)); // Red
                    case NM_ListenServer:    return FSlateColor(FLinearColor(1.0f, 0.6f, 0.1f, 1.f)); // Orange
                    case NM_Client:          return FSlateColor(FLinearColor(0.3f, 0.8f, 1.0f, 1.f)); // Cyan
                    default:                 break;
                }
            }
            return FSlateColor(FLinearColor(0.55f, 0.55f, 0.55f, 1.f));
        });
        RoleEntry.Visibility = MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_NetMode);
        RoleEcsAndJoltRow.Add(MoveTemp(RoleEntry));
    }

    RoleEcsAndJoltRow.Add({
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
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_EcsDebugger)
    });

    RoleEcsAndJoltRow.Add({
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
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_EcsEntityMap)
    });

    RoleEcsAndJoltRow.Add({
        FText::FromString(TEXT("CS")),
        TAttribute<FText>::CreateLambda([]() -> FText
        {
            const bool bCpp = UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Cpp();
            const bool bBP  = UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Blueprint();
            const bool bAS  = UCk_Utils_Ecs_Settings_UE::Get_CaptureCallstack_Angelscript();
            if (!bCpp && !bBP && !bAS)
            { return FText::FromString(TEXT("---")); }
            FString Result;
            if (bCpp)
            { Result += TEXT("C++"); }
            if (bBP)
            { Result += Result.IsEmpty() ? TEXT("BP") : TEXT(" BP"); }
            if (bAS)
            { Result += Result.IsEmpty() ? TEXT("AS") : TEXT(" AS"); }
            return FText::FromString(Result);
        }),
        MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_EcsCallstacks)
    });

    {
        FCkWatermarkInfoBarEntry JoltEntry;
        JoltEntry.Key = FText::FromString(TEXT("Jolt"));
        JoltEntry.Value = TAttribute<FText>::CreateWeakLambda(this, [this]() -> FText
        {
            if (const UWorld* World = GetWorld())
            {
                if (const auto* Sub = World->GetSubsystem<UCk_Jolt_Subsystem>())
                {
                    FString Result = Sub->Get_ParallelPhysicsEnabled()
                        ? ck::Format_UE(TEXT("MT({}t)"), Sub->Get_PhysicsThreadCount())
                        : FString(TEXT("ST"));

                    if (Sub->Get_AsyncPhysicsUpdate())
                    { Result += TEXT(" Async"); }

                    return FText::FromString(Result);
                }
            }
            return FText::FromString(TEXT("---"));
        });
        JoltEntry.ValueColorOverride = TAttribute<FSlateColor>::CreateWeakLambda(this, [this]() -> FSlateColor
        {
            if (const UWorld* World = GetWorld())
            {
                if (const auto* Sub = World->GetSubsystem<UCk_Jolt_Subsystem>())
                {
                    if (Sub->Get_ParallelPhysicsEnabled() && Sub->Get_AsyncPhysicsUpdate())
                    { return FSlateColor(FLinearColor(0.2f, 1.0f, 0.4f, 1.f)); }
                }
            }
            return FSlateColor(FLinearColor::White);
        });
        JoltEntry.Visibility = MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Jolt);
        RoleEcsAndJoltRow.Add(MoveTemp(JoltEntry));
    }

    auto PolicyMet = [](ECk_Watermark_DisplayPolicy P, ECk_Watermark_DisplayPolicy MinPolicy) -> bool
    {
        return MinPolicy != ECk_Watermark_DisplayPolicy::Hidden && P >= MinPolicy;
    };

    const auto VisDeviceInfoRow = TAttribute<EVisibility>::CreateWeakLambda(this, [this, PolicyMet]() -> EVisibility
    {
        const auto P = _CurrentDisplayPolicy;
        const bool bAny =
            PolicyMet(P, UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_CpuBrand())    ||
            PolicyMet(P, UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_OsVersion())   ||
            PolicyMet(P, UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_CpuCores())    ||
            PolicyMet(P, UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_NetworkType());
        return bAny ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
    });

    const auto VisRoleEcsAndJoltRow = TAttribute<EVisibility>::CreateWeakLambda(this, [this, PolicyMet]() -> EVisibility
    {
        const auto P = _CurrentDisplayPolicy;
        const bool bAny =
            PolicyMet(P, UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Jolt())          ||
            PolicyMet(P, UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_NetMode())       ||
            PolicyMet(P, UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_EcsDebugger())   ||
            PolicyMet(P, UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_EcsEntityMap())  ||
            PolicyMet(P, UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_EcsCallstacks());
        return bAny ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
    });

    const auto VisBuildInfoRow = TAttribute<EVisibility>::CreateWeakLambda(this, [this, PolicyMet]() -> EVisibility
    {
        const auto P = _CurrentDisplayPolicy;
        const bool bAny =
            PolicyMet(P, UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_BuildId());
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

    const auto BuildIdVis = MakeInfoVis(&UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_BuildId);

    TArray<FCkWatermarkInfoBarEntry> BuildInfoRow;
    FCkWatermarkInfoBarEntry BuildEntry;
    BuildEntry.Key   = FText::FromString(TEXT("Build"));
    BuildEntry.Value = TAttribute<FText>(BuildLabel);
    BuildEntry.Visibility = BuildIdVis;
    BuildEntry.ValueColorOverride = BuildColorAttr;
    BuildInfoRow.Add(MoveTemp(BuildEntry));

    {
        static const FString BakedHead(UTF8_TO_TCHAR(CkCoreBuildId::HeadHash));

        const TArray<FString>& EnabledBranches =
            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BuildId_EnabledBranches();

        bool HeadMatchesAny = false;
        for (int32 i = 0; i < CkCoreBuildId::BranchCount; ++i)
        {
            const FString BranchName(UTF8_TO_TCHAR(CkCoreBuildId::BranchNames[i]));
            if (!EnabledBranches.Contains(BranchName))
            {
                continue;
            }

            const bool IsActive =
                BakedHead == FString(UTF8_TO_TCHAR(CkCoreBuildId::MergeBaseHashes[i]));
            if (IsActive)
            { HeadMatchesAny = true; }

            FCkWatermarkInfoBarEntry BranchEntry;
            BranchEntry.Key        = FText::FromString(BranchName);
            BranchEntry.Value      = TAttribute<FText>(FText::FromString(
                                         FString(UTF8_TO_TCHAR(CkCoreBuildId::MergeBaseHashes[i]))));
            BranchEntry.Visibility = BuildIdVis;
            BranchEntry.ValueColorOverride = TAttribute<FSlateColor>::CreateLambda([IsActive]() -> FSlateColor
            {
                return FSlateColor(IsActive
                    ? UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BuildId_Active_Color()
                    : UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BuildId_Inactive_Color());
            });
            BuildInfoRow.Add(MoveTemp(BranchEntry));
        }

        // On a feature branch HEAD is ahead of every reference merge-base, so nothing else shows it.
        if (!HeadMatchesAny)
        {
            FCkWatermarkInfoBarEntry HeadEntry;
            HeadEntry.Key        = FText::FromString(TEXT("HEAD"));
            HeadEntry.Value      = TAttribute<FText>(FText::FromString(BakedHead));
            HeadEntry.Visibility = BuildIdVis;
            HeadEntry.ValueColorOverride = TAttribute<FSlateColor>::CreateLambda([]() -> FSlateColor
            {
                return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_BuildId_Active_Color());
            });
            BuildInfoRow.Add(MoveTemp(HeadEntry));
        }
    }

    TArray<FCkWatermarkInfoBarEntry> CustomFieldRow;
    {
        TWeakObjectPtr<const UCkWatermark_Panel_UWidget_UE> WeakSelf(this);

        auto GetSubsystem = [WeakSelf]() -> const UCk_Watermark_Subsystem_UE*
        {
            if (!WeakSelf.IsValid())
            { return nullptr; }
            const auto* LP = WeakSelf->GetTypedOuter<ULocalPlayer>();
            return LP ? LP->GetSubsystem<UCk_Watermark_Subsystem_UE>() : nullptr;
        };

        FCkWatermarkInfoBarEntry CustomEntry;

        CustomEntry.Key = TAttribute<FText>::CreateWeakLambda(this,
            [GetSubsystem]() -> FText
            {
                if (const auto* Sub = GetSubsystem();
                    Sub && Sub->Get_IsCustomFieldSet())
                { return Sub->Get_CustomFieldKey(); }
                return FText::GetEmpty();
            });

        CustomEntry.Value = TAttribute<FText>::CreateWeakLambda(this,
            [GetSubsystem]() -> FText
            {
                if (const auto* Sub = GetSubsystem();
                    Sub && Sub->Get_IsCustomFieldSet())
                { return Sub->Get_CustomFieldValue(); }
                return FText::GetEmpty();
            });

        const auto CustomFieldMinPolicy =
            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_CustomField();
        CustomEntry.Visibility = TAttribute<EVisibility>::CreateWeakLambda(this,
            [this, GetSubsystem, CustomFieldMinPolicy]() -> EVisibility
            {
                const auto* Sub = GetSubsystem();
                return (Sub
                        && Sub->Get_IsCustomFieldSet()
                        && CustomFieldMinPolicy != ECk_Watermark_DisplayPolicy::Hidden
                        && _CurrentDisplayPolicy >= CustomFieldMinPolicy)
                    ? EVisibility::SelfHitTestInvisible
                    : EVisibility::Collapsed;
            });

        CustomEntry.KeyColorOverride = TAttribute<FSlateColor>::CreateWeakLambda(this,
            [GetSubsystem]() -> FSlateColor
            {
                if (const auto* Sub = GetSubsystem())
                {
                    if (const auto& Override = Sub->Get_CustomFieldKeyColorOverride();
                        Override.IsSet())
                    { return Override.GetValue(); }
                }
                return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE
                    ::Get_Watermark_CustomField_DefaultKeyColor());
            });

        CustomEntry.ValueColorOverride = TAttribute<FSlateColor>::CreateWeakLambda(this,
            [GetSubsystem]() -> FSlateColor
            {
                if (const auto* Sub = GetSubsystem())
                {
                    if (const auto& Override = Sub->Get_CustomFieldValueColorOverride();
                        Override.IsSet())
                    { return Override.GetValue(); }
                }
                return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE
                    ::Get_Watermark_CustomField_DefaultValueColor());
            });

        CustomFieldRow.Add(MoveTemp(CustomEntry));
    }

    TSharedRef<SVerticalBox> InfoGroupBox = SNew(SVerticalBox)

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, RowVPad)
        [
            SNew(SCkWatermarkInfoBar)
            .Entries(MoveTemp(DeviceInfoRow))
            .Font(IBFont)
            .Separator(IBSep)
            .KeyValueSeparator(IBKVSep)
            .KeyColor(IBKeyCol)
            .ValueColor(IBValCol)
            .SeparatorColor(IBSepCol)
            .ShadowOffset(ShadowOffsetAttr)
            .ShadowColorAndOpacity(ShadowColorAttr)
            .Visibility(VisDeviceInfoRow)
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, RowVPad)
        [
            SNew(SCkWatermarkInfoBar)
            .Entries(MoveTemp(RoleEcsAndJoltRow))
            .Font(IBFont)
            .Separator(IBSep)
            .KeyValueSeparator(IBKVSep)
            .KeyColor(IBKeyCol)
            .ValueColor(IBValCol)
            .SeparatorColor(IBSepCol)
            .ShadowOffset(ShadowOffsetAttr)
            .ShadowColorAndOpacity(ShadowColorAttr)
            .Visibility(VisRoleEcsAndJoltRow)
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, RowVPad)
        [
            SNew(SCkWatermarkInfoBar)
            .Entries(MoveTemp(BuildInfoRow))
            .Font(IBFont)
            .Separator(IBSep)
            .KeyValueSeparator(IBKVSep)
            .KeyColor(IBKeyCol)
            .ValueColor(IBValCol)
            .SeparatorColor(IBSepCol)
            .ShadowOffset(ShadowOffsetAttr)
            .ShadowColorAndOpacity(ShadowColorAttr)
            .Visibility(VisBuildInfoRow)
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, RowVPad)
        [
            SNew(SCkWatermarkInfoBar)
            .Entries(MoveTemp(CustomFieldRow))
            .Font(IBFont)
            .Separator(IBSep)
            .KeyValueSeparator(IBKVSep)
            .KeyColor(IBKeyCol)
            .ValueColor(IBValCol)
            .SeparatorColor(IBSepCol)
            .ShadowOffset(ShadowOffsetAttr)
            .ShadowColorAndOpacity(ShadowColorAttr)
        ];

    // ---- Dynamic stats group (rows determined by Project Settings) ----------
    struct FStatEntry
    {
        TFunction<TSharedRef<SCkWatermarkStat>()> Factory;
        int32                                      Row;
        ECk_Watermark_DisplayPolicy                MinPolicy;
        TOptional<TAttribute<EVisibility>>         VisOverride;
    };

    TArray<FStatEntry> StatEntries;
    StatEntries.Reserve(10);

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
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Ensures(),
        {}
    });

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
                            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_UniqueEnsureCount_ColorBands()
                            .GetColorForValue(static_cast<float>(Sub->Get_UniqueEnsureCount())));
                    }
                    return FSlateColor(FLinearColor::White);
                }),
                FText::FromString(TEXT("Unique Ensures")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_UniqueEnsures(),
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_UniqueEnsures(),
        {}
    });

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
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Ram(),
        {}
    });

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
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Vram(),
        {}
    });

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
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_MemoryPressure(),
        {}
    });

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
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Ping(),
        {}
    });

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
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_ServerFps(),
        {}
    });

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
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Fps(),
        {}
    });

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
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Time(),
        {}
    });

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
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Frame(),
        {}
    });

    // ---- Replication stats (CVAR-gated) -----------------------------------------
    const auto RepVisOverride = TAttribute<EVisibility>::CreateLambda([]() -> EVisibility
    {
        return ck_watermark::cvar::ReplicationEnabled
            ? EVisibility::SelfHitTestInvisible
            : EVisibility::Collapsed;
    });

    StatEntries.Add({
        [&]() -> TSharedRef<SCkWatermarkStat>
        {
            return MakeStat(
                TAttribute<FText>::CreateWeakLambda(this, [this]() -> FText
                {
                    DoRefreshReplicationCacheIfNeeded();
                    return FText::AsNumber(_CachedRepActorCount,
                                           &FNumberFormattingOptions::DefaultNoGrouping());
                }),
                UncoloredAttr,
                FText::FromString(TEXT("Rep Actors")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_RepActors(),
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_RepActors(),
        RepVisOverride
    });

    StatEntries.Add({
        [&]() -> TSharedRef<SCkWatermarkStat>
        {
            return MakeStat(
                TAttribute<FText>::CreateWeakLambda(this, [this]() -> FText
                {
                    DoRefreshReplicationCacheIfNeeded();
                    return FText::AsNumber(_CachedRepComponentCount,
                                           &FNumberFormattingOptions::DefaultNoGrouping());
                }),
                UncoloredAttr,
                FText::FromString(TEXT("Rep Comps")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_RepComponents(),
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_RepComponents(),
        RepVisOverride
    });

    StatEntries.Add({
        [&]() -> TSharedRef<SCkWatermarkStat>
        {
            return MakeStat(
                TAttribute<FText>::CreateWeakLambda(this, [this]() -> FText
                {
                    DoRefreshReplicationCacheIfNeeded();
                    return FText::AsNumber(_CachedRepObjectCount,
                                           &FNumberFormattingOptions::DefaultNoGrouping());
                }),
                UncoloredAttr,
                FText::FromString(TEXT("Rep Objects")));
        },
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Row_RepObjects(),
        UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_RepObjects(),
        RepVisOverride
    });

    TArray<int32> RowIndices;
    for (const FStatEntry& E : StatEntries)
    {
        RowIndices.AddUnique(E.Row);
    }
    RowIndices.Sort();

    TSharedRef<SVerticalBox> StatsGroupBox = SNew(SVerticalBox).Visibility(VisMinimal);
    for (const int32 RowIdx : RowIndices)
    {
        TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox);
        for (const FStatEntry& E : StatEntries)
        {
            if (E.Row != RowIdx)
            { continue; }

            const TAttribute<EVisibility> PolicyVis = MakeVisForMinPolicy(E.MinPolicy);

            if (E.VisOverride.IsSet())
            {
                HBox->AddSlot()
                    .AutoWidth()
                    .Padding(CellHPad, 0.f)
                    .HAlign(HAlign_Fill)
                    [
                        SNew(SBox)
                        .Visibility(PolicyVis)
                        [
                            SNew(SBox)
                            .Visibility(E.VisOverride.GetValue())
                            [ E.Factory() ]
                        ]
                    ];
            }
            else
            {
                HBox->AddSlot()
                    .AutoWidth()
                    .Padding(CellHPad, 0.f)
                    [
                        SNew(SBox)
                        .Visibility(PolicyVis)
                        [ E.Factory() ]
                    ];
            }
        }
        StatsGroupBox->AddSlot()
            .AutoHeight()
            .Padding(0.f, RowVPad)
            .HAlign(HAlign_Right)
            [ HBox ];
    }

    // ---- Center group (bottom-center) — ECS pump pressure -------------------
    const auto PumpMinPolicy = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_PumpCount();

    // {0, 0} when there is no ECS world (e.g. front-end menu) — the stat then renders "---".
    auto QueryPumps = [](const UWorld* InWorld) -> TPair<int32, int32>
    {
        if (!InWorld)
        { return {0, 0}; }
        const auto* Sub = InWorld->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
        if (!Sub)
        { return {0, 0}; }
        return {Sub->Get_WorstFramePumpCount(), Sub->Get_MaxPumpIterations()};
    };

    const auto PumpWarnColorAttr = TAttribute<FSlateColor>::CreateLambda([]() -> FSlateColor
    {
        return FSlateColor(UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_PumpLimit_WarningColor());
    });

    const auto PumpWarnVis = TAttribute<EVisibility>::CreateWeakLambda(this,
        [this, QueryPumps, PumpMinPolicy]() -> EVisibility
        {
            if (PumpMinPolicy == ECk_Watermark_DisplayPolicy::Hidden || _CurrentDisplayPolicy < PumpMinPolicy)
            { return EVisibility::Collapsed; }
            const auto Info = QueryPumps(GetWorld());
            return (Info.Value > 0 && Info.Key >= Info.Value)
                ? EVisibility::SelfHitTestInvisible
                : EVisibility::Collapsed;
        });

    TSharedRef<SVerticalBox> CenterGroupBox = SNew(SVerticalBox)

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, RowVPad)
        .HAlign(HAlign_Center)
        [
            SNew(SBox)
            .Visibility(MakeVisForMinPolicy(PumpMinPolicy))
            [
                MakeStat(
                    TAttribute<FText>::CreateWeakLambda(this, [this, QueryPumps]() -> FText
                    {
                        const auto Info = QueryPumps(GetWorld());
                        if (Info.Value <= 0)
                        { return FText::FromString(TEXT("---")); }
                        return FText::FromString(FString::Printf(TEXT("%d / %d"), Info.Key, Info.Value));
                    }),
                    TAttribute<FSlateColor>::CreateWeakLambda(this, [this, QueryPumps]() -> FSlateColor
                    {
                        const auto Info = QueryPumps(GetWorld());
                        return FSlateColor(
                            UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_PumpCount_ColorBands()
                            .GetColorForValue(static_cast<float>(Info.Key)));
                    }),
                    FText::FromString(TEXT("Pumps")))
            ]
        ]

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.f, RowVPad)
        .HAlign(HAlign_Center)
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("PUMP LIMIT EXCEEDED")))
            .Font(ValueFont)
            .ColorAndOpacity(PumpWarnColorAttr)
            .ShadowOffset(ShadowOffsetAttr)
            .ShadowColorAndOpacity(ShadowColorAttr)
            .Visibility(PumpWarnVis)
        ];

    // ---- Connection rows (Server/Client build-version) — appended to the center group ----
    const auto ConnMinPolicy  = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Connection();
    const int32 MaxClientRows = UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Connection_MaxClientRows();

    const auto GetVersionSubsystem = [](const UWorld* InWorld) -> const UCk_NetVersion_WorldSubsystem_UE*
    {
        return InWorld ? InWorld->GetSubsystem<UCk_NetVersion_WorldSubsystem_UE>() : nullptr;
    };

    const auto PendingColor = FSlateColor(FLinearColor(0.55f, 0.55f, 0.55f, 1.f));

    CenterGroupBox->AddSlot()
        .AutoHeight()
        .Padding(0.f, RowVPad)
        .HAlign(HAlign_Center)
        [
            SNew(STextBlock)
            .Font(ValueFont)
            .ShadowOffset(ShadowOffsetAttr)
            .ShadowColorAndOpacity(ShadowColorAttr)
            .Text(TAttribute<FText>::CreateWeakLambda(this, [this, GetVersionSubsystem]() -> FText
            {
                const auto* Subsystem = GetVersionSubsystem(GetWorld());
                const FString ServerId = Subsystem ? Subsystem->Get_LocalServerBuildId() : FString();
                if (ServerId.IsEmpty())
                { return FText::FromString(TEXT("Server: ---")); }
                const bool Match = ServerId == ck::Get_BuildId();
                return FText::FromString(FString::Printf(TEXT("Server %s  %s"),
                    *ServerId, Match ? TEXT("[OK]") : TEXT("[VERSION MISMATCH]")));
            }))
            .ColorAndOpacity(TAttribute<FSlateColor>::CreateWeakLambda(this, [this, GetVersionSubsystem, PendingColor]() -> FSlateColor
            {
                const auto* Subsystem = GetVersionSubsystem(GetWorld());
                const FString ServerId = Subsystem ? Subsystem->Get_LocalServerBuildId() : FString();
                if (ServerId.IsEmpty())
                { return PendingColor; }
                return FSlateColor(ServerId == ck::Get_BuildId()
                    ? UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Connection_OkColor()
                    : UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Connection_MismatchColor());
            }))
            .Visibility(TAttribute<EVisibility>::CreateWeakLambda(this, [this, ConnMinPolicy]() -> EVisibility
            {
                if (ConnMinPolicy == ECk_Watermark_DisplayPolicy::Hidden || _CurrentDisplayPolicy < ConnMinPolicy)
                { return EVisibility::Collapsed; }
                const auto* World = GetWorld();
                return (World && World->GetNetMode() == NM_Client)
                    ? EVisibility::SelfHitTestInvisible
                    : EVisibility::Collapsed;
            }))
        ];

    CenterGroupBox->AddSlot()
        .AutoHeight()
        .Padding(0.f, RowVPad)
        .HAlign(HAlign_Center)
        [
            SNew(STextBlock)
            .Font(ValueFont)
            .ColorAndOpacity(LabelColor)
            .ShadowOffset(ShadowOffsetAttr)
            .ShadowColorAndOpacity(ShadowColorAttr)
            .Text(TAttribute<FText>::CreateWeakLambda(this, [this, GetVersionSubsystem]() -> FText
            {
                const auto* Subsystem = GetVersionSubsystem(GetWorld());
                const int32 Num = Subsystem ? Subsystem->Get_RemoteClientReports().Num() : 0;
                return FText::FromString(FString::Printf(TEXT("Clients (%d)"), Num));
            }))
            .Visibility(TAttribute<EVisibility>::CreateWeakLambda(this, [this, ConnMinPolicy]() -> EVisibility
            {
                if (ConnMinPolicy == ECk_Watermark_DisplayPolicy::Hidden || _CurrentDisplayPolicy < ConnMinPolicy)
                { return EVisibility::Collapsed; }
                const auto* World = GetWorld();
                return (World && World->GetNetMode() == NM_ListenServer)
                    ? EVisibility::SelfHitTestInvisible
                    : EVisibility::Collapsed;
            }))
        ];

    for (int32 RowIdx = 0; RowIdx < MaxClientRows; ++RowIdx)
    {
        CenterGroupBox->AddSlot()
            .AutoHeight()
            .Padding(0.f, RowVPad)
            .HAlign(HAlign_Center)
            [
                SNew(STextBlock)
                .Font(ValueFont)
                .ShadowOffset(ShadowOffsetAttr)
                .ShadowColorAndOpacity(ShadowColorAttr)
                .Text(TAttribute<FText>::CreateWeakLambda(this, [this, GetVersionSubsystem, RowIdx]() -> FText
                {
                    const auto* Subsystem = GetVersionSubsystem(GetWorld());
                    if (!Subsystem)
                    { return FText::GetEmpty(); }
                    const auto Reports = Subsystem->Get_RemoteClientReports();
                    if (!Reports.IsValidIndex(RowIdx))
                    { return FText::GetEmpty(); }
                    const auto* Report = Reports[RowIdx];
                    const auto* PC = Cast<APlayerController>(Report->GetOwner());
                    const FString Name = (PC && PC->PlayerState) ? PC->PlayerState->GetPlayerName() : FString(TEXT("Player"));
                    const FString Id = Report->Get_ClientBuildId();
                    if (Id.IsEmpty())
                    { return FText::FromString(FString::Printf(TEXT("%s  ---"), *Name)); }
                    const bool Match = Id == ck::Get_BuildId();
                    return FText::FromString(FString::Printf(TEXT("%s  %s  %s"),
                        *Name, *Id, Match ? TEXT("[OK]") : TEXT("[MISMATCH]")));
                }))
                .ColorAndOpacity(TAttribute<FSlateColor>::CreateWeakLambda(this, [this, GetVersionSubsystem, RowIdx, PendingColor]() -> FSlateColor
                {
                    const auto* Subsystem = GetVersionSubsystem(GetWorld());
                    if (!Subsystem)
                    { return FSlateColor(FLinearColor::White); }
                    const auto Reports = Subsystem->Get_RemoteClientReports();
                    if (!Reports.IsValidIndex(RowIdx))
                    { return FSlateColor(FLinearColor::White); }
                    const FString Id = Reports[RowIdx]->Get_ClientBuildId();
                    if (Id.IsEmpty())
                    { return PendingColor; }
                    return FSlateColor(Id == ck::Get_BuildId()
                        ? UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Connection_OkColor()
                        : UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Connection_MismatchColor());
                }))
                .Visibility(TAttribute<EVisibility>::CreateWeakLambda(this, [this, GetVersionSubsystem, RowIdx, ConnMinPolicy]() -> EVisibility
                {
                    if (ConnMinPolicy == ECk_Watermark_DisplayPolicy::Hidden || _CurrentDisplayPolicy < ConnMinPolicy)
                    { return EVisibility::Collapsed; }
                    const auto* World = GetWorld();
                    if (!(World && World->GetNetMode() == NM_ListenServer))
                    { return EVisibility::Collapsed; }
                    const auto* Subsystem = GetVersionSubsystem(World);
                    return (Subsystem && Subsystem->Get_RemoteClientReports().IsValidIndex(RowIdx))
                        ? EVisibility::SelfHitTestInvisible
                        : EVisibility::Collapsed;
                }))
            ];
    }

    // ---- Full layout --------------------------------------------------------
    TSharedRef<SOverlay> Panel = SNew(SOverlay)
        .Visibility(EVisibility::HitTestInvisible)

        + SOverlay::Slot()
        .HAlign(ck_watermark_panel_widget::GetHAlign(_StatsGroupPlacement.Anchor))
        .VAlign(ck_watermark_panel_widget::GetVAlign(_StatsGroupPlacement.Anchor))
        .Padding(_StatsGroupPlacement.EdgePadding)
        [
            StatsGroupBox
        ]

        + SOverlay::Slot()
        .HAlign(ck_watermark_panel_widget::GetHAlign(_InfoGroupPlacement.Anchor))
        .VAlign(ck_watermark_panel_widget::GetVAlign(_InfoGroupPlacement.Anchor))
        .Padding(_InfoGroupPlacement.EdgePadding)
        [
            SNew(SBox)
            .Visibility(VisMinimal)
            [
                InfoGroupBox
            ]
        ]

        // Per-element visibility (MinPolicy + over-budget) is handled inside CenterGroupBox.
        + SOverlay::Slot()
        .HAlign(ck_watermark_panel_widget::GetHAlign(_CenterGroupPlacement.Anchor))
        .VAlign(ck_watermark_panel_widget::GetVAlign(_CenterGroupPlacement.Anchor))
        .Padding(_CenterGroupPlacement.EdgePadding)
        [
            CenterGroupBox
        ]

        + SOverlay::Slot()
        .HAlign(ck_watermark_panel_widget::GetHAlign(_EcsGroupsPlacement.Anchor))
        .VAlign(ck_watermark_panel_widget::GetVAlign(_EcsGroupsPlacement.Anchor))
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
    DoRefreshReplicationCacheIfNeeded() const
    -> void
{
    if (!ck_watermark::cvar::ReplicationEnabled)
    { return; }

    const double Now = FPlatformTime::Seconds();

    if (Now - _LastRepRefreshTime < static_cast<double>(ck_watermark::cvar::ReplicationFrequency))
    { return; }

    _LastRepRefreshTime      = Now;
    _CachedRepActorCount     = UCk_Utils_Actor_UE::Get_AllReplicatedActors(this).Num();
    _CachedRepComponentCount = UCk_Utils_Actor_UE::Get_AllReplicatedComponents(this).Num();
    _CachedRepObjectCount    = UCk_Utils_Actor_UE::Get_AllReplicatedObjects(this).Num();
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
    // Placement, font, and ECS tag changes all affect the static Slate layout — force a full rebuild.
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
