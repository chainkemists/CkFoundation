# Resilience Tenets — doctrine for all CkFoundation projects

These tenets govern how gameplay systems are built and how problems in them are fixed, in every
project that consumes CkFoundation (BusterBlock included). They bind humans and AI agents
(Claude/Codex) alike. They exist because we build **nondeterministic, long-running simulations**:
replans, interrupts, despawns, and save/load will eventually exercise every path you hoped was
rare. A design that is correct only when every event is delivered perfectly is not correct.

**Origin (2026-08-14, BusterBlock):** customers froze in the rental-return line behind an empty
front slot. Root cause chain: "parking" dormant NPCs bypassed the state machine with a side-channel
flag → the parked NPC's exit cleanup never ran → it kept its queue slot forever → the queue, whose
advancement was purely event-driven, waited on a leave that would never arrive. Four tenets were
violated at once; any one of them held would have prevented or self-healed the freeze. The same
class had already struck the checkout line months earlier and was fixed there as a one-off — tenet
9 is why that wasn't enough.

**Hardened (2026-08-14, NPC pathing):** three silent-failure mechanisms were found routing around
tenet 1's *letter* — a request drop classified as a benign no-op (the crowd same-goal guard,
logging at Verbose, swallowed the AI's recovery re-issue), an error state the system could not
detect (a progress detector that measured movement, so wall-sliding read as healthy), and an error
correctly signaled to zero listeners (`OnGoalBlocked` fired; nothing was ever bound). Tenet 1 binds
only code that *knows* it has a problem; each failure sat where the code didn't classify the
condition as an error, couldn't detect it, or reported it to nobody. Tenet 1's classification
corollary, tenet 8's detector corollary, and tenet 10 close those routes.

## The tenets

1. **Never silently early-return on a potential problem.** A guard that returns on an unexpected
   condition without recording anything converts a bug into a mystery someone debugs from a
   screenshot. Every early-out on a *shouldn't-happen* condition must do one of: `CK_ENSURE_IF_NOT`,
   log with enough context to find the site, retry with a bound, or escalate. Bare `return` is
   reserved for genuinely expected control flow. *(Incident: the deposit step returned `false` into
   the void when its manager wasn't resolvable; the NPC held the front of the line indefinitely and
   nothing said why.)*

   **Corollary — dropping or coalescing a request is never a no-op.** The commonest evasion of this
   tenet is classification, not a bare `return`: "the caller asked for what's already happening, so
   ignoring it is harmless." The caller's intent is not visible at the drop site — a re-issue may
   *be* the recovery action. Any path that discards, coalesces, or debounces a caller's request
   must be observable at Log or better, and where intent genuinely forks, the API must let the
   caller state it (an explicit force/flush flag) rather than guess on the caller's behalf.
   *(Incident: the crowd same-goal guard dropped `MoveTo` re-issues at Verbose; the NPC state
   machine's re-plan was silently converted into nothing, and the agent foot-slid forever. Fix:
   the drop logs at Log, and `Set_ForceRepath` lets the caller say "re-path" explicitly.)*

2. **Never bury an error, and never route around one with a band-aid.** Null-check at the crash
   site, a sleep that "stabilizes" a race, an unbounded retry, a swallowed exception — these move
   the symptom and keep the cause. A suppression is legitimate only as a **declared** stopgap: name
   the real cause, name where it will resurface, record the follow-up, and get consent (see the
   override clause). A fix you can't explain mechanistically is a coincidence you're trusting.

3. **Cleanup is scope-bound: RAII through the StateMachine.** A resource acquired on state entry is
   released in that state's exit — one destructor, not N release calls scattered across managers,
   observers, and despawn paths, where the newest path always forgets one. The only way for an
   entity to do something else is to first exit cleanly. **Corollary: cross-cutting modes (parking,
   dormancy, combat, death) must be transitions *through* the lifecycle authority, never
   side-channel flags that bypass it.** Every bypass is an exit that never runs; each one is a
   deferred design decision about exit semantics, and the deferred cost is paid by whoever debugs
   the leak. *(Incident: `Set_AiParked` disabled the planners around the SM instead of transitioning
   through it — the parked renter's slot release lived in exits that never fired.)*

4. **Anything that can be waited on must converge from arbitrary state — level-triggered over
   edge-triggered.** Signals and events are a responsiveness optimization, never the only path to
   progress. The resource itself must periodically reconcile against observed world state (validity
   sweep, absence watchdog, gap collapse) so that a missed event costs latency, not liveness. The
   robust shape is **lease + reconciliation**: tenet 3 gives *prompt* correctness on every exit the
   engine can see; the resource-side reconcile gives *eventual* correctness on the paths that can't
   run a destructor (abrupt destroys, restores, bugs in the exit itself). Neither substitutes for
   the other.

5. **The resource owns its integrity; consumers own only their reaction to it.** Recovery machinery
   is built once, in the shared capability — never re-implemented per consumer. Duplicated
   robustness diverges, and the gap is always at the newest consumer. *(Incident: checkout had a
   full recovery ladder; the rental-return line, added later against the same queue, inherited none
   of it.)*

6. **No split-brain state.** Do not mirror another entity's state (latches, applied-flags, cached
   claims) unless a reconciler compares the mirror against the source of truth. Audit persistence
   on both sides: a persisted resource paired with a transient mirror disagrees after every load.
   Treat save/load as a state-fuzzer that will find every unreconciled pair.

