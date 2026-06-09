#pragma once

// M2 single-include contract: include this header at most once per file that
// needs net::minecraft client-level type aliases. Do not sandwich includes between
// other headers — include ClientTypes.hpp once in the include block.

#include "net/minecraft/client/ClientForward.hpp"

namespace net::minecraft {

using OtherPlayerEntity     = client::network::OtherPlayerEntity;
using ImageDownload         = client::texture::ImageDownload;
using SkinImageProcessor    = client::texture::SkinImageProcessor;
using ImageProcessor        = client::texture::ImageProcessor;
using Vertex                = client::model::Vertex;
using Quad                  = client::model::Quad;
using MultiplayerChunkCache = client::world::chunk::MultiplayerChunkCache; // canonical — never net::minecraft::world::chunk

} // namespace net::minecraft
