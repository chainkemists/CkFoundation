#include "CkPieLayoutEditor_Subsystem.h"

#include "CkPieLayoutEditor/CkPieLayoutEditor_Log.h"

#include "CkPieLayoutEditor/Settings/CkPieLayoutEditor_Settings.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"
#include "CkCore/Validation/CkIsValid.h"

#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericApplication.h"
#include "Interfaces/IMainFrameModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SWindow.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pie_layout
{
    struct FPieWindow
    {
        TSharedRef<SWindow> Window;
        ENetMode            NetMode = NM_Standalone;
        int32               PieInstance = 0;

        FPieWindow(
            TSharedRef<SWindow> InWindow,
            ENetMode InNetMode,
            int32 InPieInstance)
            : Window(MoveTemp(InWindow))
            , NetMode(InNetMode)
            , PieInstance(InPieInstance)
        {
        }
    };

    struct FRect
    {
        double X = 0.0;
        double Y = 0.0;
        double W = 0.0;
        double H = 0.0;
    };

    static auto
    Get_IsServerNetMode(
        ENetMode InNetMode)
        -> bool
    {
        return InNetMode == NM_ListenServer || InNetMode == NM_DedicatedServer;
    }

    static auto
    Make_Rect(
        const FPlatformRect& InPlatformRect)
        -> FRect
    {
        return FRect
        {
            static_cast<double>(InPlatformRect.Left),
            static_cast<double>(InPlatformRect.Top),
            static_cast<double>(InPlatformRect.Right - InPlatformRect.Left),
            static_cast<double>(InPlatformRect.Bottom - InPlatformRect.Top),
        };
    }

    static auto
    Get_DoesRectContain(
        const FPlatformRect& InRect,
        FVector2D InPoint)
        -> bool
    {
        return InPoint.X >= InRect.Left && InPoint.X < InRect.Right &&
               InPoint.Y >= InRect.Top  && InPoint.Y < InRect.Bottom;
    }

    static auto
    Collect_PieWindows(
        bool InIncludeListenServer)
        -> TArray<FPieWindow>
    {
        auto Out = TArray<FPieWindow>{};

        if (ck::Is_NOT_Valid(GEditor))
        { return Out; }

        for (const auto& Context : GEditor->GetWorldContexts())
        {
            if (Context.WorldType != EWorldType::PIE)
            { continue; }

            // A dedicated-server PIE context has no window to arrange.
            if (Context.RunAsDedicated)
            { continue; }

            auto* Viewport = Context.GameViewport.Get();
            if (ck::Is_NOT_Valid(Viewport))
            { continue; }

            const auto Window = Viewport->GetWindow();
            if (ck::Is_NOT_Valid(Window))
            { continue; }

            const auto* World = Context.World();
            const auto NetMode = ck::IsValid(World) ? World->GetNetMode() : NM_Standalone;

            if (NOT InIncludeListenServer && NetMode == NM_ListenServer)
            { continue; }

            Out.Emplace(Window.ToSharedRef(), NetMode, Context.PIEInstance);
        }

        return Out;
    }

    static auto
    Restore_PieWindowRects(
        const TArray<FCk_PieLayout_WindowRestoreEntry>& InEntries)
        -> void
    {
        ck::algo::ForEach(InEntries, [](const FCk_PieLayout_WindowRestoreEntry& InEntry)
        {
            const auto Window = InEntry.Window.Pin();
            if (ck::Is_NOT_Valid(Window))
            { return; }

            Window->ReshapeWindow(InEntry.Position, InEntry.Size);
        });
    }

    static auto
    Resolve_TargetWorkArea(
        ECk_PieLayout_TargetMonitor InTargetMonitor,
        int32 InSpecificMonitorIndex)
        -> FRect
    {
        auto Metrics = FDisplayMetrics{};
        FDisplayMetrics::RebuildDisplayMetrics(Metrics);

        const auto& Monitors = Metrics.MonitorInfo;
        if (Monitors.Num() == 0)
        { return Make_Rect(Metrics.PrimaryDisplayWorkAreaRect); }

        const auto Find_MonitorContaining = [&](FVector2D InPoint) -> int32
        {
            return ck::algo::FindIndex(Monitors, [&](const FMonitorInfo& InMonitor)
            { return Get_DoesRectContain(InMonitor.DisplayRect, InPoint); });
        };

        const auto Find_PrimaryIndex = [&]() -> int32
        {
            const auto PrimaryIndex = ck::algo::FindIndex(Monitors, [](const FMonitorInfo& InMonitor)
            { return InMonitor.bIsPrimary; });
            return PrimaryIndex != INDEX_NONE ? PrimaryIndex : 0;
        };

        auto ChosenIndex = Find_PrimaryIndex();

        switch (InTargetMonitor)
        {
            case ECk_PieLayout_TargetMonitor::PrimaryMonitor:
            {
                ChosenIndex = Find_PrimaryIndex();
                break;
            }
            case ECk_PieLayout_TargetMonitor::SpecificMonitor:
            {
                ChosenIndex = FCk_IntRange{0, Monitors.Num() - 1}.Get_ClampedValue(InSpecificMonitorIndex);
                break;
            }
            case ECk_PieLayout_TargetMonitor::MouseCursorMonitor:
            {
                if (FSlateApplication::IsInitialized())
                {
                    const auto CursorPos = FVector2D{FSlateApplication::Get().GetCursorPos()};
                    if (const auto Found = Find_MonitorContaining(CursorPos);
                        Found != INDEX_NONE)
                    { ChosenIndex = Found; }
                }
                break;
            }
            case ECk_PieLayout_TargetMonitor::EditorMonitor:
            {
                if (auto* MainFrame = FModuleManager::GetModulePtr<IMainFrameModule>(TEXT("MainFrame"));
                    ck::IsValid(MainFrame, ck::IsValid_Policy_NullptrOnly{}))
                {
                    if (const auto EditorWindow = MainFrame->GetParentWindow();
                        ck::IsValid(EditorWindow))
                    {
                        const auto WindowCenter = FVector2D{EditorWindow->GetPositionInScreen()} +
                                                  (FVector2D{EditorWindow->GetSizeInScreen()} * 0.5f);
                        if (const auto Found = Find_MonitorContaining(WindowCenter);
                            Found != INDEX_NONE)
                        { ChosenIndex = Found; }
                    }
                }
                break;
            }
        }

        return Make_Rect(Monitors[ChosenIndex].WorkArea);
    }

    // Produce N cell rects within InArea using the given column count. When InStretch is false (Compact),
    // cells stay at their minimum size; otherwise cells fill the area, clamped to the minimum.
    static auto
    Make_GridRects(
        int32 InCount,
        const FRect& InArea,
        int32 InGap,
        int32 InMinW,
        int32 InMinH,
        int32 InCols,
        bool InStretch)
        -> TArray<FRect>
    {
        auto Rects = TArray<FRect>{};
        if (InCount <= 0)
        { return Rects; }

        const auto Cols = FMath::Clamp(InCols, 1, InCount);
        const auto Rows = FMath::DivideAndRoundUp(InCount, Cols);

        auto CellW = InStretch
            ? (InArea.W - static_cast<double>(Cols - 1) * InGap) / Cols
            : static_cast<double>(InMinW);
        auto CellH = InStretch
            ? (InArea.H - static_cast<double>(Rows - 1) * InGap) / Rows
            : static_cast<double>(InMinH);

        CellW = FCk_IntRange{InMinW, FMath::Max(InMinW, static_cast<int32>(InArea.W))}.Get_ClampedValue(static_cast<int32>(CellW));
        CellH = FCk_IntRange{InMinH, FMath::Max(InMinH, static_cast<int32>(InArea.H))}.Get_ClampedValue(static_cast<int32>(CellH));

        for (auto Index = 0; Index < InCount; ++Index)
        {
            const auto Col = Index % Cols;
            const auto Row = Index / Cols;

            auto Rect = FRect{};
            Rect.X = InArea.X + static_cast<double>(Col) * (CellW + InGap);
            Rect.Y = InArea.Y + static_cast<double>(Row) * (CellH + InGap);
            Rect.W = CellW;
            Rect.H = CellH;
            Rects.Add(Rect);
        }

        return Rects;
    }

    // Auto Balanced: choose the column count whose per-cell aspect best matches the preferred ratio, with a
    // penalty for cells that would go below the minimum landscape ratio (i.e. become portrait).
    static auto
    Find_BestColumnCount(
        int32 InCount,
        const FRect& InArea,
        int32 InGap,
        float InPreferredAspect,
        float InMinLandscapeRatio)
        -> int32
    {
        auto BestCols = 1;
        auto BestScore = TNumericLimits<double>::Max();

        for (auto Cols = 1; Cols <= InCount; ++Cols)
        {
            const auto Rows = FMath::DivideAndRoundUp(InCount, Cols);
            const auto CellW = (InArea.W - static_cast<double>(Cols - 1) * InGap) / Cols;
            const auto CellH = (InArea.H - static_cast<double>(Rows - 1) * InGap) / Rows;

            if (CellW <= 0.0 || CellH <= 0.0)
            { continue; }

            const auto Aspect = CellW / CellH;
            auto Score = FMath::Abs(Aspect - static_cast<double>(InPreferredAspect));

            if (Aspect < static_cast<double>(InMinLandscapeRatio))
            { Score += (static_cast<double>(InMinLandscapeRatio) - Aspect) * 10.0; }

            if (Score < BestScore)
            {
                BestScore = Score;
                BestCols = Cols;
            }
        }

        return BestCols;
    }

    static auto
    Compute_HostFocusRects(
        int32 InCount,
        const FRect& InArea,
        int32 InGap,
        int32 InMinW,
        int32 InMinH,
        float InPreferredAspect,
        float InMinLandscapeRatio)
        -> TArray<FRect>
    {
        auto Rects = TArray<FRect>{};
        if (InCount <= 0)
        { return Rects; }

        if (InCount == 1)
        {
            Rects.Add(InArea);
            return Rects;
        }

        constexpr auto HostWidthFraction = 0.62;
        const auto HostW = FMath::Max(static_cast<double>(InMinW), (InArea.W - InGap) * HostWidthFraction);

        Rects.Add(FRect{InArea.X, InArea.Y, HostW, InArea.H});

        auto ClientArea = FRect{};
        ClientArea.X = InArea.X + HostW + InGap;
        ClientArea.Y = InArea.Y;
        ClientArea.W = FMath::Max(static_cast<double>(InMinW), InArea.W - HostW - InGap);
        ClientArea.H = InArea.H;

        const auto ClientCount = InCount - 1;
        const auto ClientCols = Find_BestColumnCount(ClientCount, ClientArea, InGap, InPreferredAspect, InMinLandscapeRatio);

        constexpr auto Stretch = true;
        Rects.Append(Make_GridRects(ClientCount, ClientArea, InGap, InMinW, InMinH, ClientCols, Stretch));

        return Rects;
    }

    static auto
    Compute_CellRects(
        int32 InCount,
        const FRect& InUsable,
        ECk_PieLayout_Mode InMode,
        int32 InGap,
        int32 InMinW,
        int32 InMinH,
        float InPreferredAspect,
        float InMinLandscapeRatio)
        -> TArray<FRect>
    {
        switch (InMode)
        {
            case ECk_PieLayout_Mode::EqualGrid:
            {
                const auto Cols = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(InCount)));
                constexpr auto Stretch = true;
                return Make_GridRects(InCount, InUsable, InGap, InMinW, InMinH, Cols, Stretch);
            }
            case ECk_PieLayout_Mode::Compact:
            {
                const auto Cols = FMath::Max(1, static_cast<int32>((InUsable.W + InGap) / (InMinW + InGap)));
                constexpr auto Stretch = false;
                return Make_GridRects(InCount, InUsable, InGap, InMinW, InMinH, Cols, Stretch);
            }
            case ECk_PieLayout_Mode::HostFocus:
            {
                return Compute_HostFocusRects(InCount, InUsable, InGap, InMinW, InMinH, InPreferredAspect, InMinLandscapeRatio);
            }
            case ECk_PieLayout_Mode::AutoBalanced:
            default:
            {
                const auto Cols = Find_BestColumnCount(InCount, InUsable, InGap, InPreferredAspect, InMinLandscapeRatio);
                constexpr auto Stretch = true;
                return Make_GridRects(InCount, InUsable, InGap, InMinW, InMinH, Cols, Stretch);
            }
        }
    }

    static auto
    DoArrangePieWindows(
        bool InAllowSingleWindow,
        TArray<FCk_PieLayout_WindowRestoreEntry>& InOutRestore)
        -> int32
    {
        auto Windows = Collect_PieWindows(UCk_Utils_PieLayout_Settings_UE::Get_IncludeListenServer());

        if (Windows.Num() == 0)
        { return 0; }

        if (Windows.Num() == 1 && NOT InAllowSingleWindow)
        { return 0; }

        const auto LayoutMode = UCk_Utils_PieLayout_Settings_UE::Get_LayoutMode();

        // Server first when explicitly requested, or implicitly when Host Focus wants the host in slot 0.
        const auto ServerFirst = UCk_Utils_PieLayout_Settings_UE::Get_ServerFirst() ||
                                 LayoutMode == ECk_PieLayout_Mode::HostFocus;

        ck::algo::Sort(Windows, [ServerFirst](const FPieWindow& InA, const FPieWindow& InB) -> bool
        {
            if (ServerFirst)
            {
                const auto AServer = Get_IsServerNetMode(InA.NetMode);
                const auto BServer = Get_IsServerNetMode(InB.NetMode);
                if (AServer != BServer)
                { return AServer; }
            }
            return InA.PieInstance < InB.PieInstance;
        });

        const auto WorkArea = Resolve_TargetWorkArea(
            UCk_Utils_PieLayout_Settings_UE::Get_TargetMonitor(),
            UCk_Utils_PieLayout_Settings_UE::Get_SpecificMonitorIndex());

        const auto OuterMargin = UCk_Utils_PieLayout_Settings_UE::Get_OuterMargin();
        const auto Gap = UCk_Utils_PieLayout_Settings_UE::Get_WindowGap();
        const auto MinW = UCk_Utils_PieLayout_Settings_UE::Get_MinimumWindowWidth();
        const auto MinH = UCk_Utils_PieLayout_Settings_UE::Get_MinimumWindowHeight();

        auto Usable = FRect{};
        Usable.X = WorkArea.X + OuterMargin;
        Usable.Y = WorkArea.Y + OuterMargin;
        Usable.W = FMath::Max(static_cast<double>(MinW), WorkArea.W - 2.0 * OuterMargin);
        Usable.H = FMath::Max(static_cast<double>(MinH), WorkArea.H - 2.0 * OuterMargin);

        const auto Rects = Compute_CellRects(
            Windows.Num(),
            Usable,
            LayoutMode,
            Gap,
            MinW,
            MinH,
            UCk_Utils_PieLayout_Settings_UE::Get_PreferredAspectRatio(),
            UCk_Utils_PieLayout_Settings_UE::Get_MinimumLandscapeRatio());

        if (Rects.Num() < Windows.Num())
        {
            ck::pie_layout::Warning(TEXT("Computed [{}] cell rects for [{}] PIE windows; skipping arrange"),
                Rects.Num(), Windows.Num());
            return 0;
        }

        auto ArrangedCount = 0;

        for (auto Index = 0; Index < Windows.Num(); ++Index)
        {
            const auto& Target = Rects[Index];
            const auto& Window = Windows[Index].Window;

            const auto TargetPos  = FVector2D{Target.X, Target.Y};
            const auto TargetSize = FVector2D{Target.W, Target.H};

            const auto CurrentPos  = FVector2D{Window->GetPositionInScreen()};
            const auto CurrentSize = FVector2D{Window->GetSizeInScreen()};

            // Skip windows that are already where we want them — avoids visible thrash during the retry window.
            constexpr auto Tolerance = 2.0;
            const auto IsUnchanged =
                FMath::Abs(CurrentPos.X  - TargetPos.X)  <= Tolerance &&
                FMath::Abs(CurrentPos.Y  - TargetPos.Y)  <= Tolerance &&
                FMath::Abs(CurrentSize.X - TargetSize.X) <= Tolerance &&
                FMath::Abs(CurrentSize.Y - TargetSize.Y) <= Tolerance;

            if (IsUnchanged)
            { continue; }

            // Captured lazily on first move (not as an up-front snapshot) so late-spawning client windows are
            // covered too, and unconditionally — the restore setting is read solely in OnPrePIEEnded, so
            // toggling it on mid-session still works.
            const auto IsAlreadyTracked = ck::algo::AnyOf(InOutRestore,
                [&](const FCk_PieLayout_WindowRestoreEntry& InEntry)
                { return InEntry.Window.Pin().Get() == &Window.Get(); });

            if (NOT IsAlreadyTracked)
            {
                auto Entry = FCk_PieLayout_WindowRestoreEntry{};
                Entry.Window   = Window;
                Entry.Position = CurrentPos;
                Entry.Size     = CurrentSize;
                InOutRestore.Add(Entry);
            }

            Window->ReshapeWindow(TargetPos, TargetSize);
            ++ArrangedCount;
        }

        if (ArrangedCount > 0)
        {
            ck::pie_layout::Verbose(TEXT("Arranged [{}] of [{}] PIE window(s)"), ArrangedCount, Windows.Num());
        }

        return ArrangedCount;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PieLayout_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    _PostPIEStarted_DelegateHandle = FEditorDelegates::PostPIEStarted.AddUObject(this, &UCk_PieLayout_Subsystem_UE::OnPostPIEStarted);

    // Restore on PrePIEEnded (NOT EndPIE): PrePIEEnded fires before the viewport / UI-layout / CommonInput
    // teardown begins, so the windows are still fully alive and reshaping them can't poke a half-destroyed
    // input subsystem. Reshaping during the EndPIE teardown broadcast crashed in CommonInput's preprocessor.
    _PrePIEEnded_DelegateHandle = FEditorDelegates::PrePIEEnded.AddUObject(this, &UCk_PieLayout_Subsystem_UE::OnPrePIEEnded);
}

