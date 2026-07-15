#include "intelligence/intelligence_types.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace cryptex::intelligence {

namespace {

std::string lowercase(std::string_view text) {
    std::string out(text.begin(), text.end());
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

} // namespace

std::string to_string(ProviderKind kind) {
    switch (kind) {
    case ProviderKind::None: return "none";
    case ProviderKind::RulesOnly: return "rules_only";
    case ProviderKind::Gemini: return "gemini";
    case ProviderKind::Groq: return "groq";
    }
    return "none";
}

std::optional<ProviderKind> parse_provider_kind(std::string_view text) {
    const auto normalized = lowercase(text);
    if (normalized == "none") return ProviderKind::None;
    if (normalized == "rules_only" || normalized == "rules-only" || normalized == "rules") return ProviderKind::RulesOnly;
    if (normalized == "gemini") return ProviderKind::Gemini;
    if (normalized == "groq") return ProviderKind::Groq;
    return std::nullopt;
}

std::string to_string(AdvisoryKind kind) {
    switch (kind) {
    case AdvisoryKind::Unknown: return "unknown";
    case AdvisoryKind::NetworkHealth: return "network_health";
    case AdvisoryKind::TransactionRisk: return "transaction_risk";
    case AdvisoryKind::MiningHealth: return "mining_health";
    case AdvisoryKind::CommunicationSafety: return "communication_safety";
    case AdvisoryKind::ChainActivity: return "chain_activity";
    case AdvisoryKind::ProtocolExplanation: return "protocol_explanation";
    }
    return "unknown";
}

std::optional<AdvisoryKind> parse_advisory_kind(std::string_view text) {
    const auto normalized = lowercase(text);
    if (normalized == "unknown") return AdvisoryKind::Unknown;
    if (normalized == "network_health" || normalized == "network-health") return AdvisoryKind::NetworkHealth;
    if (normalized == "transaction_risk" || normalized == "transaction-risk") return AdvisoryKind::TransactionRisk;
    if (normalized == "mining_health" || normalized == "mining-health") return AdvisoryKind::MiningHealth;
    if (normalized == "communication_safety" || normalized == "communication-safety" || normalized == "comms") {
        return AdvisoryKind::CommunicationSafety;
    }
    if (normalized == "chain_activity" || normalized == "chain-activity") return AdvisoryKind::ChainActivity;
    if (normalized == "protocol_explanation" || normalized == "protocol-explanation") {
        return AdvisoryKind::ProtocolExplanation;
    }
    return std::nullopt;
}

std::string to_string(Severity severity) {
    switch (severity) {
    case Severity::Info: return "info";
    case Severity::Low: return "low";
    case Severity::Medium: return "medium";
    case Severity::High: return "high";
    case Severity::Critical: return "critical";
    }
    return "info";
}

std::optional<Severity> parse_severity(std::string_view text) {
    const auto normalized = lowercase(text);
    if (normalized == "info") return Severity::Info;
    if (normalized == "low") return Severity::Low;
    if (normalized == "medium") return Severity::Medium;
    if (normalized == "high") return Severity::High;
    if (normalized == "critical") return Severity::Critical;
    return std::nullopt;
}

std::string iso8601_utc_now() {
    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &tt);
#else
    gmtime_r(&tt, &tm);
#endif
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

std::string make_request_id(std::string_view prefix) {
    static std::atomic<uint64_t> counter{0};
    const auto sequence = counter.fetch_add(1, std::memory_order_relaxed);
    const auto ticks = static_cast<uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::ostringstream out;
    out << prefix << "-" << ticks << "-" << sequence;
    return out.str();
}

AdvisoryResult AdvisoryResult::rejected(AdvisoryKind kind, const std::string& diagnostic) {
    AdvisoryResult result;
    result.kind = kind;
    result.severity = Severity::Info;
    result.generated_at = iso8601_utc_now();
    result.summary = "analysis blocked by policy";
    result.valid = false;
    result.diagnostic = diagnostic;
    return result;
}

AdvisoryResult AdvisoryResult::unavailable(AdvisoryKind kind, const std::string& diagnostic) {
    AdvisoryResult result;
    result.kind = kind;
    result.severity = Severity::Info;
    result.generated_at = iso8601_utc_now();
    result.summary = "analysis provider unavailable";
    result.valid = false;
    result.diagnostic = diagnostic;
    return result;
}

} // namespace cryptex::intelligence
