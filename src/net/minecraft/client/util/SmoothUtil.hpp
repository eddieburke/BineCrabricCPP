#pragma once

namespace net::minecraft::client::util {

class SmoothUtil {
public:
    float smooth(float original, float smoother)
    {
        actualSum += original;
        original = (actualSum - smoothSum) * smoother;
        movementLatency += (original - movementLatency) * 0.5f;
        if ((original > 0.0f && original > movementLatency) || (original < 0.0f && original < movementLatency)) {
            original = movementLatency;
        }
        smoothSum += original;
        return original;
    }

private:
    float actualSum = 0.0f;
    float smoothSum = 0.0f;
    float movementLatency = 0.0f;
};

} // namespace net::minecraft::client::util
