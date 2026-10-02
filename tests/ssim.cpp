#include "Capture/Ssim.h"
#include "Harness.h"
#include <QDir>
#include <QImage>
#include <QTemporaryDir>
#include <cmath>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <utility>

namespace {
using Bench::Ssim::ComputeSsim;
using Bench::Ssim::CropUv;
using Bench::Ssim::DecodeGray;
using Bench::Ssim::GrayImage;
using Bench::Ssim::ScoreAgainstGolden;

std::uint8_t Checker(int a_x, int a_y) {
    return ((a_x / 4) + (a_y / 4)) % 2 == 0 ? 220 : 30;
}

QString Write(
    const QTemporaryDir& a_directory,
    const char* a_name,
    int a_width,
    int a_height,
    const std::function<std::uint8_t(int, int)>& a_pixel
) {
    QImage image(a_width, a_height, QImage::Format_Grayscale8);
    for (int row = 0; row < a_height; ++row) {
        for (int column = 0; column < a_width; ++column) {
            const auto value = a_pixel(column, row);
            image.setPixel(column, row, qRgb(value, value, value));
        }
    }
    const auto path = QDir(a_directory.path()).filePath(a_name);
    image.save(path, "PNG");
    return path;
}

std::uint8_t Inverted(int a_x, int a_y) {
    return static_cast<std::uint8_t>(255 - Checker(a_x, a_y));
}

std::uint8_t HalfInverted(int a_x, int a_y) {
    return a_x >= 12 ? Inverted(a_x, a_y) : Checker(a_x, a_y);
}

GrayImage Decode(const QString& a_path) {
    auto image = DecodeGray(a_path);
    if (!image) {
        throw std::runtime_error("Cannot decode " + a_path.toStdString());
    }
    return std::move(*image);
}

double Score(const QString& a_left, const QString& a_right) {
    return ComputeSsim(Decode(a_left), Decode(a_right));
}

int CheckScores(const QTemporaryDir& a_directory) {
    const auto checker = Write(a_directory, "checker.png", 24, 24, Checker);
    const auto inverted = Write(a_directory, "inverted.png", 24, 24, Inverted);
    const auto half = Write(a_directory, "half.png", 24, 24, HalfInverted);
    const auto flat = Write(a_directory, "flat.png", 16, 16, [](int, int) { return 128; });
    const auto thin = Write(a_directory, "thin.png", 16, 1, [](int, int) { return 128; });
    const auto tiny = Write(a_directory, "tiny.png", 4, 4, [](int a_x, int a_y) {
        return ((a_x / 2) + (a_y / 2)) % 2 == 0 ? 220 : 30;
    });
    if (DecodeGray(QDir(a_directory.path()).filePath("missing.png")).has_value()) {
        return 1;
    }
    if (std::abs(Score(checker, checker) - 1.0) > 0.01 || std::abs(Score(flat, flat) - 1.0) > 0.01) {
        return 2;
    }
    // Non-overlapping blocks scored this inversion 0.54 because flat blocks hide the contrast sign.
    if (Score(checker, inverted) >= -0.5) {
        return 3;
    }
    const auto partial = Score(checker, half);
    if (partial >= 1.0 || partial <= -0.5) {
        return 4;
    }
    if (Score(thin, thin) != 0.0 || std::abs(Score(tiny, tiny) - 1.0) > 0.01) {
        return 5;
    }
    return 0;
}

int CheckCrop(const QTemporaryDir& a_directory) {
    const auto source = Write(a_directory, "gradient.png", 10, 10, [](int a_x, int a_y) {
        return static_cast<std::uint8_t>(a_x + a_y);
    });
    const auto right = CropUv(Decode(source), {{"x", 0.5}, {"y", 0.0}, {"w", 0.5}, {"h", 1.0}});
    return right.width == 5 && right.height == 10 && std::abs(right.px[0] - 5.0F) < 0.5F ? 0 : 6;
}

int CheckGoldens(const QTemporaryDir& a_directory) {
    const auto checker = Write(a_directory, "golden.png", 24, 24, Checker);
    const auto same = Write(a_directory, "same.png", 24, 24, Checker);
    const auto inverted = Write(a_directory, "golden-inverted.png", 24, 24, Inverted);
    const auto half = Write(a_directory, "golden-half.png", 24, 24, HalfInverted);
    const auto small = Write(a_directory, "small.png", 8, 8, [](int, int) { return 100; });

    if (const auto
            missing = ScoreAgainstGolden(same, QDir(a_directory.path()).filePath("none.png"), Bench::Json::object());
        missing.ok || missing.error.empty()) {
        return 7;
    }
    if (const auto mismatch = ScoreAgainstGolden(small, checker, Bench::Json::object());
        mismatch.ok || mismatch.error.empty()) {
        return 8;
    }
    const auto pass = ScoreAgainstGolden(same, checker, Bench::Json::object());
    if (!pass.ok || !pass.passed || std::abs(pass.threshold - 0.98) > 1e-9 || !pass.regions.empty()) {
        return 9;
    }
    if (const auto fail = ScoreAgainstGolden(inverted, checker, {{"threshold", 0.98}}); !fail.ok || fail.passed) {
        return 10;
    }

    const Bench::Json regions = {
        {"threshold", 0.98},
        {
            "regions",
            Bench::Json::array({
                {{"name", "left"}, {"x", 0.0}, {"y", 0.0}, {"w", 0.5}, {"h", 1.0}},
                {{"name", "right"}, {"x", 0.5}, {"y", 0.0}, {"w", 0.5}, {"h", 1.0}, {"threshold", -1.0}},
            }),
        },
    };
    const auto scored = ScoreAgainstGolden(half, checker, regions);
    if (!scored.ok
        || scored.regions.size() != 2
        || !scored.regions[0].passed
        || std::abs(scored.regions[0].score - 1.0) > 0.05
        || !scored.regions[1].passed
        || !scored.passed
        || std::abs(scored.score - scored.regions[1].score) > 1e-9) {
        return 11;
    }
    return 0;
}
}

int main() {
    return Tests::Run([] {
        const QTemporaryDir directory;
        return Tests::First({
            [&] { return CheckScores(directory); },
            [&] { return CheckCrop(directory); },
            [&] { return CheckGoldens(directory); },
        });
    });
}
