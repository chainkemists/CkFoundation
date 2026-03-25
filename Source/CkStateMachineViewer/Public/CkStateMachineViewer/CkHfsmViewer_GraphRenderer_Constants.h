#pragma once

#include "CkHfsmViewer_Types.h"

// --------------------------------------------------------------------------------------------------------------------
// Shared constants and small helpers used across all GraphRenderer split .cpp files
// --------------------------------------------------------------------------------------------------------------------

namespace Colors
{
    constexpr auto CurrentStateBorder    = IM_COL32(0x43, 0xA0, 0x47, 0xFF); // #43A047 deeper green
    constexpr auto InactiveStateBorder   = IM_COL32(0x54, 0x6E, 0x7A, 0xFF); // #546E7A softer grey-blue
    constexpr auto TransitionQueuedBorder = IM_COL32(0xFF, 0xB7, 0x4D, 0xFF); // #FFB74D orange
    constexpr auto NodeBackground        = IM_COL32(0x20, 0x20, 0x30, 0xFF); // #202030 slightly brighter
    constexpr auto NodeHeader            = IM_COL32(0x2A, 0x2A, 0x3C, 0xFF); // #2A2A3C
    constexpr auto CanvasBackground      = IM_COL32(0x12, 0x12, 0x1A, 0xFF); // #12121A slightly warmer
    constexpr auto CanvasGridLines       = IM_COL32(0x1E, 0x1E, 0x2C, 0x50); // softer grid
    constexpr auto TransitionSatisfied   = IM_COL32(0x43, 0xA0, 0x47, 0xFF); // #43A047 match green
    constexpr auto TransitionUnsatisfied = IM_COL32(0x4A, 0x5A, 0x68, 0xFF); // #4A5A68 softer
    constexpr auto TransitionHovered     = IM_COL32(0x90, 0xCA, 0xF9, 0xFF); // #90CAF9 light blue
    constexpr auto TextPrimary           = IM_COL32(0xE0, 0xE0, 0xE0, 0xFF);
    constexpr auto TextSecondary         = IM_COL32(0x90, 0x90, 0x90, 0xFF);
    constexpr auto TaskRunning           = IM_COL32(0xFF, 0xC1, 0x07, 0xFF); // #FFC107 amber
    constexpr auto TaskSucceeded         = IM_COL32(0x43, 0xA0, 0x47, 0xFF); // #43A047 match green
    constexpr auto TaskFailed            = IM_COL32(0xEF, 0x53, 0x50, 0xFF); // #EF5350 red
    constexpr auto ConditionSatisfied    = IM_COL32(0x43, 0xA0, 0x47, 0xFF); // match green
    constexpr auto ConditionUnsatisfied  = IM_COL32(0xEF, 0x53, 0x50, 0xFF); // red
    constexpr auto ConditionUnknown      = IM_COL32(0x90, 0x90, 0x90, 0xFF); // grey
    constexpr auto Breakpoint            = IM_COL32(0xEF, 0x53, 0x50, 0xFF); // red
    constexpr auto BreakpointOutline     = IM_COL32(0xEF, 0x53, 0x50, 0xC0); // red, slightly transparent

    constexpr auto NodeShadow            = IM_COL32(0x00, 0x00, 0x00, 0x30); // faint drop shadow
    constexpr auto HeaderSeparator       = IM_COL32(0x40, 0x40, 0x55, 0x80); // subtle line below header
    constexpr auto TransitionBadgeBg     = IM_COL32(0x1A, 0x1A, 0x2A, 0xE0); // dark badge fill

    constexpr auto SubSmNodeBackground   = IM_COL32(0x1A, 0x1E, 0x2E, 0xFF); // blue-tinted dark
    constexpr auto SubSmNodeHeader       = IM_COL32(0x2A, 0x2D, 0x44, 0xFF); // blue-tinted header
    constexpr auto SubSmCurrentBorder    = IM_COL32(0x42, 0xA5, 0xF5, 0xFF); // blue active border
    constexpr auto SubSmInactiveBorder   = IM_COL32(0x5C, 0x6B, 0xC0, 0xFF); // indigo-grey inactive border
    constexpr auto SubSmBadge            = IM_COL32(0x42, 0xA5, 0xF5, 0xFF); // light blue badge
    constexpr auto SubSmConnector        = IM_COL32(0x42, 0xA5, 0xF5, 0x80); // dashed connector line
    constexpr auto SubSmLabel            = IM_COL32(0x42, 0xA5, 0xF5, 0xC0); // cluster label
}

namespace Layout
{
    constexpr auto NodePadding     = 6.0f;
    constexpr auto HeaderHeight    = 24.0f;
    constexpr auto TaskRowHeight   = 18.0f;
    constexpr auto CornerRadius    = 6.0f;
    constexpr auto BorderThickness = 2.0f;
    constexpr auto ActiveBorderThickness = 2.0f;
    constexpr auto GridSpacing     = 64.0f;
    constexpr auto ArrowSize       = 6.0f;
    constexpr auto BiDirectionalOffset = 8.0f;
    constexpr auto LineHoverThreshold = 6.0f;
    constexpr auto SubSmGap          = 60.0f;
    constexpr auto SubSmConnectorDash = 6.0f;
    constexpr auto SubSmClusterPadding = 16.0f;

    constexpr auto AccentBarWidth        = 3.0f;
    constexpr auto StateIconSize         = 5.0f;
    constexpr auto StateIconGap          = 6.0f;
    constexpr auto TransitionBadgeRadius = 10.0f;
    constexpr auto TransitionBadgeFontSize = 10.0f;
}

// --------------------------------------------------------------------------------------------------------------------

static auto
    PointToLineSegmentDistanceSq(
        ImVec2 InPoint,
        ImVec2 InLineA,
        ImVec2 InLineB)
    -> float
{
    auto Abx = InLineB.x - InLineA.x;
    auto Aby = InLineB.y - InLineA.y;
    auto Apx = InPoint.x - InLineA.x;
    auto Apy = InPoint.y - InLineA.y;

    auto LenSq = Abx * Abx + Aby * Aby;

    if (LenSq < 0.0001f)
    { return Apx * Apx + Apy * Apy; }

    auto T = FMath::Clamp((Apx * Abx + Apy * Aby) / LenSq, 0.0f, 1.0f);

    auto ClosestX = InLineA.x + T * Abx;
    auto ClosestY = InLineA.y + T * Aby;

    auto Dx = InPoint.x - ClosestX;
    auto Dy = InPoint.y - ClosestY;

    return Dx * Dx + Dy * Dy;
}

static auto
    GetTaskResultColor(
        ECk_SmTaskResult InResult)
    -> ImU32
{
    switch (InResult)
    {
    case ECk_SmTaskResult::Running:   return Colors::TaskRunning;
    case ECk_SmTaskResult::Succeeded: return Colors::TaskSucceeded;
    case ECk_SmTaskResult::Failed:    return Colors::TaskFailed;
    default:                          return Colors::TextSecondary;
    }
}

static auto
    ApplyDimAlpha(
        ImU32 InColor,
        float InAlphaMultiplier)
    -> ImU32
{
    auto A = static_cast<uint8_t>((InColor >> IM_COL32_A_SHIFT) & 0xFF);
    auto NewA = static_cast<uint8_t>(A * InAlphaMultiplier);
    return (InColor & ~(0xFFu << IM_COL32_A_SHIFT)) | (static_cast<ImU32>(NewA) << IM_COL32_A_SHIFT);
}

// --------------------------------------------------------------------------------------------------------------------
