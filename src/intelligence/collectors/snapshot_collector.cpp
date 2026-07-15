#include "intelligence/collectors/snapshot_collector.hpp"

#include "constants.hpp"

#include <algorithm>
#include <cmath>

namespace cryptex::intelligence {

namespace {

double average_block_interval_seconds(const Blockchain& chain, size_t window) {
    if (chain.best_height() == 0 || window == 0) {
        return 0.0;
    }

    const uint64_t end_height = chain.best_height();
    const uint64_t start_height = (end_height > window) ? (end_height - window) : 1;
    uint64_t total = 0;
    size_t intervals = 0;

    for (uint64_t height = start_height; height <= end_height; ++height) {
        auto current = chain.get_block(height);
        auto previous = chain.get_block(height - 1);
        if (!current || !previous) {
            continue;
        }
        if (current->header.timestamp >= previous->header.timestamp) {
            total += static_cast<uint64_t>(current->header.timestamp - previous->header.timestamp);
            ++intervals;
        }
    }

    if (intervals == 0) {
        return 0.0;
    }
    return static_cast<double>(total) / static_cast<double>(intervals);
}

uint64_t count_recent_large_transfers(const Blockchain& chain, size_t window, int64_t threshold_sats) {
    if (chain.best_height() == 0 || window == 0 || threshold_sats <= 0) {
        return 0;
    }

    const uint64_t end_height = chain.best_height();
    const uint64_t start_height = (end_height > window) ? (end_height - window + 1) : 0;
    uint64_t count = 0;
    for (uint64_t height = start_height; height <= end_height; ++height) {
        auto block = chain.get_block(height);
        if (!block) {
            continue;
        }
        for (const auto& tx : block->transactions) {
            for (const auto& output : tx.outputs) {
                if (output.value >= threshold_sats) {
                    ++count;
                }
            }
        }
    }
    return count;
}

} // namespace

ChainActivitySnapshot SnapshotCollector::collect_chain(const Blockchain& chain) {
    ChainActivitySnapshot snapshot;
    snapshot.best_height = chain.best_height();
    snapshot.current_bits = chain.tip_bits();
    snapshot.mempool_transactions = chain.mempool().size();
    snapshot.avg_block_interval_seconds = average_block_interval_seconds(chain, 12);
    snapshot.recent_large_transfers = count_recent_large_transfers(chain,
                                                                   24,
                                                                   10LL * 1000LL * 1000LL * 1000LL);
    snapshot.active_address_count_24h = 0;
    snapshot.fee_rate_median_sats = 0.0;
    return snapshot;
}

NetworkSnapshot SnapshotCollector::collect_network(const Blockchain& chain,
                                                   const net::NetworkNode& node,
                                                   const NetworkTelemetry& telemetry) {
    (void) chain;
    const auto sync = node.sync_status();

    NetworkSnapshot snapshot;
    snapshot.local_height = sync.local_height;
    snapshot.best_peer_height = sync.best_peer_height;
    snapshot.connected_peers = static_cast<uint32_t>(sync.connected_peers);
    snapshot.validated_peers = static_cast<uint32_t>(sync.validated_peers);
    snapshot.inbound_peers = telemetry.inbound_peers;
    snapshot.outbound_peers = telemetry.outbound_peers;
    snapshot.reconnect_events_1h = telemetry.reconnect_events_1h;
    snapshot.malformed_messages_1h = telemetry.malformed_messages_1h;
    snapshot.syncing = sync.syncing;
    snapshot.lan_only = telemetry.lan_only;
    return snapshot;
}

MiningSnapshot SnapshotCollector::collect_mining(const Blockchain& chain,
                                                 const net::NetworkNode* node,
                                                 const MiningTelemetry& telemetry) {
    MiningSnapshot snapshot;
    snapshot.chain_height = chain.best_height();
    snapshot.active_threads = telemetry.active_threads;
    snapshot.stale_candidates_1h = telemetry.stale_candidates_1h;
    snapshot.rejected_candidates_1h = telemetry.rejected_candidates_1h;
    snapshot.hash_rate_mhps = telemetry.hash_rate_mhps;
    snapshot.avg_block_interval_seconds =
        telemetry.avg_block_interval_seconds > 0.0
            ? telemetry.avg_block_interval_seconds
            : average_block_interval_seconds(chain, 12);
    snapshot.sync_approved = chain.wallet_state_approved();
    snapshot.external_worker_online = telemetry.external_worker_online;
    if (node) {
        const auto sync = node->sync_status();
        if (sync.syncing && snapshot.sync_approved) {
            snapshot.sync_approved = false;
        }
    }
    return snapshot;
}

WalletRiskSnapshot SnapshotCollector::collect_wallet(const Wallet& wallet,
                                                     Blockchain& chain,
                                                     const TransactionDraftSignals& signals) {
    const auto balance = wallet.balance_summary(chain);
    const auto unspent = wallet.list_unspent(chain);

    WalletRiskSnapshot snapshot;
    snapshot.wallet_loaded = !wallet.address.empty();
    snapshot.wallet_approved = balance.approved;
    snapshot.input_count = signals.input_count;
    snapshot.output_count = signals.output_count;
    snapshot.spend_value_sats = signals.spend_value_sats;
    snapshot.available_balance_sats = balance.spendable;
    snapshot.new_recipient = signals.new_recipient;
    snapshot.high_value_relative_to_balance =
        signals.high_value_relative_to_balance_hint ||
        (balance.spendable > 0 && signals.spend_value_sats >= (balance.spendable / 2));
    snapshot.fragmented_outputs =
        signals.fragmented_outputs_hint ||
        (unspent.size() >= 20 && signals.input_count >= 4);
    snapshot.dust_like_pattern =
        signals.dust_like_pattern_hint ||
        (signals.output_count >= 8 && signals.spend_value_sats > 0 &&
         signals.spend_value_sats <= 100'000);
    snapshot.contains_private_note = signals.contains_private_note;
    return snapshot;
}

CommsSnapshot SnapshotCollector::collect_comms(const net::NetworkNode& node,
                                               const CommsTelemetry& telemetry) {
    const auto status = node.dht_mailbox_status();

    CommsSnapshot snapshot;
    snapshot.dht_peers = static_cast<uint32_t>(status.active_peers);
    snapshot.delivery_failures_1h = telemetry.delivery_failures_1h;
    snapshot.proof_failures_1h = telemetry.proof_failures_1h;
    snapshot.suspicious_contacts_1h = telemetry.suspicious_contacts_1h;
    snapshot.relay_only_routing =
        telemetry.relay_only_routing ||
        (!status.port_mapping_active && status.relay_fallback && status.relay_successes > 0);
    snapshot.mail_2fa_enabled = telemetry.mail_2fa_enabled;
    snapshot.contains_private_content = telemetry.contains_private_content;
    return snapshot;
}

SnapshotBundle SnapshotCollector::collect_overview(const Blockchain& chain,
                                                   const net::NetworkNode& node,
                                                   const NetworkTelemetry& network_telemetry,
                                                   const MiningTelemetry& mining_telemetry,
                                                   const CommsTelemetry& comms_telemetry) {
    SnapshotBundle bundle;
    bundle.chain = collect_chain(chain);
    bundle.network = collect_network(chain, node, network_telemetry);
    bundle.mining = collect_mining(chain, &node, mining_telemetry);
    bundle.comms = collect_comms(node, comms_telemetry);
    return bundle;
}

} // namespace cryptex::intelligence