auto
    UCk_PieLayout_Subsystem_UE::
    Deinitialize()
    -> void
{
    FEditorDelegates::PostPIEStarted.Remove(_PostPIEStarted_DelegateHandle);
    FEditorDelegates::PrePIEEnded.Remove(_PrePIEEnded_DelegateHandle);

    DoStopTickers();

    Super::Deinitialize();
}

auto
    UCk_PieLayout_Subsystem_UE::
    Request_ArrangeNow()
    -> void
{
    constexpr auto AllowSingleWindow = true;
    ck::pie_layout::DoArrangePieWindows(AllowSingleWindow, _OriginalWindowRects);
}

auto
    UCk_PieLayout_Subsystem_UE::
    OnPostPIEStarted(
        bool InIsSimulating)
    -> void
{
    DoStopTickers();

    // Fresh session — new restore rects are captured lazily as windows are moved
    _OriginalWindowRects.Reset();

    // Simulate-In-Editor runs inside the level viewport — there are no standalone PIE windows to arrange.
    if (InIsSimulating)
    { return; }

    if (NOT UCk_Utils_PieLayout_Settings_UE::Get_AutoArrangeOnPIE())
    { return; }

    const auto Delay = static_cast<float>(FMath::Max(0.0,
        UCk_Utils_PieLayout_Settings_UE::Get_InitialArrangeDelay().Get_Seconds()));
    _InitialDelayTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UCk_PieLayout_Subsystem_UE::OnInitialArrangeTick), Delay);
}

