#pragma once

#include <string>
#include <vector>

namespace news_alpha {

struct NewsRecord {
    std::string timestamp;
    std::string date;
    std::string source;
    std::string ticker;
    std::string headline;
    std::string body;
};

struct PriceRecord {
    std::string date;
    std::string ticker;
    double open {};
    double high {};
    double low {};
    double close {};
    double volume {};
    double benchmark_close {};
};

struct FeatureRow {
    std::string timestamp;
    std::string date;
    std::string ticker;
    std::string source;
    std::string headline;
    double sentiment_score {};
    double confidence_score {};
    double sentiment_surprise {};
    double attention_24h {};
    double burst_score {};
    double event_intensity {};
    std::string event_type;
    double same_day_return {};
    double next_day_return {};
    double next_day_abnormal_return {};
    double five_day_return {};
    double alpha_score {};
};

struct EventStudyBucket {
    std::string label;
    std::size_t count {};
    double avg_next_day_abnormal_return {};
    double avg_five_day_return {};
};

struct DailyPortfolioReturn {
    std::string date;
    double long_average_return {};
    double short_average_return {};
    double spread_return {};
    int long_count {};
    int short_count {};
};

struct BacktestSummary {
    std::vector<DailyPortfolioReturn> daily_returns;
    double cumulative_return {};
    double average_spread_return {};
    double win_rate {};
    double turnover_proxy {};
};

}  // namespace news_alpha
