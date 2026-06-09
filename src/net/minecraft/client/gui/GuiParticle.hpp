#pragma once

namespace net::minecraft::client::gui {

class ParticlesGui;

class GuiParticle {
public:
    double x = 0.0;
    double y = 0.0;
    double lastX = 0.0;
    double lastY = 0.0;
    double velocityX = 0.0;
    double velocityY = 0.0;
    double velocityChange = 0.0;
    bool removed = false;
    int age = 0;
    int lifetime = 0;
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    double a = 0.0;
    double lastR = 0.0;
    double lastG = 0.0;
    double lastB = 0.0;
    double lastA = 0.0;

    void tickPosition(ParticlesGui* gui)
    {
        (void)gui;
        x += velocityX;
        y += velocityY;
        velocityX *= velocityChange;
        velocityY *= velocityChange;
        velocityY += 0.1;
        if (++age > lifetime) {
            remove();
        }
        a = lifetime > 0 ? 2.0 - static_cast<double>(age) / static_cast<double>(lifetime) * 2.0 : 0.0;
        if (a > 1.0) {
            a = 1.0;
        }
        a *= a;
        a *= 0.5;
    }

    void tickColor()
    {
        lastR = r;
        lastG = g;
        lastB = b;
        lastA = a;
        lastX = x;
        lastY = y;
    }

    void remove()
    {
        removed = true;
    }
};

} // namespace net::minecraft::client::gui
