#pragma once

#include "config.hpp"
#include "intelligence/intelligence_policy.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace cryptex::intelligence {

struct SidecarConfig {
    std::filesystem::path executable;
    std::vector<std::string> args;
    uint32_t timeout_ms{5000};
};

struct RemoteProviderConfig {
    bool enabled{false};
    ProviderKind kind{ProviderKind::None};
    std::string endpoint;
    std::string model;
    std::string api_key_env;
    uint32_t timeout_ms{8000};
};

struct IntelligenceConfig {
    bool enabled{true};
    bool audit_log_enabled{true};
    ProviderKind default_provider{ProviderKind::RulesOnly};
    IntelligencePolicy policy{};
    std::optional<SidecarConfig> sidecar;
    RemoteProviderConfig gemini{
        false,
        ProviderKind::Gemini,
        "https://generativelanguage.googleapis.com",
        "gemini-2.5-flash",
        "GEMINI_API_KEY",
        8000,
    };
    RemoteProviderConfig groq{
        false,
        ProviderKind::Groq,
        "https://api.groq.com",
        "openai/gpt-oss-20b",
        "GROQ_API_KEY",
        6000,
    };

    static IntelligenceConfig from_config(const ConfigFile& config);
};

} // namespace cryptex::intelligence
