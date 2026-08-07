# CkEditorTools

**Purpose:** Shared styling tokens for Ck editor tooling UIs — the `CkStyle::` palette/typography
namespace (`Style/CkStyle.h`), the `ECk_Tone` semantic tone enum, and the backing per-user
settings class `UCk_Style_UserSettings_UE` (Editor Preferences → Ck → Style). Migrated from
CkGameplayDebugger's `CkDebuggerCommon/Style/CkDebugStyle` so its debugger modules and
CkFoundation editor tools share one look.

**Type:** Runtime (NOT UncookedOnly) — deliberate: the consumers include Runtime-type debugger
modules (`CkDebuggerCommon`, `CkEntityDebugOverlay` in the CkGameplayDebugger plugin), and a
Runtime module cannot depend on an UncookedOnly one. Same precedent as those modules.

**Depends on:** `CkSettings` (user-settings base). No other Ck deps.
**Used by:** the CkGameplayDebugger plugin's Slate debugger modules, including
`CkInsightsDebugger` (directly and via `CkDebuggerCommon`).

---

## Key API

- `CkStyle::` — lazy accessors for every palette/typography token (`Bg1()`, `Text()`, `Accent()`,
  `FontSizeBody()`, …), spacing constants (`SpaceS/M/L/…`), `GetFilledBrush()`,
  `GetToneColor(ECk_Tone)`.
- `ECk_Tone` — Neutral/Info/Ok/Warn/Err/Accent semantic tones.
- `UCk_Style_UserSettings_UE` — the tunables; read live via `GetDefault<>`, never cached.

---

## Anti-patterns

- Don't hardcode colors/font sizes in Ck editor tool Slate — route through `CkStyle::`.
- Don't add a constructor to the settings class (base only accepts FObjectInitializer); defaults
  are inline on each UPROPERTY.
- Don't allocate brushes in paint paths — use `CkStyle::GetFilledBrush()`.
