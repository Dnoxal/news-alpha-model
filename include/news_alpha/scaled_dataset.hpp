#pragma once

#include <string>
#include <vector>

namespace news_alpha {

struct ScaledFeatureRecord {
    std::string date;
    std::string ticker;
    double alpha_score {};
    double next_day_return {};
    double next_day_abnormal_return {};
    double five_day_abnormal_return {};
};

struct EigenAnalyticsSummary {
    std::size_t row_count {};
    std::size_t date_count {};
    double mean_alpha {};
    double rank_ic {};
    double long_short_average {};
    double long_short_cumulative {};
    double win_rate {};
};

std::vector<ScaledFeatureRecord> load_scaled_features(const std::string& path);
EigenAnalyticsSummary run_scaled_analytics(const std::vector<ScaledFeatureRecord>& rows);
std::string render_scaled_summary(const EigenAnalyticsSummary& summary);

}  // namespace news_alpha
