#include "news_alpha/backtest.hpp"
#include "news_alpha/data_loader.hpp"
#include "news_alpha/event_study.hpp"
#include "news_alpha/feature_engineering.hpp"
#include "news_alpha/scaled_dataset.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {

void assert_true(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string project_data_path(const std::string& suffix) {
    return (std::filesystem::path(NEWS_ALPHA_SOURCE_DIR) / "data" / suffix).string();
}

}  // namespace

int main() {
    try {
        const auto news = news_alpha::load_news(project_data_path("raw/news.csv"));
        const auto prices = news_alpha::load_prices(project_data_path("raw/prices.csv"));
        const auto features = news_alpha::build_feature_rows(news, prices);
        const auto event_buckets = news_alpha::run_event_study(features);
        const auto summary = news_alpha::run_backtest(features);
        const auto rendered = news_alpha::render_scaled_summary(news_alpha::EigenAnalyticsSummary {});

        assert_true(news.size() == 12, "Expected 12 fixture news rows");
        assert_true(prices.size() == 27, "Expected 27 fixture price rows");
        assert_true(features.size() >= 9, "Expected enough aligned feature rows");
        assert_true(event_buckets.size() == 3, "Expected positive, neutral, negative buckets");
        assert_true(!summary.daily_returns.empty(), "Expected backtest to produce daily returns");
        assert_true(summary.win_rate >= 0.0 && summary.win_rate <= 1.0, "Win rate must be bounded");
        assert_true(std::isfinite(summary.cumulative_return), "Cumulative return should be finite");
        assert_true(!rendered.empty(), "Scaled summary renderer should produce output");

        bool found_positive = false;
        bool found_negative = false;
        for (const auto& row : features) {
            found_positive = found_positive || row.sentiment_score > 0.0;
            found_negative = found_negative || row.sentiment_score < 0.0;
        }
        assert_true(found_positive, "Fixture should include positive sentiment");
        assert_true(found_negative, "Fixture should include negative sentiment");

        std::cout << "All tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
