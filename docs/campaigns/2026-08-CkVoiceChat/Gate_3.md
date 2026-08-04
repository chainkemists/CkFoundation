# Gate 3 — Relay transport, routing, channels, control-plane replication (P3)

> **Status:** 🟡 In progress (opened 2026-08-03)
> **Depends on:** Gate 2 machine portion ✅ (audit GO WITH CONDITIONS — resolved, 5fa775d0b).
> The two Gate-2 HUMAN items (mic `[EDITOR-VERIFY]`, N5 packaged smoke) remain OPEN as
> **P2-verification obligations**: they gate the campaign's ship (P5), not this gate's transport
> work — encoded bundles move the same regardless of how they sound; a mic-test failure re-opens
> P2's audio path, not P3. Decision made at open (2026-08-03) interpreting the maintainer's
> "continue"; revert hook: say "wait for the human items" and P3 pauses at the next commit.
> **Estimate:** 2–4 sessions — the campaign's largest phase. Re-date at each session.

## Goal

After this gate: a talker on one client speaks and a nearby listener on ANOTHER client hears it —
frames ride Unreliable relay-actor RPCs client→server, the server routing processor re-validates
and forwards them only to in-range/member listeners under the byte budget and top-N cap, and the
receiving client decodes through the P2 jitter/synth machinery attached at the talker's location.
Positional3D (probe-driven proximity) and Global2D (membership-driven) channel policies both work.
Late joiners get the control plane before any audio; voice arriving before the registry is dropped
and counted, never stashed (N1).

## Binding constraints carried into this gate

- **N1 (standing condition of the green-light):** unresolvable-`ChannelIdx` packets are DROPPED,
  counted in a stat, NEVER stashed; a voice-before-registry net test proves it.
- **N2:** packet-path validation per non-negotiable #3 — hoisted validity local, ensure with empty
  body (fires once per site), separate always-on ordinary drop branch, per-talker attribution via
  throttled `ck::voice_chat` Warning + stat counter.
- **N3:** `ChannelIdx` u8 registry indices are session-append-only; ensure on wrap past 255.
- **N7:** routing-policy matrix table (per-policy send-set formula) lands in the module Claude.md.
- **S1:** NO reliable RPCs on the voice relay actor, ever (reliable attachments on the same object
  head-of-line-block the unreliable section — spike-proven).
- **S2:** pacing bounds FRESHNESS, not just bytes — drop stale frames server-side BEFORE enqueue
  (the transport queues ~4096 bundles silently; latency replaces loss otherwise).
- **S3:** bundle ≤ 3 × 20 ms frames AND serialized RPC < 256 B (the server→client unreliable
  split threshold makes bigger payloads all-or-nothing multi-part).
- **S4:** teardown/travel windows stop sends BEFORE destroying channel actors (RPCs at an
  unresolvable object = unthrottled `LogIrisRpc` Errors client-side).
- **S5:** measure the per-connection drain budget under production net config at this gate; set
  the default voice byte budget below it with headroom. Numbers recorded here before the claim.

  **S5 MEASURED (2026-08-03, `Ck.VoiceChat.Net.DrainBudgetMeasure`, Test-VoiceChatP3-drain2.log):**
  burst of 2000 × 240 B (480,000 B) queued in one server call through `Client_ReceiveVoiceBundle`;
  arrivals counted at the client RPC boundary (pre-inbox-cap seam added for this measurement).
  Mark 1 (30 ticks): 87 bundles / 20,880 B. Mark 2 (+60): 264 / 63,360 → **708.0 B/tick
  (2.95 bundles/tick)**. Mark 3 (+60): 441 / 105,840 → **708.0 B/tick** again — slope identical
  across both windows, queue still draining at mark 3 (441 < 2000), so this is the SATURATED
  drain ceiling, not noise. Config caveat: editor net-PIE (localhost, Iris, project
  DefaultEngine.ini rates) — a packaged/dedicated server under real WAN throttles may differ;
  re-measure there before ship-tuning (the spec stays in the suite as the re-measurement tool).
  **Default set from these numbers:** `MaxVoiceBytesPerConnectionPerTick` 4096 → **640**
  (pre-measurement optimism corrected): 640 ≥ the 8-talker steady worst case
  (8 × 240 B ÷ 3.6 ticks ≈ 533 B/tick, ~20% margin) yet ~10% UNDER the measured drain, so
  sustained overage drops at the counted budget layer (S2) instead of silently queueing —
  the spike's exact pathology. Note the spike's ~850 B/tick drain estimate was HIGH; the real
  ceiling (708) sits below its "~716 B/tick needed for 8 talkers," which is why the budget, not
  the transport queue, must be the binding constraint.
