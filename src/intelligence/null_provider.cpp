#include "intelligence/null_provider.hpp"

namespace cryptex::intelligence {

ProviderCapabilities NullInferenceProvider::capabilities() const {
    ProviderCapabilities caps;
    caps.kind = ProviderKind::None;
    caps.display_name = "null";
    caps.remote = false;
    caps.supports_structured_output = true;
    return caps;
}

ProviderResponse NullInferenceProvider::analyze(const ProviderRequest& request) const {
    ProviderResponse response;
    response.provider = ProviderKind::None;
    response.accepted = true;
    response.diagnostic = "no inference provider configured";
    response.advisory = AdvisoryResult::unavailable(request.kind, response.diagnostic);
    return response;
}

} // namespace cryptex::intelligence
