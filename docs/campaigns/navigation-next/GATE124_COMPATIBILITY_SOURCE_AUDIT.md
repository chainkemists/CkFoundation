# Gate 124: BusterBlock compatibility source audit

**Scope.** This is a read-only source contract for the existing
`D:\Repos\BusterBlock` checkout, recorded before any compatibility migration.
Paths below are relative to that BusterBlock root unless explicitly stated
otherwise. It establishes source facts only; it does not prove editor, cooked,
packaged, visual, performance, or runtime behavior.

**Runtime baseline status.** The separate unchanged-game `NpcAI` baseline completed
22/27, exit 1, one editor boot. Exact named failures, diagnostics and identities are
in `GATE124_RECAST_BASELINE_EVIDENCE.json` and PROGRESS; source claims below remain
distinct from those focused runtime results.

## Recast baseline

The game explicitly configures `RecastNavMesh` with cell height `10`, agent
radius `45`, agent height `144`, and dynamic runtime generation in
`Config/DefaultEngine.ini:413-467`. Production NPC CrowdAgents use a different
movement capsule: radius `42`, height `192` in
`Script/Npc/BB_Npc_Feature.as:1361-1373`. Any provider A/B fixture must preserve
the actual game profile for its corresponding layer; it must not substitute the
host benchmark capsule.

The existing sidewalk detector directly requires the default navigation data to
cast to `ARecastNavMesh`:

- `Source/BusterBlock/AI/Navigation/Bb_SidewalkPathNetworkDetector.cpp:2219-2233`
  rejects a missing default Recast surface while constructing its evaluator.
- `...:2394-2429` returns a shaped empty mask when required backing is absent.
- `...:3012-3038` fails generated-ribbon validation without it.
- `...:3324-3369` projects candidate surfaces through that Recast data and
  admits them only within 50 cm planar and vertical deltas.

Therefore a provider-neutral navigation selection does not make the current
sidewalk extraction provider-neutral. Recast backing remains required for this
game-owned source path until an explicitly designed bridge changes it.

## Query-filter contract

All eight game tags are registered in
`Config/DefaultCkFoundation.ini:122-126`. The filter constructors are in
`Source/BusterBlock/AI/Navigation/BB_NavQueryFilters.cpp:8-75`; their public
contract and strict-pair rationale are in the adjacent header at
`BB_NavQueryFilters.h:10-112`.

| Tag | Filter class | Excluded area classes | Cost override |
| --- | --- | --- | --- |
| `Nav.Filter.Customer` | `UBb_NavFilter_Customer` | `UCk_NavArea_Restricted` | None |
| `Nav.Filter.Customer.Strict` | `UBb_NavFilter_Customer_Strict` | `Restricted`, `UCk_NavArea_CrowdAgent` | None |
| `Nav.Filter.CustomerShopping` | `UBb_NavFilter_CustomerShopping` | `Restricted`, `UBb_NavArea_CustomerShoppingBoundary` | None |
| `Nav.Filter.CustomerShopping.Strict` | `UBb_NavFilter_CustomerShopping_Strict` | `Restricted`, shopping boundary, `CrowdAgent` | None |
| `Nav.Filter.Employee` | `UBb_NavFilter_Employee` | None | None |
| `Nav.Filter.Employee.Strict` | `UBb_NavFilter_Employee_Strict` | `CrowdAgent` | None |
| `Nav.Filter.Goon` | `UBb_NavFilter_Goon` | None | None |
| `Nav.Filter.Goon.Strict` | `UBb_NavFilter_Goon_Strict` | `CrowdAgent` | None |

"None" in the cost column means no game filter cost override; exclusions are
listed separately. The underlying `UCk_NavArea_CrowdAgent` constructor sets
`DefaultCost = 64.0f` (`Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_NavArea.cpp:7-9`),
so permissive traversal is not uniformly cost 1. Restricted and shopping-boundary
constructors only set drawing colors and inherit their base costs. The strict
variants exclude stationary crowd for the two-phase crowd planning pass.

## Area ownership and route callers