auto
    UCk_PieLayout_Subsystem_UE::
    OnPrePIEEnded(
        bool InIsSimulating)
    -> void
{
    // Stop any pending arrange/retry first so nothing reshapes a window as PIE is ending.
    DoStopTickers();

    if (UCk_Utils_PieLayout_Settings_UE::Get_RestoreWindowPositionsOnStop() && _OriginalWindowRects.Num() > 0)
    {
        ck::pie_layout::Restore_PieWindowRects(_OriginalWindowRects);
    }
    _OriginalWindowRects.Reset();
}

auto
    UCk_PieLayout_Subsystem_UE::
    OnInitialArrangeTick(
        float InDeltaTime)
    -> bool
{
    _InitialDelayTickerHandle.Reset();

    ck::pie_layout::DoArrangePieWindows(UCk_Utils_PieLayout_Settings_UE::Get_ArrangeSingleWindow(), _OriginalWindowRects);

    const auto RetryDuration = UCk_Utils_PieLayout_Settings_UE::Get_RetryDuration();
    if (RetryDuration.Get_Seconds() > 0.0)
    {
        _RetryChrono = FCk_Chrono{RetryDuration};

        const auto RetryInterval = FMath::Max(0.02f,
            static_cast<float>(UCk_Utils_PieLayout_Settings_UE::Get_RetryInterval().Get_Seconds()));
        _RetryTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateUObject(this, &UCk_PieLayout_Subsystem_UE::OnRetryTick), RetryInterval);
    }

    // One-shot: never re-fire the initial-delay ticker.
    return false;
}

auto
    UCk_PieLayout_Subsystem_UE::
    OnRetryTick(
        float InDeltaTime)
    -> bool
{
    const auto State = _RetryChrono.Tick(FCk_Time{static_cast<double>(InDeltaTime)});

    ck::pie_layout::DoArrangePieWindows(UCk_Utils_PieLayout_Settings_UE::Get_ArrangeSingleWindow(), _OriginalWindowRects);

    if (State == ECk_Chrono_TickState::Done)
    {
        _RetryTickerHandle.Reset();
        return false;
    }

    return true;
}

auto
    UCk_PieLayout_Subsystem_UE::
    DoStopTickers()
    -> void
{
    if (_InitialDelayTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_InitialDelayTickerHandle);
        _InitialDelayTickerHandle.Reset();
    }

    if (_RetryTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_RetryTickerHandle);
        _RetryTickerHandle.Reset();
    }

    _RetryChrono = FCk_Chrono{};
}

// --------------------------------------------------------------------------------------------------------------------
