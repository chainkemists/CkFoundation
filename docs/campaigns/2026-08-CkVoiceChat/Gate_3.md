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
