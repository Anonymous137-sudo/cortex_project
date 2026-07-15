#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cryptex::intelligence {

enum class ProviderKind {
    None = 0,
    RulesOnly = 1,
    Gemini = 2,
    Groq = 3,
};

enum class AdvisoryKind {
    Unknown = 0,
    NetworkHealth = 1,
    TransactionRisk = 2,
    MiningHealth = 3,
    CommunicationSafety = 4,
    ChainActivity = 5,
    ProtocolExplanation = 6,
};

enum class Severity {
    Info = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Critical = 4,
};

std::string to_string(ProviderKind kind);
std::optional<ProviderKind> parse_provider_kind(std::string_view text);

std::string to_string(AdvisoryKind kind);
std::optional<AdvisoryKind> parse_advisory_kind(std::string_view text);

std::string to_string(Severity severity);
std::optional<Severity> parse_severity(std::string_view text);

std::string iso8601_utc_now();
std::string make_request_id(std::string_view prefix = "ai");

struct EvidenceItem {
    std::string key;
    std::string value;
    std::optional<double> numeric_value;
    bool sensitive{false};
};

struct Recommendation {
    std::string code;
    std::string text;
    bool operator_confirmation_required{true};
};

struct AdvisoryResult {
    AdvisoryKind kind{AdvisoryKind::Unknown};
    ProviderKind provider{ProviderKind::None};
    Severity severity{Severity::Info};
    std::string generated_at;
    std::optional<double> confidence;
    std::optional<int64_t> score;
    std::string summary;
    std::vector<std::string> reasons;
    std::vector<Recommendation> recommendations;
    std::vector<EvidenceItem> evidence;
    bool operator_confirmation_required{true};
    bool remote_generated{false};
    bool valid{true};
    std::string diagnostic;

    static AdvisoryResult rejected(AdvisoryKind kind, const std::string& diagnostic);
    static AdvisoryResult unavailable(AdvisoryKind kind, const std::string& diagnostic);
};

struct NetworkSnapshot {
    uint64_t local_height{0};
    uint64_t best_peer_height{0};
    uint32_t connected_peers{0};
    uint32_t validated_peers{0};
    uint32_t inbound_peers{0};
    uint32_t outbound_peers{0};
    uint32_t reconnect_events_1h{0};
    uint32_t malformed_messages_1h{0};
    bool syncing{false};
    bool lan_only{false};
};

struct MiningSnapshot {
    uint64_t chain_height{0};
    uint32_t active_threads{0};
    uint32_t stale_candidates_1h{0};
    uint32_t rejected_candidates_1h{0};
    double hash_rate_mhps{0.0};
    double avg_block_interval_seconds{0.0};
    bool sync_approved{false};
    bool external_worker_online{false};
};

struct WalletRiskSnapshot {
    bool wallet_loaded{false};
    bool wallet_approved{false};
    uint32_t input_count{0};
    uint32_t output_count{0};
    int64_t spend_value_sats{0};
    int64_t available_balance_sats{0};
    bool new_recipient{false};
    bool high_value_relative_to_balance{false};
    bool fragmented_outputs{false};
    bool dust_like_pattern{false};
    bool contains_private_note{false};
};

struct CommsSnapshot {
    uint32_t dht_peers{0};
    uint32_t delivery_failures_1h{0};
    uint32_t proof_failures_1h{0};
    uint32_t suspicious_contacts_1h{0};
    bool relay_only_routing{false};
    bool mail_2fa_enabled{false};
    bool contains_private_content{false};
};

struct ChainActivitySnapshot {
    uint64_t best_height{0};
    uint32_t current_bits{0};
    uint64_t mempool_transactions{0};
    uint64_t recent_large_transfers{0};
    uint64_t active_address_count_24h{0};
    double avg_block_interval_seconds{0.0};
    double fee_rate_median_sats{0.0};
};

struct SnapshotBundle {
    std::optional<NetworkSnapshot> network;
    std::optional<MiningSnapshot> mining;
    std::optional<WalletRiskSnapshot> wallet;
    std::optional<CommsSnapshot> comms;
    std::optional<ChainActivitySnapshot> chain;
};

struct ProviderRequest {
    std::string request_id;
    AdvisoryKind kind{AdvisoryKind::Unknown};
    SnapshotBundle snapshots;
    std::optional<ProviderKind> preferred_provider;
    bool requires_remote_provider{false};
    bool contains_sensitive_wallet_data{false};
    bool contains_sensitive_message_content{false};
    bool allow_public_web_grounding{false};
    bool allow_remote_storage{false};
    std::string operator_prompt;
    std::string system_context;
};

struct ProviderCapabilities {
    ProviderKind kind{ProviderKind::None};
    std::string display_name;
    bool remote{false};
    bool supports_structured_output{false};
    bool supports_function_calls{false};
    bool supports_background_tasks{false};
    std::vector<AdvisoryKind> supported_kinds;

    bool supports(AdvisoryKind advisory_kind) const {
        return supported_kinds.empty() ||
               std::find(supported_kinds.begin(), supported_kinds.end(), advisory_kind) != supported_kinds.end();
    }
};

struct ProviderResponse {
    ProviderKind provider{ProviderKind::None};
    bool accepted{false};
    std::string diagnostic;
    std::optional<AdvisoryResult> advisory;
};

} // namespace cryptex::intelligence
