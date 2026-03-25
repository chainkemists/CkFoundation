#pragma once

#include "CkHfsmViewer_Types.h"

// --------------------------------------------------------------------------------------------------------------------
// Shared constants and small helpers used across all GraphRenderer split .cpp files
// --------------------------------------------------------------------------------------------------------------------

namespace Colors
{
    constexpr auto CurrentStateBorder    = IM_COL32(0x4C, 0xAF, 0x50, 0xFF); // #4CAF50
    constexpr auto InactiveStateBorder   = IM_COL32(0x60, 0x7D, 0x8B, 0xFF); // #607D8B
    constexpr auto TransitionQueuedBorder = IM_COL32(0xFF, 0xB7, 0x4D, 0xFF); // #FFB74D orange
    constexpr auto NodeBackground        = IM_COL32(0x1E, 0x1E, 0x2E, 0xFF); // #1E1E2E
    constexpr auto NodeHeader            = IM_COL32(0x2D, 0x2D, 0x3D, 0xFF); // #2D2D3D
    constexpr auto CanvasBackground      = IM_COL32(0x0D, 0x0D, 0x14, 0xFF); // #0D0D14
    constexpr auto CanvasGridLines       = IM_COL32(0x20, 0x20, 0x30, 0x80); // subtle grid
    constexpr auto TransitionSatisfied   = IM_COL32(0x4C, 0xAF, 0x50, 0xFF); // #4CAF50
    constexpr auto TransitionUnsatisfied = IM_COL32(0x45, 0x5A, 0x64, 0xFF); // #455A64
    constexpr auto TransitionHovered     = IM_COL32(0x90, 0xCA, 0xF9, 0xFF); // #90CAF9 light blue
    constexpr auto TextPrimary           = IM_COL32(0xE0, 0xE0, 0xE0, 0xFF);
    constexpr auto TextSecondary         = IM_COL32(0x90, 0x90, 0x90, 0xFF);
    constexpr auto TaskRunning           = IM_COL32(0xFF, 0xC1, 0x07, 0xFF); // #FFC107 amber
    constexpr auto TaskSucceeded         = IM_COL32(0x4C, 0xAF, 0x50, 0xFF); // #4CAF50 green
    constexpr auto TaskFailed            = IM_COL32(0xEF, 0x53, 0x50, 0xFF); // #EF5350 red
    constexpr auto ConditionSatisfied    = IM_COL32(0x4C, 0xAF, 0x50, 0xFF); // green
    constexpr auto ConditionUnsatisfied  = IM_COL32(0xEF, 0x53, 0x50, 0xFF); // red
    constexpr auto ConditionUnknown      = IM_COL32(0x90, 0x90, 0x90, 0xFF); // grey
    constexpr auto Breakpoint            = IM_COL32(0xEF, 0x53, 0x50, 0xFF); // red
    constexpr auto BreakpointOutline     = IM_COL32(0xEF, 0x53, 0x50, 0xC0); // red, slightly transparent
}

namespace Layout
{
    constexpr auto NodePadding     = 8.0f;
    constexpr auto HeaderHeight    = 28.0f;
    constexpr auto TaskRowHeight   = 18.0f;
    constexpr auto CornerRadius    = 6.0f;
    constexpr auto BorderThickness = 2.0f;
    constexpr auto ActiveBorderThickness = 3.0f;
    constexpr auto GridSpacing     = 64.0f;
    constexpr auto ArrowSize       = 8.0f;
    constexpr auto BiDirectionalOffset = 8.0f;
    constexpr auto LineHoverThreshold = 6.0f;
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
