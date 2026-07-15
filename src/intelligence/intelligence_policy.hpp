#pragma once

#include "intelligence/intelligence_types.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace cryptex::intelligence {

std::vector<std::string> default_blocked_rpc_methods();

struct IntelligencePolicy {
    bool remote_providers_enabled{false};
    bool allow_public_web_grounding{false};
    bool allow_remote_storage{false};
    bool allow_sensitive_wallet_export{false};
    bool allow_sensitive_message_export{false};
    std::vector<std::string> blocked_rpc_methods{default_blocked_rpc_methods()};

    bool can_invoke_rpc_method(std::string_view method) const;
};

struct DispatchDecision {
    bool allowed{false};
    ProviderKind provider{ProviderKind::None};
    bool remote_path{false};
    std::string reason;
};

bool request_carries_sensitive_data(const ProviderRequest& request);
DispatchDecision evaluate_dispatch(const IntelligencePolicy& policy,
                                   const ProviderCapabilities& capabilities,
                                   const ProviderRequest& request);

} // namespace cryptex::intelligence
