#include "intelligence/rules_provider.hpp"

#include "constants.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace cryptex::intelligence {

namespace {

int severity_score(Severity severity) {
    switch (severity) {
    case Severity::Info: return 5;
    case Severity::Low: return 25;
    case Severity::Medium: return 55;
    case Severity::High: return 80;
    case Severity::Critical: return 95;
    }
    return 5;
}

AdvisoryResult make_result(AdvisoryKind kind,
                           Severity severity,
                           const std::string& summary,
                           double confidence = 0.8) {
    AdvisoryResult result;
    result.kind = kind;
    result.provider = ProviderKind::RulesOnly;
    result.severity = severity;
    result.generated_at = iso8601_utc_now();
    result.confidence = std::clamp(confidence, 0.0, 1.0);
    result.score = severity_score(severity);
    result.summary = summary;
    result.operator_confirmation_required = false;
    return result;
}

void add_reason(AdvisoryResult& result, const std::string& text) {
    if (!text.empty()) {
        result.reasons.push_back(text);
    }
}

void add_recommendation(AdvisoryResult& result,
                        const std::string& code,
                        const std::string& text,
                        bool operator_confirmation_required = false) {
    Recommendation rec;
    rec.code = code;
    rec.text = text;
    rec.operator_confirmation_required = operator_confirmation_required;
    result.recommendations.push_back(std::move(rec));
}

void add_numeric_evidence(AdvisoryResult& result,
                          const std::string& key,
                          double value,
                          const std::string& suffix = {}) {
    EvidenceItem item;
    item.key = key;
    std::ostringstream out;
    if (std::fabs(value - std::round(value)) < 0.0001) {
        out << static_cast<int64_t>(std::llround(value));
    } else {
        out.setf(std::ios::fixed);
        out.precision(2);
        out << value;
    }
    out << suffix;
    item.value = out.str();
    item.numeric_value = value;
    result.evidence.push_back(std::move(item));
}

AdvisoryResult analyze_network(const ProviderRequest& request) {
    if (!request.snapshots.network) {
        return AdvisoryResult::unavailable(request.kind, "missing network snapshot");
    }
    const auto& net = *request.snapshots.network;

    AdvisoryResult result = make_result(request.kind, Severity::Info, "Peer connectivity looks stable.", 0.87);

    if (net.local_height == 0 && net.connected_peers == 0) {
        result = make_result(request.kind, Severity::High, "Node has no peers and no confirmed local progress.", 0.95);
        add_reason(result, "No connected peers are visible and the local chain has not advanced beyond genesis state.");
        add_recommendation(result, "verify-seeds", "Verify seed reachability or configure at least one known-good direct peer.");
    } else if (net.syncing && net.validated_peers == 0) {
        result = make_result(request.kind, Severity::High, "Node is syncing without a validated peer anchor.", 0.92);
        add_reason(result, "Best-peer height is ahead of local height, but no validated peer session is currently established.");
        add_recommendation(result, "pause-sensitive-actions", "Delay sensitive operations until a validated peer confirms the active chain.");
    } else if (net.malformed_messages_1h >= 25) {
        result = make_result(request.kind, Severity::High, "Malformed peer traffic is elevated.", 0.9);
        add_reason(result, "Malformed message volume is above the normal hygiene threshold for routine operation.");
        add_recommendation(result, "inspect-peers", "Inspect noisy peers and tighten inbound filtering if the pattern persists.");
    } else if (net.syncing) {
        result = make_result(request.kind, Severity::Medium, "Node is still catching up to the best observed chain.", 0.85);
        add_reason(result, "The best peer height is ahead of the local height or block queues are still draining.");
        add_recommendation(result, "hold-finality", "Avoid treating balances or chain-derived analytics as final until sync approval is restored.");
    } else if (net.reconnect_events_1h >= 10 || net.connected_peers < 2) {
        result = make_result(request.kind, Severity::Low, "Connectivity is thin and may become brittle.", 0.8);
        add_reason(result, "Peer count or recent reconnect churn suggests the node has limited redundancy.");
        add_recommendation(result, "add-redundancy", "Add another stable peer path so transient disconnects do not isolate the node.");
    }

    if (net.lan_only) {
        add_reason(result, "LAN-only networking is enabled, which limits peer diversity and resilience.");
    }
    add_numeric_evidence(result, "local_height", static_cast<double>(net.local_height));
    add_numeric_evidence(result, "best_peer_height", static_cast<double>(net.best_peer_height));
    add_numeric_evidence(result, "connected_peers", static_cast<double>(net.connected_peers));
    add_numeric_evidence(result, "validated_peers", static_cast<double>(net.validated_peers));
    add_numeric_evidence(result, "reconnect_events_1h", static_cast<double>(net.reconnect_events_1h));
    add_numeric_evidence(result, "malformed_messages_1h", static_cast<double>(net.malformed_messages_1h));
    return result;
}

AdvisoryResult analyze_mining(const ProviderRequest& request) {
    if (!request.snapshots.mining) {
        return AdvisoryResult::unavailable(request.kind, "missing mining snapshot");
    }
    const auto& mining = *request.snapshots.mining;

    AdvisoryResult result = make_result(request.kind, Severity::Info, "Mining telemetry is within expected operating bounds.", 0.84);

    if (!mining.sync_approved) {
        result = make_result(request.kind, Severity::High, "Mining should remain gated until chain sync approval returns.", 0.96);
        add_reason(result, "The local chain is not currently approved against observed network state.");
        add_recommendation(result, "sync-before-mine", "Wait for sync approval before treating newly found work as trustworthy.");
    } else if (mining.rejected_candidates_1h > 0) {
        result = make_result(request.kind, Severity::High, "Recently found candidates were rejected by chain validation.", 0.95);
        add_reason(result, "Candidate discovery is not the issue; acceptance by the active chain is failing.");
        add_recommendation(result, "inspect-template", "Inspect block template inputs, previous-tip alignment, and candidate validation logs.");
    } else if (mining.stale_candidates_1h >= 5) {
        result = make_result(request.kind, Severity::Medium, "Stale candidate rate is elevated.", 0.82);
        add_reason(result, "The miner is solving work that is being superseded too often for efficient operation.");
        add_recommendation(result, "refresh-work", "Shorten work refresh cadence and verify the miner is tracking the current tip.");
    } else if (mining.active_threads == 0 || mining.hash_rate_mhps <= 0.0) {
        result = make_result(request.kind, Severity::Low, "Mining is enabled but no effective hash throughput is visible.", 0.76);
        add_reason(result, "Worker threads or measured hash rate indicate the mining path is idle.");
        add_recommendation(result, "verify-worker", "Verify worker launch, process affinity, and thread configuration.");
    }

    add_numeric_evidence(result, "chain_height", static_cast<double>(mining.chain_height));
    add_numeric_evidence(result, "active_threads", static_cast<double>(mining.active_threads));
    add_numeric_evidence(result, "stale_candidates_1h", static_cast<double>(mining.stale_candidates_1h));
    add_numeric_evidence(result, "rejected_candidates_1h", static_cast<double>(mining.rejected_candidates_1h));
    add_numeric_evidence(result, "hash_rate_mhps", mining.hash_rate_mhps, " MH/s");
    add_numeric_evidence(result, "avg_block_interval_seconds", mining.avg_block_interval_seconds, " s");
    return result;
}

AdvisoryResult analyze_comms(const ProviderRequest& request) {
    if (!request.snapshots.comms) {
        return AdvisoryResult::unavailable(request.kind, "missing communications snapshot");
    }
    const auto& comms = *request.snapshots.comms;

    AdvisoryResult result = make_result(request.kind, Severity::Info, "P2P communications telemetry looks healthy.", 0.83);

    if (comms.proof_failures_1h >= 3 || comms.suspicious_contacts_1h >= 5) {
        result = make_result(request.kind, Severity::High, "Communication trust signals are degraded.", 0.88);
        add_reason(result, "Proof-of-storage or contact-behavior anomalies are above the normal background level.");
        add_recommendation(result, "review-replica-peers", "Review replica peers and contact trust data before relying on the affected paths.");
    } else if (comms.delivery_failures_1h >= 10) {
        result = make_result(request.kind, Severity::Medium, "Message delivery failures are elevated.", 0.82);
        add_reason(result, "Recent delivery failures suggest routing instability or unreachable endpoints.");
        add_recommendation(result, "inspect-routing", "Check relay paths, NAT traversal, and recipient availability.");
    } else if (comms.relay_only_routing || !comms.mail_2fa_enabled) {
        result = make_result(request.kind, Severity::Low, "Communication security posture can be strengthened.", 0.78);
        if (comms.relay_only_routing) {
            add_reason(result, "Traffic is relying on relay-only routing, which reduces direct-path assurance.");
        }
        if (!comms.mail_2fa_enabled) {
            add_reason(result, "Mail 2FA is not enabled for the current workflow.");
        }
        add_recommendation(result, "raise-comms-hardening", "Enable stronger account protections and diversify direct delivery paths where possible.");
    }

    if (comms.contains_private_content) {
        add_reason(result, "Private content is present in the analyzed communication context.");
    }
    add_numeric_evidence(result, "dht_peers", static_cast<double>(comms.dht_peers));
    add_numeric_evidence(result, "delivery_failures_1h", static_cast<double>(comms.delivery_failures_1h));
    add_numeric_evidence(result, "proof_failures_1h", static_cast<double>(comms.proof_failures_1h));
    add_numeric_evidence(result, "suspicious_contacts_1h", static_cast<double>(comms.suspicious_contacts_1h));
    return result;
}

AdvisoryResult analyze_chain_activity(const ProviderRequest& request) {
    if (!request.snapshots.chain) {
        return AdvisoryResult::unavailable(request.kind, "missing chain snapshot");
    }
    const auto& chain = *request.snapshots.chain;

    AdvisoryResult result = make_result(request.kind, Severity::Info, "Recent chain activity is within the expected operating envelope.", 0.8);

    const double slow_threshold = static_cast<double>(constants::BLOCK_TIME_SECONDS * 2);
    const double fast_threshold = static_cast<double>(std::max(1, constants::BLOCK_TIME_SECONDS / 3));
    if (chain.avg_block_interval_seconds > slow_threshold) {
        result = make_result(request.kind, Severity::Medium, "Block cadence is materially slower than target.", 0.78);
        add_reason(result, "Average observed block spacing is above the tolerance band for the configured target interval.");
        add_recommendation(result, "inspect-liveness", "Check network liveness, miner participation, and timestamp consistency.");
    } else if (chain.avg_block_interval_seconds > 0.0 && chain.avg_block_interval_seconds < fast_threshold) {
        result = make_result(request.kind, Severity::Medium, "Block cadence is materially faster than target.", 0.78);
        add_reason(result, "Observed block spacing is significantly below the target interval, which may indicate unstable difficulty response.");
        add_recommendation(result, "inspect-difficulty", "Review difficulty damping and timestamp behavior before changing protocol parameters.");
    } else if (chain.mempool_transactions >= 1000) {
        result = make_result(request.kind, Severity::Low, "Mempool activity is elevated.", 0.74);
        add_reason(result, "Queued transaction count is high enough to warrant watchlist status.");
        add_recommendation(result, "watch-fee-pressure", "Monitor fee pressure and transaction clearance times.");
    }

    add_numeric_evidence(result, "best_height", static_cast<double>(chain.best_height));
    add_numeric_evidence(result, "mempool_transactions", static_cast<double>(chain.mempool_transactions));
    add_numeric_evidence(result, "avg_block_interval_seconds", chain.avg_block_interval_seconds, " s");
    add_numeric_evidence(result, "recent_large_transfers", static_cast<double>(chain.recent_large_transfers));
    return result;
}

AdvisoryResult analyze_transaction_risk(const ProviderRequest& request) {
    if (!request.snapshots.wallet) {
        return AdvisoryResult::unavailable(request.kind, "missing wallet risk snapshot");
    }
    const auto& wallet = *request.snapshots.wallet;

    AdvisoryResult result = make_result(request.kind, Severity::Info, "Draft transaction risk looks routine.", 0.82);
    result.operator_confirmation_required = false;

    const bool exceeds_balance = wallet.spend_value_sats > wallet.available_balance_sats && wallet.available_balance_sats > 0;
    if (!wallet.wallet_approved) {
        result = make_result(request.kind, Severity::High, "Wallet state is not currently chain-approved.", 0.94);
        result.operator_confirmation_required = true;
        add_reason(result, "The wallet is attached to a chain state that is not yet approved against observed network conditions.");
        add_recommendation(result, "wait-approval", "Wait for chain approval before treating this spend as final.", true);
    } else if (exceeds_balance) {
        result = make_result(request.kind, Severity::Critical, "Requested spend exceeds the available approved balance.", 0.98);
        result.operator_confirmation_required = true;
        add_reason(result, "The draft spend is larger than the currently spendable approved balance.");
        add_recommendation(result, "rebuild-draft", "Rebuild the transaction with a lower amount or different funding set.", true);
    } else if (wallet.high_value_relative_to_balance && wallet.new_recipient) {
        result = make_result(request.kind, Severity::High, "High-value payment is targeting a new recipient.", 0.9);
        result.operator_confirmation_required = true;
        add_reason(result, "The spend is large relative to available balance and the recipient has not been seen before.");
        add_recommendation(result, "confirm-recipient", "Verify the recipient out-of-band before broadcasting.", true);
    } else if (wallet.high_value_relative_to_balance) {
        result = make_result(request.kind, Severity::Medium, "Spend value is large relative to current balance.", 0.85);
        result.operator_confirmation_required = true;
        add_reason(result, "A large portion of the available balance is being moved in a single draft.");
        add_recommendation(result, "double-check-amount", "Double-check the amount and fee assumptions before signing.", true);
    } else if (wallet.fragmented_outputs || wallet.dust_like_pattern) {
        result = make_result(request.kind, Severity::Medium, "Draft transaction has fragmentation or dust-like characteristics.", 0.79);
        add_reason(result, "Input/output shape suggests consolidation or dust handling rather than a simple payment.");
        add_recommendation(result, "review-coin-selection", "Review coin selection and destination splitting before broadcast.", true);
    }

    if (wallet.contains_private_note) {
        add_reason(result, "The transaction context includes a private note or message payload.");
    }
    add_numeric_evidence(result, "input_count", static_cast<double>(wallet.input_count));
    add_numeric_evidence(result, "output_count", static_cast<double>(wallet.output_count));
    add_numeric_evidence(result, "spend_value_sats", static_cast<double>(wallet.spend_value_sats));
    add_numeric_evidence(result, "available_balance_sats", static_cast<double>(wallet.available_balance_sats));
    return result;
}

AdvisoryResult analyze_protocol_explanation(const ProviderRequest& request) {
    if (request.operator_prompt.empty() && request.system_context.empty()) {
        return AdvisoryResult::unavailable(request.kind, "missing explanation prompt");
    }

    AdvisoryResult result = make_result(
        request.kind,
        Severity::Info,
        "Rules-only mode can provide structured operational guidance, but long-form protocol explanation should be delegated to a configured reasoning model.",
        0.72);
    add_reason(result, "Local rules are designed for deterministic advisory scoring rather than narrative reasoning.");
    add_recommendation(result,
                       "enable-remote-explainer",
                       "Use Gemini or Groq through the sidecar boundary when you need richer protocol explanation.");
    return result;
}

} // namespace

