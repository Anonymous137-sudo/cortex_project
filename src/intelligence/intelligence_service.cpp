#include "intelligence/intelligence_service.hpp"

#include "intelligence/rules_provider.hpp"

namespace cryptex::intelligence {

IntelligenceService::IntelligenceService(IntelligenceConfig config)
    : config_(std::move(config)),
      bus_(config_.policy) {
    bus_.register_provider(std::make_shared<RulesOnlyProvider>());
}

const IntelligenceConfig& IntelligenceService::config() const {
    return config_;
}

void IntelligenceService::set_config(const IntelligenceConfig& config) {
    config_ = config;
    bus_.set_policy(config_.policy);
}

IntelligenceBus& IntelligenceService::bus() {
    return bus_;
}

const IntelligenceBus& IntelligenceService::bus() const {
    return bus_;
}

void IntelligenceService::register_provider(const InferenceProviderPtr& provider) {
    bus_.register_provider(provider);
}

void IntelligenceService::apply_defaults(ProviderRequest& request) const {
    if (request.request_id.empty()) {
        request.request_id = make_request_id("cortex-ai");
    }
    if (!request.preferred_provider && config_.default_provider != ProviderKind::None) {
        request.preferred_provider = config_.default_provider;
    }
}

ProviderRequest IntelligenceService::build_network_health_request(const Blockchain& chain,
                                                                  const net::NetworkNode& node,
                                                                  const NetworkTelemetry& telemetry,
                                                                  std::string operator_prompt,
                                                                  std::string system_context) const {
    ProviderRequest request;
    request.kind = AdvisoryKind::NetworkHealth;
    request.snapshots.network = SnapshotCollector::collect_network(chain, node, telemetry);
    request.snapshots.chain = SnapshotCollector::collect_chain(chain);
    request.operator_prompt = std::move(operator_prompt);
    request.system_context = std::move(system_context);
    apply_defaults(request);
    return request;
}

ProviderRequest IntelligenceService::build_mining_health_request(const Blockchain& chain,
                                                                 const net::NetworkNode* node,
                                                                 const MiningTelemetry& telemetry,
                                                                 std::string operator_prompt,
                                                                 std::string system_context) const {
    ProviderRequest request;
    request.kind = AdvisoryKind::MiningHealth;
    request.snapshots.mining = SnapshotCollector::collect_mining(chain, node, telemetry);
    request.snapshots.chain = SnapshotCollector::collect_chain(chain);
    request.operator_prompt = std::move(operator_prompt);
    request.system_context = std::move(system_context);
    apply_defaults(request);
    return request;
}

ProviderRequest IntelligenceService::build_chain_activity_request(const Blockchain& chain,
                                                                  std::string operator_prompt,
                                                                  std::string system_context) const {
    ProviderRequest request;
    request.kind = AdvisoryKind::ChainActivity;
    request.snapshots.chain = SnapshotCollector::collect_chain(chain);
    request.operator_prompt = std::move(operator_prompt);
    request.system_context = std::move(system_context);
    apply_defaults(request);
    return request;
}

ProviderRequest IntelligenceService::build_transaction_risk_request(const Wallet& wallet,
                                                                    Blockchain& chain,
                                                                    const TransactionDraftSignals& signals,
                                                                    std::string operator_prompt,
                                                                    std::string system_context) const {
    ProviderRequest request;
    request.kind = AdvisoryKind::TransactionRisk;
    request.snapshots.wallet = SnapshotCollector::collect_wallet(wallet, chain, signals);
    request.snapshots.chain = SnapshotCollector::collect_chain(chain);
    request.contains_sensitive_wallet_data = signals.spend_value_sats > 0 || signals.contains_private_note;
    request.contains_sensitive_message_content = signals.contains_private_note;
    request.operator_prompt = std::move(operator_prompt);
    request.system_context = std::move(system_context);
    apply_defaults(request);
    return request;
}

ProviderRequest IntelligenceService::build_communication_safety_request(const net::NetworkNode& node,
                                                                        const CommsTelemetry& telemetry,
                                                                        std::string operator_prompt,
                                                                        std::string system_context) const {
    ProviderRequest request;
    request.kind = AdvisoryKind::CommunicationSafety;
    request.snapshots.comms = SnapshotCollector::collect_comms(node, telemetry);
    request.contains_sensitive_message_content = telemetry.contains_private_content;
    request.operator_prompt = std::move(operator_prompt);
    request.system_context = std::move(system_context);
    apply_defaults(request);
    return request;
}

ProviderRequest IntelligenceService::build_protocol_explanation_request(std::string operator_prompt,
                                                                        std::string system_context,
                                                                        std::optional<ProviderKind> preferred_provider,
                                                                        bool allow_public_web_grounding) const {
    ProviderRequest request;
    request.kind = AdvisoryKind::ProtocolExplanation;
    request.preferred_provider = preferred_provider;
    request.allow_public_web_grounding = allow_public_web_grounding;
    request.requires_remote_provider = preferred_provider.has_value() &&
                                       *preferred_provider != ProviderKind::RulesOnly &&
                                       *preferred_provider != ProviderKind::None;
    request.operator_prompt = std::move(operator_prompt);
    request.system_context = std::move(system_context);
    apply_defaults(request);
    return request;
}

AdvisoryResult IntelligenceService::analyze_network_health(const Blockchain& chain,
                                                           const net::NetworkNode& node,
                                                           const NetworkTelemetry& telemetry,
                                                           std::string operator_prompt,
                                                           std::string system_context) const {
    return bus_.analyze(build_network_health_request(chain,
                                                     node,
                                                     telemetry,
                                                     std::move(operator_prompt),
                                                     std::move(system_context)));
}

AdvisoryResult IntelligenceService::analyze_mining_health(const Blockchain& chain,
                                                          const net::NetworkNode* node,
                                                          const MiningTelemetry& telemetry,
                                                          std::string operator_prompt,
                                                          std::string system_context) const {
    return bus_.analyze(build_mining_health_request(chain,
                                                    node,
                                                    telemetry,
                                                    std::move(operator_prompt),
                                                    std::move(system_context)));
}

AdvisoryResult IntelligenceService::analyze_chain_activity(const Blockchain& chain,
                                                           std::string operator_prompt,
                                                           std::string system_context) const {
    return bus_.analyze(build_chain_activity_request(chain,
                                                     std::move(operator_prompt),
                                                     std::move(system_context)));
}

AdvisoryResult IntelligenceService::analyze_transaction_risk(const Wallet& wallet,
                                                             Blockchain& chain,
                                                             const TransactionDraftSignals& signals,
                                                             std::string operator_prompt,
                                                             std::string system_context) const {
    return bus_.analyze(build_transaction_risk_request(wallet,
                                                       chain,
                                                       signals,
                                                       std::move(operator_prompt),
                                                       std::move(system_context)));
}

AdvisoryResult IntelligenceService::analyze_communication_safety(const net::NetworkNode& node,
                                                                 const CommsTelemetry& telemetry,
                                                                 std::string operator_prompt,
                                                                 std::string system_context) const {
    return bus_.analyze(build_communication_safety_request(node,
                                                           telemetry,
                                                           std::move(operator_prompt),
                                                           std::move(system_context)));
}

AdvisoryResult IntelligenceService::explain_protocol(std::string operator_prompt,
                                                     std::string system_context,
                                                     std::optional<ProviderKind> preferred_provider,
                                                     bool allow_public_web_grounding) const {
    return bus_.analyze(build_protocol_explanation_request(std::move(operator_prompt),
                                                           std::move(system_context),
                                                           preferred_provider,
                                                           allow_public_web_grounding));
}

} // namespace cryptex::intelligence
