# Yoga Slate authoring campaign

Written: 2026-09-07. Stable mission; current evidence and branches live in [PROGRESS.md](PROGRESS.md).
Retire these campaign documents when the feature ships; retain the module's permanent integration documentation.

## Goal

Make attractive native Unreal tool and game interfaces substantially easier to author and iterate, using Yoga for a defined Flexbox layout subset and Slate for native widgets, rendering, and interaction.

## Locked decisions

- Use Yoga 3.2.1 from the supplied source archive, with unchanged native sources and recorded provenance.
- Integrate the dependency into CkFoundation; keep the future Slate adapter separate from the vendor module.
- Work only in the selected existing checkout. No clones, linked worktrees, or duplicate verification repositories.
- Maintain campaign branches and rebase onto freshly fetched `origin/dev` at phase boundaries. Never merge or push this campaign into `dev` before the entire feature is complete and delivery is authorized.
- Preserve unrelated dirty work. Commit only explicitly scoped campaign paths if committing is authorized; root publication must not reference unavailable plugin commits.
- Authorization now covers the complete shared authoring pipeline, a browser design reference, a production-pipeline resource-inspector test app, all debugger non-graph layouts, and game validation. Git publication remains gated separately.

## Success criteria for the full feature

1. Every existing debugger's non-graph layout is authored through the shared pipeline without feature-specific Slate layout construction, preserving actions, focus, selection, virtualization, and teardown. Graphs may remain registered native widgets.
2. A game menu uses the same runtime-safe adapter and authoring primitives in a packaged build, with controller navigation and localized text.
3. Layout and style can be edited and previewed without recompiling native behavior, with invalid declarations rejected atomically and useful diagnostics.
4. The documented layout subset has measured text, overflow, DPI, sizing, and invalidation contracts and focused automated coverage.
5. Visual quality and authoring effort are evaluated against the original screens; timing and allocation measurements meet budgets agreed before measurement.
6. Final rebased source passes its relevant build, runtime, and visual gates. A dependency smoke test or static mockup alone does not complete this campaign.
7. A complex interactive browser resource-inspector reference is translated into a native test app that loads the production markup and stylesheet through CkSlateLayout. There is no separate test renderer or hand-built Slate replica.
8. A source-backed coverage matrix accounts for every declared control, layout/style feature, binding, reusable component, native extension, interaction, failure path, and lifecycle contract. Each entry links to implementation and appropriate automated or manual evidence. No percentage or passing sample substitutes for missing requirements.
9. Future custom native widgets register typed properties and behavior once; composite widgets are reusable authored components. Invalid registrations and documents fail atomically, with no partial publication or downstream callbacks.

## Non-goals

Full browser HTML/CSS compatibility, JavaScript execution in the native runtime, a browser runtime, and replacement of Slate's rendering/input system. Browser-reference scripting is only a design aid. Debugger behavior and data collection remain native; their non-graph presentation is migrated.

## Read next

[PLAN.md](PLAN.md), [PROGRESS.md](PROGRESS.md), [Plan/Gate_00_Integration.md](Plan/Gate_00_Integration.md), and [Plan/Gate_01_Prototype.md](Plan/Gate_01_Prototype.md).
