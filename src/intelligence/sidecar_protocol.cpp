#include "intelligence/sidecar_protocol.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <sstream>
#include <stdexcept>

namespace cryptex::intelligence {

namespace pt = boost::property_tree;

namespace {

pt::ptree parse_tree(const std::string& json) {
    std::istringstream in(json);
    pt::ptree tree;
    pt::read_json(in, tree);
    return tree;
}

std::string write_tree(const pt::ptree& tree) {
    std::ostringstream out;
    pt::write_json(out, tree, false);
    return out.str();
}

template <typename T>
T required(const pt::ptree& tree, const std::string& path) {
    auto value = tree.get_optional<T>(path);
    if (!value) {
        throw std::runtime_error("missing required field: " + path);
    }
    return *value;
}

} // namespace

std::string encode_sidecar_request_json(const SidecarRequestEnvelope& envelope) {
    pt::ptree tree;
    tree.put("version", envelope.version);
    tree.put("request_id", envelope.request_id);
    tree.put("kind", to_string(envelope.kind));
    tree.put("preferred_provider", to_string(envelope.preferred_provider));
    tree.put("remote_allowed", envelope.remote_allowed);
    tree.put("allow_public_web_grounding", envelope.allow_public_web_grounding);
    tree.put("allow_remote_storage", envelope.allow_remote_storage);
    tree.put("contains_sensitive_wallet_data", envelope.contains_sensitive_wallet_data);
    tree.put("contains_sensitive_message_content", envelope.contains_sensitive_message_content);
    tree.put("payload_json", envelope.payload_json);
    return write_tree(tree);
}

SidecarRequestEnvelope decode_sidecar_request_json(const std::string& json) {
    const auto tree = parse_tree(json);
    SidecarRequestEnvelope envelope;
    envelope.version = required<uint32_t>(tree, "version");
    envelope.request_id = required<std::string>(tree, "request_id");
    envelope.remote_allowed = tree.get<bool>("remote_allowed", false);
    envelope.allow_public_web_grounding = tree.get<bool>("allow_public_web_grounding", false);
    envelope.allow_remote_storage = tree.get<bool>("allow_remote_storage", false);
    envelope.contains_sensitive_wallet_data = tree.get<bool>("contains_sensitive_wallet_data", false);
    envelope.contains_sensitive_message_content = tree.get<bool>("contains_sensitive_message_content", false);
    envelope.payload_json = tree.get<std::string>("payload_json", "");

    const auto kind = parse_advisory_kind(required<std::string>(tree, "kind"));
    if (!kind) {
        throw std::runtime_error("invalid advisory kind");
    }
    envelope.kind = *kind;

    const auto provider = parse_provider_kind(tree.get<std::string>("preferred_provider", "none"));
    if (!provider) {
        throw std::runtime_error("invalid preferred provider");
    }
    envelope.preferred_provider = *provider;
    return envelope;
}

std::string encode_sidecar_response_json(const SidecarResponseEnvelope& envelope) {
    pt::ptree tree;
    tree.put("version", envelope.version);
    tree.put("request_id", envelope.request_id);
    tree.put("provider", to_string(envelope.provider));
    tree.put("success", envelope.success);
    tree.put("diagnostic", envelope.diagnostic);
    tree.put("advisory_json", envelope.advisory_json);
    return write_tree(tree);
}

SidecarResponseEnvelope decode_sidecar_response_json(const std::string& json) {
    const auto tree = parse_tree(json);
    SidecarResponseEnvelope envelope;
    envelope.version = required<uint32_t>(tree, "version");
    envelope.request_id = required<std::string>(tree, "request_id");
    envelope.success = tree.get<bool>("success", false);
    envelope.diagnostic = tree.get<std::string>("diagnostic", "");
    envelope.advisory_json = tree.get<std::string>("advisory_json", "");

    const auto provider = parse_provider_kind(tree.get<std::string>("provider", "none"));
    if (!provider) {
        throw std::runtime_error("invalid response provider");
    }
    envelope.provider = *provider;
    return envelope;
}

} // namespace cryptex::intelligence
