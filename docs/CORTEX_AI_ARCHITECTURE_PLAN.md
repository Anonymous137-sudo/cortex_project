# Cortex AI Architecture Plan

## Purpose

This document turns the school whitepaper's "Intelligent Operations Subsystem" into a concrete implementation plan for `cortex_project` before we start changing core source files.

For provider-specific design using Google Gemini and Groq, see [`/Users/gitanshchakravarty/Desktop/cortex_project/docs/CORTEX_AI_PROVIDER_DESIGN.md`](./CORTEX_AI_PROVIDER_DESIGN.md).

The most important rule is simple:

- the AI layer must stay outside consensus
- the AI layer may observe validated state, but it must not redefine protocol truth
- the AI layer may recommend or explain actions, but it must not silently sign, spend, or mutate chain state

That boundary is what keeps the project technically defensible.

## Current Base System We Already Have

The current codebase already provides the main surfaces an intelligence layer can observe:

- backend node with chainstate, mempool, peer state, mining, and wallet logic in [`/Users/gitanshchakravarty/Desktop/cortex_project/src`](./../src)
- Qt desktop client driving the daemon over loopback JSON-RPC
- peer-to-peer transport for sync, relay, secure messaging, and mail routing
- wallet and transaction history workflows
- mining telemetry and external PoW worker integration
- communication features documented in [`/Users/gitanshchakravarty/Desktop/cortex_project/docs/communication-systems.md`](./communication-systems.md)

That means the first Cortex AI versions should be built as an observability and explanation layer on top of existing node behavior, not as a replacement for existing behavior.

## Hard Architectural Constraints

### 1. Consensus Must Stay Deterministic

AI outputs must never participate in:

- block validation
- transaction validation
- difficulty calculation
- header acceptance
- wallet ownership rules
- reward rules

Consensus continues to live only in deterministic C++ validation code.

### 2. Advisory Before Autonomous

Initial Cortex AI features should:

- score
- summarize
- predict
- explain
- recommend

They should not automatically:

- broadcast transactions
- refuse valid transactions by policy
- disconnect peers permanently
- alter mining rules
- adjust wallet balances

Any later automation must sit behind explicit policy flags and operator approval.

### 3. Local-First Execution

The school project should assume local inference or rule-based analysis first.

Reasons:

- works on offline or lab environments
- avoids leaking wallet and peer metadata by default
- is easier to explain in a technical review
- avoids tying core functionality to an external service

If remote model providers are added later, they should be strictly optional and disabled by default.

### 4. Explainability Is Required

Every intelligence output should include:

- a severity or confidence value
- the signals used
- a short textual explanation
- a timestamp
- the subsystem that produced it

If Cortex says a transaction is risky or a peer graph looks unhealthy, it should show why.

### 5. Auditability Over Magic

We should persist structured advisory outputs so the GUI and CLI can show:

- what was detected
- when it was detected
- what evidence was used
- whether the operator ignored or acted on it

That makes the subsystem inspectable instead of vague.

## Proposed Subsystem Layout

Before modifying broad areas of `src`, the cleanest structure is to introduce a new intelligence module tree:

```text
src/intelligence/
  intelligence_types.hpp
  intelligence_bus.hpp
  intelligence_bus.cpp
  collectors/
    chain_collector.cpp
    network_collector.cpp
    wallet_collector.cpp
    mining_collector.cpp
    comms_collector.cpp
  features/
    feature_snapshot.hpp
    feature_extractor.cpp
  analyzers/
    rules_engine.cpp
    network_health_analyzer.cpp
    tx_risk_analyzer.cpp
    mining_advisor.cpp
    comms_safety_analyzer.cpp
  inference/
    inference_provider.hpp
    local_model_provider.cpp
    null_provider.cpp
  rpc/
    intelligence_rpc.cpp
  storage/
    intelligence_store.cpp
```

This keeps the AI additions isolated from consensus-critical files.

## Data Collection Plan

The intelligence layer should collect read-only snapshots from existing subsystems.

### Network And Peer Signals

Read from node and peer state:

- connected peers
- validated peers
- best peer height
- reconnect churn
- malformed message counts
- inbound versus outbound mix
- sync lag
- observed relay delays

Primary use:

- peer risk scoring
- eclipse or partition hints
- network health summaries

### Chain And Activity Signals

Read from validated chain data:

- block cadence
- difficulty trend
- transaction volume
- fee trend
- address activity summaries
- miner concentration indicators
- large-value movement clusters

Primary use:

