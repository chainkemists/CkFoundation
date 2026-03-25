#include "CkHfsmViewer_GraphRenderer.h"
#include "CkHfsmViewer_GraphRenderer_Constants.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    RenderStateNode(
        ImDrawList* InDrawList,
        const FCkHfsmViewer_StateInfo& InState,
        ImVec2 InCanvasOrigin,
        bool InIsTransitionQueued,
        bool InIsDimmed,
        bool InIsSubSmMuted,
        float InBorderFade,
        float InDwellFlash)
    -> void
{
    constexpr auto DimAlpha = 0.15f;
    constexpr auto SubSmMutedAlpha = 0.5f;
    auto Dim = [InIsDimmed, InIsSubSmMuted](ImU32 InColor)
    {
        if (InIsDimmed) { return ApplyDimAlpha(InColor, DimAlpha); }
        if (InIsSubSmMuted) { return ApplyDimAlpha(InColor, SubSmMutedAlpha); }
        return InColor;
    };

    auto NodeMin = ImVec2{
        InCanvasOrigin.x + InState.NodePosition.x * _CanvasZoom,
        InCanvasOrigin.y + InState.NodePosition.y * _CanvasZoom
    };

    auto NodeWidth = ComputeNodeWidth(InState) * _CanvasZoom;
    auto NodeHeight = ComputeNodeHeight(InState) * _CanvasZoom;
    auto NodeMax = ImVec2{NodeMin.x + NodeWidth, NodeMin.y + NodeHeight};

    auto ShowDwell = _ExpandAllNodes && InState.HasBeenVisited;
    auto TaskCount = _ExpandAllNodes ? InState.Tasks.Num() : 0;
    constexpr auto DwellBadgeRowHeight = 18.0f;
    auto DwellAreaHeight = ShowDwell ? DwellBadgeRowHeight * _CanvasZoom : 0.0f;

    auto Rounding = Layout::CornerRadius * _CanvasZoom;

    // Sub-SM nodes use blue-tinted palette
    auto NodeBgColor = InState.IsSubSmNode ? Colors::SubSmNodeBackground : Colors::NodeBackground;
    auto HeaderBgColor = InState.IsSubSmNode ? Colors::SubSmNodeHeader : Colors::NodeHeader;

    // Drop shadow
    auto ShadowOffset = 2.0f * _CanvasZoom;
    InDrawList->AddRectFilled(
        {NodeMin.x + ShadowOffset, NodeMin.y + ShadowOffset},
        {NodeMax.x + ShadowOffset, NodeMax.y + ShadowOffset},
        Dim(Colors::NodeShadow), Rounding);

    // Node background
    InDrawList->AddRectFilled(NodeMin, NodeMax, Dim(NodeBgColor), Rounding);

    // Header background
    auto HeaderMax = ImVec2{NodeMax.x, NodeMin.y + Layout::HeaderHeight * _CanvasZoom};

    InDrawList->AddRectFilled(
        NodeMin, HeaderMax, Dim(HeaderBgColor),
        Rounding, ImDrawFlags_RoundCornersTop);

    // Header separator line (only when node has content below header)
    auto HasContentBelowHeader = ShowDwell || TaskCount > 0;
    if (HasContentBelowHeader)
    {
        auto SepLeft = NodeMin.x + Layout::AccentBarWidth * _CanvasZoom;
        InDrawList->AddLine(
            {SepLeft, HeaderMax.y}, {NodeMax.x, HeaderMax.y},
            Dim(Colors::HeaderSeparator), 1.0f * _CanvasZoom);
    }

    // Border — determine if this is an "active" node (current, queued, or fading)
    auto IsActiveNode = InIsTransitionQueued || InState.IsCurrentState || (InBorderFade > 0.0f && NOT InState.IsSubSmNode);

    if (IsActiveNode)
    {
        // Active nodes get a full border
        auto BorderColor = Colors::InactiveStateBorder;
        auto BorderThickness = Layout::ActiveBorderThickness;

        if (InIsTransitionQueued)
        {
            BorderColor = Colors::TransitionQueuedBorder;
        }
        else if (InState.IsCurrentState)
        {
            BorderColor = InState.IsSubSmNode ? Colors::SubSmCurrentBorder : Colors::CurrentStateBorder;
        }
        else if (InBorderFade > 0.0f)
        {
            auto LerpChannel = [](float InFrom, float InTo, float InAlpha) -> uint8_t
            {
                return static_cast<uint8_t>(InFrom + (InTo - InFrom) * InAlpha);
            };

            auto Gr = LerpChannel(0x54, 0x43, InBorderFade);
            auto Gg = LerpChannel(0x6E, 0xA0, InBorderFade);
            auto Gb = LerpChannel(0x7A, 0x47, InBorderFade);
            BorderColor = IM_COL32(Gr, Gg, Gb, 0xFF);

            BorderThickness = Layout::BorderThickness + (Layout::ActiveBorderThickness - Layout::BorderThickness) * InBorderFade;
        }

        InDrawList->AddRect(NodeMin, NodeMax, Dim(BorderColor), Rounding, ImDrawFlags_None, BorderThickness * _CanvasZoom);
    }
    else
    {
        // Inactive nodes get a left accent bar only
        auto AccentColor = InState.IsSubSmNode
            ? Colors::SubSmInactiveBorder
            : Colors::InactiveStateBorder;

        auto AccentWidth = Layout::AccentBarWidth * _CanvasZoom;
        InDrawList->AddRectFilled(
            NodeMin,
            {NodeMin.x + AccentWidth, NodeMax.y},
            Dim(AccentColor), Rounding, ImDrawFlags_RoundCornersLeft);
    }

    // Breakpoint hit glow — pulsing red border when this state triggered a breakpoint
    if (InState.IsBreakpointHit)
    {
        auto PulsePhase = static_cast<float>(ImGui::GetTime()) * 4.0f;
        auto PulseAlpha = static_cast<uint8_t>(140.0f + 115.0f * FMath::Sin(PulsePhase));
        auto GlowColor = IM_COL32(0xEF, 0x33, 0x30, PulseAlpha);
        constexpr auto GlowThickness = 3.0f;
        InDrawList->AddRect(
            {NodeMin.x - 2.0f * _CanvasZoom, NodeMin.y - 2.0f * _CanvasZoom},
            {NodeMax.x + 2.0f * _CanvasZoom, NodeMax.y + 2.0f * _CanvasZoom},
            GlowColor, Rounding + 2.0f * _CanvasZoom, ImDrawFlags_None,
            GlowThickness * _CanvasZoom);
    }

    // State type icon — small colored square for ALL states
    auto IconSize = Layout::StateIconSize * _CanvasZoom;
    auto IconMin = ImVec2{
        NodeMin.x + (Layout::AccentBarWidth + Layout::NodePadding) * _CanvasZoom,
        NodeMin.y + (Layout::HeaderHeight * 0.5f - Layout::StateIconSize) * _CanvasZoom
    };
    auto IconMax = ImVec2{
        IconMin.x + IconSize * 2.0f,
        IconMin.y + IconSize * 2.0f
    };

    auto IconColor = Colors::InactiveStateBorder;
    if (InIsTransitionQueued)
    {
        IconColor = Colors::TransitionQueuedBorder;
    }
    else if (InState.IsCurrentState)
    {
        IconColor = InState.IsSubSmNode ? Colors::SubSmCurrentBorder : Colors::CurrentStateBorder;
    }
    else if (InState.IsSubSmNode)
    {
        IconColor = ApplyDimAlpha(Colors::SubSmInactiveBorder, 0.6f);
    }
    else
    {
        IconColor = ApplyDimAlpha(Colors::InactiveStateBorder, 0.6f);
    }

    constexpr auto IconRounding = 1.5f;
    InDrawList->AddRectFilled(IconMin, IconMax, Dim(IconColor), IconRounding * _CanvasZoom);

    // State name text
    auto TextOffsetX = (Layout::AccentBarWidth + Layout::NodePadding + Layout::StateIconSize * 2.0f + Layout::StateIconGap) * _CanvasZoom;

    auto TextPos = ImVec2{
        NodeMin.x + TextOffsetX,
        NodeMin.y + (Layout::HeaderHeight * 0.5f - 6.5f) * _CanvasZoom
    };

    auto NameAnsi = StringCast<ANSICHAR>(*InState.StateName);
    InDrawList->AddText(
        nullptr,
        13.0f * _CanvasZoom,
        TextPos,
        Dim(Colors::TextPrimary),
        NameAnsi.Get());

    // Sub-SM badge icon on header (to the left of breakpoint indicators)
    if (InState.HasSubStateMachine)
    {
        auto BadgeSize = 5.0f * _CanvasZoom;
        auto BadgeX = NodeMax.x - Layout::NodePadding * _CanvasZoom - BadgeSize * 2.0f;
        auto BadgeY = NodeMin.y + Layout::HeaderHeight * 0.5f * _CanvasZoom;

        if (InState.HasEntryBreakpoint || InState.HasExitBreakpoint)
        {
            BadgeX -= 16.0f * _CanvasZoom;
        }

        auto BadgeColor = Colors::SubSmBadge;

        InDrawList->AddRect(
            {BadgeX - BadgeSize, BadgeY - BadgeSize},
            {BadgeX + BadgeSize, BadgeY + BadgeSize},
            Dim(BadgeColor), 1.0f * _CanvasZoom, ImDrawFlags_None, 1.0f * _CanvasZoom);

        InDrawList->AddRect(
            {BadgeX - BadgeSize * 0.4f, BadgeY - BadgeSize * 0.4f},
            {BadgeX + BadgeSize * 0.4f, BadgeY + BadgeSize * 0.4f},
            Dim(BadgeColor), 0.0f, ImDrawFlags_None, 1.0f * _CanvasZoom);
    }

    // Breakpoint indicators (top-right corner of header)
    if (InState.HasEntryBreakpoint || InState.HasExitBreakpoint)
    {
        auto BpRadius = 5.0f * _CanvasZoom;
        auto BpSpacing = 3.0f * _CanvasZoom;
        auto BpX = NodeMax.x - Layout::NodePadding * _CanvasZoom - BpRadius;
        auto BpY = NodeMin.y + Layout::HeaderHeight * 0.5f * _CanvasZoom;

        if (InState.HasEntryBreakpoint && InState.HasExitBreakpoint)
        {
            InDrawList->AddCircleFilled({BpX, BpY}, BpRadius, Dim(Colors::Breakpoint));
            InDrawList->AddCircle({BpX, BpY}, BpRadius + BpSpacing, Dim(Colors::BreakpointOutline), 0, 1.5f * _CanvasZoom);
        }
        else if (InState.HasEntryBreakpoint)
        {
            InDrawList->AddCircleFilled({BpX, BpY}, BpRadius, Dim(Colors::Breakpoint));
        }
        else
        {
            InDrawList->AddCircle({BpX, BpY}, BpRadius, Dim(Colors::BreakpointOutline), 0, 1.5f * _CanvasZoom);
        }
    }

    // Dwell time badge (only when expanded)
    if (ShowDwell)
    {
        auto BadgeY = NodeMin.y + (Layout::HeaderHeight + Layout::NodePadding * 0.5f) * _CanvasZoom;

        auto DwellText = InState.IsCurrentDwellLive
            ? FString::Printf(TEXT("Active for %.2fs"), InState.DwellTimeSeconds)
            : FString::Printf(TEXT("Was active for %.2fs"), InState.DwellTimeSeconds);

        auto DwellAnsi = StringCast<ANSICHAR>(*DwellText);
        auto TextSize = ImGui::CalcTextSize(DwellAnsi.Get());

        constexpr auto PillPadX = 6.0f;
        constexpr auto PillPadY = 2.0f;
        auto PillWidth = TextSize.x + PillPadX * 2.0f * _CanvasZoom;
        auto PillHeight = TextSize.y + PillPadY * 2.0f * _CanvasZoom;

        auto PillMin = ImVec2{
            NodeMin.x + (Layout::AccentBarWidth + Layout::NodePadding) * _CanvasZoom,
            BadgeY
        };
        auto PillMax = ImVec2{PillMin.x + PillWidth, PillMin.y + PillHeight};

        auto BaseBgAlpha = InState.IsCurrentDwellLive ? 0x40 : 0x30;
        auto FlashBgBoost = static_cast<int32>(InDwellFlash * 0xB0);
        auto FinalBgAlpha = static_cast<uint8_t>(FMath::Min(BaseBgAlpha + FlashBgBoost, 0xFF));

        auto PillBg = InState.IsCurrentDwellLive
            ? Dim(IM_COL32(0x43, 0xA0, 0x47, FinalBgAlpha))
            : Dim(IM_COL32(0x54, 0x6E, 0x7A, FinalBgAlpha));

        auto PillTextColor = Dim(InState.IsCurrentDwellLive
            ? Colors::CurrentStateBorder
            : Colors::TextSecondary);

        if (InDwellFlash > 0.0f)
        {
            auto BaseTextColor = InState.IsCurrentDwellLive
                ? Colors::CurrentStateBorder
                : Colors::TextSecondary;

            auto LerpChannel = [](float InFrom, float InTo, float InAlpha) -> uint8_t
            {
                return static_cast<uint8_t>(InFrom + (InTo - InFrom) * InAlpha);
            };

            auto BaseR = static_cast<float>((BaseTextColor >> IM_COL32_R_SHIFT) & 0xFF);
            auto BaseG = static_cast<float>((BaseTextColor >> IM_COL32_G_SHIFT) & 0xFF);
            auto BaseB = static_cast<float>((BaseTextColor >> IM_COL32_B_SHIFT) & 0xFF);

            auto Fr = LerpChannel(BaseR, 0xFF, InDwellFlash);
            auto Fg = LerpChannel(BaseG, 0xFF, InDwellFlash);
            auto Fb = LerpChannel(BaseB, 0xFF, InDwellFlash);
            PillTextColor = Dim(IM_COL32(Fr, Fg, Fb, 0xFF));
        }

        constexpr auto PillRounding = 4.0f;
        InDrawList->AddRectFilled(PillMin, PillMax, PillBg, PillRounding * _CanvasZoom);

        if (InDwellFlash > 0.0f)
        {
            auto GlowAlpha = static_cast<uint8_t>(InDwellFlash * 0xA0);
            auto GlowColor = Dim(IM_COL32(0x43, 0xA0, 0x47, GlowAlpha));
            InDrawList->AddRect(PillMin, PillMax, GlowColor, PillRounding * _CanvasZoom, ImDrawFlags_None, 1.5f * _CanvasZoom);
        }

        auto DwellTextPos = ImVec2{
            PillMin.x + PillPadX * _CanvasZoom,
            PillMin.y + PillPadY * _CanvasZoom
        };

        InDrawList->AddText(
            nullptr,
            11.0f * _CanvasZoom,
            DwellTextPos,
            PillTextColor,
            DwellAnsi.Get());
    }

    // Task list (only for current state)
    if (TaskCount > 0)
    {
        auto TaskY = NodeMin.y + (Layout::HeaderHeight + Layout::NodePadding) * _CanvasZoom + DwellAreaHeight;

        for (const auto& Task : InState.Tasks)
        {
            auto StatusColor = GetTaskResultColor(Task.LastResult);

            auto StatusDotRadius = 3.0f * _CanvasZoom;
            auto StatusDotCenter = ImVec2{
                NodeMin.x + (Layout::AccentBarWidth + Layout::NodePadding) * _CanvasZoom + StatusDotRadius,
                TaskY + Layout::TaskRowHeight * 0.5f * _CanvasZoom
            };
            InDrawList->AddCircleFilled(StatusDotCenter, StatusDotRadius, Dim(StatusColor));

            auto TaskNamePos = ImVec2{
                NodeMin.x + (Layout::AccentBarWidth + Layout::NodePadding * 2.0f + 8.0f) * _CanvasZoom,
                TaskY + (Layout::TaskRowHeight * 0.5f - 5.0f) * _CanvasZoom
            };

            auto TaskNameAnsi = StringCast<ANSICHAR>(*Task.ClassName);
            InDrawList->AddText(
                nullptr,
                11.0f * _CanvasZoom,
                TaskNamePos,
                Dim(Colors::TextSecondary),
                TaskNameAnsi.Get());

            // Sub-SM badge: nested rectangles icon in light blue
            if (Task.HasSubStateMachine)
            {
                auto TextSize = ImGui::CalcTextSize(TaskNameAnsi.Get());
                auto BadgeX = TaskNamePos.x + TextSize.x + 4.0f * _CanvasZoom;
                auto BadgeY = TaskY + Layout::TaskRowHeight * 0.5f * _CanvasZoom;
                auto S = 4.0f * _CanvasZoom;

                // Outer rectangle
                InDrawList->AddRect(
                    {BadgeX, BadgeY - S},
                    {BadgeX + S * 2.0f, BadgeY + S},
                    Dim(Colors::SubSmBadge), 1.0f * _CanvasZoom, ImDrawFlags_None, 1.0f * _CanvasZoom);

                // Inner rectangle (offset)
                InDrawList->AddRect(
                    {BadgeX + S * 0.5f, BadgeY - S * 0.5f},
                    {BadgeX + S * 1.5f, BadgeY + S * 0.5f},
                    Dim(Colors::SubSmBadge), 0.0f, ImDrawFlags_None, 1.0f * _CanvasZoom);
            }

            TaskY += Layout::TaskRowHeight * _CanvasZoom;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkHfsmViewer_GraphRenderer::
    RenderTransitionLine(
        ImDrawList* InDrawList,
        const FCkHfsmViewer_StateInfo& InSource,
        const FCkHfsmViewer_StateInfo& InTarget,
        const FCkHfsmViewer_TransitionInfo& InTransition,
        ImVec2 InCanvasOrigin,
        float InPerpOffset,
        bool InIsDimmed,
        bool InIsSubSmMuted,
        float InFlash,
        float InSourcePortOffset,
        float InTargetPortOffset)
    -> void
{
    constexpr auto DimAlpha = 0.15f;
    constexpr auto SubSmMutedAlpha = 0.5f;
    auto Dim = [InIsDimmed, InIsSubSmMuted](ImU32 InColor)
    {
        if (InIsDimmed) { return ApplyDimAlpha(InColor, DimAlpha); }
        if (InIsSubSmMuted) { return ApplyDimAlpha(InColor, SubSmMutedAlpha); }
        return InColor;
    };

    auto SourceNodeWidth = ComputeNodeWidth(InSource) * _CanvasZoom;
    auto TargetNodeWidth = ComputeNodeWidth(InTarget) * _CanvasZoom;
    auto SourceNodeHeight = ComputeNodeHeight(InSource) * _CanvasZoom;
    auto TargetNodeHeight = ComputeNodeHeight(InTarget) * _CanvasZoom;

    auto SourceCenter = ImVec2{
        InCanvasOrigin.x + InSource.NodePosition.x * _CanvasZoom + SourceNodeWidth * 0.5f,
        InCanvasOrigin.y + InSource.NodePosition.y * _CanvasZoom + SourceNodeHeight * 0.5f
    };
    auto TargetCenter = ImVec2{
        InCanvasOrigin.x + InTarget.NodePosition.x * _CanvasZoom + TargetNodeWidth * 0.5f,
        InCanvasOrigin.y + InTarget.NodePosition.y * _CanvasZoom + TargetNodeHeight * 0.5f
    };

    auto BaseLineColor = InTransition.AreAllConditionsSatisfied
        ? Colors::TransitionSatisfied
        : Colors::TransitionUnsatisfied;

    auto LineColor = Dim(BaseLineColor);
    auto LineThickness = 1.5f;

    if (InFlash > 0.0f)
    {
        auto LerpChannel = [](float InFrom, float InTo, float InAlpha) -> uint8_t
        {
            return static_cast<uint8_t>(InFrom + (InTo - InFrom) * InAlpha);
        };

        auto BaseR = static_cast<float>((BaseLineColor >> IM_COL32_R_SHIFT) & 0xFF);
        auto BaseG = static_cast<float>((BaseLineColor >> IM_COL32_G_SHIFT) & 0xFF);
        auto BaseB = static_cast<float>((BaseLineColor >> IM_COL32_B_SHIFT) & 0xFF);

        auto Fr = LerpChannel(BaseR, 0x80, InFlash);
        auto Fg = LerpChannel(BaseG, 0xFF, InFlash);
        auto Fb = LerpChannel(BaseB, 0x80, InFlash);
        LineColor = IM_COL32(Fr, Fg, Fb, 0xFF);
        LineThickness = 1.5f + 2.0f * InFlash;
    }

    // ---- Self-loop rendering ------------------------------------------------
    if (InTransition.SourceStateIndex == InTransition.TargetStateIndex)
    {
        constexpr auto LoopExtent = 40.0f;
        constexpr auto LoopSpread = 25.0f;

        auto NodeRight = InCanvasOrigin.x + InSource.NodePosition.x * _CanvasZoom + SourceNodeWidth;
        auto P0 = ImVec2{NodeRight, SourceCenter.y - LoopSpread * _CanvasZoom};
        auto P3 = ImVec2{NodeRight, SourceCenter.y + LoopSpread * _CanvasZoom};
        auto Cp1 = ImVec2{NodeRight + LoopExtent * _CanvasZoom, SourceCenter.y - LoopSpread * _CanvasZoom};
        auto Cp2 = ImVec2{NodeRight + LoopExtent * _CanvasZoom, SourceCenter.y + LoopSpread * _CanvasZoom};

        InDrawList->AddBezierCubic(P0, Cp1, Cp2, P3, LineColor, LineThickness * _CanvasZoom);

        auto ArrowSize = Layout::ArrowSize * _CanvasZoom;
        constexpr auto ArrowChannel = 2;
        InDrawList->ChannelsSetCurrent(ArrowChannel);

        auto ArrowTip = P3;
        auto ArrowLeft = ImVec2{P3.x + ArrowSize * 0.8f, P3.y - ArrowSize * 0.5f};
        auto ArrowRight = ImVec2{P3.x + ArrowSize * 0.8f, P3.y + ArrowSize * 0.5f};
        InDrawList->AddTriangleFilled(ArrowTip, ArrowLeft, ArrowRight, LineColor);

        // Circular badge for self-loop
        auto BadgeCenter = ImVec2{
            NodeRight + (LoopExtent + 4.0f) * _CanvasZoom,
            SourceCenter.y
        };
        auto BadgeRadius = Layout::TransitionBadgeRadius * _CanvasZoom;

        InDrawList->AddCircleFilled(BadgeCenter, BadgeRadius, Dim(Colors::TransitionBadgeBg));
        InDrawList->AddCircle(BadgeCenter, BadgeRadius, Dim(BaseLineColor), 0, 1.0f * _CanvasZoom);

        auto LabelText = FString::Printf(TEXT("%d/%d"), InTransition.SatisfiedCount, InTransition.TotalCount);
        auto LabelAnsi = StringCast<ANSICHAR>(*LabelText);
        auto LabelTextSize = ImGui::CalcTextSize(LabelAnsi.Get());
        auto LabelPos = ImVec2{
            BadgeCenter.x - LabelTextSize.x * 0.5f,
            BadgeCenter.y - LabelTextSize.y * 0.5f
        };
        InDrawList->AddText(nullptr, Layout::TransitionBadgeFontSize * _CanvasZoom, LabelPos, Dim(Colors::TextSecondary), LabelAnsi.Get());

        constexpr auto LineChannel = 0;
        InDrawList->ChannelsSetCurrent(LineChannel);
        return;
    }

    // ---- Polyline or straight-line rendering --------------------------------

    auto Dx = TargetCenter.x - SourceCenter.x;
    auto Dy = TargetCenter.y - SourceCenter.y;
    auto Len = FMath::Sqrt(Dx * Dx + Dy * Dy);

    if (Len < 0.001f)
    { return; }

    auto AbsDx = FMath::Abs(Dx);
    auto AbsDy = FMath::Abs(Dy);

    auto SourcePos = ImVec2{};
    auto TargetPos = ImVec2{};

    auto PortUsableFraction = _LayoutPortUsableFraction;

    if (AbsDx >= AbsDy)
    {
        auto SourcePortY = SourceCenter.y + InSourcePortOffset * SourceNodeHeight * PortUsableFraction;
        auto TargetPortY = TargetCenter.y + InTargetPortOffset * TargetNodeHeight * PortUsableFraction;

        if (Dx >= 0.0f)
        {
            SourcePos = {SourceCenter.x + SourceNodeWidth * 0.5f, SourcePortY};
            TargetPos = {TargetCenter.x - TargetNodeWidth * 0.5f, TargetPortY};
        }
        else
        {
            SourcePos = {SourceCenter.x - SourceNodeWidth * 0.5f, SourcePortY};
            TargetPos = {TargetCenter.x + TargetNodeWidth * 0.5f, TargetPortY};
        }
    }
    else
    {
        auto SourcePortX = SourceCenter.x + InSourcePortOffset * SourceNodeWidth * PortUsableFraction;
        auto TargetPortX = TargetCenter.x + InTargetPortOffset * TargetNodeWidth * PortUsableFraction;

        if (Dy >= 0.0f)
        {
            SourcePos = {SourcePortX, SourceCenter.y + SourceNodeHeight * 0.5f};
            TargetPos = {TargetPortX, TargetCenter.y - TargetNodeHeight * 0.5f};
        }
        else
        {
            SourcePos = {SourcePortX, SourceCenter.y - SourceNodeHeight * 0.5f};
            TargetPos = {TargetPortX, TargetCenter.y + TargetNodeHeight * 0.5f};
        }
    }

    auto DirX = Dx / Len;
    auto DirY = Dy / Len;
    auto PerpX = -DirY;
    auto PerpY = DirX;

    SourcePos.x += PerpX * InPerpOffset;
    SourcePos.y += PerpY * InPerpOffset;
    TargetPos.x += PerpX * InPerpOffset;
    TargetPos.y += PerpY * InPerpOffset;

    auto Polyline = TArray<ImVec2>{};
    Polyline.Add(SourcePos);

    if (NOT InTransition.RouteWaypoints.IsEmpty())
    {
        auto SourceOffsetY = SourcePos.y - SourceCenter.y;
        auto TargetOffsetY = TargetPos.y - TargetCenter.y;

        for (const auto& Wp : InTransition.RouteWaypoints)
        {
            auto WpScreenX = InCanvasOrigin.x + Wp.x * _CanvasZoom;
            auto WpScreenY = InCanvasOrigin.y + Wp.y * _CanvasZoom;

            auto TotalDx = TargetPos.x - SourcePos.x;
            auto T = (FMath::Abs(TotalDx) > 0.001f)
                ? FMath::Clamp((WpScreenX - SourcePos.x) / TotalDx, 0.0f, 1.0f)
                : 0.5f;
            auto LerpedOffsetY = FMath::Lerp(SourceOffsetY, TargetOffsetY, T);

            Polyline.Add({WpScreenX, WpScreenY + LerpedOffsetY});
        }
    }

    Polyline.Add(TargetPos);

    for (auto SegIdx = 0; SegIdx < Polyline.Num() - 1; ++SegIdx)
    {
        InDrawList->AddLine(Polyline[SegIdx], Polyline[SegIdx + 1], LineColor, LineThickness * _CanvasZoom);
    }

    if (InFlash > 0.0f)
    {
        auto GlowAlpha = static_cast<uint8_t>(InFlash * 0x40);
        auto GlowColor = IM_COL32(0x80, 0xFF, 0x80, GlowAlpha);
        for (auto SegIdx = 0; SegIdx < Polyline.Num() - 1; ++SegIdx)
        {
            InDrawList->AddLine(Polyline[SegIdx], Polyline[SegIdx + 1], GlowColor, (LineThickness + 4.0f) * _CanvasZoom);
        }
    }

    // Breakpoint diamond at midpoint of the polyline
    if (InTransition.HasBreakpoint)
    {
        auto MidSegIdx = Polyline.Num() / 2;
        auto SegStart = Polyline[FMath::Max(0, MidSegIdx - 1)];
        auto SegEnd = Polyline[FMath::Min(Polyline.Num() - 1, MidSegIdx)];

        auto DiamondCenter = ImVec2{
            (SegStart.x + SegEnd.x) * 0.5f,
            (SegStart.y + SegEnd.y) * 0.5f
        };
        auto DiamondSize = 6.0f * _CanvasZoom;

        auto DiamondTop    = ImVec2{DiamondCenter.x, DiamondCenter.y - DiamondSize};
        auto DiamondRight  = ImVec2{DiamondCenter.x + DiamondSize, DiamondCenter.y};
        auto DiamondBottom = ImVec2{DiamondCenter.x, DiamondCenter.y + DiamondSize};
        auto DiamondLeft   = ImVec2{DiamondCenter.x - DiamondSize, DiamondCenter.y};

        InDrawList->AddQuadFilled(DiamondTop, DiamondRight, DiamondBottom, DiamondLeft, Dim(Colors::Breakpoint));
    }

    // Arrow head at the target end using final segment direction
    auto FinalStart = Polyline[Polyline.Num() - 2];
    auto FinalEnd = Polyline[Polyline.Num() - 1];
    auto FinalDx = FinalEnd.x - FinalStart.x;
    auto FinalDy = FinalEnd.y - FinalStart.y;
    auto FinalLen = FMath::Sqrt(FinalDx * FinalDx + FinalDy * FinalDy);

    if (FinalLen > 0.001f)
    {
        auto FinalDirX = FinalDx / FinalLen;
        auto FinalDirY = FinalDy / FinalLen;
        auto FinalPerpX = -FinalDirY;
        auto FinalPerpY = FinalDirX;

        auto ArrowSize = Layout::ArrowSize * _CanvasZoom;

        if (InFlash > 0.0f)
        {
            ArrowSize *= (1.0f + 0.5f * InFlash);
        }

        auto ArrowTip = FinalEnd;
        auto ArrowBase = ImVec2{
            ArrowTip.x - FinalDirX * ArrowSize * 1.2f,
            ArrowTip.y - FinalDirY * ArrowSize * 1.2f
        };
        auto ArrowLeft = ImVec2{
            ArrowBase.x + FinalPerpX * ArrowSize * 0.4f,
            ArrowBase.y + FinalPerpY * ArrowSize * 0.4f
        };
        auto ArrowRight = ImVec2{
            ArrowBase.x - FinalPerpX * ArrowSize * 0.4f,
            ArrowBase.y - FinalPerpY * ArrowSize * 0.4f
        };

        constexpr auto ArrowChannel = 2;
        InDrawList->ChannelsSetCurrent(ArrowChannel);
        InDrawList->AddTriangleFilled(ArrowTip, ArrowLeft, ArrowRight, LineColor);
    }

    // Circular badge at midpoint of the polyline
    auto LabelText = FString::Printf(TEXT("%d/%d"), InTransition.SatisfiedCount, InTransition.TotalCount);
    auto LabelAnsi = StringCast<ANSICHAR>(*LabelText);

    auto MidIdx = Polyline.Num() / 2;
    auto LabelSegStart = Polyline[FMath::Max(0, MidIdx - 1)];
    auto LabelSegEnd = Polyline[FMath::Min(Polyline.Num() - 1, MidIdx)];

    auto BadgeCenter = ImVec2{
        (LabelSegStart.x + LabelSegEnd.x) * 0.5f,
        (LabelSegStart.y + LabelSegEnd.y) * 0.5f
    };

    auto MousePos = ImGui::GetIO().MousePos;
    auto HoverThresholdSq = Layout::LineHoverThreshold * Layout::LineHoverThreshold * _CanvasZoom * _CanvasZoom;
    auto IsHovered = false;

    if (NOT InIsDimmed)
    {
        for (auto SegIdx = 0; SegIdx < Polyline.Num() - 1; ++SegIdx)
        {
            auto DistSq = PointToLineSegmentDistanceSq(MousePos, Polyline[SegIdx], Polyline[SegIdx + 1]);
            if (DistSq <= HoverThresholdSq)
            {
                IsHovered = true;
                break;
            }
        }

        // Also hover if mouse is inside the badge circle
        if (NOT IsHovered)
        {
            auto BadgeDx = MousePos.x - BadgeCenter.x;
            auto BadgeDy = MousePos.y - BadgeCenter.y;
            auto BadgeDistSq = BadgeDx * BadgeDx + BadgeDy * BadgeDy;
            auto BadgeRadiusSq = Layout::TransitionBadgeRadius * Layout::TransitionBadgeRadius * _CanvasZoom * _CanvasZoom;
            IsHovered = BadgeDistSq <= BadgeRadiusSq;
        }
    }

    if (IsHovered)
    {
        for (auto SegIdx = 0; SegIdx < Polyline.Num() - 1; ++SegIdx)
        {
            InDrawList->AddLine(Polyline[SegIdx], Polyline[SegIdx + 1], Colors::TransitionHovered, 2.5f * _CanvasZoom);
        }
    }

    constexpr auto ArrowChannel = 2;
    InDrawList->ChannelsSetCurrent(ArrowChannel);

    // Badge background + border
    auto BadgeRadius = Layout::TransitionBadgeRadius * _CanvasZoom;
    auto BadgeBorderColor = IsHovered ? Colors::TransitionHovered : Dim(BaseLineColor);

    InDrawList->AddCircleFilled(BadgeCenter, BadgeRadius, Dim(Colors::TransitionBadgeBg));
    InDrawList->AddCircle(BadgeCenter, BadgeRadius, BadgeBorderColor, 0, 1.0f * _CanvasZoom);

    // Centered text inside badge
    auto LabelTextSize = ImGui::CalcTextSize(LabelAnsi.Get());
    auto LabelPos = ImVec2{
        BadgeCenter.x - LabelTextSize.x * 0.5f,
        BadgeCenter.y - LabelTextSize.y * 0.5f
    };
    auto LabelTextColor = IsHovered ? Colors::TransitionHovered : Dim(Colors::TextSecondary);
    InDrawList->AddText(nullptr, Layout::TransitionBadgeFontSize * _CanvasZoom, LabelPos, LabelTextColor, LabelAnsi.Get());

    constexpr auto LineChannel = 0;
    InDrawList->ChannelsSetCurrent(LineChannel);

    // Condition details tooltip on hover (skip when dimmed)
    if (IsHovered && NOT InIsDimmed && NOT InTransition.Conditions.IsEmpty())
    {
        ImGui::BeginTooltip();

        for (const auto& Cond : InTransition.Conditions)
        {
            auto CondNameAnsi = StringCast<ANSICHAR>(*Cond.ClassName);
            auto HasLiveData = ck::IsValid(Cond.Handle);

            if (HasLiveData)
            {
                auto StatusColor = Cond.IsSatisfied
                    ? ImVec4{0.3f, 0.69f, 0.31f, 1.0f}
                    : ImVec4{0.94f, 0.33f, 0.31f, 1.0f};

                auto StatusIcon = Cond.IsSatisfied ? "[+]" : "[-]";

                ImGui::TextColored(StatusColor, "%s", StatusIcon);
                ImGui::SameLine();
                ImGui::Text("%s", CondNameAnsi.Get());
            }
            else
            {
                ImGui::TextColored({0.56f, 0.56f, 0.56f, 1.0f}, "[?]");
                ImGui::SameLine();
                ImGui::TextColored({0.56f, 0.56f, 0.56f, 1.0f}, "%s", CondNameAnsi.Get());
            }

            auto ModeText = Cond.Mode == ECk_SmConditionMode::Polled ? "Polled" : "Event";
            ImGui::SameLine();
            ImGui::TextColored({0.45f, 0.45f, 0.45f, 1.0f}, "(%s)", ModeText);
        }

        ImGui::EndTooltip();
    }
}

// --------------------------------------------------------------------------------------------------------------------
