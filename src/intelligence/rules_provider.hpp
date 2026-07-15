#pragma once

#include "intelligence/inference_provider.hpp"

namespace cryptex::intelligence {

class RulesOnlyProvider final : public InferenceProvider {
public:
    ProviderCapabilities capabilities() const override;
    ProviderResponse analyze(const ProviderRequest& request) const override;
};

} // namespace cryptex::intelligence