- blockchain interpretation
- trend summaries
- operator dashboards

### Wallet And Transaction Signals

Read from wallet and transaction construction flows:

- output fragmentation
- spend frequency
- abnormal transfer size
- new recipient patterns
- dust-like activity
- repeated suspicious counterparties

Primary use:

- transaction-risk analysis
- safe-send guidance
- account activity interpretation

### Mining Signals

Read from mining runtime and external worker reporting:

- hash rate
- thread count
- stale/rejected candidate count
- block discovery intervals
- sync approval state
- energy or thermal metrics when available

Primary use:

- mining efficiency advice
- difficulty expectation summaries
- sync-aware mining recommendations

### Communication Signals

Read from the messenger and P2P mail systems:

- mailbox delivery failures
- repeated challenge/proof failures
- relay-only behavior
- unusual contact patterns
- spam-like burst traffic
- suspicious address-book activity

Primary use:

- mail abuse detection
- communication trust hints
- identity and delivery diagnostics

## Execution Model

The safest first design is:

1. daemon collects telemetry snapshots
2. daemon runs lightweight analyzers locally
3. results are stored in a small advisory state store
4. GUI and CLI fetch results over RPC

That keeps the first version simple.

If we later add heavier model inference, do it behind an interface such as `InferenceProvider` so we can swap between:

- rules only
- local compact model
- optional external provider

without changing the rest of the system.

## RPC And GUI Surfaces To Add

The intelligence layer should be visible through explicit new RPC methods rather than mixed into unrelated endpoints.

Suggested RPC additions:

- `getintelligenceoverview`
- `getnetworkhealthreport`
- `analyzetransactionrisk`
- `getminingadvice`
- `getactivitysummary`
- `getcommssecurityreport`

Suggested GUI additions:

- Network Health panel
- Transaction Risk panel in send flow
- Mining Advisor panel
- Communication Safety panel for messenger and P2P mail
- Activity Summary panel for wallet and chain interpretation

## Storage Model

Advisory data should be stored separately from consensus state.

Suggested storage classes:

- rolling snapshots
- recent alerts
- operator acknowledgements
- cached summaries

Do not mix advisory state into block index, chainstate, or wallet ownership records.

## Phased Implementation Plan

### Phase 0. Boundary And Naming Cleanup

Before AI logic:

- define the new `src/intelligence` module boundary
- add shared types for alerts, scores, and evidence
- keep current node, wallet, and messaging behavior unchanged

### Phase 1. Telemetry And Rule Engine

First shippable intelligence layer:

- read-only collectors
- feature snapshots
- deterministic rules engine
- simple network health and mining advisory outputs

This phase gives us immediate value without model risk.

### Phase 2. Wallet And Transaction Safety

Add:

- transaction-risk scoring
- unusual wallet activity summaries
- send-flow warnings with explanations

Still advisory only.

### Phase 3. Messenger And P2P Mail Intelligence

Add:

- abuse and spam heuristics
- suspicious relay pattern detection
- delivery-failure explanations
- contact trust hints

This is a strong fit for the communication systems already in the repo.

### Phase 4. Mining And Operational Forecasting

Add:

- expected discovery interval estimates
- sync-aware mining recommendations
- rejected/stale pattern diagnosis
- difficulty trend summaries

### Phase 5. Optional Model Providers

Only after the earlier phases are stable:

- local compact model integration
- optional remote inference provider
- prompt templates or retrieval summaries for complex explanations

This phase must remain isolated from consensus-critical paths.

## Immediate Source Changes To Plan Before Heavy Modding

Before broad implementation, the repo should next introduce:

1. `src/intelligence/` module skeleton
2. a typed advisory result format shared by CLI, RPC, and GUI
3. a telemetry snapshot interface for network, wallet, mining, chain, and communications
4. RPC endpoints dedicated to intelligence outputs
5. GUI placeholders that can render advisory results without changing existing wallet or node logic

## What The First School Windows Bundle Should Honestly Represent

The current Windows bundle can already demonstrate:

- node startup and sync behavior
- wallet flows
- mining runtime
- peer-to-peer communication primitives
- the school whitepaper
- this architecture plan

What it should not claim yet is that full AI decision-making is already implemented in the shipped binaries.

The honest framing is:

- the current codebase provides the deterministic banking, wallet, node, and communication substrate
- the whitepaper defines the advisory intelligence direction
- this plan defines how Cortex can add AI safely without corrupting protocol correctness

That is technically stronger than pretending the architecture already exists in production.
