#include "intelligence/intelligence_policy.hpp"

#include <algorithm>
#include <cctype>

namespace cryptex::intelligence {

namespace {

std::string normalize_method(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return out;
}

bool contains_method(const std::vector<std::string>& methods, std::string_view method) {
    const auto normalized = normalize_method(method);
    return std::find(methods.begin(), methods.end(), normalized) != methods.end();
}

DispatchDecision blocked(ProviderKind provider, bool remote_path, const std::string& reason) {
    DispatchDecision decision;
    decision.allowed = false;
    decision.provider = provider;
    decision.remote_path = remote_path;
    decision.reason = reason;
    return decision;
}

} // namespace

std::vector<std::string> default_blocked_rpc_methods() {
    return {
        "sendtoaddress",
        "submitblock",
        "stop",
        "importprivkey",
        "dumpprivkey",
        "dumpmnemonic",
        "backupwallet",
        "setaddresslabel",
        "setprimaryaddress",
        "setchatproxyconfig",
        "setp2pmailproxyconfig",
        "setp2pmailsecurity",
        "setp2pmailpolicy",
        "upsertchatprivatecontact",
        "startvoicecall",
        "acceptvoicecall",
        "declinevoicecall",
        "endvoicecall",
        "sendvoicecallaudio",
        "pincheckpoint",
        "clearcheckpointpin"
    };
}

bool IntelligencePolicy::can_invoke_rpc_method(std::string_view method) const {
    if (method.empty()) {
        return false;
    }
    return !contains_method(blocked_rpc_methods, method);
}

bool request_carries_sensitive_data(const ProviderRequest& request) {
    return request.contains_sensitive_wallet_data || request.contains_sensitive_message_content;
}

DispatchDecision evaluate_dispatch(const IntelligencePolicy& policy,
                                   const ProviderCapabilities& capabilities,
                                   const ProviderRequest& request) {
    if (request.kind == AdvisoryKind::Unknown) {
        return blocked(capabilities.kind, capabilities.remote, "unknown advisory kind");
    }
    if (!capabilities.supports(request.kind)) {
        return blocked(capabilities.kind, capabilities.remote, "provider does not support requested advisory kind");
    }
    if (!capabilities.remote) {
        DispatchDecision decision;
        decision.allowed = true;
        decision.provider = capabilities.kind;
        decision.remote_path = false;
        decision.reason = "local provider allowed";
        return decision;
    }
    if (!policy.remote_providers_enabled) {
        return blocked(capabilities.kind, true, "remote providers are disabled by policy");
    }
    if (request.allow_public_web_grounding && !policy.allow_public_web_grounding) {
        return blocked(capabilities.kind, true, "public web grounding is disabled by policy");
    }
    if (request.allow_remote_storage && !policy.allow_remote_storage) {
        return blocked(capabilities.kind, true, "remote storage is disabled by policy");
    }
    if (request.contains_sensitive_wallet_data && !policy.allow_sensitive_wallet_export) {
        return blocked(capabilities.kind, true, "sensitive wallet data cannot be exported to a remote provider");
    }
    if (request.contains_sensitive_message_content && !policy.allow_sensitive_message_export) {
        return blocked(capabilities.kind, true, "sensitive message content cannot be exported to a remote provider");
    }

    DispatchDecision decision;
    decision.allowed = true;
    decision.provider = capabilities.kind;
    decision.remote_path = true;
    decision.reason = "remote provider allowed";
    return decision;
}

} // namespace cryptex::intelligence
