#pragma once

namespace net::minecraft::client::render::atmosphere {

class StarFieldRenderer {
public:
    void build();
    void draw(float brightness) const;
    void release();

    [[nodiscard]] int glList() const { return glList_; }

private:
    int glList_ = 0;
};

} // namespace net::minecraft::client::render::atmosphere
