#include "news_alpha/backtest.hpp"

#include <algorithm>
#include <map>
#include <set>

namespace news_alpha {

BacktestSummary run_backtest(const std::vector<FeatureRow>& features) {
    std::map<std::string, std::vector<FeatureRow>> by_date;
    for (const auto& feature : features) {
        by_date[feature.date].push_back(feature);
    }

    std::vector<DailyPortfolioReturn> daily_returns;
    std::set<std::string> prior_long_names;
    std::set<std::string> prior_short_names;
    double cumulative = 1.0;
    int wins = 0;
    int evaluated_days = 0;
    double turnover_accumulator = 0.0;

    for (auto& [date, rows] : by_date) {
        if (rows.size() < 2) {
            continue;
        }

        std::sort(rows.begin(), rows.end(), [](const FeatureRow& lhs, const FeatureRow& rhs) {
            if (lhs.alpha_score == rhs.alpha_score) {
                return lhs.ticker < rhs.ticker;
            }
            return lhs.alpha_score > rhs.alpha_score;
        });

        const std::size_t bucket_size = std::max<std::size_t>(1, rows.size() / 3);
        std::set<std::string> long_names;
        std::set<std::string> short_names;
        double long_sum = 0.0;
        double short_sum = 0.0;

        for (std::size_t i = 0; i < bucket_size; ++i) {
            long_names.insert(rows[i].ticker);
            long_sum += rows[i].next_day_return;
        }
        for (std::size_t i = 0; i < bucket_size; ++i) {
            const auto& row = rows[rows.size() - 1 - i];
            short_names.insert(row.ticker);
            short_sum += row.next_day_return;
        }

        const double long_average = long_sum / static_cast<double>(bucket_size);
        const double short_average = short_sum / static_cast<double>(bucket_size);
        const double spread = long_average - short_average;
        cumulative *= (1.0 + spread);
        wins += spread > 0.0 ? 1 : 0;
        evaluated_days += 1;

        if (!prior_long_names.empty() || !prior_short_names.empty()) {
            int changed = 0;
            for (const auto& name : long_names) {
                changed += prior_long_names.contains(name) ? 0 : 1;
            }
            for (const auto& name : short_names) {
                changed += prior_short_names.contains(name) ? 0 : 1;
            }
            turnover_accumulator += static_cast<double>(changed) / static_cast<double>(long_names.size() + short_names.size());
        }

        prior_long_names = long_names;
        prior_short_names = short_names;

        daily_returns.push_back(DailyPortfolioReturn {
            .date = date,
            .long_average_return = long_average,
            .short_average_return = short_average,
            .spread_return = spread,
            .long_count = static_cast<int>(bucket_size),
            .short_count = static_cast<int>(bucket_size),
        });
    }

    double spread_sum = 0.0;
    for (const auto& day : daily_returns) {
        spread_sum += day.spread_return;
    }

    return BacktestSummary {
        .daily_returns = daily_returns,
        .cumulative_return = cumulative - 1.0,
        .average_spread_return = daily_returns.empty() ? 0.0 : spread_sum / static_cast<double>(daily_returns.size()),
        .win_rate = evaluated_days == 0 ? 0.0 : static_cast<double>(wins) / static_cast<double>(evaluated_days),
        .turnover_proxy = daily_returns.size() < 2 ? 0.0 : turnover_accumulator / static_cast<double>(daily_returns.size() - 1),
    };
}

}  // namespace news_alpha
