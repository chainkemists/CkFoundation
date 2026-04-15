# CkCore / Debug

Debug diagnostics: name-printing verbosity, stack-trace formatting, debug-draw primitives, ASCII progress bars, and the debug-draw subsystem.

**Key files:** `CkDebug_Utils.h`, `CkDebugDraw_Utils.h`, `CkDebugDraw_Subsystem.h`

## Public types

```cpp
UENUM() enum class ECk_DebugNameVerbosity_Policy : uint8 { Default, Verbose, Compact };
UENUM() enum class ECk_StackTraceVerbosity_Policy : uint8 { Compact, Verbose };
UENUM() enum class ECk_ASCII_ProgressBar_Style   : uint8 { Equal_Symbol, HashTag_Symbol, FilledBlock_Symbol };
```

## Key classes

- `UCk_Utils_Debug_UE` (`CkDebug_Utils.h`) — name-verbosity helpers, stack-trace formatting, type-name pretty-printing for diagnostics and log output. Honors project-settings default for name verbosity.
- `UCk_Utils_DebugDraw_UE` (`CkDebugDraw_Utils.h`) — wrappers over UE's `DrawDebug*` that respect a central on/off toggle and add ASCII progress-bar rendering.
- `UCk_DebugDraw_Subsystem` (`CkDebugDraw_Subsystem.h`) — world subsystem that owns debug-draw lifetimes (so drawing survives beyond the call site until a frame tick).

## Why not just `DrawDebugLine` / `UE_LOG`?

The ensure and log systems format strings through `ck::Format_UE` with optional `ECk_DebugNameVerbosity_Policy`. Using these utilities means:

- Handle / tag / UObject names print consistently across log levels.
- Debug draw can be globally muted via project settings or the subsystem.
- Stack traces come out with the project's chosen verbosity (Compact for shipping, Verbose for dev).

## Depends on
`Macros/`, `TypeTraits/`, `Enums/`, `Math/ValueRange/`.

## Used by
`CkLog`, `CkEcs` (ensure messages format handles), `CkInsightsAnalyzer`, and any code that calls `DrawDebug*`.

## See also
- `/Source/CLAUDE.md` section "Function formatting standards" — source of the name-verbosity defaults.
- `Format/README.md` — the formatters consume these verbosity policies when stringifying handles.
