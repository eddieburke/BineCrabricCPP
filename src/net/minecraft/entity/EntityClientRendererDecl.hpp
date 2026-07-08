#pragma once
#include <memory>

namespace net::minecraft::client::render::entity {
class EntityRenderer;
}

namespace net::minecraft::entity {
// Nested in each entity class as `ClientRenderer`.
struct EntityClientRendererDecl {
    static std::unique_ptr<::net::minecraft::client::render::entity::EntityRenderer> create();
};
}  // namespace net::minecraft::entity
