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
        float InBorderFade,
        float InDwellFlash)
    -> void
{
    constexpr auto DimAlpha = 0.15f;
    auto Dim = [InIsDimmed](ImU32 InColor) { return InIsDimmed ? ApplyDimAlpha(InColor, DimAlpha) : InColor; };

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

    // Node background
    InDrawList->AddRectFilled(NodeMin, NodeMax, Dim(Colors::NodeBackground), Rounding);

    // Header background
    auto HeaderMax = ImVec2{NodeMax.x, NodeMin.y + Layout::HeaderHeight * _CanvasZoom};

    InDrawList->AddRectFilled(
        NodeMin, HeaderMax, Dim(Colors::NodeHeader),
        Rounding, ImDrawFlags_RoundCornersTop);

    // Border — orange if transition queued, green if current, fading green→inactive if just left, grey otherwise
    auto BorderColor = Colors::InactiveStateBorder;
    auto BorderThickness = Layout::BorderThickness;

    if (InIsTransitionQueued)
    {
        BorderColor = Colors::TransitionQueuedBorder;
        BorderThickness = Layout::ActiveBorderThickness;
    }
    else if (InState.IsCurrentState)
    {
        BorderColor = Colors::CurrentStateBorder;
        BorderThickness = Layout::ActiveBorderThickness;
    }
    else if (InBorderFade > 0.0f)
    {
        auto LerpChannel = [](float InFrom, float InTo, float InAlpha) -> uint8_t
        {
            return static_cast<uint8_t>(InFrom + (InTo - InFrom) * InAlpha);
        };

        auto Gr = LerpChannel(0x60, 0x4C, InBorderFade);
        auto Gg = LerpChannel(0x7D, 0xAF, InBorderFade);
        auto Gb = LerpChannel(0x8B, 0x50, InBorderFade);
        BorderColor = IM_COL32(Gr, Gg, Gb, 0xFF);

        BorderThickness = Layout::BorderThickness + (Layout::ActiveBorderThickness - Layout::BorderThickness) * InBorderFade;
    }

    InDrawList->AddRect(NodeMin, NodeMax, Dim(BorderColor), Rounding, ImDrawFlags_None, BorderThickness * _CanvasZoom);

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

    // Current state indicator dot
    if (InState.IsCurrentState)
    {
        auto DotRadius = 4.0f * _CanvasZoom;
        auto DotCenter = ImVec2{
            NodeMin.x + Layout::NodePadding * _CanvasZoom + DotRadius,
            NodeMin.y + Layout::HeaderHeight * 0.5f * _CanvasZoom
        };

        auto DotColor = InIsTransitionQueued
            ? Colors::TransitionQueuedBorder
            : Colors::CurrentStateBorder;

        InDrawList->AddCircleFilled(DotCenter, DotRadius, Dim(DotColor));
    }

    // State name text
    auto TextOffsetX = InState.IsCurrentState
        ? (Layout::NodePadding * 2.0f + 8.0f) * _CanvasZoom
        : Layout::NodePadding * _CanvasZoom;

    auto TextPos = ImVec2{
        NodeMin.x + TextOffsetX,
        NodeMin.y + (Layout::HeaderHeight * 0.5f - 7.0f) * _CanvasZoom
    };

    auto NameAnsi = StringCast<ANSICHAR>(*InState.StateName);
    InDrawList->AddText(
        nullptr,
        14.0f * _CanvasZoom,
        TextPos,
        Dim(Colors::TextPrimary),
        NameAnsi.Get());

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
            NodeMin.x + Layout::NodePadding * _CanvasZoom,
            BadgeY
        };
        auto PillMax = ImVec2{PillMin.x + PillWidth, PillMin.y + PillHeight};

        auto BaseBgAlpha = InState.IsCurrentDwellLive ? 0x40 : 0x30;
        auto FlashBgBoost = static_cast<int32>(InDwellFlash * 0xB0);
        auto FinalBgAlpha = static_cast<uint8_t>(FMath::Min(BaseBgAlpha + FlashBgBoost, 0xFF));

        auto PillBg = InState.IsCurrentDwellLive
            ? Dim(IM_COL32(0x4C, 0xAF, 0x50, FinalBgAlpha))
            : Dim(IM_COL32(0x60, 0x7D, 0x8B, FinalBgAlpha));

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
            auto GlowColor = Dim(IM_COL32(0x4C, 0xAF, 0x50, GlowAlpha));
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
                NodeMin.x + Layout::NodePadding * _CanvasZoom + StatusDotRadius,
                TaskY + Layout::TaskRowHeight * 0.5f * _CanvasZoom
            };
            InDrawList->AddCircleFilled(StatusDotCenter, StatusDotRadius, Dim(StatusColor));

            auto TaskNamePos = ImVec2{
                NodeMin.x + (Layout::NodePadding * 2.0f + 8.0f) * _CanvasZoom,
                TaskY + (Layout::TaskRowHeight * 0.5f - 5.0f) * _CanvasZoom
            };

            auto TaskNameAnsi = StringCast<ANSICHAR>(*Task.ClassName);
            InDrawList->AddText(
                nullptr,
                11.0f * _CanvasZoom,
                TaskNamePos,
                Dim(Colors::TextSecondary),
                TaskNameAnsi.Get());

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
        float InFlash,
        float InSourcePortOffset,
        float InTargetPortOffset)
    -> void
{
    constexpr auto DimAlpha = 0.15f;
    auto Dim = [InIsDimmed](ImU32 InColor) { return InIsDimmed ? ApplyDimAlpha(InColor, DimAlpha) : InColor; };

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
    auto LineThickness = 2.0f;

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
        LineThickness = 2.0f + 3.0f * InFlash;
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

        auto LabelText = FString::Printf(TEXT("%d/%d"), InTransition.SatisfiedCount, InTransition.TotalCount);
        auto LabelAnsi = StringCast<ANSICHAR>(*LabelText);
        auto LabelPos = ImVec2{NodeRight + (LoopExtent + 4.0f) * _CanvasZoom, SourceCenter.y - 6.0f * _CanvasZoom};
        InDrawList->AddText(nullptr, 12.0f * _CanvasZoom, LabelPos, Dim(Colors::TextSecondary), LabelAnsi.Get());

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
        for (const auto& Wp : InTransition.RouteWaypoints)
        {
            Polyline.Add({
                InCanvasOrigin.x + Wp.x * _CanvasZoom,
                InCanvasOrigin.y + Wp.y * _CanvasZoom
            });
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
            ArrowTip.x - FinalDirX * ArrowSize * 1.5f,
            ArrowTip.y - FinalDirY * ArrowSize * 1.5f
        };
        auto ArrowLeft = ImVec2{
            ArrowBase.x + FinalPerpX * ArrowSize * 0.5f,
            ArrowBase.y + FinalPerpY * ArrowSize * 0.5f
        };
        auto ArrowRight = ImVec2{
            ArrowBase.x - FinalPerpX * ArrowSize * 0.5f,
            ArrowBase.y - FinalPerpY * ArrowSize * 0.5f
        };

        constexpr auto ArrowChannel = 2;
        InDrawList->ChannelsSetCurrent(ArrowChannel);
        InDrawList->AddTriangleFilled(ArrowTip, ArrowLeft, ArrowRight, LineColor);
    }

    // Condition label at midpoint of the polyline
    auto LabelText = FString::Printf(TEXT("%d/%d"), InTransition.SatisfiedCount, InTransition.TotalCount);
    auto LabelAnsi = StringCast<ANSICHAR>(*LabelText);

    auto MidIdx = Polyline.Num() / 2;
    auto LabelSegStart = Polyline[FMath::Max(0, MidIdx - 1)];
    auto LabelSegEnd = Polyline[FMath::Min(Polyline.Num() - 1, MidIdx)];

    auto MidPoint = ImVec2{
        (LabelSegStart.x + LabelSegEnd.x) * 0.5f + 6.0f * _CanvasZoom,
        (LabelSegStart.y + LabelSegEnd.y) * 0.5f - 12.0f * _CanvasZoom
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
    }

    if (IsHovered)
    {
        for (auto SegIdx = 0; SegIdx < Polyline.Num() - 1; ++SegIdx)
        {
            InDrawList->AddLine(Polyline[SegIdx], Polyline[SegIdx + 1], Colors::TransitionHovered, 3.0f * _CanvasZoom);
        }
    }

    constexpr auto ArrowChannel = 2;
    InDrawList->ChannelsSetCurrent(ArrowChannel);

    InDrawList->AddText(
        nullptr,
        12.0f * _CanvasZoom,
        MidPoint,
        IsHovered ? Colors::TransitionHovered : Dim(Colors::TextSecondary),
        LabelAnsi.Get());

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
