#pragma once

#include "intelligence/collectors/snapshot_collector.hpp"
#include "intelligence/intelligence_bus.hpp"
#include "intelligence/intelligence_config.hpp"

namespace cryptex::intelligence {

class IntelligenceService {
public:
    explicit IntelligenceService(IntelligenceConfig config = {});

    const IntelligenceConfig& config() const;
    void set_config(const IntelligenceConfig& config);

    IntelligenceBus& bus();
    const IntelligenceBus& bus() const;

    void register_provider(const InferenceProviderPtr& provider);

    ProviderRequest build_network_health_request(const Blockchain& chain,
                                                 const net::NetworkNode& node,
                                                 const NetworkTelemetry& telemetry = {},
                                                 std::string operator_prompt = {},
                                                 std::string system_context = {}) const;

    ProviderRequest build_mining_health_request(const Blockchain& chain,
                                                const net::NetworkNode* node = nullptr,
                                                const MiningTelemetry& telemetry = {},
                                                std::string operator_prompt = {},
                                                std::string system_context = {}) const;

    ProviderRequest build_chain_activity_request(const Blockchain& chain,
                                                 std::string operator_prompt = {},
                                                 std::string system_context = {}) const;

    ProviderRequest build_transaction_risk_request(const Wallet& wallet,
                                                   Blockchain& chain,
                                                   const TransactionDraftSignals& signals = {},
                                                   std::string operator_prompt = {},
                                                   std::string system_context = {}) const;

    ProviderRequest build_communication_safety_request(const net::NetworkNode& node,
                                                       const CommsTelemetry& telemetry = {},
                                                       std::string operator_prompt = {},
                                                       std::string system_context = {}) const;

    ProviderRequest build_protocol_explanation_request(std::string operator_prompt,
                                                       std::string system_context = {},
                                                       std::optional<ProviderKind> preferred_provider = std::nullopt,
                                                       bool allow_public_web_grounding = false) const;

    AdvisoryResult analyze_network_health(const Blockchain& chain,
                                          const net::NetworkNode& node,
                                          const NetworkTelemetry& telemetry = {},
                                          std::string operator_prompt = {},
                                          std::string system_context = {}) const;

    AdvisoryResult analyze_mining_health(const Blockchain& chain,
                                         const net::NetworkNode* node = nullptr,
                                         const MiningTelemetry& telemetry = {},
                                         std::string operator_prompt = {},
                                         std::string system_context = {}) const;

    AdvisoryResult analyze_chain_activity(const Blockchain& chain,
                                          std::string operator_prompt = {},
                                          std::string system_context = {}) const;

    AdvisoryResult analyze_transaction_risk(const Wallet& wallet,
                                            Blockchain& chain,
                                            const TransactionDraftSignals& signals = {},
                                            std::string operator_prompt = {},
                                            std::string system_context = {}) const;

    AdvisoryResult analyze_communication_safety(const net::NetworkNode& node,
                                                const CommsTelemetry& telemetry = {},
                                                std::string operator_prompt = {},
                                                std::string system_context = {}) const;

    AdvisoryResult explain_protocol(std::string operator_prompt,
                                    std::string system_context = {},
                                    std::optional<ProviderKind> preferred_provider = std::nullopt,
                                    bool allow_public_web_grounding = false) const;

private:
    void apply_defaults(ProviderRequest& request) const;

    IntelligenceConfig config_;
    IntelligenceBus bus_;
};

} // namespace cryptex::intelligence
