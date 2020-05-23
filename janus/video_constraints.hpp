// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from janus-client.djinni

#pragma once

#include "camera.hpp"
#include <cstdint>
#include <utility>

namespace Janus {

struct VideoConstraints final {
    int32_t width;
    int32_t height;
    int32_t fps;
    Camera camera;

    VideoConstraints(int32_t width_,
                     int32_t height_,
                     int32_t fps_,
                     Camera camera_)
    : width(std::move(width_))
    , height(std::move(height_))
    , fps(std::move(fps_))
    , camera(std::move(camera_))
    {}
};

}  // namespace Janus
