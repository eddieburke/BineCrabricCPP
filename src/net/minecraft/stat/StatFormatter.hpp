#pragma once
#include <cmath>
#include <cstdio>
#include <string>

namespace net::minecraft::stat {
enum class StatFormatterKind {
    Integer,
    Time,
    Distance,
};

inline std::string formatStatValue(StatFormatterKind kind, int value) {
    switch (kind) {
        case StatFormatterKind::Integer:
            return std::to_string(value);
        case StatFormatterKind::Time:
            {
                const double seconds = static_cast<double>(value) / 20.0;
                const double minutes = seconds / 60.0;
                const double hours = minutes / 60.0;
                const double days = hours / 24.0;
                const double years = days / 365.0;
                char buffer[32];
                if (years > 0.5) {
                    std::snprintf(buffer, sizeof(buffer), "%.1f y", years);
                    return buffer;
                }
                if (days > 0.5) {
                    std::snprintf(buffer, sizeof(buffer), "%.1f d", days);
                    return buffer;
                }
                if (hours > 0.5) {
                    std::snprintf(buffer, sizeof(buffer), "%.1f h", hours);
                    return buffer;
                }
                if (minutes > 0.5) {
                    std::snprintf(buffer, sizeof(buffer), "%.1f m", minutes);
                    return buffer;
                }
                std::snprintf(buffer, sizeof(buffer), "%.1f s", seconds);
                return buffer;
            }
        case StatFormatterKind::Distance:
            {
                const double centimeters = static_cast<double>(value);
                const double meters = centimeters / 100.0;
                const double kilometers = meters / 1000.0;
                char buffer[32];
                if (kilometers > 0.5) {
                    std::snprintf(buffer, sizeof(buffer), "%.1f km", kilometers);
                    return buffer;
                }
                if (meters > 0.5) {
                    std::snprintf(buffer, sizeof(buffer), "%.1f m", meters);
                    return buffer;
                }
                return std::to_string(value) + " cm";
            }
    }
    return std::to_string(value);
}
}  // namespace net::minecraft::stat