- **Gate-2 audit carry-forwards:** F3 — create the loopback synth only AFTER capture `Start()`
  succeeds; F6 — `_AmplitudeQ8` last-frame-hold/decay instead of zeroing on frameless ticks
  (matters now: amplitude feeds the top-N fairness clamp).
- **Amplitude fairness (CTO):** accept self-report v1 + server-side per-talker envelope clamp
  with decay + least-recently-served tiebreak at top-N saturation; document as known v1 limit.
- **Dep budget (N4):** this gate earns `CkActorRelay` (relay actor + group subsystem),
  `CkSpatialQuery` (routing probe), and engine deps ONLY as code consumes them. Still never
  `CkRelationship`.
- **Mute privacy property:** listener mute replicates upstream as a routing EXCLUSION — the
  server stops forwarding; the client never receives what it muted.

## Entry criteria (run 2026-08-03 at open)

- [x] Gate-2 exit re-verified on current HEAD: CkFoundation `feature/voice-chat` @ 5fa775d0b,
      CkTests @ 5a0141c; trees clean; nothing pushed.
- [x] Baseline captured: VoiceChat **18/18** on the post-counters binary
      (Test-VoiceChatP2-statscounters.log, lanes EXIT 0, 0 AS errors) + RenderTarget **22/22**
      (Test-P2-Regression.log). Every later "no regressions" diffs against these names/counts.
- [x] Exemplar shapes re-read at open (not from memory): `ACk_RenderTargetRelay_UE`
      (CkRenderTargetRelay_Actor.h — RPC surface + server-side sender stamping note),
      `UCk_RenderTargetRelay_Subsystem_UE` (subsystem = group tag + actor class, base auto-spawns
      per-player at PostLogin), pacing budget drain (CkRenderTarget_Processor.cpp:1534),
      `Register_NetOnly` contract (root CLAUDE.md persistence-handler section; exemplar
      CkTeam_Fragment.cpp).

## Work items (sequenced; each names its exemplar or is flagged NEW)

1. **Transport skeleton:** native group tag `ActorRelay.VoiceChat`; `ACk_VoiceChatRelay_UE`
   (`Server_SendVoiceBundles` / `Client_ReceiveVoiceBundles`, both **Unreliable**, enqueue-only
   bodies, sender stamped server-side from the channel owner — pattern
   `CkRenderTargetRelay_Actor.h` inverted to Unreliable per S1) + `UCk_VoiceChatRelay_Subsystem_UE`
   (pattern `CkRenderTargetRelay_Subsystem.h`, verbatim shape). Inbox/outbox fragments on the
   transient/world entity. Wire payload = P1 `Pack_Bundle` bytes + talker `FCk_Handle` +
   `ChannelIdx` in the signature.
2. **Control plane:** channel registry (tag → u8, session-append-only + wrap ensure per N3, with
   policy/range snapshot) + membership requests on VoiceChannel
   (`Request_Join/Leave/SetMemberFlags/ServerMute/ServerUnmute`, full completion contract per
   CkTimer) + `FCk_RepData_VoiceChat` registered `Register_NetOnly`, `NetApply` NotReady until
   the voice features compose (exemplar CkTeam_Fragment.cpp; four named shapes at
   `CkPersistenceHandlerRegistry.h`).
3. **Talker outbound:** Capture processor pushes encoded frames into an outbound queue;
   per-tick `Select_BundlesToSend` (P1, already unit-tested — stale-drop BEFORE budget per S2)
   → `Server_SendVoiceBundles` on the owner's relay channel. S3 enforced at bundle build.
   **F3 reorder and F6 amplitude-hold land here** (talker-side fixes, same files).
