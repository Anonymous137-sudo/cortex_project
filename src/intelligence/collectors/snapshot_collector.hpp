#pragma once

#include "blockchain.hpp"
#include "network.hpp"
#include "wallet.hpp"
#include "intelligence/intelligence_types.hpp"

#include <optional>

namespace cryptex::intelligence {

struct NetworkTelemetry {
    uint32_t inbound_peers{0};
    uint32_t outbound_peers{0};
    uint32_t reconnect_events_1h{0};
    uint32_t malformed_messages_1h{0};
    bool lan_only{false};
};

struct MiningTelemetry {
    uint32_t active_threads{0};
    uint32_t stale_candidates_1h{0};
    uint32_t rejected_candidates_1h{0};
    double hash_rate_mhps{0.0};
    double avg_block_interval_seconds{0.0};
    bool external_worker_online{false};
};

struct TransactionDraftSignals {
    uint32_t input_count{0};
    uint32_t output_count{0};
    int64_t spend_value_sats{0};
    bool new_recipient{false};
    bool high_value_relative_to_balance_hint{false};
    bool fragmented_outputs_hint{false};
    bool dust_like_pattern_hint{false};
    bool contains_private_note{false};
};

struct CommsTelemetry {
    uint32_t delivery_failures_1h{0};
    uint32_t proof_failures_1h{0};
    uint32_t suspicious_contacts_1h{0};
    bool relay_only_routing{false};
    bool mail_2fa_enabled{false};
    bool contains_private_content{false};
};

class SnapshotCollector {
public:
    static ChainActivitySnapshot collect_chain(const Blockchain& chain);
    static NetworkSnapshot collect_network(const Blockchain& chain,
                                           const net::NetworkNode& node,
                                           const NetworkTelemetry& telemetry = {});
    static MiningSnapshot collect_mining(const Blockchain& chain,
                                         const net::NetworkNode* node = nullptr,
                                         const MiningTelemetry& telemetry = {});
    static WalletRiskSnapshot collect_wallet(const Wallet& wallet,
                                             Blockchain& chain,
                                             const TransactionDraftSignals& signals = {});
    static CommsSnapshot collect_comms(const net::NetworkNode& node,
                                       const CommsTelemetry& telemetry = {});
    static SnapshotBundle collect_overview(const Blockchain& chain,
                                           const net::NetworkNode& node,
                                           const NetworkTelemetry& network_telemetry = {},
                                           const MiningTelemetry& mining_telemetry = {},
                                           const CommsTelemetry& comms_telemetry = {});
};

} // namespace cryptex::intelligence
