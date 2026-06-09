#pragma once

namespace net::minecraft::client::render::atmosphere {

class SkyDomeRenderer {
public:
    void build();
    void drawLight() const;
    void drawDark(float r, float g, float b) const;
    void release();

private:
    int lightList_ = 0;
    int darkList_ = 0;
};

} // namespace net::minecraft::client::render::atmosphere
