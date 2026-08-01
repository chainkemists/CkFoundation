# CkWebUmg — gate index (PLAN.md)

> **Written:** 2026-07-31. Status column updates land in the SAME commit as the gate landing —
> both here and in the gate file's own header (ck-methodology §7, Exhibit A).
> Volatile detail lives in [PROGRESS.md](PROGRESS.md); scope lives in [PROMPT.md](PROMPT.md) + [CampaignBrief.md](CampaignBrief.md).

| Gate | Name (brief phase) | Deliverable | Exit gate | Status |
|---|---|---|---|---|
| 0 | Recon + build/buy (§9 P0) | PriorArt.md, VERIFIED.md, DECISION 0 | Adam decides build/buy | ✅ Done 2026-07-31 (BUILD) |
| 1 | IR + extraction (§9 P1) | `ckwebumg-extract` CLI, `*.ckui.json` schema v1, `data-ck-*` spec, 20-page corpus + golden PNGs | Deterministic extraction; schema reviewed; **DECISIONS 1–4 made by Adam** | ✅ Done 2026-07-31 (D1=C Yoga panel, D2=DataAsset-runtime, D3=read-only regen, D4=allowlist ratified) |
| 2 | Layout runtime (§9 P2) | Yoga vendored (CkThirdParty), `CkWebUmg` module, `SCk_WebUmgFlexPanel`, IR loader, rect-diff harness | Layout-only corpus (L1–L9) within ±1px of IR boxes in automated run | ✅ Done 2026-08-01 (9/9 ≤ ±1px ratified; 884/884 full suite) |
| 3 | Paint layer (§9 P3) | Pixel-diff harness (FWidgetRenderer), rounded-rect/border paint, typed gradients, shadow strategy, text styling + tolerance regime | Paint corpus at threshold; text at §8.1 tolerance | 🟡 In progress — [Plan/Gate_03_Paint_Layer.md](Plan/Gate_03_Paint_Layer.md) |
| 4 | Emission (§9 P4) | DECISION 2 output form; read-only generated assets per DECISION 3 | Round-trip diff within threshold; idempotent regen | ⏳ Pending |
| 5 | Ergonomics (§9 P5) | `data-ck-*` end-to-end, live-reload preview, report UI | Real project screen converted + wired | ⏳ Pending |
| 6 | Hardening (§9 P6) | Coverage report, diagnostics, CI, module Claude.md docs | Corpus green in CI; hostile-page diagnostics fire | ⏳ Pending |

**Estimates:** 17–33 sessions total (Phase 0 estimate, inferred — re-date at each gate entry; ck-methodology §1 says real campaigns overrun several-fold).

**Post-ship cleanup:** gate files and this index are deleted when the campaign ships; `Source/CkWebUmg*/Claude.md` are the permanent survivors. PriorArt/VERIFIED/DECISIONS are kept (decision archaeology).
