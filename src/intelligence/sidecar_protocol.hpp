#pragma once

#include "intelligence/intelligence_types.hpp"

#include <cstdint>
#include <string>

namespace cryptex::intelligence {

constexpr uint32_t kSidecarProtocolVersion = 1;

struct SidecarRequestEnvelope {
    uint32_t version{kSidecarProtocolVersion};
    std::string request_id;
    AdvisoryKind kind{AdvisoryKind::Unknown};
    ProviderKind preferred_provider{ProviderKind::None};
    bool remote_allowed{false};
    bool allow_public_web_grounding{false};
    bool allow_remote_storage{false};
    bool contains_sensitive_wallet_data{false};
    bool contains_sensitive_message_content{false};
    std::string payload_json;
};

struct SidecarResponseEnvelope {
    uint32_t version{kSidecarProtocolVersion};
    std::string request_id;
    ProviderKind provider{ProviderKind::None};
    bool success{false};
    std::string diagnostic;
    std::string advisory_json;
};

std::string encode_sidecar_request_json(const SidecarRequestEnvelope& envelope);
SidecarRequestEnvelope decode_sidecar_request_json(const std::string& json);

std::string encode_sidecar_response_json(const SidecarResponseEnvelope& envelope);
SidecarResponseEnvelope decode_sidecar_response_json(const std::string& json);

} // namespace cryptex::intelligence
