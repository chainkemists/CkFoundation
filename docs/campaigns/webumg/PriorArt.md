# PriorArt.md — CkStyle Phase 0 (Recon)

**Date:** 2026-07-31 · **Author:** Phase 0 session · **Brief:** `CkStyle_CSS-to-UMG_Campaign.md`
**Engine on disk:** UnrealEngine-Angelscript 5.7.4 at `D:\Repositories\UnrealEngine-Angelscript` (`Engine\Build\Build.version`)
**Clones evaluated at:** `D:\tmp\ckstyle-phase0\thirdparty\{StyledWidgets, yoga, litehtml}` — all read at source level; path:line citations are relative to each repo root. Full citation ledger: `VERIFIED.md`.

**Headline: no evaluated target meets the §10 fidelity definition today, and none can be verified to.** The closest overlapping product (WebToUMG) explicitly declines to promise a measurable fidelity number and cannot be inspected without purchase. The campaign's two architectural bets (Chromium-as-extractor, Yoga-in-Slate) both survived source-level scrutiny — details per target and in the recommendation.

---

## 1. WebToUMG (commercial, Fab) — Verdict: **Reference only — do not adopt** (R3 purchase CLOSED 2026-08-01: Adam declined; public documentation sufficed)

> **R3 closure addendum (2026-08-01, from the vendor's public documentation, supplied by Adam):**
> the docs answer what the listing withheld, and every answer confirms BUILD:
> - **Bundled Chromium 90** (2021) — vendor admits no `color-mix()`, `:has()`, container queries,
>   `position:sticky`. Our contract records live-Chrome versions per extraction.
> - **Layout maps to stock UMG panels** ("UHorizontalBox / UVerticalBox / UUniformGridPanel /
>   UCanvasPanel / UOverlay") — no flex engine; grow/shrink/basis/wrap semantics cannot generally
>   survive that mapping (the exact gap DECISION 1's Yoga panel exists to close).
> - **Editable output + merge reimport** ("Everything stays editable after import"; reimport
>   "preserves your Event Graph logic") — the hand-edit/regeneration drift model DECISION 3's
>   read-only regeneration rejects by design.
> - **Fidelity stays unquantified** ("reproduces … faithfully" — no number, no harness, no
>   diagnostics contract), consistent with the listing's refusal to promise one.
> The baseline-number question R3 was meant to answer is moot: our own harness now measures
> per-page fidelity directly (Gate 3: exact-to-§8-bounded), which is strictly stronger evidence
> than a comparative score against an unmeasured product.
>
> **Fair-credit notes from the full listing (Adam-supplied)** — capabilities ahead of our current
> scope, recorded as candidate ideas, not as verdict-changers:
> - **State-aware capture**: drives live JS/React pages (click-and-reload discovery) and emits
>   every reachable screen/modal/tab (tabs → one `UWidgetSwitcher`, modals → collapsible dialogs,
>   nav wired). Empirical DOM-driven, framework-agnostic. Relevant to Gate 5 ergonomics scope.
> - **Animations**: CSS transition/:hover/:active/group-hover/@keyframes → `UWidgetAnimation`
>   tracks. Out of our v1 surface entirely; candidate for a later gate.
> - **Tiered material strategy**: 3 master materials (fill, shadow/glow, noise) with deduplicated
>   parametric instances; SVG/complex effects rasterized to Texture2D. Cross-check for our Gate 4
>   bake-at-import fork: their shadow choice (parametric MI) is the unmeasured alternative to our
>   measured baked-Gaussian; their dedup idea applies to our baked textures regardless.
> - **16 bundled font families** — the shipped-asset font answer Gate 4 needs (bundled faces, not
>   the harness's machine-local OS mapping); `oklch()/oklab()` pre-conversion to sRGB for their
>   frozen Chromium 90 (our live-Chrome extraction gets modern color spaces for free).

**Access:** No source available pre-purchase. Evidence = Fab listing (read 2026-07-31 in-browser: `fab.com/listings/9a3687aa-26a6-4d88-a841-2769a39e00a9`), seller Michel Brito, published 2026-06-19, last update 2026-07-29, $56.03–$112.08, Fab Standard License. Everything below from the listing is a **vendor claim**, labeled ⟨V⟩, not a finding.

**What it does (mechanism, per vendor):** ⟨V⟩ Imports HTML/CSS files, ZIPs, or live URLs and emits native Widget Blueprints: layouts become `UCanvasPanel`/`UVerticalBox`/`UHorizontalBox`/`UOverlay` hierarchies; gradients/shadows/glows become material instances (3 master materials); inline SVG and complex effects are rasterized to textures; CSS transitions/`:hover`/`@keyframes` become `UWidgetAnimation` tracks; semantic form controls map to `USlider`/`UCheckBox`/`UComboBoxString`. ⟨V⟩ A "state capture" mode drives the live page (click-and-reload) to discover screens/modals/tabs. ⟨V⟩ Reimport regenerates the widget tree in place with stable widget names, preserving Event Graph logic but **overwriting manual layout edits** ("A merge-preserving refresh is on the roadmap") — i.e. it independently arrived at the brief's DECISION 3 posture.
- Mechanically it evidently rides the engine's embedded browser: the listing requires the `WebBrowserWidget` plugin and names a "bundled Chromium 90 engine". So its extractor is the same architectural bet as this campaign's (ask a browser), but pinned to UE 5.5's legacy CEF era. Note: the engine on this machine (5.7.4) already defaults to CEF Chromium **128** — `bUseExperimentalVersion = true` at `CEF3.build.cs:16`, version table at `:31` — so the vendor's "Chromium 90 ceiling" (⟨V⟩ no `color-mix()`, `:has()`, container queries, `position:sticky`) is a self-imposed floor an external headless-Chromium extractor would not have.

**Licence:** Fab **Standard License** (listing, verbatim field "License terms: Standard License"). Permits use in shipped commercial products under Fab's EULA. Whether **C++ source ships with purchase: UNKNOWN — could not verify.** Settled by: purchasing a copy, or asking the seller. (Fab distributes code plugins with Epic-built binaries; source inclusion policy per listing is not stated.)

**UE compatibility:** ⟨V⟩ UE 5.5–5.7, editor on Windows/macOS, output targets anything UMG supports. Editor-only; nothing ships at runtime. No retargeting work — it's maintained (updated 2 days before this evaluation).

**Fidelity ceiling vs §10:**
- ⟨V⟩ verbatim: *"We avoid promising a fixed 'pixel-perfect %' because it depends entirely on which CSS a given design uses."* There is **no fidelity number, no pixel-diff harness, no conversion report with per-property diagnostics** mentioned anywhere in the listing. §10's coverage requirement ("zero silent drops … every unsupported property appears in the report with file:line") has no claimed counterpart; the limitations list says unsupported effects are "approximated or skipped".
- ⟨V⟩ Imports at a **single desktop viewport**; responsive is "on the roadmap". Layout is lowered to native panels — flexbox is baked, not live (the brief's DECISION 1 option A/B hybrid).
- ⟨V⟩ Chromium-90-era CSS ceiling (see above); non-.ttf fonts fall back; CSS-art/`filter:`/conic gradients approximated or skipped.
- Actual output fidelity: **UNKNOWN — could not verify** without purchase. What would settle it: buy it (~$56), run the campaign's would-be golden corpus through it, pixel-diff against Chromium screenshots. That experiment is cheap and produces exactly the baseline number §10 asks the campaign to beat.

**Reusable parts:** None extractable (no source pre-purchase). Its *decisions* are reusable evidence: single-dev product shipped in months on the browser-extraction bet; native-panel lowering; read-only-regeneration; stable-name reimport; 3-master-material paint strategy; `data-d2w-*` escape-hatch attributes (parallel to the brief's `data-ck-*`).

**Why not "buy instead of build":** it demonstrably cannot satisfy §10 as a *system* — no measurable fidelity contract, no diagnostics contract, no responsive layout, closed pipeline (UNKNOWN source access), single-viewport bake. It solves the adjacent problem ("get a one-shot editable approximation into UMG fast") — which is legitimately useful as a **baseline to beat**, hence the buy-one-copy recommendation.

---

## 2. rengeln/StyledWidgets — Verdict: **Reject (out of scope) — keep two ideas, reuse no code**

**Access:** cloned, read at source level. HEAD `86bdbec` (2025-01-05, shallow clone — single-commit history).

**What it actually is:** a runtime **tag-based styling** system for five reimplemented UMG widgets — not a layout or conversion tool.
- Stylesheets are UObject assets holding instanced style objects: `UWidgetStyleSheet` with `TArray<UWidgetStyleBase*> Styles` + a named color palette (`Source/StyledWidgets/Public/WidgetStyleSheet.h:20-42`). **No CSS text parsing anywhere** — the only parser is a one-line selector tokenizer (`#id`, `!inverted`, `+priority` tags), `FWidgetStyleSelector::Parse` (`Source/StyledWidgets/Private/WidgetStyleBase.cpp:74-144`).
- Matching/cascade: ~40-line tag-set matcher with an integer specificity score (identifier `0x70000000`, `+tag` `0x7000`, plain tag +1) (`WidgetStyleBase.cpp:34-72`); matched styles sort ascending and copy only their `OverriddenProperties` onto the widget's style instance via FProperty reflection (`WidgetStyleSheet.cpp:43-47`, `WidgetStyleBase.cpp:153-168`). No combinators, no pseudo-classes, no attribute selectors.
- Application: styled widgets write values directly onto their cached SWidget in `ApplyStyle` (e.g. `Source/StyledWidgets/Private/Widgets/StyledTextWidget.cpp:71-96`). Interaction states (hover/pressed/focus/disabled) are ordinary tags flipped by Slate event handlers, triggering a full re-match of the subtree (`StyledWidgetBase.cpp:77-131,190-212`).
- **Layout: none.** Zero flex hits across the entire source tree (grep verified this session); the only layout-adjacent properties are pass-through paddings/margins. Zero overlap with the conversion problem.
- **HTML: none.** No markup ingestion of any kind.
- Coverage: exactly five widget types (Text, Image, Border, Button, ProgressBar) + a BP hook. Requires CommonUI (`StyledWidgets.uplugin:29-34`). One README claim is false in code: runtime stylesheet swapping is documented but `UWidgetStyleManager` ships only a getter (`WidgetStyleManager.h:13-20`; no setter exists in source).

**Licence:** **MIT** — `LICENSE:1` "Copyright 2022 Robert Engeln" + standard MIT grant (verified verbatim this session). Commercial shipped use: permitted.

**UE compatibility:** README claims UE 5.1; `.uplugin` has no `EngineVersion` field. Raw `UObject*` UPROPERTYs throughout, one brush-ctor signature risk — a mechanical day-or-two port to 5.7 (inferred), but there is no reason to port it.

**Fidelity ceiling vs §10:** not applicable — it converts nothing and lays out nothing. As prior art for the *campaign's* problem it is a category error; it answers "how do I theme hand-built UMG at runtime", which the campaign does not ask.

**Reusable parts:** code — no (the reusable core is ~175 lines; reimplementing under CkFoundation conventions costs the same as adapting). Ideas — two worth keeping for later phases: (a) per-style `OverriddenProperties` + reflection copy as a minimal cascade primitive; (b) interaction states modeled as ordinary matcher inputs so hover/pressed/disabled reuse the same path (relevant to the IR `states` block in §5 of the brief).

---

## 3. Yoga (Meta flexbox engine) — Verdict: **Adopt (conditional on DECISION 1 landing on option C or D) — vendor as a CkThirdParty-style static lib**

**Access:** cloned, read at source level. HEAD `c766885` (2026-07-24, main, shallow) — untagged snapshot; pin by SHA when vendoring (no version string exists in-tree; `package.json:3` is `0.0.0`).

**What it actually does:** a dependency-free C++ flexbox layout engine with a C API. Host builds a node tree, sets styles, calls layout, reads back rects.
- **Layout entry:** `YGNodeCalculateLayout(YGNodeRef, float availableWidth, float availableHeight, YGDirection)` (`yoga/YGNode.h:77-81`, verified verbatim this session; impl shims to `yoga::calculateLayout`, `yoga/algorithm/CalculateLayout.cpp:2842-2935`). `YGUndefined` (quiet NaN, `yoga/YGValue.h:20`) means unconstrained. Results read per node: `YGNodeLayoutGetLeft/Top/Width/Height` (`yoga/YGNodeLayout.h:16-23`) — **relative to parent**; the host accumulates absolutes.
- **Measure callback (the text-node mechanism):** `typedef YGSize (*YGMeasureFunc)(YGNodeConstRef, float width, YGMeasureMode widthMode, float height, YGMeasureMode heightMode)` (`yoga/YGNode.h:204-209`, verified verbatim), registered via `YGNodeSetMeasureFunc` (`:216`). Modes: `Undefined` / `Exactly` / `AtMost` (`yoga/enums/MeasureMode.h:18-22`). The algorithm checks measure-func nodes FIRST, before any flex logic (`CalculateLayout.cpp:1791-1808`, dispatch at `:525-529`, verified), passes inner size (padding+border deducted, `:493-499`), skips the callback entirely when both axes are already exact (`:500-521`), and caches up to 8 measurements per node (`yoga/node/LayoutResults.h:25,44-45`). Measure funcs are **leaf-only**, enforced by fatal assert both directions (`yoga/node/Node.cpp:138-143`, `yoga/YGNode.cpp:141-144`); content changes require an explicit `YGNodeMarkDirty` (`yoga/YGNode.h:100-103`).
- **This is the critical fit:** Yoga hands the host a *width-constrained* measure question (`AtMost`/`Exactly`) — which is exactly what wrapped text needs and **more than Slate's own measure pass provides**. Engine evidence: Slate's desired-size pass is unconstrained — `virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const = 0` (`Engine/Source/Runtime/SlateCore/Public/Widgets/SWidget.h:731`) has no available-width parameter; real geometry only arrives in `OnArrangeChildren(const FGeometry& AllottedGeometry, …)` (`SWidget.h:1659`). Slate's own `STextBlock` auto-wrap solves this with a cache-and-invalidate loop: the wrap width is the **previous paint's** geometry (`Slate/Private/Framework/Text/SlateTextBlockLayout.cpp:235` — `CachedSize` written in `OnPaint`; `:394-399` — wrapping width taken from `CachedSize.X`), with a layout invalidation when the answer changes (`STextBlock.cpp:316-321`). So the brief's "highest-risk assumption" (§4 DECISION 1-C: reconciling Slate's two-pass model with Yoga's measure model) **survives recon**: a Yoga-backed `SPanel` can run the real constrained layout inside `OnArrangeChildren` where the true width is known, expose an unconstrained Yoga pass as `ComputeDesiredSize`, and is no worse off than Slate's own text pipeline — which already lives with the same one-frame settle. Full adversarial verification with a prototype remains Phase 1/2 work; the perf cost of layout-in-arrange is a thing to *measure*, not assume.

**Feature census (cited):** `gap`/row/column incl. percentage (`yoga/YGNodeStyle.h:104-107`, verified verbatim); `justify-content`/`align-content` incl. `space-evenly` (`yoga/enums/Justify.h`, `Align.h`); `wrap` incl. `wrap-reverse` (`yoga/YGEnums.h:157-160`); `aspect-ratio` (`YGNodeStyle.h:156-157`); `box-sizing` (`:109-110`); `position: static` and `display: contents` present. **Missing: the CSS `order` property** (zero hits in `yoga/style/Style.h` / `YGNodeStyle.h`, verified) — children lay out in insertion order; the extractor must emit children pre-sorted by computed `order`, which CDP makes trivial. **CSS Grid: not implemented — with a trap.** `Display::Grid` and a full set of grid-template/placement style setters exist at HEAD (`yoga/enums/Display.h:22`, `yoga/YGNodeStyle.h:160-236`) but **no layout code reads them**: `Display::Grid` appears nowhere in the layout algorithm outside abspos containing-block rules (`grep 'Display::Grid' yoga/algorithm/` excl. `AbsoluteLayout.cpp` → zero hits, verified this session) — a grid container silently lays out as flex. Do not expose these knobs if vendored.

**Defaults trap:** stock Yoga defaults are NOT web defaults (`flexDirection: column`, `flexShrink: 0`, `alignContent: flex-start`); `YGConfigSetUseWebDefaults` corrects this (`yoga/YGConfig.h:52-60`, `yoga/node/Node.h:336-339`, `Style.h:47`). An IR-driven pipeline sets every property explicitly anyway, but set web defaults regardless — silent-default divergence is exactly the §10 failure mode.

**Licence:** **MIT** — `LICENSE:1` "MIT License", `:3` "Copyright (c) Facebook, Inc. and its affiliates." (verified verbatim). Commercial shipped use: permitted (grant at `LICENSE:5-13`).

**UE retarget cost — small:** core = the `yoga/` dir only: 19 .cpp / 59 headers / **12,365 LOC** total; **zero external dependencies** (only an `#ifdef ANDROID` log path, `yoga/debug/Log.cpp:10-11`); **C++20** (`cmake/project-defaults.cmake:6`, verified — matches `CkModuleRules`); exceptions used only in the fatal-assert path with a `std::terminate` fallback when disabled (`yoga/debug/AssertFatal.cpp:18-25`); zero `dynamic_cast` in core. This is the same vendoring shape as EnTT/Jolt/fmt already in `CkThirdParty`. Integrator cautions: process-global generation counter + default-config singleton (`CalculateLayout.cpp:38`, `yoga/config/Config.cpp:130-132`) — fine for game-thread-only layout; post-layout pixel-grid snapping is applied in place (`CalculateLayout.cpp:2929-2931`, `PixelGrid.cpp:15-64`) with raw values still readable (`YGNodeLayoutGetRawWidth/Height`, `YGNodeLayout.h:33-41`) — the harness must fix `pointScaleFactor` to make ±1px comparisons deterministic.

**Fidelity ceiling vs §10:** highest available for live layout. Real flex semantics (grow/shrink/basis, wrap, gap, alignment) with host-measured text — the residual risk is Yoga-vs-Blink divergence on edge cases (errata flags exist precisely because deviations are known: `yoga/enums/Errata.h:18-26`), which is what the pixel harness exists to catch. What blocks doing better: nothing structural at the layout tier; text metrics parity is a Slate-side problem (§8.1 of the brief) regardless of layout engine.

**Reusable parts:** the whole core library — that's the point. Also its test corpus generator (`gentest/` produces HTML fixtures compared against browser layout) is prior art for the campaign's own golden-corpus harness design.

---

---

## 4. litehtml — Verdict: **Reject as pipeline foundation; Reference for container/API design**

**Access:** cloned, read at source level. HEAD `b9e89f0` (2026-07-28, master). No release versioning at HEAD (CMake soname 0.0.0, `CMakeLists.txt:14-15`) — pin by SHA if ever used.

**What it actually does:** a full HTML/CSS box-model layout+paint engine where the host supplies fonts, text metrics, images, and drawing through a `document_container` interface; litehtml owns parsing (Gumbo HTML5 parser), cascade, and layout.
- **The container abstraction** (`include/litehtml/document_container.h:33-89`): ~30 pure virtuals — font create/delete/measure (`:36-39`), text draw (`:40-41`), image size/load/draw (`:46-49`), solid/gradient fills (`:50-56`), borders with radii (`:57-58`), clip push/pop (`:72-73`), viewport and media features (`:74,78`), external CSS fetch (`:71`). The `uint_ptr hdc` handle passed to every draw call is host-opaque — it could be a widget-builder rather than a device context.
- **Widget emission would not even need the draw callbacks.** The laid-out render tree is public API: `document::render(max_width)` performs layout without drawing (`include/litehtml/document.h:91`), then `document::root_render()` (`document.h:115-116`) exposes a walkable `render_item` tree with absolute placement (`render_item.h:497` `get_placement()`, impl `src/render_item.cpp:1387-1410`), per-side box metrics (`render_item.h:119-207`), a back-pointer to the source element (`:379-382`), and full computed styles per element via `element::css()` → ~60 typed getters (`include/litehtml/css_properties.h:112-252`). This is architecturally exactly the extraction the campaign wants — box model + computed styles per node.
- **Text metrics are the host's problem:** one `text_width` call per word cached at style-compute time (`src/el_text.cpp:86-102`, split in `src/document.cpp:378-385`) — so layout fidelity is bounded by Slate's font measurement, not by a browser's (see fidelity ceiling).

**Licence:** **BSD 3-Clause** — `LICENSE:1` "Copyright (c) 2013, Yuri Kobets (tordex)". Bundled Gumbo parser is **Apache-2.0** (`src/gumbo/LICENSE:1-3`) and a PUBLIC link dependency (`CMakeLists.txt:212`). Both permit commercial shipped use (attribution obligations apply).

**UE retarget cost:** C++17 (`CMakeLists.txt:198-200`), no external deps beyond Gumbo, ≈71.5k LOC vendored total (29.5k litehtml src + 8.4k headers + 33.5k Gumbo C). Two real frictions for UE defaults: **exceptions** in the encoding prescan (`src/encodings.cpp:7895,8031,8126,8140`) and **RTTI** — a load-bearing `dynamic_cast` on the custom-property inheritance walk (`src/html_tag.cpp:347`, verified this session). Patchable (per-module `bUseRTTI`/exceptions are switchable in UBT), moderate effort.

**Fidelity ceiling vs §10 — this is the rejection:**
- **No CSS Grid at all**: the display enum has 18 values and none are grid (`include/litehtml/css_values.h:79-96`); the only "grid" hits repo-wide are HTML `<table>` internals and a media-feature comment (`src/media_query.cpp:437-438`).
- **Flexbox is substantial but has no `gap`**: grow/shrink/basis (`src/flex_item.cpp:10-20,161-176`), wrap incl. reverse (`src/render_flex.cpp:33,90,164`), full justify-content incl. space-evenly (`src/flex_line.cpp:485-563`), align-items/self/content with baseline (`src/flex_item.cpp:28-75`, `src/render_flex.cpp:115-189`), `order` (`src/flex_item.cpp:24`, `src/render_flex.cpp:259-296`) — but **zero `gap` support** (no property getter exists in `css_properties.h`; repo-wide grep clean). `gap` is in the brief's v1 IR sketch and in essentially every modern mockup.
- **Absent outright:** `calc()` (tokenizer comment only, `include/litehtml/css_tokenizer.h:37`), CSS transforms, `box-shadow` (no property, no draw callback — `document_container.h` has none), `position: sticky` (`css_values.h:375-382`). Present: border-radius (`include/litehtml/borders.h:50,130`), linear/radial/conic gradients incl. repeating (`src/gradient.cpp:102-131`), custom properties/var() with cycle guard (`src/style.cpp:1850-1928`), media queries (867-LOC parser, `src/media_query.cpp`).
- Net: litehtml's own layout diverges from the browser the designer previewed in (its README self-describes as "not fully compatible with HTML/CSS standards", `README.md:19`). Under §10 the golden screenshot comes from Chromium; an extractor whose box model is *not* Chromium's guarantees a fidelity gap the pipeline can never close. Using litehtml as the extractor would re-import exactly the risk the Chromium bet was chosen to eliminate.

**Reusable parts:** as **reference architecture**, genuinely valuable: (a) `document_container.h` is a battle-tested enumeration of *everything* a host must supply to render HTML — a checklist for what the UMG emitter must cover; (b) the public `render_item`/`css_properties` surface is a proven shape for "walkable box tree + computed styles", useful when designing the IR; (c) its per-word host-measured text flow is the pattern a Yoga measure-callback integration will end up with anyway. Vendor nothing.

---

## 5. Chromium DevTools Protocol — Verdict: **Adopt (as the extraction interface)**

**Access:** official protocol docs (`chromedevtools.github.io/devtools-protocol`), read 2026-07-31. No source read — the protocol *is* the contract; the implementation is Chromium's.

**Does it return what §5 of the brief needs? Yes — verified per method:**

| Brief needs (IR §5) | CDP method | Verified shape | Status |
|---|---|---|---|
| DOM tree | `DOM.getDocument` | `depth: -1` returns entire subtree; `pierce` traverses iframes/shadow roots; returns `root: Node` | **stable** (not experimental) |
| Computed, resolved styles per node | `CSS.getComputedStyleForNode(nodeId)` | flat array of `CSSComputedStyleProperty {name, value}` — computed values, resolved (cascade/specificity/inheritance/var()/calc() already applied) | **stable**; an `extraFields` param is experimental but unneeded |
| Box model per node | `DOM.getBoxModel` | `content`/`padding`/`border`/`margin` quads + `width`/`height` | **stable** |
| Golden screenshot | `Page.captureScreenshot` | png/jpeg/webp, optional `clip` viewport | **stable**; only the `fromSurface`/`captureBeyondViewport`/`optimizeForSpeed` *options* are experimental |
| Cascade provenance (for diagnostics) | `CSS.getMatchedStylesForNode` | matched rules + inherited chain — enables the "unsupported property at file:line" report | available |

**Licence:** the protocol is an interface; the runtime is headless Chromium/Chrome driven via Puppeteer or Playwright (both permissively licensed toolchains; Chromium is BSD-style). **Editor-time only** per the brief's constraint — nothing ships. No licence obstacle for a dev-tool dependency.

**Fidelity ceiling:** highest possible by construction — the extractor and the golden-screenshot oracle are the same engine, so "faithful to the mockup" reduces to "faithful to the extracted numbers". The residual gaps are on the UMG side (text shaping, paint parity), not the extraction side.

**Caveats found (none blocking):** computed values come back as CSS strings (`"12px"`, `"rgb(…)"`) that the extractor must parse into the IR's numeric fields — mechanical, since values are already absolute; `DOM.getBoxModel` quads are geometry (post-transform), so the extractor should record both quads and the computed `transform` and reconcile; per-node round-trips are chatty — batch via `DOMSnapshot.captureSnapshot` is an optimization to evaluate in Phase 1 (it is marked experimental, unlike the four core methods).

**Reusable parts:** the whole interface. Puppeteer/Playwright as the driver removes protocol plumbing.

---

## 6. UE SWebBrowser / Tracer WebUI — the "just embed a browser" alternative — Verdict: **Reject for shipped game UI (dossier below); keep CEF/WebView out of the runtime path**

**Access:** engine source on disk (UnrealEngine-Angelscript 5.7.4), read this session; Tracer WebUI Fab listing read in-browser 2026-07-31 (`fab.com/s/67cbc9dcdad6`).

**What embedding actually is (engine facts, cited):**
- The engine's browser is CEF, offscreen-rendered: CEF hands the UI thread a rasterized buffer in `FCEFWebBrowserWindow::OnPaint` (`Engine/Source/Runtime/WebBrowser/Private/CEF/CEFWebBrowserWindow.cpp:1968`), which is uploaded into a pair of `FSlateUpdatableTexture`s (`CEFWebBrowserWindow.h:652`). **The page is a texture blit inside Slate.** Widgets in the page do not exist as SWidgets/UWidgets in any form.
- CEF requires a separate helper **process** per browser context: `EpicWebHelper.exe` runtime dependency (`WebBrowser.Build.cs:103-115`).
- Platform reality: CEF is linked only on Win64/Mac/Linux (`WebBrowser.Build.cs:94-99`); Android/iOS fall back to OS webviews (`:31-33`); **no console platform appears anywhere in the module's build gating** — there is no engine path for this UI on PlayStation/Xbox/Switch.
- The UMG wrapper `UWebBrowser` is a thin `UWidget` around `SWebBrowser` (`Engine/Plugins/Runtime/WebBrowserWidget/Source/WebBrowserWidget/Public/WebBrowser.h:15,97`), in a plugin that is **not** enabled by default (`WebBrowserWidget.uplugin:13`).
- Engine CEF version on 5.7.4: Chromium 128.0.6613 (`CEF3.build.cs:16,31`) — current enough for modern CSS, so the *rendering* argument against embedding is not "old Chromium" on this engine.

**Why it is rejected for shipped game UI — write-down so it stays settled:**
1. **No native UMG interop, by construction.** The page is pixels on a texture (`OnPaint` → `FSlateUpdatableTexture`, cited above). No `BindWidget`, no per-element widget animation, no UMG focus metadata, no CommonUI, no Slate invalidation granularity, no designer editing of what's on screen. Every game↔UI interaction is marshalled through a JS bridge (Tracer's value-add is precisely a JSON bridge — which concedes the point).
2. **Console platforms: absent.** Engine build files contain no console CEF path (cited above). Tracer's listing says, verbatim, "COMING SOON: Xbox and PlayStation support using WebKit" — a vendor roadmap claim, unverified, and *WebKit*, i.e. a different engine with different CSS behavior than the Chromium the mockups were authored against — which would reopen the exact fidelity problem this campaign exists to close.
3. **Process/memory footprint.** A Chromium instance plus `EpicWebHelper` subprocess per browser riding along with a shipped game, for UI that UMG renders natively — permanent cost, every SKU, versus an editor-time-only toolchain. (No benchmark run this session; the *structural* costs — extra process, full web engine in memory, texture upload per dirty rect — are build-file and source facts, cited above.)
4. **Input/focus integration is simulated.** Input is re-injected into CEF as synthesized browser events; focus lives in two worlds (Slate's and the page's). Workable for menus; hostile to gamepad-first console UX certification.
5. **It abandons the mission.** The campaign's goal is *UMG that matches a mockup*. An embedded browser produces *a browser*, and its output is unhookable pixels (point 1) — §10's per-element rect comparison isn't even expressible.

**When embedding IS the right call (recorded so the rejection stays honest):** genuinely web-native content (account/store/news pages, rich text from a CMS), desktop-only tooling, or in-editor preview panes. Tracer WebUI is a competent, maintained product for those cases (UE 4.23–5.8, CEF 128, accelerated paint, updated 2026-06-21 — Fab listing). The rejection is scoped to *shipped, console-reaching, gameplay-integrated UI* — the campaign's target.

**Reusable parts:** the engine's own CEF (Chromium 128) could serve as a zero-install extraction backend for the Phase 1 CLI instead of a Node/Puppeteer toolchain — worth a Phase 1 look at whether CDP can be spoken to `UnrealCEFSubProcess`/CEF directly (CEF supports the DevTools protocol). Flagged as an option, not a commitment.

---

## 7. UIBridge (commercial, Fab) — Verdict: **Reference only — non-overlapping input domain; three ideas adopted as candidates** (added 2026-08-02)

**Access:** Fab listing text + the full 12-page vendor documentation PDF, both supplied by Adam, read 2026-08-02. No source pre-purchase. Listing: `fab.com/listings/79a975fc-a155-4758-b2bf-73d89f8e3ab0` — one `UIBridgeEditor` (Editor) module, 57 C++ classes, UE 5.8, Windows only, Photoshop CS6+/Figma. Claims labeled ⟨V⟩ (listing) / ⟨D⟩ (documentation, "Version 1.0 · Schema 2").

**What it does:** ⟨D⟩ Photoshop/Figma layer tree → `layout.json` (one shared schema for both exporters) → `WBP_*` Widget Blueprints. The layer *name* decides the widget class (~45 prefixes: `CAN_`→CanvasPanel, `VBX_`/`HBX_` boxes, `BTN_` compound groups whose `IMG_Normal/Hover/Pressed` children supply button states), with name-embedded modifiers (`ROT_`, `RAD_`, `ANC_`, `_IDN_` shared-texture keys). **Not a competitor:** the input domain is design-tool layer trees — no HTML, no CSS, no browser anywhere in the pipeline. Changes no verdict: DECISION 0 and all gate outcomes stand.

**The part that matters — the merge model (the "second import" problem, solved properly):** ⟨D⟩
- A `UIBM_*` manifest asset maps **layer id → widget name + content hash**; matching is on the design tool's own layer ID, not the name, so renames survive.
- Merge table: hash unchanged → widget untouched; hash differs → position/size/brush/text updated; new layer → created; deleted layer → removed; prefix changed → rebuilt (class change); **not in the manifest → you made it, never touched**.
- Generated graph nodes carry a "UIBridge generated" comment; re-import replaces exactly those; event nodes are *unwired, not deleted*, so user logic on the same event survives.
- Deleting the manifest degrades to create-everything-fresh — hand-built work is lost (their own warning).

This is the reference design if D2 ("also emit an editable WBP") is ever revisited. Our D3 read-only regeneration deliberately avoids needing any of it; our `data-ck-name` is the analogous stable ID — authored rather than tool-assigned, so it survives DOM restructuring even better than their layer ids survive Photoshop edits.

**Corroborations of our own choices (independent agreement = evidence):**
- **Font size ×0.75:** ⟨D⟩ they multiply by 0.75 (72÷96) with the same rationale (design-tool px@72dpi vs Slate points@96dpi) that Gate 3 derived empirically (`SizePx * 0.75f`). They expose the factor as a user knob; ours is exact by construction.
- **Shapes without textures:** ⟨D⟩ solid vector fills → RoundedBox brush, no PNG; gradients/masks/effects stay textures. Same split we already ship (`FSlateRoundedBoxBrush` for solid fills, `CkWebUmg_Builder.cpp:693,720`, verified this session; baked textures for gradients/shadows).
- **The editable-output fork, priced:** they took editable+merge (the road D3 declined) and their own Limits table shows the cost — the style DataAsset is created but "widgets do not reference it yet", and three wizard options (CommonUI emission, String Table, transparent-trim) are "present in the wizard, not yet acted on by the builder". Shipped no-op options are exactly the silent-failure shape our doctrine forbids.

**Ideas adopted as Gate 5/6 candidates (recorded, not committed):**
1. **Live-reload debounce:** ⟨D⟩ Live Link waits **1.25 s of quiet** rather than reacting to the first file change, because the exporter writes textures first and `layout.json` last — verbatim applicable to our extractor's output ordering when the deferred live-reload item is built. Two more invariants worth copying: the live path *always* merges regardless of settings (a save can never destroy work), and the watcher outlives the wizard ("switch it off before packaging").
2. **Validation pre-pass:** ⟨D⟩ "Read and validate" reads and checks **without touching the project**; errors block the import, warnings don't; every finding carries a one-sentence fix. Our conversion report is post-conversion; a pre-import validate mode over the IR is cheap (loader + report machinery already exist).
3. **Hit-test defaults:** ⟨D⟩ `BG_` layers become `HitTestInvisible`, text blocks `SelfHitTestInvisible` — a full-screen background must not eat clicks aimed at buttons above it. **Confirmed gap in our builder:** zero hit-test handling in `Source/CkWebUmg` (grep this session — no `HitTestInvisible` anywhere; the only visibility call is the `Hidden` case at `CkWebUmg_Builder.cpp:811`). When emitted pages become interactive, decorative nodes should default to (Self)HitTestInvisible.
4. **Font import pairing** (feeds Gate 4's shipped-font question): ⟨D⟩ a .ttf import must build **both** the FontFace and the Font asset whose typeface points at it — "A FontFace on its own cannot be used by a TextBlock". Anchors the bundled-font work item; unmatched fonts still import at correct size with the default face (layout survives, styling fixable later — a good degradation shape).
5. **Prefab/reusable-widget detection** (frontier candidate, nothing in our scope covers it): ⟨D⟩ repeated structures become one WBP instanced N times; detection hashes types + relative geometry + texture keys while ignoring names, position, and text content; the differing text becomes an instance-editable, expose-on-spawn variable. HTML equivalent: repeated card/button subtrees.
6. **Anchor inference** for aspect-ratio survival: ⟨D⟩ covers ≥85% of an axis → stretch; center in the first/last third → anchored to that edge; else centered; `ANC_` overrides per layer. Less urgent for us (Yoga owns responsive layout), but relevant to any absolute-positioned screen we emit.

**Fidelity ceiling vs §10:** not comparable — no fidelity number, no pixel harness, no diagnostics contract anywhere in the 12 pages. The Limits table is honest but qualitative (blend modes → imported as Normal "with a note"; gradient/glow/bevel stay baked into textures). Same posture as WebToUMG: fidelity by construction effort, not by measurement.

---

## Build vs. buy — recommendation (evidence chain exposed)

**Recommendation: BUILD — with three evidence-driven scope reductions, and a $56 hedge.** DECISION 0 remains Adam's; the chain that leads there:

### The evidence chain

1. **The only product that solves this problem (WebToUMG) cannot be shown to meet §10, and its own vendor declines to claim it.** No fidelity number ("We avoid promising a fixed 'pixel-perfect %'" — listing, verbatim), no diagnostics contract, single-viewport bake, no responsive layout, closed pipeline, UNKNOWN source access. §10 requires a *measured* ±1px layout bound, a text-tolerance regime, and zero silent drops — none of these are purchasable today. **Buy-outright fails on §10, not on taste.**
2. **The open-source "whole pipeline" candidates are structurally disqualified.** litehtml is the only one that even owns a box model, and its layout is not the browser's: no Grid, no flex `gap`, no `calc()`, no transforms, no box-shadow (all cited in §4) — using it as extractor reintroduces the divergence the Chromium bet exists to kill. StyledWidgets is runtime theming, zero layout (cited in §2) — different problem.
3. **Both of the campaign's architectural bets survived source-level scrutiny.** (a) CDP: all four required methods verified stable with the right return shapes (§5) — cascade/specificity/inheritance/`calc()`/var() genuinely leave the project's scope. (b) Yoga-in-Slate: the measure-callback model is *richer* than Slate's own unconstrained desired-size pass, and Slate's own text pipeline already lives with the same constraint-arrives-late reality via cache-and-invalidate (engine citations in §3). The single highest-risk assumption in the brief did not fall in recon.
4. **The expensive parts of "build" shrink on inspection.** Yoga is a 12.4k-LOC, zero-dependency, MIT, C++20 vendor drop — the same shape as EnTT/Jolt/fmt already in CkThirdParty. Extraction is a thin CDP client (Puppeteer/Playwright do the plumbing). What remains genuinely novel is exactly the part nobody sells: the Slate flex panel, the SDF paint brushes, the emitter, and the fidelity harness.
5. **The browser-embedding alternative is rejected on engine-source grounds** (§6): texture-blit output with no widget interop by construction, no console path in the engine build files, a helper process per browser — and it abandons the mission (§10's per-element comparison isn't expressible against pixels).

### Scope reductions to take (from the evidence)

- **R1 — Adopt CDP + Puppeteer/Playwright for extraction** (kills the in-engine CSS-parser scope entirely). Phase 1 should also spend half a day probing whether the engine's own CEF (Chromium 128 on 5.7.4 — `CEF3.build.cs:16,31`) can serve CDP for a zero-Node-toolchain variant; option, not commitment.
- **R2 — Vendor Yoga for the layout runtime** (kills flex-algorithm authorship; what's left is the `SCkFlexBox` bridge). Pre-sort children by computed `order` at extraction (Yoga lacks `order`); set `UseWebDefaults`; never expose the inert grid knobs.
- **R3 — Buy one WebToUMG license (~$56) in Phase 1** and run the golden corpus through it. It converts the §10-baseline question from rhetoric into a measured number, and its output is a live answer key for panel-lowering choices (DECISION 1-B/D) and material-brush design. Cheapest de-risking available.

### Cost picture (phases from brief §9; estimates are inferred, not measured — stated in campaign sessions, this repo's working unit)

| Phase | Sizing rationale | Estimate |
|---|---|---|
| 1 — IR + extraction | Thin CDP client + schema + corpus authoring; no cascade work (R1) | 2–4 sessions |
| 2 — Layout runtime | Yoga vendor drop is mechanical (R2); `SCkFlexBox : SPanel` + measure bridge + harness is the real work; harness runs headless via existing CkTests/toolbox infra | 4–8 sessions |
| 3 — Paint layer | SDF rounded-rect/gradient/shadow materials + text styling + asset import; border-radius blocker is here by design | 4–8 sessions |
| 4 — Emission | `UWidgetBlueprint` generation is documented-fiddly (DECISION 2 pending) | 3–6 sessions |
| 5 — Ergonomics | `data-ck-*` + live preview | 2–4 sessions |
| 6 — Hardening | Diagnostics sweep + CI + docs | 2–3 sessions |

Total: roughly **17–33 focused sessions** across 6 gated phases — a multi-month campaign at typical cadence. Uncertainty is dominated by Phases 2–4; the Phase 2 gate (layout-only corpus at threshold) is the earliest honest kill-point, and R3's baseline number arrives before any expensive phase starts.

### The alternatives priced honestly

| Option | Cost | What you get | What §10 says |
|---|---|---|---|
| Buy WebToUMG, stop there | ~$56–112 | One-shot editable imports, maintained, 5.5–5.7 | Fails: no fidelity contract, no diagnostics, single viewport; UNKNOWN extensibility (source access unverified) |
| Embed a browser (SWebBrowser / Tracer WebUI, $70–210) | Plugin cost + permanent runtime footprint | Real Chromium fidelity, no conversion at all | Fails: not UMG, no console path in engine, per-element comparison inexpressible |
| Adapt litehtml as extractor+layout | ~71.5k LOC vendored, RTTI/exception patches | A non-browser box model | Fails: layout ≠ the golden screenshot's engine; Grid/gap/calc absent |
| **Build per the brief (with R1–R3)** | 17–33 sessions, editor-time Node/Chromium toolchain | Measured-fidelity, responsive, diagnostic-complete pipeline owned in-tree | The only option that can meet it |

### If the recommendation is wrong, it's most likely wrong here

The estimate that would most plausibly move DECISION 0 to "buy": if the R3 benchmark shows WebToUMG already lands within a few px on the real corpus *and* its purchase includes extensible source, the remaining §10 gaps (harness, diagnostics, responsiveness) might be cheaper to build *around* it than to build the pipeline. That is a measurable question — see DECISIONS.md for the explicit movers.