`Script/ECS/AccessZone/BB_AccessZone_EntityScript.as:1-4,58-80` owns
`UCk_NavArea_Restricted` markup for `DenyCustomerEntry`. This blocks Customer
and CustomerShopping filters; Employee and Goon filters traverse it.

`Script/ECS/Entryway/BB_Entryway_Processor_ShoppingBoundary.as:1-4,72-139`
owns a separate dynamic `UBb_NavArea_CustomerShoppingBoundary` markup for each
enabled Entryway. It is independent of ordinary entry permission, covers the
outside portal half from the seam, and is cleaned up on destruction/end play
(`...:19-52,142-164`). Only CustomerShopping filters exclude it.

At NPC construction, `Script/Npc/BB_Npc_EntityScript.as:170-207` assigns
Customer/Customer.Strict by default, Employee pairs for employees, and Goon
pairs for RentNet goons. `Script/Npc/AI/BB_NpcAI_Shopping.as:317-403` changes
customer/tourist policy for an entire shopping trip rather than one request:
CustomerShopping applies after ingress while egress is not authorized;
Customer applies in transit and for terminal egress. A forced change asks the
CrowdAgent to replan its active route.

Queue-owned movement is relevant to the compatibility contract. Checkout and
shelf paths deliberately stop generic locomotion from issuing another MoveTo:
the CkQueue adapter owns assignment, MoveTo, retry, and facing in
`Script/Npc/AI/BB_NpcAI_Sm.as:1042-1103`. Those routes therefore consume the
agent's active filter. Checkout service additionally requires both queue-front
state and counter readiness in
`Script/Npc/AI/BB_NpcAI_SmTask_StoreLink.as:1443-1460`.

Routine NPC locomotion may opt into exactly one built sidewalk PathNetwork, but
missing, unbuilt, ambiguous, or foreign-owned followers retain ordinary MoveTo
behavior (`Script/Npc/AI/BB_NpcAI_Sm.as:1386-1448`). This is separate from the
direct-Recast sidewalk detector prerequisite above.

## Existing focused evidence candidates

Exact game functional-test names discovered from the current test cache and
source are:

- `Project.Functional Tests.BusterBlock.Map.AutoTests.AutoTests_BB_MAP.Bb_AutoTest_NpcAI_SidewalkForeignFollower`
- `Project.Functional Tests.BusterBlock.Map.AutoTests.AutoTests_BB_MAP.Bb_AutoTest_NpcAI_SidewalkOwnedRoute`
- `Project.Functional Tests.BusterBlock.Map.AutoTests.AutoTests_BB_MAP.Bb_AutoTest_NpcAI_SidewalkProviders`
- `Project.Functional Tests.BusterBlock.Map.AutoTests.AutoTests_BB_MAP.Bb_AutoTest_PathNetworkDetector_SurfaceMask`

`Saved/UnrealToolbox/TestBatchCmds.txt` is the existing Toolbox command cache.
`--test-pattern=NpcAI_Sidewalk` is the focused selection for the three sidewalk
tests; `--test-pattern=PathNetworkDetector_SurfaceMask` selects the detector
companion. The source tests live at
`Plugins/BusterBlockTests/Script/Tests/NpcAI/` and
`Plugins/BusterBlockTests/Script/Tests/PathNetwork/` respectively.

The Ck framework sibling `Ck_AutoTest_Crowd_NavQueryFilter_ForceReplan`
(`Plugins/CkTests/Script/CkCrowd/CkAutoTest_Crowd_NavQueryFilter_ForceReplan.as:6-15`)
checks policy-only replanning and same-batch policy-plus-MoveTo ordering. It is
useful compatible coverage but does not prove BusterBlock's eight mappings.

## Coverage boundary

The present source/test inventory does **not** prove, at runtime, that every
mapping resolves, that each role observes its intended AccessZone behavior, or
that Entryway ingress, shopping lock-in, authorized egress, strict crowd
avoidance, and queue routing all remain equivalent. It also does not prove a
GroundNav route, a cooked asset/config registration, package behavior, or a
matched performance result. Those are explicit Gate 124 follow-up evidence
requirements, not conclusions from this audit.