4. **Server routing processor** (`FProcessor_VoiceChat_Route`, AuthorityOnly — the framework
   host-gates it, no body-gate needed): drain inbox → N2-shaped validation (talker resolvable,
   membership, mode, server-mute, **ChannelIdx resolvable — N1 drop+count**) → send-set by
   policy → sender exclusion (never forward back to origin; listen-server host injects into the
   inbox directly, no self-RPC) → MaxAudibleSpeakers top-N with envelope-clamped amplitude +
   least-recently-served tiebreak → per-connection byte budget + freshness pacing (clause (d) +
   S2) → `Client_ReceiveVoiceBundles`. NEW INFRASTRUCTURE — the schedule risk lives here.
5. **Proximity routing set (Positional3D):** persistent probe, event-driven overlap set with
   hysteresis margin (ADR-6; filter direction `ProbeName.MatchesAny(Filter)` —
   CkSpatialQuery/CLAUDE.md:47). Global2D = membership set, no probe. NEW in this module; probe
   composition mimics existing CkSpatialQuery adopters.
6. **Client receive path:** relay inbox → per-talker jitter buffer + persistent decoder + synth
   attached at the talker entity's transform (generalize the P2 loopback machinery from
   `_Loopback*` fields into a receive path usable for any remote talker) + VoiceListener
   processors (mute/volume application at playback; `Get_AudibleTalkers`).
7. **Listener mute upstream:** `Request_MuteTalker/UnmuteTalker` → replicated exclusion → server
   stops forwarding (net-tested: muted stream STOPS ARRIVING, not just stops playing).
8. **Teardown/travel (S4):** talker EndPlay flushes the routing entry and stops sends before the
   relay/channel actors go; explicit teardown-mid-transmit net coverage.
9. **S5 measurement:** production-config drain budget measured (numbers in this doc) → default
   `PerConnectionByteBudget` set with stated headroom.
10. **N7:** routing-policy matrix into `Source/CkVoiceChat/Claude.md`.

Net tests land WITH their work item, not batched at the end: late-join + voice-before-registry
(N1) with item 2; forward-in-range/hysteresis + sender exclusion with items 4–5; host asymmetry
(host talks / host hears / client↔client) with item 6; mute-stops-forwarding with item 7.

## Expected observations at the gate — and what to do on each branch

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| Two-world net test: client A talks, client B in range | B's talker entity accumulates decoded PCM (Debug seam); seq contiguous ± jitter reorder | Nothing arrives | Stage-isolate: inbox count (client→server leg) vs outbound count (routing) vs client inbox (server→client leg) — the counters from item 1 exist for exactly this |
| Voice-before-registry test (N1) | Bundles dropped + `FramesDroppedUnresolvable` counter rises; ZERO stash; registry arrival does not resurrect old frames | Frames stash or apply late | That's the N1 violation the CTO named — fix before anything else lands on top |
| Overdrive test (talkers > budget) | Freshness pacing drops OLDEST server-side; mouth-to-ear stays bounded; drop counters rise | Latency grows unbounded | S2 regression — the spike's exact pathology; pacing is dropping in the wrong place (transport queue instead of pre-enqueue) |
| Listen-server host paths | Host-talks / host-hears / client↔client all pass; host never self-forwards | Host echo or deaf host | Check the direct-inbox injection path (no self-RPC) and sender exclusion — the two host asymmetries the spec calls out |
| Mute test | Muted talker's bundles stop ARRIVING at the muting client | Bundles arrive, playback silent | Privacy property violated — exclusion must gate the server send-set, not client playback |
| `stat CkVoiceChat` during net PIE | Route/drop/forward counters move | Counters flat while audio flows | Stats wired to dead code — instrument the real path |

## Exit criteria — machine portion complete 2026-08-03

- [x] Net tests green: late-join + voice-before-registry/N1 (`Routing_ForwardsAndNeverStashes`,
      `ControlPlane_Replicates`), host asymmetry (`HostHearsClient`, `ClientToClient`, host-talks
      in the routing spec), in-range routing + hysteresis (`ProximityRouting`, 4-phase), sender
      exclusion (no-echo asserts in `ClientToClient` + `RouteRejections`), mute-stops-forwarding
      (`ListenerMute_StopsForwarding`), teardown mid-transmit (`TeardownMidTransmit`).
      **Top-N culling: the selection policy is unit-pinned (`Test_VoiceChat_TopN`: cap,
      cross-bucket wins, LRS rotation) and the flush path runs in every forwarding spec, but a
      NET test with >8 concurrent talker connections was not built — the harness practically
      hosts 3 worlds. Recorded as a known coverage bound for the audit, not claimed.**
