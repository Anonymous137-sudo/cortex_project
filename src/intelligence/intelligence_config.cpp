#include "intelligence/intelligence_config.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace cryptex::intelligence {

namespace {

std::vector<std::string> split_csv(const std::string& text) {
    std::vector<std::string> out;
    std::string current;
    for (char ch : text) {
        if (ch == ',') {
            auto begin = current.find_first_not_of(" \t\r\n");
            auto end = current.find_last_not_of(" \t\r\n");
            if (begin != std::string::npos && end != std::string::npos) {
                out.push_back(current.substr(begin, end - begin + 1));
            }
            current.clear();
            continue;
        }
        current.push_back(ch);
    }
    auto begin = current.find_first_not_of(" \t\r\n");
    auto end = current.find_last_not_of(" \t\r\n");
    if (begin != std::string::npos && end != std::string::npos) {
        out.push_back(current.substr(begin, end - begin + 1));
    }
    return out;
}

std::vector<std::string> merge_repeated_or_csv(const ConfigFile& config,
                                               const std::string& repeated_key,
                                               const std::string& csv_key) {
    auto values = config.get_all(repeated_key);
    if (!values.empty()) {
        return values;
    }
    if (auto csv = config.get_string(csv_key)) {
        return split_csv(*csv);
    }
    return {};
}

void apply_bool(const ConfigFile& config, const std::string& key, bool& target) {
    if (auto value = config.get_bool(key)) {
        target = *value;
    }
}

void apply_uint(const ConfigFile& config, const std::string& key, uint32_t& target) {
    if (auto value = config.get_uint(key)) {
        target = static_cast<uint32_t>(*value);
    }
}

void apply_string(const ConfigFile& config, const std::string& key, std::string& target) {
    if (auto value = config.get_string(key)) {
        target = *value;
    }
}

void load_remote_provider(const ConfigFile& config,
                          const std::string& prefix,
                          RemoteProviderConfig& provider) {
    apply_bool(config, prefix + "_enabled", provider.enabled);
    apply_string(config, prefix + "_endpoint", provider.endpoint);
    apply_string(config, prefix + "_model", provider.model);
    apply_string(config, prefix + "_api_key_env", provider.api_key_env);
    apply_uint(config, prefix + "_timeout_ms", provider.timeout_ms);
}

} // namespace

IntelligenceConfig IntelligenceConfig::from_config(const ConfigFile& config) {
    IntelligenceConfig out;

    apply_bool(config, "ai_enabled", out.enabled);
    apply_bool(config, "ai_audit_log", out.audit_log_enabled);

    if (auto provider_text = config.get_string("ai_default_provider")) {
        const auto provider = parse_provider_kind(*provider_text);
        if (!provider) {
            throw std::runtime_error("invalid ai_default_provider value");
        }
        out.default_provider = *provider;
    }

    apply_bool(config, "ai_remote_enabled", out.policy.remote_providers_enabled);
    apply_bool(config, "ai_allow_public_web_grounding", out.policy.allow_public_web_grounding);
    apply_bool(config, "ai_allow_remote_storage", out.policy.allow_remote_storage);
    apply_bool(config, "ai_allow_sensitive_wallet_export", out.policy.allow_sensitive_wallet_export);
    apply_bool(config, "ai_allow_sensitive_message_export", out.policy.allow_sensitive_message_export);

    const auto blocked_methods = merge_repeated_or_csv(config,
                                                       "ai_blocked_rpc_method",
                                                       "ai_blocked_rpc_methods");
    if (!blocked_methods.empty()) {
        for (const auto& method : blocked_methods) {
            std::string normalized;
            normalized.reserve(method.size());
            for (char ch : method) {
                if (!std::isspace(static_cast<unsigned char>(ch))) {
                    normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
                }
            }
            if (!normalized.empty() &&
                std::find(out.policy.blocked_rpc_methods.begin(),
                          out.policy.blocked_rpc_methods.end(),
                          normalized) == out.policy.blocked_rpc_methods.end()) {
                out.policy.blocked_rpc_methods.push_back(std::move(normalized));
            }
        }
    }

    if (auto executable = config.get_string("ai_sidecar_executable")) {
        SidecarConfig sidecar;
        sidecar.executable = *executable;
        sidecar.args = merge_repeated_or_csv(config, "ai_sidecar_arg", "ai_sidecar_args");
        apply_uint(config, "ai_sidecar_timeout_ms", sidecar.timeout_ms);
        out.sidecar = std::move(sidecar);
    }

    load_remote_provider(config, "ai_gemini", out.gemini);
    load_remote_provider(config, "ai_groq", out.groq);
    return out;
}

} // namespace cryptex::intelligence
