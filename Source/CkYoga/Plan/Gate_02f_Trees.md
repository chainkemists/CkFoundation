# Shared authored trees

Next control family after Resource Inspector host lifecycle gate. Moving evidence belongs in ../PROGRESS.md.

## Model and publication

Use a dedicated typed tree collection with stable nonempty keys, optional parent key and typed fields. Keep ordinary table collections flat. Reuse the existing field schema/value contract and validation where practical; do not publish hierarchy as a listener repair after records change.

Validate the complete candidate before mutation: duplicate/empty keys, absent parents, self-parenting, cycles, invalid typed fields, node and depth limits. Publish record values, roots, child lists, lookup map and one revision together, then issue one notification. Reject reentrant publication. Preserve shared node identity for retained keys and input-order sibling ordering. Invalid candidates leave revision, records, topology and callbacks unchanged.

## Native view and authoring

Use STreeView for native virtualization, indentation, expanders and keyboard navigation. Author the row content through production child views, reusing table field binding/staging contracts; do not author the whole navigation pane in bespoke Slate. Expose typed tree binding and selected/expanded stable-key state. Retain native widget identity on compatible reload.

Filtering includes matching nodes and their ancestors. Effective expansion may expose filtered matches without overwriting user expansion choices; clearing a filter restores those choices. Selection follows stable key and the documented visibility policy, never a neighboring index. Topology changes must request a native tree refresh even when node pointers survive. Guard callbacks caused by programmatic refresh and preserve atomic outer document publication.

## First consumers and verification

Resource Inspector navigation is the first consumer. Existing follow-on needs include Scheduler ProcessorTree, ECS EntityTree hierarchy/group/filter presentation, and SaveDebugger entity/value trees. Single-column rows are the first increment; multicolumn trees and heterogeneous value/actions remain required wherever the debugger inventory needs them.

Prove valid nested records; atomic invalid input; cycle/depth rejection without recursive overflow; subtree moves/removal; filter/expansion/selection retention; real keyboard expansion/navigation; bounded row count at10k nodes; valid/rejected reload identity/focus; and model/view owner release. Inspect native narrow/wide app captures. These tests do not substitute for migrating actual debugger consumers.