- [x] Full VoiceChat pattern green + RenderTarget delta-zero on the final binary:
      **VoiceChat 30/30** (Test-VoiceChatP3-exitsweep2.log, EXIT 0, 0 AS errors) +
      **RenderTarget 22/22** (Test-P3-RenderTarget-exitsweep2.log) — RenderTarget run with NO
      rebuild after the VoiceChat run, same binary; no source edits between build and either run.
      Entry baseline was VoiceChat 18/18 + RenderTarget 22/22 → +12 new tests, zero regressions.
- [x] S5 numbers recorded above (708 B/tick saturated drain); default budget 4096 → 640,
      justified against them.
- [x] N7 matrix in module Claude.md; module doc updated for the P3 surface (deps, fail-closed
      proximity rule, probe-read pattern).
- [x] Invalid-input test per new validation boundary: forged talker handle + non-member send
      (`RouteRejections`, with positive control), unresolvable ChannelIdx/N1 (routing spec),
      malformed + out-of-contract pack (`CkTests.UnitTests.CkVoiceChat.Wire.*`; oversize is
      unbuildable send-side — the S3 guard at bundle build — and malformed-reject covers the
      receive boundary).
- [x] AS boot clean: 0 `Angelscript: Error` in every exit-sweep log.
- [x] PROGRESS.md dated entries; **audit requested 2026-08-03 — response appended below when it
      lands.**

## Gate-3 top-tier audit — 2026-08-03

Fresh-context adversarial audit; every claim below is grounded in a file:line or log line I read
myself. Nothing was taken from the executor's narrative on trust.

### Verdict: **GO WITH CONDITIONS** for opening P4

The transport/routing/control-plane machinery is real, house-conformant, and the run evidence is
authentic (logs, mtimes, and name-level diffs all check out). The conditions are proof-strength
gaps in two net specs — the *code* for both properties exists and reads correct; the *tests*
assert one layer too far downstream to prove what the gate text claims they prove — plus one
unrecorded deviation from N2's letter. None require re-opening the design. Conditions 1–3 below
land as the first P3-followup commits (or the first P4 commits); they are small, mechanical, and
use a seam that already exists.

### Findings (ranked)