7. **Fail-closed must pair with a bounded escape.** Failing closed (withhold completion, suppress,
   refuse) is a fine default only when a timeout, retry budget, or declared give-up eventually
   opens it. Fail-closed without an escape is a permanent wedge with good intentions.

8. **A system that can stall must be able to say so — and its self-descriptions must be true.**
   Build liveness telemetry (a watchdog that names the stalled entity, its state, and how long) so
   wedges surface in logs, not screenshots. And neither logs nor comments may lie: a log line
   claiming a cleanup happened above code that doesn't do it, or a comment asserting "conditions
   poll this query" about conditions that are event-driven, doesn't just fail to help — it actively
   steers the next reader (human or agent) away from the bug. *(Incident: the park bypass survived
   audit twice because two comments — and the test named after the feature — asserted a polling
   behavior the code never had; the test asserted the query, never the state, so the missing exit
   had zero coverage.)*

   **Corollary — the watchdog must measure progress toward the goal, not activity.** You cannot
   report what you cannot detect; a detector whose proxy metric can stay healthy while the goal
   recedes is a detector that lies by omission. Movement is not progress (wall-sliding), retries
   are not progress (a frozen queue), ticking is not progress. Bound every in-progress state with
   a metric that provably shrinks on real progress — remaining path distance, queue position, work
   items left — so an undetected error degrades into a detected timeout instead of silence.
   *(Incident: the crowd block detector sampled a positional centroid ring — lateral wall-sliding
   kept it "moving" — so a blocked agent stayed `Walking` indefinitely and no error existed
   anywhere in the system to report.)*

9. **Edge cases are instances of a class — fix the class.** When a failure looks like a one-off,
   ask what family it belongs to and harden the family: same-shaped call sites, sibling consumers,
   the framework primitive underneath. If the instance genuinely is unique, say so explicitly and
   why. Test the class, too: convergence-under-churn property tests, not only the happy edges the
   API makes easy to drive.

10. **A terminal outcome must reach a consumer — reported-to-nobody is still silent.** A producer
    that fires a failure signal has satisfied the letter of tenet 1 while the system stays silent
    end-to-end if nothing is bound. For outcomes that end a request's life (failed, blocked,
    aborted, refused), delivery is part of the contract: prefer channels the caller must handle
    (result callback, promise, required delegate) over fire-and-forget broadcasts; where a
    broadcast signal is the mechanism, the feature owner names which consumer reacts, and a
    terminal signal with zero bindings in a shipping consumer is itself a *shouldn't-happen* worth
    a dev-time warning. *(Incident: `OnGoalBlocked` fired correctly for months; no BusterBlock
    system ever bound it, so every blocked move died unobserved while the framework "reported"
    each one.)*

## Override clause

Any tenet may be broken **only** with the maintainer's/user's explicit consent, after a clear,
unmissable statement of the trade: *"This is a band-aid / a bypass / an unreconciled mirror; the
underlying cause remains and will resurface as X; the systemic fix would be Y."* Then record the
follow-up. The violation is never the sin — the silent violation is.

## Asking for work on high-blast-radius problems

When commissioning a fix whose failure would spread (shared capabilities, lifetimes, persistence,
anything other entities wait on), say explicitly: **"Do NOT use band-aids or shortcuts — I am
looking for the correct systemic solution."** Agents: treat any task touching such a system as
carrying that instruction by default, surface patch-vs-fix decisions instead of making them
silently, and hold fixes to the root-cause bar — the mechanism must explain what failed, why it
appeared when it did, and why the sibling paths don't show it.
