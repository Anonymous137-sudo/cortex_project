#include "intelligence/intelligence_bus.hpp"

#include "debug.hpp"

namespace cryptex::intelligence {

IntelligenceBus::IntelligenceBus(IntelligencePolicy policy)
    : policy_(std::move(policy)),
      fallback_(std::make_shared<NullInferenceProvider>()) {}

void IntelligenceBus::set_policy(const IntelligencePolicy& policy) {
    policy_ = policy;
}

const IntelligencePolicy& IntelligenceBus::policy() const {
    return policy_;
}

void IntelligenceBus::register_provider(const InferenceProviderPtr& provider) {
    if (!provider) {
        return;
    }
    const auto caps = provider->capabilities();
    if (caps.kind == ProviderKind::None) {
        fallback_ = provider;
        return;
    }
    providers_[caps.kind] = provider;
}

void IntelligenceBus::clear_providers() {
    providers_.clear();
}

const InferenceProvider* IntelligenceBus::select_provider(const ProviderRequest& request) const {
    if (request.preferred_provider) {
        const auto it = providers_.find(*request.preferred_provider);
        if (it == providers_.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    const InferenceProvider* best_local = nullptr;
    const InferenceProvider* best_remote = nullptr;
    for (const auto& entry : providers_) {
        const auto& provider = entry.second;
        const auto caps = provider->capabilities();
        if (!caps.supports(request.kind)) {
            continue;
        }
        if (caps.remote) {
            if (!best_remote) {
                best_remote = provider.get();
            }
        } else if (!best_local) {
            best_local = provider.get();
        }
    }

    if (request.requires_remote_provider) {
        return best_remote;
    }
    return best_local ? best_local : best_remote;
}

DispatchDecision IntelligenceBus::inspect(const ProviderRequest& request) const {
    const auto* provider = select_provider(request);
    if (!provider) {
        DispatchDecision decision;
        decision.allowed = false;
        decision.provider = request.preferred_provider.value_or(ProviderKind::None);
        decision.reason = request.preferred_provider
            ? "preferred provider is not registered"
            : "no provider registered for requested advisory kind";
        return decision;
    }
    return evaluate_dispatch(policy_, provider->capabilities(), request);
}

AdvisoryResult IntelligenceBus::analyze(const ProviderRequest& request) const {
    const auto* provider = select_provider(request);
    if (!provider) {
        const auto diagnostic = request.preferred_provider
            ? "preferred provider is not registered"
            : "no provider registered for requested advisory kind";
        log_warn("ai", diagnostic);
        return AdvisoryResult::unavailable(request.kind, diagnostic);
    }

    const auto decision = evaluate_dispatch(policy_, provider->capabilities(), request);
    if (!decision.allowed) {
        log_warn("ai", decision.reason);
        return AdvisoryResult::rejected(request.kind, decision.reason);
    }

    auto response = provider->analyze(request);
    if (!response.accepted || !response.advisory) {
        const auto diagnostic = response.diagnostic.empty()
            ? "provider returned no advisory payload"
            : response.diagnostic;
        log_warn("ai", diagnostic);
        return AdvisoryResult::unavailable(request.kind, diagnostic);
    }

    auto result = *response.advisory;
    result.provider = response.provider;
    result.remote_generated = provider->capabilities().remote;
    if (result.generated_at.empty()) {
        result.generated_at = iso8601_utc_now();
    }
    return result;
}

} // namespace cryptex::intelligence