ProviderCapabilities RulesOnlyProvider::capabilities() const {
    ProviderCapabilities caps;
    caps.kind = ProviderKind::RulesOnly;
    caps.display_name = "rules-only";
    caps.remote = false;
    caps.supports_structured_output = true;
    caps.supports_function_calls = false;
    caps.supported_kinds = {
        AdvisoryKind::NetworkHealth,
        AdvisoryKind::TransactionRisk,
        AdvisoryKind::MiningHealth,
        AdvisoryKind::CommunicationSafety,
        AdvisoryKind::ChainActivity,
        AdvisoryKind::ProtocolExplanation,
    };
    return caps;
}

ProviderResponse RulesOnlyProvider::analyze(const ProviderRequest& request) const {
    ProviderResponse response;
    response.provider = ProviderKind::RulesOnly;
    response.accepted = true;

    switch (request.kind) {
    case AdvisoryKind::NetworkHealth:
        response.advisory = analyze_network(request);
        break;
    case AdvisoryKind::TransactionRisk:
        response.advisory = analyze_transaction_risk(request);
        break;
    case AdvisoryKind::MiningHealth:
        response.advisory = analyze_mining(request);
        break;
    case AdvisoryKind::CommunicationSafety:
        response.advisory = analyze_comms(request);
        break;
    case AdvisoryKind::ChainActivity:
        response.advisory = analyze_chain_activity(request);
        break;
    case AdvisoryKind::ProtocolExplanation:
        response.advisory = analyze_protocol_explanation(request);
        break;
    case AdvisoryKind::Unknown:
    default:
        response.advisory = AdvisoryResult::unavailable(request.kind, "unsupported advisory kind");
        break;
    }

    response.diagnostic = response.advisory ? response.advisory->summary : "no advisory";
    return response;
}

} // namespace cryptex::intelligence