**F1 — HIGH (proof gap, not a code defect): the mute-privacy spec cannot distinguish the server
exclusion from the local defense-in-depth drop.**
`Ck.VoiceChat.Net.ListenerMute_StopsForwarding` asserts only decoded PCM
(`Debug_Get_LoopbackDecodedPcm`, CkTests `Net/CkVoiceChat_ListenerMute.spec.cpp:129,258-259,297`).
But the muting client's local gate (`CkVoiceTalker_Processor.cpp:628-640`) resets arriving bundles
for muted talkers BEFORE decode — so "decoded bytes frozen" holds even if the server kept
forwarding, and unmute reopens the local gate too. The binding constraint ("the client never
receives what it muted", tested as "STOPS ARRIVING, not just stops playing") is therefore not
proven by this spec, only made plausible. The server-side exclusion IS implemented
(`CkVoiceChat_Route_Processor.cpp:518-522` via `Get_IsMutedByRecipient`, :104-119), and the right
seam exists and is already used by two sibling specs: `Debug_Get_ReceiveArrivedBundles` counts at
the client RPC boundary before any cap or gate (`CkVoiceChatRelay_Actor.cpp:79-80`).
**Required action (Condition 1):** add an arrival-counter freeze assert across the muted spurt
(and growth across the unmuted one) to the mute spec.

**F2 — MEDIUM (proof gap): the N1 never-stash assert was silently weakened by the item-6 spec
"upgrade."** The original spec (CkTests `5c8930a`) asserted the client `ReceiveInboxNum`; the
current one asserts decoded-PCM equality (`Net/CkVoiceChat_Routing.spec.cpp:285-287`). A stashed
bundle resurrected after idx allocation would carry seq 1 against a client playhead near ~40 and
be late-dropped by the jitter buffer before decoding (exactly the behavior pinned by
`Jitter.ReorderConcealLateDrop`), so a stash could false-pass the equality. Mitigation: I read the
route path end-to-end — the unresolvable-idx branch drops and counts with no stash container
anywhere (`CkVoiceChat_Route_Processor.cpp:383-392`; the inbox is drained/reset at :352-353), so I
believe the property holds; the spec just doesn't prove it. Also a brittleness nit: `FutureIdx = 2`
is hardcoded (spec :238) — nothing asserts it is actually unresolvable at inject time.
**Required action (Condition 2):** snapshot `Debug_Get_ReceiveArrivedBundles` at the
forward-assert and re-assert equality at the never-stash assert; assert
`TryGet_ChannelByIdx(..., FutureIdx)` is invalid at inject time.

**F3 — MEDIUM-LOW (unrecorded deviation from N2's letter): the "throttled per-talker
`ck::voice_chat` Warning" does not exist.** N2 (this doc, :27-28) specifies per-talker attribution
via throttled Warning + stat counter. As built, the packet-path drops carry: ensure-once-per-site
(talker named in the message) + stat + `VeryVerbose` (`CkVoiceChat_Route_Processor.cpp:369-378,
399-409`); the spoof drop path has no per-event log at all after the single ensure fire, so a
*second* offending talker is visible only as a counter increment. The relay boundary comment even
delegates the job ("per-talker throttled attribution is the Route processor's job",
`CkVoiceChatRelay_Actor.cpp:26`) — and the Route processor doesn't do it. The only three
`Warning` calls in the module are RPC-boundary sender-resolution failures. This may well be an
acceptable engineering call (ensure + stat is arguably better than a throttled Warning), but it
deviates from a binding constraint and was never recorded as a deviation.
**Required action (Condition 3):** either implement a throttled per-talker Warning at the
spoof/malformed drop sites, or record the substitution as a numbered deviation in PROGRESS.md for
maintainer sign-off.

**F4 — LOW (doctrine): process-breadcrumb comments in shipped code.** Root CLAUDE.md bans comments
naming a Gate/Phase/PROMPT/campaign/review. Present: "amendment S1"
(`CkVoiceChatControlRelay_Actor.h:16`), "the campaign PROMPT (S1-S5)" (`CkVoiceChatRelay_Actor.h:19`),
"campaign amendment S2" / "amendment S3" (`CkVoiceTalker_Processor.cpp:55,59`), "review N1" / "the
CTO amplitude-fairness ruling" (`CkVoiceChat_Route_Processor.h:50,83-84`), "review N6" / "measured
cheap at P1" (`CkVoiceChatSynth_Component.h:41,14`), plus spec-§ and ADR references. The technical
*why* content in these comments is load-bearing and should stay; the campaign/review labels are the
breadcrumbs. Acceptable to defer the strip to the P5 module-doc pass — recording it here so it
isn't lost.

**F5 — LOW (slow leak): unbounded transient-entity maps.**
`FFragment_VoiceChat_ServeHistory::_LastServedFrame` (outer key weak `APlayerState`, inner key
talker handle — written at `CkVoiceChat_Route_Processor.cpp:568,622`) and
`FFragment_VoiceChat_ListenerMuteMatrix::_MutedByPlayer` (`CkVoiceListener_Processor.cpp:197,235`)
never prune entries for departed players or destroyed talkers. Harmless at session scale, but a
long-lived server with churn accumulates stale weak keys and dead handles. P4 hygiene item, not
gate-blocking.

**F6 — INFO (doc drift).** (a) Work item 1 above names `Server_SendVoiceBundles` /
`Client_ReceiveVoiceBundles` (plural); the RPCs are singular (`CkVoiceChatRelay_Actor.h:33,41`).
(b) The `Source/CLAUDE.md` tier-table row for CkVoiceChat still lists the P0 dep set; Build.cs now
carries `+ActorRelay, +Shapes, +SpatialQuery` and engine `AudioMixer/GameplayTags/NetCore/Voice/
DeveloperSettings` — the row's own parenthetical anticipates this, so it's a docs fix at the next
tier-table sweep, not a code fix. The module Claude.md dep list is accurate.

### Claims verified (exact evidence per exit criterion)

- **VoiceChat 30/30, EXIT 0, 0 AS errors** — read `Test-VoiceChatP3-exitsweep2.log` summary block
  (`Total: 30 / Passed: 30 / Failed: 0 / Contaminated: 0`), all five `**** TEST COMPLETE. EXIT
  CODE: 0 ****` lines (3 lanes + serial + net), `grep -c "Angelscript: Error"` = 0,
  `grep -c "Result={Fail"` = 0.
- **RenderTarget 22/22 delta-zero** — read `Test-P3-RenderTarget-exitsweep2.log` summary
  (`Total: 22 / Passed: 22 / Failed: 0`), EXIT 0 ×5, 0 AS errors; name-level `diff` of Success
  paths vs the entry baseline `Test-P2-Regression.log` → **identical name sets**.
- **Same binary, correct ordering, no post-build edits** — binaries
  `BusterBlockEditor-CkVoiceChat.dll` 16:19:17 / `-CkTests.dll` 16:19:35; VoiceChat sweep ran
  20:20:26–20:23:03 UTC (file mtime 16:23:04 EDT); RenderTarget sweep ended 16:26:48 EDT; no
  binary anywhere newer than 16:19:35; `find -newermt` over `Source/CkVoiceChat` and CkTests
  `Source/` returned nothing newer than the build.
- **+12 new tests, zero regressions** — name diff of the 18 baseline Success paths
  (`Test-VoiceChatP2-statscounters.log`) against the 30: all 18 present, exactly 12 new
  (11 `Ck.VoiceChat.Channel/Net.*` + `TopN.CapEnvelopeAndRotation`).
- **S5 numbers** — `Test-VoiceChatP3-drain2.log:7559-7561` prints mark1 87/20,880 B, mark2
  264/63,360 → 708.0 B/tick, mark3 441/105,840 → 708.0 B/tick — verbatim match to §S5 above;
  default 640 at `CkVoiceChat_Settings.h:29` with the 708-citing why-comment.
- **N1 enforcement** — drop+count, no ensure, `VeryVerbose` at
  `CkVoiceChat_Route_Processor.cpp:383-392`; no stash container exists on the route path.
- **N2 shape** — hoisted local + empty-body ensure + separate ordinary drop branch at both
  packet-path ensure sites (:368-378, :399-409). Throttled-Warning component missing (F3).
- **N3** — wrap ensure at `CkVoiceChannel_Processor.cpp:117-125` (session-append-only rationale in
  the message); registry entries only ever `Emplace`d; channel EndPlay removes nothing; a stale
  idx resolves to nothing via the `IsValid(Entry.Get_Channel())` check
  (`CkVoiceChannel_Utils.cpp:339-340`).
- **S1** — `ACk_VoiceChatRelay_UE` has exactly two RPCs, both `Unreliable`
  (`CkVoiceChatRelay_Actor.h:31,39`); the reliable `Server_SetMutedTalkers` lives on the separate
  `ACk_VoiceChatControlRelay_UE` (`CkVoiceChatControlRelay_Actor.h:30`) with the S1 rationale in
  its class comment.
- **S2** — stale-drop before budget/send via the P1-unit-tested `Select_BundlesToSend`
  (`CkVoiceTalker_Processor.cpp:149-160`, `MaxOutboundFrameAge` 0.15 s at :56); relay inboxes
  capped at 256 with drop+count (`CkVoiceTalker_Fragment.h:156`, relay actor :48-52, :84-88).
- **S3** — `MaxPackedBundleBytes = 240` (:60) + `FramesPerBundle` clamp 1..3 (:210) enforced
  incrementally at bundle build (:263-265); out-of-contract pack rejects at
  `CkVoiceChat_Codec.cpp:226` (`Wire.PackRejectsOutOfContract` green).
- **F3/F6 carry-forwards** — synth created only after `Start()` succeeds
  (`CkVoiceTalker_Processor.cpp:384-393`, with the why-comment); amplitude last-frame-hold
  (:575-581).
- **N7** — routing-policy matrix present in `Source/CkVoiceChat/Claude.md` (gate order,
  per-policy recipient set, fail-closed rule, listen-host row) including the NPC-sender v1 gap
  stated in the table, plus the probe-read anti-pattern paragraph.
- **Control plane** — `Register_NetOnly` (`CkVoiceChat_Replication.cpp:16-22`) delegating to
  `Apply_ReplicatedControlPlane` with NotReady-before-ANY-mutation (`CkVoiceChannel_Utils.cpp:
  357-362`), client registry mirror upsert (:415-431); pushed at idx-assign and every successful
  request (`CkVoiceChannel_Processor.cpp:133,162`).
- **RouteRejections** — non-member and forged-sender negatives both asserted at the ARRIVAL
  counter with a positive control through the identical seam plus a no-echo assert
  (`Net/CkVoiceChat_RouteRejections.spec.cpp:227,265,304-309`). This spec is the model the two
  condition specs should copy.
- **ProximityRouting** — all four phases present exactly as claimed (500 receives / 9000 never;
  4050 holds; 4300 stops; 4050 re-entry stays stopped) with per-phase buffer resets and the
  transform-sync diagnostic (spec :245-434). Decode-layer asserts are adequate HERE because no
  client-side gate exists between arrival and decode for the proximity property.
- **TeardownMidTransmit** — voice-provably-flowing precondition, whole-subject destroy, replica
  gone on client, listener survives (spec :172-218).
- **Top-N** — `Test_VoiceChat_TopN.cpp` pins cap, cross-bucket win, LRS tie rotation,
  serve-updates-rotation, and degenerate inputs (:32-81); hysteresis/envelope constants and the
  stage/flush split read as described (`CkVoiceChat_Route_Processor.cpp:54-55,431-434,534-626`).
- **Style/doctrine sweep** — zero stock `ensure/check` (all 17 sites `CK_ENSURE_IF_NOT`); zero
  `ck::StaticCast`; trailing returns + UFUNCTION concrete-type-on-own-line conform throughout;
  request completion contract (trailing `InDelegate`, `AutoCreateRefTerm`, `MakeCompletionGuard`,
  cancel processors) present on all three features; no `TODO/FIXME`.

### Deviations ruling

1. **Deviation 5 (probe `_CurrentOverlaps` read instead of ADR-6's `BindTo_OnBeginOverlap`) —
   ACCEPTED.** The claimed exemplar is real and exact: `CkCrowdAgent_Neighbors_Processor.cpp:60`
   is the same `Get<FFragment_Probe_Current>().Get_CurrentOverlaps()` read. The "no C++ adopter
   binds it" claim holds: grep finds zero users of `UCk_Utils_Probe_UE::BindTo_OnBeginOverlap`
   outside CkSpatialQuery itself (the CkOverlapBody hits are that module's own Sensor feature, a
   different system). ADR-6's *mechanism* — event-maintained set, no queries or world scans in the
   packet path — is preserved; its *letter* named a bind surface no native code uses. The
   deviation was recorded, reasoned, and is the better engineering choice.
2. **Top-N-under-net-load coverage bound — ACCEPTED as recorded.** The selection policy is
   unit-pinned, the flush path runs in every forwarding spec, and the harness realistically hosts
   3 worlds; building a >8-connection net rig for this gate would buy little. Bound is honestly
   stated in the exit criteria rather than claimed. Re-examine only if a packaged soak (P5) shows
   fairness anomalies.
3. **S5 config caveat (editor net-PIE, not packaged) — ACCEPTED with its own stated condition.**
   The measurement methodology is sound (pre-cap arrival seam, two identical 60-tick windows,
   queue still draining at mark 3 ⇒ saturated ceiling), the numbers in this doc match the log
   verbatim, and the 640 default is internally consistent (≈20% above the 8-talker steady case,
   ≈10% under the measured drain). The packaged/dedicated re-measure before ship-tuning is already
   a recorded P5 obligation; the spec staying in the suite as the re-measurement tool is the right
   call.

### Scope check

All twelve audited commits touch only `Source/CkVoiceChat`, CkTests VoiceChat specs/units, and
campaign docs; the `FCk_Time` sweep (2905dcf79) was maintainer-directed per PROGRESS. No
unrecorded scope creep found. Dep budget N4 honored: Build.cs earns exactly
`CkActorRelay/CkShapes/CkSpatialQuery` + `NetCore/Voice/AudioMixer/GameplayTags/DeveloperSettings`,
each with a recorded earning event; `CkRelationship` absent.

## Audit-condition re-gate — 2026-08-04 (post-rebase)

**All three conditions RESOLVED and re-gated green. Gate 3 machine portion CLOSED.**

Conditions landed on the post-rebase lineage — CkFoundation `feature/voice-chat` @ `cc95c7760`,
CkTests `feature/voice-chat-wip` @ `52a4d9f` (branch renamed from `feature/voice-chat` upstream;
both tips also present on `origin`):

| # | Condition | As landed |
|---|---|---|
| C1 | Arrival-counter freeze assert in the mute spec (F1) | `Ck.VoiceChat.Net.ListenerMute_StopsForwarding` asserts the RPC-boundary `Debug_Get_ReceiveArrivedBundles` counter is FROZEN across the muted spurt and grows across the unmuted one — proves the server exclusion, not just the local pre-decode gate |
| C2 | N1 never-stash arrival equality + FutureIdx assert (F2) | `Ck.VoiceChat.Net.Routing_ForwardsAndNeverStashes` snapshots the arrival counter at the forward-assert and re-asserts equality at the never-stash assert; also asserts `TryGet_ChannelByIdx(FutureIdx)` is invalid at inject time |
| C3 | Throttled per-talker N2 drop Warning (F3) | `FProcessor_VoiceChat_Route` tallies all six packet-path drop reasons per drain and emits at most one Warning per talker per 5s naming the per-reason breakdown; cooldown rides `FFragment_VoiceTalker_ServerInbox`. Implemented rather than recorded as a deviation — the fuller of F3's two options |

**Re-gate run (BusterBlock host, the same host every prior gate used):**

- `--build --config=Development --target=Editor --test --test-pattern VoiceChat --discover-fresh`
  → **VoiceChat 30/30, Failed 0, Skipped 0, Contaminated 0, EXIT 0, 3m21s, 0 `Angelscript: Error`**
  (`Saved/Logs/Test-VoiceChatP3-postrebase2.log`).
- `--test --test-pattern RenderTarget --discover-fresh` on the SAME binary, **no rebuild between
  runs** (0 compile/link actions in the log) → **RenderTarget 22/22, Failed 0, EXIT 0, 2m47s**
  (`Saved/Logs/Test-P3-RenderTarget-postrebase2.log`) — delta-zero vs the recorded baseline.
- Build: 0 `error C####`, 0 `LNK`. **C3's code compiled for the first time in this run** (it was
  committed but never built at authoring time).
- **Freshness chain verified (no stale-green):** sources checked out 02:14:27 → `CkVoiceChat`
  compiled + linked in THIS run (actions 43-72 of 117) → `BusterBlockEditor-CkVoiceChat.dll`
  02:21:14 → VoiceChat log 02:27:13 → RenderTarget on that same DLL. Monotonic.

**Post-rebase blocker found and fixed (not campaign code).** Upstream CkFoundation `dd3632bd7`
("carry a completion delegate on every deferred request, repo-wide") added a trailing delegate to
`UCk_Utils_IntegerAttributeModifier_UE::Request_ClearAllModifiers`, which DROPPED the C++ default on
`InAttributeComponent` — source-breaking for AngelScript callers, exactly the hazard CkFoundation's
own doctrine warns about. `CkAttributeGym_Integer_Modifiers_Steps.as:115` still passed one argument,
failing the whole AS compile ("Keeping all old script code") and taking the run with it. `origin/dev`
already carried the one-line fix (CkTests `e5bb948b`); our branch was exactly 1 commit behind and had
never touched that file. Applied as a working-tree edit for the run. **Carried item: CkTests
`feature/voice-chat-wip` still needs `e5bb948b` merged/rebased in — until then the branch cannot AS-compile
standalone.** Worth a repo-wide sweep for other AS callers that omitted a now-defaulted-away argument.

**Host note.** An attempt to run this re-gate on the CkPlugins host instead was abandoned: CkPlugins
sets `DisableEnginePluginsByDefault: true` with a slimmed plugin closure that excludes
OnlineSubsystem/OnlineSubsystemUtils and defines no `NetDriverDefinitions`, so PIE net worlds fail
with `NetDriverCreateFailure ... Driver = NONE` and **all 11 net specs cannot run there** (19/19
non-net specs did pass, and the C++/AS compile was clean — useful corroboration on an independent
host, but no gate evidence). BusterBlock defines `GameNetDriver` with an `OnlineSubsystemUtils.IpNetDriver`
fallback and remains the only net-capable host for this campaign.
