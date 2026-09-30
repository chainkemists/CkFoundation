# CkChain

Head-driven follower chains over world-space Transform entities. `PathHistory` places each link
where the head travelled a configured arc distance ago. The head's current pose is the leading end
of that path: a link whose distance falls inside the chord the head has travelled since the newest
recorded sample interpolates between that sample and the head, so links advance every frame the
head moves, not once per recorded sample. `DistanceConstraint` keeps successive
segment lengths and cuts corners. The chain does not create or destroy links, drive the head,
smooth poses, or resolve collisions.

## Composition and requests

`UCk_Utils_Chain_UE::Add(HeadTransform, FCk_Chain_Spec)` creates a lifetime child of the head. Attach existing
Transforms using `Request_AttachLink(Chain, FCk_Request_Chain_AttachLink{Link, FCk_ChainLink_Spec}, Delegate)`.
`Add` unpacks the Spec (`FCk_Chain_Spec::Get_IsValid` gates it): the name becomes the GameplayLabel, a disabled
starting state becomes `FTag_Chain_Disabled` (the one mode a processor view filters on), and `ck::FFragment_Chain_Params`
retains solver, spacing, seed, teleport distance, the normalized up vector and net policy. `FFragment_ChainLink_Params`
retains the link's offset and orientation. `Request_Split` composes the new chain from the source's Params and label; the
new chain takes the source's enable state when the split drains, so an EnableDisable queued ahead of it applies to both.
Until that drain the new chain carries `FTag_Chain_SplitPending`: requests queued on it wait and apply after the split.

`Get_PoseAtDistance` returns `FCk_Chain_PoseAtDistance_Result`: check `Get_IsValid` before reading `Get_Pose`. An
empty history or an uncovered HoldUntilCovered distance is invalid without a diagnostic; an invalid chain, head or
distance, or a DistanceConstraint chain, diagnoses and is invalid. There is no head-pose fallback.

Requests drain in Transform Derived; location/rotation requests then settle through the local
Transform barrier, including each link's SceneNode children. A link cannot be a parent-driven
SceneNode, a static actor root, the head itself, or a member of another chain.

Distances are cumulative centimetres behind the head. The roster is stable on equal distances.
`Request_SetLinkDistance` resorts it. `Request_DetachLink` releases a link's chain fragments and
leaves its pose intact. Destroying the chain releases every link. Destroying the head destroys its
chains through normal child-entity lifetime.
`ck::FFragment_ChainLink_TargetPose` holds the pose the chain last drove a link to: it is added on the link's first
publish and removed on detach, split reassignment and chain destruction, so its absence means the link has not been
driven yet (`Get_HasTargetPose`).

`Request_Split` returns a new, initially empty chain synchronously, headed by the selected link.
On drain the selected link leaves the source roster, later links move to the new chain with rebased
distances/history, and the source fires `OnSplit`. Cancelling the source destroys the pending new
chain. Give the new head a game-owned mover to make a detached train continue travelling.

Request delegates complete once: boundary rejection `Failed_NotEnqueued`, drain failure `Failed`,
completed intent `Succeeded`, owner teardown `Failed_Cancelled`. Disable holds poses and stops
recording history; enable resumes with one chord across the gap. Use `Request_ReseedHistory` when
that gap should become a fresh seeded line. Teleport detection is opt-in through
`_TeleportDistanceCm`; zero leaves jumps as recorded straight segments.

## Ordering and replication

Heads driven before `FGroup_Transform_Derived` produce same-frame link poses. Writes from a later
tail pump converge on the next frame. Do not give Update a head Transform dirty marker or move it
into `FGroup_Transform`. Publish link poses through `Request_SetLocationAndRotation`; never write
Transforms directly from Derived. Link scale is retained.

Two legal configurations:

- `AuthorityOnly`: authority solves; link Transforms may replicate normally to clients.
- `Everywhere`: each machine composes its roster and solves locally; link Transforms must not
  replicate. Replicating links are rejected. Locally observed head history is cosmetic and may
  differ across machines; it is not authoritative hit-validation data.

The roster is not replicated. Recorded history is not persistent: load/rebuild composition
reseeds and reattaches links. No snapshot registrations belong in this module.

## OnChainLinkDetached binder

Consumers that wait on link ownership must bind `OnLinkDetached` and handle `Requested`,
`LinkDestroyed`, `ChainDestroyed`, and `MovedBySplit`. The Venus centipede executor is the intended
game-side consumer for damage/coasting policy; those policies stay outside CkChain.

## Diagnostics and tests

`ck.Chain.DrawHistory` and `ck.Chain.DrawLinkTargets` default off and mirror per-user Chain Debug
settings. The Chain gym in CkTests demonstrates a figure-eight train, a rope on the same loop (P chases the pawn instead), held
authored poses, and wagons with SceneNode lamp children. Kernel automation names are
`Ck.Chain.PathHistory.*` and `Ck.Chain.DistanceConstraint.*`; integration scripts are
`CkAutoTest_Chain_*`. Interactive observation is required to judge the rendered gym.

## Boundaries

The ordered roster is a TArray, never a Record. The head-to-chains Record is unordered. Geometry
lives in the world-free kernel. Debug UI lives in CkGameplayDebugger; this module has no dependency
on it, CkSpline, or CkTween. SceneNode children are allowed on links; SceneNode-driven links are not.

History records only chords at least SampleSpacingCm long. A stopped head can leave a final
sub-spacing chord unrecorded until it moves farther; links still follow that chord through the
head's current pose. Sample-spacing changes recreate the train
fixture in the gym.
