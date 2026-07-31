# VERIFIED.md — CkStyle Phase 0 claim ledger

**Session date:** 2026-07-31. Method key: **[read]** = read directly in this session's main context; **[agent+spot]** = deep-read by a session subagent, with the listed load-bearing citations independently re-verified in main context; **[web]** = fetched from the cited URL this session; **⟨V⟩** = vendor claim, recorded not verified; **UNKNOWN** = could not verify, settling evidence named.

Repos cloned (shallow) at `D:\tmp\ckstyle-phase0\thirdparty\`: StyledWidgets @ `86bdbec`, yoga @ `c766885`, litehtml @ `b9e89f0`. Engine: `D:\Repositories\UnrealEngine-Angelscript` (paths below abbreviated `Engine/`).

## Preconditions

| Claim | Evidence | Method |
|---|---|---|
| Engine source available; resolved via project helper | `CkAuto\Get-ProjectEnginePath.ps1` → `D:\Repositories\UnrealEngine-Angelscript` | [read] |
| Engine version 5.7.4 | `Engine\Build\Build.version` — Major 5, Minor 7, Patch 4 | [read] |
| Slate/SlateCore/WebBrowser source present | `Engine/Source/Runtime/{Slate,SlateCore,WebBrowser}` dirs listed | [read] |
| Proposed module name `CkStyle` collides with existing symbol + filename | `namespace CkStyle` at `Source/CkEditorTools/Public/CkEditorTools/Style/CkStyle.h:32`; `CkStyleSettings.h` exists; module `CkEditorStyle` at `CkFoundation.uplugin:370` | [read] |
| No `*Harness` module precedent in CkFoundation | `rg -l "Harness"` over `Source/*.Build.cs` → 0 hits; test harness = CkTests plugin | [read] |
| Campaign brief located | `C:\Users\Adam\Downloads\CkStyle_CSS-to-UMG_Campaign.md`, read in full | [read] |
| Nothing added to CkFoundation | mis-cloned yoga/litehtml moved out immediately; `git status --porcelain` shows only pre-existing sibling-session paths | [read] |

## WebToUMG (target 1)

| Claim | Evidence | Method |
|---|---|---|
| Listing exists; seller Michel Brito; published 2026-06-19; updated 2026-07-29; $56.03–$112.08; "License terms: Standard License"; UE 5.5–5.7; editor Win/macOS | `fab.com/listings/9a3687aa-26a6-4d88-a841-2769a39e00a9` (read in browser pane 2026-07-31; WebFetch is 403-blocked by Fab) | [web] |
| Output = native WBPs; layouts → UCanvasPanel/VerticalBox/HorizontalBox/UOverlay; gradients/shadows → material instances (3 masters); SVG/effects rasterized; transitions → UWidgetAnimation; forms → USlider/UCheckBox/UComboBoxString; state-capture crawling; reimport preserves Event Graph, overwrites layout edits | listing text, verbatim quotes in PriorArt §1 | ⟨V⟩ |
| No fidelity number claimed | listing verbatim: "We avoid promising a fixed 'pixel-perfect %'…" | ⟨V⟩ (the *absence* of a claim is itself the finding) |
| Requires WebBrowserWidget plugin; "bundled Chromium 90 engine"; ceiling: no color-mix()/:has()/container queries/sticky; single desktop viewport; responsive "on roadmap" | listing Limitations section | ⟨V⟩ |
| C++ source included with purchase? | **UNKNOWN — could not verify.** Settled by: purchasing, or asking seller/Fab support | — |
| Actual conversion fidelity vs §10 | **UNKNOWN — could not verify** without purchase. Settled by: buy + run golden corpus + pixel-diff (recommendation R3) | — |
| Listing claim "WebBrowserWidget… enabled by default in UE 5.5+" is wrong for this engine | `Engine/Plugins/Runtime/WebBrowserWidget/WebBrowserWidget.uplugin:13` — `"EnabledByDefault": false` | [read] |

## StyledWidgets (target 2)

| Claim | Evidence | Method |
|---|---|---|
| MIT license, © 2022 Robert Engeln | `LICENSE:1` + MIT grant text | [read] (re-verified verbatim) |
| Selector matcher = ~40-line tag/id scorer | `Source/StyledWidgets/Private/WidgetStyleBase.cpp:34-72` (`Match` at :34 re-verified) | [agent+spot] |
| Only parser is one-line selector tokenizer; no CSS text syntax anywhere | `WidgetStyleBase.cpp:74-144`; agent grep for lexers → none | [agent+spot] |
| Zero layout functionality | `grep -c "flex\|Flex" -r Source/` → 0 in every file (main-context grep) | [read] |
| Stylesheet = UObject asset, instanced style objects | `Public/WidgetStyleSheet.h:20-42` | [agent] |
| Cascade = ascending specificity sort + reflection copy of `OverriddenProperties` | `WidgetStyleSheet.cpp:43-47`, `WidgetStyleBase.cpp:153-168` | [agent] |
| States = tags flipped by Slate handlers, full subtree re-match | `StyledWidgetBase.cpp:77-131,190-212` | [agent] |
| 5 widget types only; CommonUI required; no EngineVersion field; README's runtime-swap claim unimplemented (getter-only manager) | `.uplugin:17-34`; `WidgetStyleManager.h:13-20`; agent grep for setters → 0 | [agent] |
| HEAD `86bdbec` 2025-01-05 | `git log -1` (shallow — history claims limited to tip) | [agent] |

## Yoga (target 3)

| Claim | Evidence | Method |
|---|---|---|
| MIT; "Copyright (c) Facebook, Inc. and its affiliates." | `LICENSE:1,3` | [read] (re-verified verbatim) |
| Layout entry `YGNodeCalculateLayout(node, availableWidth, availableHeight, ownerDirection)` | `yoga/YGNode.h:77-81` (re-verified verbatim); impl → `yoga/algorithm/CalculateLayout.cpp:2842-2935` | [agent+spot] |
| Measure typedef `YGSize (*)(YGNodeConstRef, float, YGMeasureMode, float, YGMeasureMode)`; modes Undefined/Exactly/AtMost | `yoga/YGNode.h:204-209` (re-verified verbatim); `yoga/enums/MeasureMode.h:18-22` | [agent+spot] |
| Measure invoked before flex logic; inner-size args; skip when both axes exact; leaf-only (fatal assert) | `CalculateLayout.cpp:1791-1808` (:1791 re-verified), `:493-529`, `:500-521`; `yoga/node/Node.cpp:138-143`, `yoga/YGNode.cpp:141-144` | [agent+spot] |
| CSS Grid NOT implemented; style scaffolding exists but layout never reads it | `Display::Grid` at `yoga/enums/Display.h:22`; setters `yoga/YGNodeStyle.h:160-236`; `grep -rn "Display::Grid" yoga/algorithm/ \| grep -v AbsoluteLayout` → **exit 1, zero hits** (re-run in main context) | [agent+spot] |
| `gap` + percentage gap present | `yoga/YGNodeStyle.h:104-107` (re-verified verbatim) | [agent+spot] |
| CSS `order` property absent | grep over `yoga/style/Style.h` + `YGNodeStyle.h` → 0 non-"border" hits (re-run in main context) | [agent+spot] |
| space-evenly, wrap-reverse, aspect-ratio, box-sizing, display:contents present | `yoga/enums/{Justify,Align}.h`, `YGEnums.h:157-160`, `YGNodeStyle.h:156-157,109-110`, `enums/Display.h:21` | [agent] |
| C++20; zero external deps; 19 .cpp / 59 h / 12,365 LOC in `yoga/` | `cmake/project-defaults.cmake:6` (re-verified); agent include-sweep + wc | [agent+spot] |
| Exceptions only in fatal-assert path w/ terminate fallback; no dynamic_cast in core | `yoga/debug/AssertFatal.cpp:18-25`; agent grep | [agent] |
| Stock defaults ≠ web defaults; `UseWebDefaults` fixes | `yoga/YGConfig.h:52-60`, `yoga/node/Node.h:336-339`, `style/Style.h:47` | [agent] |
| Pixel-grid snapping in place post-layout; raw values readable; global generation counter + default-config singleton | `CalculateLayout.cpp:2929-2931,38`, `PixelGrid.cpp:15-64`, `YGNodeLayout.h:33-41`, `config/Config.cpp:130-132` | [agent] |
| HEAD `c766885` 2026-07-24, untagged (`package.json:3` = 0.0.0) | `git log -1` | [agent] |

## litehtml (target 4)

| Claim | Evidence | Method |
|---|---|---|
| BSD 3-Clause, © 2013 Yuri Kobets (tordex); bundled Gumbo = Apache-2.0, PUBLIC link dep | `LICENSE:1-13`; `src/gumbo/LICENSE:1-3`; `CMakeLists.txt:212` | [agent] |
| `document_container` = ~30 pure virtuals (fonts, text width, draw, images, gradients, borders, clip, viewport, media) | `include/litehtml/document_container.h:33-89` (full table in PriorArt §4 / agent report) | [agent] |
| Post-layout render tree is public: layout w/o drawing + walkable boxes + absolute placement + per-node computed styles | `document.h:91,115-116` (:115-116 re-verified verbatim); `render_item.h:497` / `src/render_item.cpp:1387-1410`; `element.h:48` → `css_properties.h:112-252` | [agent+spot] |
| Flex: grow/shrink/basis, wrap(+reverse), full justify/align incl. space-evenly, order — present | `src/flex_item.cpp:10-20,24,161-176` (:10 re-verified); `render_flex.cpp:33,90,115-189,259-296`; `flex_line.cpp:485-563` | [agent+spot] |
| Flex `gap` ABSENT | grep over `css_properties.h` → 0 hits (re-run in main context); repo-wide only gumbo char table | [agent+spot] |
| CSS Grid ABSENT (18-value display enum, no grid) | `css_values.h:79-96`; repo grep → table internals + media-feature comment only | [agent] |
| calc() absent; transforms absent; box-shadow absent (no property, no draw callback); sticky absent | `css_tokenizer.h:37` (comment only); `css_values.h:375-382`; `document_container.h` (no shadow callback) | [agent] |
| var()/custom properties present (delayed parse + cycle guard); media queries present; border-radius present; linear/radial/conic(+repeating) gradients present | `style.cpp:1850-1928` (`subst_var` :1880 re-verified); `media_query.cpp` (867 LOC); `borders.h:50,130`; `gradient.cpp:102-131` | [agent+spot] |
| Text measured per word at style-compute via host `text_width` | `el_text.cpp:86-102`; `document.cpp:378-385` | [agent] |
| C++17; exceptions in encodings prescan; load-bearing `dynamic_cast` in var() walk | `CMakeLists.txt:198-200`; `encodings.cpp` throw/catch ×5 (re-counted); `html_tag.cpp:347` (re-verified verbatim) | [agent+spot] |
| ≈71.5k LOC vendored (litehtml 29.5k src + 8.4k headers + gumbo 33.5k) | agent wc | [agent] |
| HEAD `b9e89f0` 2026-07-28; no release versioning (soname 0.0.0) | `git log -1`; `CMakeLists.txt:14-15` | [agent] |

## Chromium DevTools Protocol (target 5)

| Claim | Evidence | Method |
|---|---|---|
| `DOM.getDocument(depth:-1, pierce)` returns full tree; stable | `chromedevtools.github.io/devtools-protocol/tot/DOM/` | [web] |
| `DOM.getBoxModel` returns content/padding/border/margin quads + width/height; stable | same page | [web] |
| `CSS.getComputedStyleForNode` returns flat `{name, value}` computed list; stable (only `extraFields` experimental); `CSS.getMatchedStylesForNode` available for provenance | `…/tot/CSS/` | [web] |
| `Page.captureScreenshot` png/jpeg/webp + clip; stable (only fromSurface/captureBeyondViewport/optimizeForSpeed options experimental) | `…/tot/Page/` | [web] |

## SWebBrowser / embedding (target 6)

| Claim | Evidence | Method |
|---|---|---|
| CEF page output is a rasterized buffer uploaded to Slate textures (no widgets) | `Engine/Source/Runtime/WebBrowser/Private/CEF/CEFWebBrowserWindow.cpp:1968` (`OnPaint(CefRenderHandler::PaintElementType, DirtyRects, Buffer, W, H)`); `CEFWebBrowserWindow.h:652` (`FSlateUpdatableTexture* UpdatableTextures[2]`) | [read] |
| CEF linked only Win64/Mac/Linux; Android/iOS native webviews; **no console platform in build gating** | `Engine/Source/Runtime/WebBrowser/WebBrowser.Build.cs:94-99` (CEF3 dep), `:31-33` (mobile branch); no console platform identifiers in file | [read] |
| Helper process per browser (`EpicWebHelper.exe`) | `WebBrowser.Build.cs:103-115` | [read] |
| Engine 5.7.4 CEF default = Chromium 128.0.6613 (90 is legacy fallback) | `Engine/Source/ThirdParty/CEF3/CEF3.build.cs:16` (`bUseExperimentalVersion = true`), `:31` (version ternary); both binary drops on disk | [read] |
| `UWebBrowser` is a thin UWidget over SWebBrowser; plugin not enabled by default | `WebBrowserWidget/Public/WebBrowser.h:15,97`; `WebBrowserWidget.uplugin:13` | [read] |
| Tracer WebUI: maintained, UE 4.23–5.8, CEF 128.4.13/Chromium 128, updated 2026-06-21, $70.04–$210.16, accelerated paint, mobile via native webviews | `fab.com/s/67cbc9dcdad6` (browser pane, 2026-07-31) | [web] |
| Tracer console support = "COMING SOON: Xbox and PlayStation support using WebKit" | same listing, verbatim | ⟨V⟩ (roadmap claim; also note WebKit ≠ Chromium fidelity) |

## Gate 1 additions (2026-07-31, session 2)

| Claim | Evidence | Method |
|---|---|---|
| Engine CEF supports CDP remote debugging via `-cefdebug=<port>` (sets `Settings.remote_debugging_port`) | `Engine/Source/Runtime/WebBrowser/Private/WebBrowserSingleton.cpp:343-347` | [read] |
| Extraction is deterministic on smoke page: IR and golden PNG byte-identical across two independent runs (Chrome 150.0.7871.188) | `diff` empty + `cmp` clean, run 2026-07-31 | [read] |
| Unsupported-property diagnostic carries exact file:line (`backdrop-filter` → `smoke.html:25`, matches `sed -n '25p'`) | extractor output vs file content, this session | [read] |
| CkFoundation `.gitignore:49` ignores `*.md` repo-wide; prior campaign docs were force-added (46 tracked files under docs/) | `git check-ignore -v` + `git ls-files docs \| wc -l` | [read] |
| Campaign branch `feature/webumg-campaign` = dev + 2 commits, zero `node_modules`/`corpus/out` files in tree | `git ls-tree -r … \| grep -cE "node_modules\|corpus/out"` → 0 | [read] |

## Slate layout contract (supports Yoga §3 assessment)

| Claim | Evidence | Method |
|---|---|---|
| Desired-size pass is width-unconstrained | `Engine/Source/Runtime/SlateCore/Public/Widgets/SWidget.h:731` — `virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const = 0` | [read] |
| Real geometry arrives only at arrange | `SWidget.h:1659` — `OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren&)` | [read] |
| STextBlock auto-wrap width = previous paint's cached geometry, corrected by invalidation | `Slate/Private/Framework/Text/SlateTextBlockLayout.cpp:235` (`CachedSize` set in `OnPaint`), `:394-399` (wrap width from `CachedSize.X`, comment at :394); `STextBlock.cpp:316-321` (`Invalidate(EInvalidateWidgetReason::Layout)`) | [read] |
