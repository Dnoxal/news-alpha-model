#include "news_alpha/backtest.hpp"
#include "news_alpha/csv_reader.hpp"
#include "news_alpha/data_loader.hpp"
#include "news_alpha/event_study.hpp"
#include "news_alpha/feature_engineering.hpp"
#include "news_alpha/reporting.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

std::string data_path(const std::string& suffix) {
    return (std::filesystem::current_path() / "data" / suffix).string();
}

void ensure_processed_dir() {
    std::filesystem::create_directories(std::filesystem::current_path() / "data" / "processed");
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const std::string command = argc > 1 ? argv[1] : "full-run";

        const auto news = news_alpha::load_news(data_path("raw/news.csv"));
        const auto prices = news_alpha::load_prices(data_path("raw/prices.csv"));
        const auto features = news_alpha::build_feature_rows(news, prices);

        ensure_processed_dir();

        if (command == "features") {
            const auto feature_csv = news_alpha::render_feature_table(features);
            news_alpha::write_text_file(data_path("processed/features.csv"), feature_csv);
            std::cout << feature_csv;
            return 0;
        }

        if (command == "event-study") {
            const auto event_results = news_alpha::run_event_study(features);
            const auto report = news_alpha::render_event_study_table(event_results);
            news_alpha::write_text_file(data_path("processed/event_study.csv"), report);
            std::cout << report;
            return 0;
        }

        if (command == "backtest") {
            const auto summary = news_alpha::run_backtest(features);
            const auto report = news_alpha::render_backtest_summary(summary);
            news_alpha::write_text_file(data_path("processed/backtest.txt"), report);
            std::cout << report;
            return 0;
        }

        if (command == "full-run") {
            const auto feature_csv = news_alpha::render_feature_table(features);
            const auto event_results = news_alpha::run_event_study(features);
            const auto event_report = news_alpha::render_event_study_table(event_results);
            const auto summary = news_alpha::run_backtest(features);
            const auto backtest_report = news_alpha::render_backtest_summary(summary);

            news_alpha::write_text_file(data_path("processed/features.csv"), feature_csv);
            news_alpha::write_text_file(data_path("processed/event_study.csv"), event_report);
            news_alpha::write_text_file(data_path("processed/backtest.txt"), backtest_report);

            std::cout << "Generated data/processed/features.csv\n\n";
            std::cout << event_report << "\n";
            std::cout << backtest_report;
            return 0;
        }

        throw std::runtime_error("Unknown command: " + command);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
