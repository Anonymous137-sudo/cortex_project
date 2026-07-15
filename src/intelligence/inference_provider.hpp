#pragma once

#include "intelligence/intelligence_types.hpp"

#include <memory>

namespace cryptex::intelligence {

class InferenceProvider {
public:
    virtual ~InferenceProvider() = default;

    virtual ProviderCapabilities capabilities() const = 0;
    virtual ProviderResponse analyze(const ProviderRequest& request) const = 0;
};

using InferenceProviderPtr = std::shared_ptr<InferenceProvider>;

} // namespace cryptex::intelligence
