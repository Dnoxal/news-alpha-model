#include "news_alpha/scaled_dataset.hpp"

#include "news_alpha/csv_reader.hpp"

#include <Eigen/Dense>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace news_alpha {

namespace {

std::unordered_map<std::string, std::size_t> build_header_index(const std::vector<std::string>& header) {
    std::unordered_map<std::string, std::size_t> index;
    for (std::size_t i = 0; i < header.size(); ++i) {
        index.emplace(header[i], i);
    }
    return index;
}

double to_double(const std::string& value) {
    return value.empty() ? 0.0 : std::stod(value);
}

Eigen::VectorXd rank_vector(const Eigen::VectorXd& values) {
    std::vector<std::pair<double, int>> indexed;
    indexed.reserve(static_cast<std::size_t>(values.size()));
    for (int i = 0; i < values.size(); ++i) {
        indexed.push_back({values[i], i});
    }

    std::sort(indexed.begin(), indexed.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.first == rhs.first) {
            return lhs.second < rhs.second;
        }
        return lhs.first < rhs.first;
    });

    Eigen::VectorXd ranks(values.size());
    for (int i = 0; i < static_cast<int>(indexed.size()); ++i) {
        ranks[indexed[static_cast<std::size_t>(i)].second] = static_cast<double>(i + 1);
    }
    return ranks;
}

double pearson_correlation(const Eigen::VectorXd& lhs, const Eigen::VectorXd& rhs) {
    if (lhs.size() != rhs.size() || lhs.size() < 2) {
        return 0.0;
    }

    const double lhs_mean = lhs.mean();
    const double rhs_mean = rhs.mean();
    const Eigen::ArrayXd lhs_centered = lhs.array() - lhs_mean;
    const Eigen::ArrayXd rhs_centered = rhs.array() - rhs_mean;
    const double numerator = (lhs_centered * rhs_centered).sum();
    const double denominator = std::sqrt((lhs_centered.square().sum()) * (rhs_centered.square().sum()));
    return denominator == 0.0 ? 0.0 : numerator / denominator;
}

}  // namespace

std::vector<ScaledFeatureRecord> load_scaled_features(const std::string& path) {
    const auto rows = read_csv_rows(path);
    if (rows.size() < 2) {
        throw std::runtime_error("Scaled feature file does not contain data rows.");
    }

    const auto header_index = build_header_index(rows.front());
    const std::vector<std::string> required_columns {
        "date",
        "ticker",
        "alpha_score",
        "next_day_return",
        "next_day_abnormal_return",
        "five_day_abnormal_return"
    };

    for (const auto& column : required_columns) {
        if (!header_index.contains(column)) {
            throw std::runtime_error("Missing required column in processed features: " + column);
        }
    }

    std::vector<ScaledFeatureRecord> records;
    records.reserve(rows.size() - 1);
    for (std::size_t i = 1; i < rows.size(); ++i) {
        const auto& row = rows[i];
        records.push_back(ScaledFeatureRecord {
            .date = row[header_index.at("date")],
            .ticker = row[header_index.at("ticker")],
            .alpha_score = to_double(row[header_index.at("alpha_score")]),
            .next_day_return = to_double(row[header_index.at("next_day_return")]),
            .next_day_abnormal_return = to_double(row[header_index.at("next_day_abnormal_return")]),
            .five_day_abnormal_return = to_double(row[header_index.at("five_day_abnormal_return")]),
        });
    }

    return records;
}

EigenAnalyticsSummary run_scaled_analytics(const std::vector<ScaledFeatureRecord>& rows) {
    std::map<std::string, std::vector<ScaledFeatureRecord>> by_date;
    for (const auto& row : rows) {
        by_date[row.date].push_back(row);
    }

    double rank_ic_sum = 0.0;
    double spread_sum = 0.0;
    double cumulative = 1.0;
    int win_count = 0;
    int evaluated_days = 0;
    double mean_alpha_accumulator = 0.0;

    for (const auto& [date, daily_rows] : by_date) {
        if (daily_rows.size() < 10) {
            continue;
        }

        Eigen::VectorXd alpha(static_cast<int>(daily_rows.size()));
        Eigen::VectorXd next_returns(static_cast<int>(daily_rows.size()));
        for (int i = 0; i < static_cast<int>(daily_rows.size()); ++i) {
            alpha[i] = daily_rows[static_cast<std::size_t>(i)].alpha_score;
            next_returns[i] = daily_rows[static_cast<std::size_t>(i)].next_day_abnormal_return;
        }

        mean_alpha_accumulator += alpha.mean();
        const double daily_rank_ic = pearson_correlation(rank_vector(alpha), rank_vector(next_returns));
        rank_ic_sum += daily_rank_ic;

        std::vector<ScaledFeatureRecord> ranked_rows = daily_rows;
        std::sort(ranked_rows.begin(), ranked_rows.end(), [](const auto& lhs, const auto& rhs) {
            if (lhs.alpha_score == rhs.alpha_score) {
                return lhs.ticker < rhs.ticker;
            }
            return lhs.alpha_score > rhs.alpha_score;
        });

        const std::size_t bucket_size = std::max<std::size_t>(1, ranked_rows.size() / 10);
        double long_sum = 0.0;
        double short_sum = 0.0;
        for (std::size_t i = 0; i < bucket_size; ++i) {
            long_sum += ranked_rows[i].next_day_abnormal_return;
            short_sum += ranked_rows[ranked_rows.size() - 1 - i].next_day_abnormal_return;
        }

        const double spread = (long_sum / static_cast<double>(bucket_size)) - (short_sum / static_cast<double>(bucket_size));
        spread_sum += spread;
        cumulative *= (1.0 + spread);
        win_count += spread > 0.0 ? 1 : 0;
        evaluated_days += 1;
    }

    return EigenAnalyticsSummary {
        .row_count = rows.size(),
        .date_count = by_date.size(),
        .mean_alpha = evaluated_days == 0 ? 0.0 : mean_alpha_accumulator / static_cast<double>(evaluated_days),
        .rank_ic = evaluated_days == 0 ? 0.0 : rank_ic_sum / static_cast<double>(evaluated_days),
        .long_short_average = evaluated_days == 0 ? 0.0 : spread_sum / static_cast<double>(evaluated_days),
        .long_short_cumulative = cumulative - 1.0,
        .win_rate = evaluated_days == 0 ? 0.0 : static_cast<double>(win_count) / static_cast<double>(evaluated_days),
    };
}

std::string render_scaled_summary(const EigenAnalyticsSummary& summary) {
    std::ostringstream output;
    output << "Scaled Analytics Summary\n";
    output << std::fixed << std::setprecision(6);
    output << "row_count=" << summary.row_count << '\n';
    output << "date_count=" << summary.date_count << '\n';
    output << "mean_alpha=" << summary.mean_alpha << '\n';
    output << "rank_ic=" << summary.rank_ic << '\n';
    output << "long_short_average=" << summary.long_short_average << '\n';
    output << "long_short_cumulative=" << summary.long_short_cumulative << '\n';
    output << "win_rate=" << summary.win_rate << '\n';
    return output.str();
}

}  // namespace news_alpha
