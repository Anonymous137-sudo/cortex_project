#pragma once

#include "intelligence/inference_provider.hpp"
#include "intelligence/intelligence_policy.hpp"
#include "intelligence/null_provider.hpp"

#include <map>
#include <memory>

namespace cryptex::intelligence {

class IntelligenceBus {
public:
    explicit IntelligenceBus(IntelligencePolicy policy = {});

    void set_policy(const IntelligencePolicy& policy);
    const IntelligencePolicy& policy() const;

    void register_provider(const InferenceProviderPtr& provider);
    void clear_providers();

    DispatchDecision inspect(const ProviderRequest& request) const;
    AdvisoryResult analyze(const ProviderRequest& request) const;

private:
    const InferenceProvider* select_provider(const ProviderRequest& request) const;

    IntelligencePolicy policy_;
    std::map<ProviderKind, InferenceProviderPtr> providers_;
    InferenceProviderPtr fallback_;
};

} // namespace cryptex::intelligence
