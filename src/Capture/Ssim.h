#pragma once
#include "Json.h"
#include <QString>
#include <optional>
#include <string>
#include <vector>

// Pure image comparison: decode, crop, SSIM and golden scoring.
namespace Bench::Ssim {
struct GrayImage {
    std::vector<float> px;
    int width = 0;
    int height = 0;
};

std::optional<GrayImage> DecodeGray(const QString& a_path);

// Crops a 0..1 UV rect {x,y,w,h}, the same convention as capture regions.
GrayImage CropUv(const GrayImage& a_image, const Json& a_uv);

// Windowed SSIM (Wang et al.) over overlapping 8px windows. Overlap keeps the contrast term sensitive to
// content that is flat within each block, which non-overlapping blocks miss.
double ComputeSsim(const GrayImage& a_left, const GrayImage& a_right);

struct RegionScore {
    std::string name;
    double score = 0.0;
    double threshold = 0.0;
    bool passed = false;
};

// A missing golden, decode failure or size mismatch is reported through error instead of thrown.
struct GoldenScore {
    bool ok = false;
    std::string error;
    double score = 0.0;
    double threshold = 0.0;
    bool passed = false;
    std::vector<RegionScore> regions;
};

// Both paths must be absolute. a_config's optional "threshold" (default 0.98) and
// "regions" ([{name,x,y,w,h,threshold?}]) control scoring.
GoldenScore ScoreAgainstGolden(const QString& a_capturedPath, const QString& a_goldenPath, const Json& a_config);
}
