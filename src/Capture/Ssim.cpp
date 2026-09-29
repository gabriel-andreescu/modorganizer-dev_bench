#include "Capture/Ssim.h"
#include <QFileInfo>
#include <QImage>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <span>

namespace Bench::Ssim {
namespace {
    constexpr double kC1 = (0.01 * 255.0) * (0.01 * 255.0);
    constexpr double kC2 = (0.03 * 255.0) * (0.03 * 255.0);
    constexpr int kWindow = 8;
    // Shorter than the window so windows overlap and straddle block-periodic content.
    constexpr int kStride = 3;

    struct Rect {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    std::size_t Index(const GrayImage& a_image, int a_x, int a_y) {
        return (static_cast<std::size_t>(a_y) * static_cast<std::size_t>(a_image.width))
               + static_cast<std::size_t>(a_x);
    }

    double RectSsim(const GrayImage& a_left, const GrayImage& a_right, const Rect& a_rect) {
        const auto count = static_cast<double>(a_rect.width) * a_rect.height;
        double meanLeft = 0.0;
        double meanRight = 0.0;
        for (int row = a_rect.y; row < a_rect.y + a_rect.height; ++row) {
            for (int column = a_rect.x; column < a_rect.x + a_rect.width; ++column) {
                meanLeft += a_left.px[Index(a_left, column, row)];
                meanRight += a_right.px[Index(a_right, column, row)];
            }
        }
        meanLeft /= count;
        meanRight /= count;

        double varianceLeft = 0.0;
        double varianceRight = 0.0;
        double covariance = 0.0;
        for (int row = a_rect.y; row < a_rect.y + a_rect.height; ++row) {
            for (int column = a_rect.x; column < a_rect.x + a_rect.width; ++column) {
                const double left = a_left.px[Index(a_left, column, row)] - meanLeft;
                const double right = a_right.px[Index(a_right, column, row)] - meanRight;
                varianceLeft += left * left;
                varianceRight += right * right;
                covariance += left * right;
            }
        }
        varianceLeft /= count - 1;
        varianceRight /= count - 1;
        covariance /= count - 1;

        const double numerator = ((2 * meanLeft * meanRight) + kC1) * ((2 * covariance) + kC2);
        const double denominator = ((meanLeft * meanLeft) + (meanRight * meanRight) + kC1)
                                   * (varianceLeft + varianceRight + kC2);
        return denominator > 0.0 ? numerator / denominator : 1.0;
    }

    GoldenScore ScoreRegions(
        const GrayImage& a_candidate,
        const GrayImage& a_golden,
        const Json& a_regions,
        double a_threshold
    ) {
        GoldenScore result {.ok = true, .threshold = a_threshold};
        double worst = 1.0;
        bool allPassed = true;
        for (const auto& region : a_regions) {
            const auto score = ComputeSsim(CropUv(a_candidate, region), CropUv(a_golden, region));
            const auto threshold = region.value("threshold", a_threshold);
            const bool passed = score >= threshold;
            result.regions.push_back({
                .name = region.value("name", std::string("?")),
                .score = score,
                .threshold = threshold,
                .passed = passed,
            });
            worst = std::min(worst, score);
            allPassed = allPassed && passed;
        }
        result.score = worst;
        result.passed = allPassed;
        return result;
    }
}

std::optional<GrayImage> DecodeGray(const QString& a_path) {
    const QImage source(a_path);
    if (source.isNull()) {
        return std::nullopt;
    }
    const auto gray = source.convertToFormat(QImage::Format_Grayscale8);
    GrayImage image {.width = gray.width(), .height = gray.height()};
    image.px.reserve(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height));
    for (int row = 0; row < gray.height(); ++row) {
        for (const auto value : std::span(gray.constScanLine(row), static_cast<std::size_t>(gray.width()))) {
            image.px.push_back(static_cast<float>(value));
        }
    }
    return image;
}

GrayImage CropUv(const GrayImage& a_image, const Json& a_uv) {
    const double left = a_uv.value("x", 0.0);
    const double top = a_uv.value("y", 0.0);
    const double width = a_uv.value("w", 1.0);
    const double height = a_uv.value("h", 1.0);
    const auto scale = [](double a_value, int a_size, int a_minimum) {
        return std::clamp(static_cast<int>(std::lround(a_value * a_size)), a_minimum, a_size);
    };
    const int firstColumn = scale(left, a_image.width, 0);
    const int firstRow = scale(top, a_image.height, 0);
    const int endColumn = scale(left + width, a_image.width, firstColumn);
    const int endRow = scale(top + height, a_image.height, firstRow);

    GrayImage crop {.width = endColumn - firstColumn, .height = endRow - firstRow};
    crop.px.reserve(static_cast<std::size_t>(crop.width) * static_cast<std::size_t>(crop.height));
    for (int row = firstRow; row < endRow; ++row) {
        for (int column = firstColumn; column < endColumn; ++column) {
            crop.px.push_back(a_image.px[Index(a_image, column, row)]);
        }
    }
    return crop;
}

double ComputeSsim(const GrayImage& a_left, const GrayImage& a_right) {
    const int width = a_left.width;
    const int height = a_left.height;
    if (width <= 0 || height <= 0) {
        return 0.0;
    }
    const int window = std::min({kWindow, width, height});
    if (window <= 1) {
        return 0.0;
    }

    double sum = 0.0;
    int windows = 0;
    for (int row = 0; row <= height - window; row += kStride) {
        for (int column = 0; column <= width - window; column += kStride) {
            sum += RectSsim(a_left, a_right, {.x = column, .y = row, .width = window, .height = window});
            ++windows;
        }
    }
    return windows > 0 ? sum / windows : RectSsim(a_left, a_right, {.width = width, .height = height});
}

GoldenScore ScoreAgainstGolden(const QString& a_capturedPath, const QString& a_goldenPath, const Json& a_config) {
    const double threshold = a_config.value("threshold", 0.98);
    if (!QFileInfo::exists(a_goldenPath)) {
        return {.error = std::format("No golden at {}", a_goldenPath.toStdString()), .threshold = threshold};
    }
    const auto candidate = DecodeGray(a_capturedPath);
    const auto golden = DecodeGray(a_goldenPath);
    if (!candidate || !golden) {
        return {
            .error = std::format(
                "Failed to decode {} {}",
                candidate ? "golden" : "candidate",
                (candidate ? a_goldenPath : a_capturedPath).toStdString()
            ),
            .threshold = threshold,
        };
    }
    if (candidate->width != golden->width || candidate->height != golden->height) {
        return {
            .error = std::format(
                "Shape mismatch: candidate {}x{} vs golden {}x{}",
                candidate->width,
                candidate->height,
                golden->width,
                golden->height
            ),
            .threshold = threshold,
        };
    }

    const auto regions = a_config.value("regions", Json::array());
    if (!regions.empty()) {
        return ScoreRegions(*candidate, *golden, regions, threshold);
    }
    const auto score = ComputeSsim(*candidate, *golden);
    return {.ok = true, .score = score, .threshold = threshold, .passed = score >= threshold};
}
}
