#include "news_alpha/feature_engineering.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>

namespace news_alpha {

namespace {

struct PriceSnapshot {
    double open {};
    double close {};
    double benchmark_close {};
};

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool contains_any(const std::string& text, const std::vector<std::string>& tokens) {
    for (const auto& token : tokens) {
        if (text.find(token) != std::string::npos) {
            return true;
        }
    }
    return false;
}

double compute_sentiment_score(const std::string& text) {
    static const std::vector<std::string> positive_words {
        "beats", "beat", "growth", "surge", "strong", "gain", "gains", "record",
        "upgrade", "raises", "raised", "launch", "partnership", "profit"
    };
    static const std::vector<std::string> negative_words {
        "miss", "misses", "cut", "cuts", "lawsuit", "probe", "fall", "falls",
        "weak", "warning", "downgrade", "delay", "decline", "slump"
    };

    const std::string lowered = lowercase(text);
    double score = 0.0;
    for (const auto& word : positive_words) {
        if (lowered.find(word) != std::string::npos) {
            score += 1.0;
        }
    }
    for (const auto& word : negative_words) {
        if (lowered.find(word) != std::string::npos) {
            score -= 1.0;
        }
    }
    return score;
}

std::string classify_event_type(const std::string& text) {
    const std::string lowered = lowercase(text);
    if (contains_any(lowered, {"earnings", "guidance", "quarter", "revenue"})) {
        return "earnings";
    }
    if (contains_any(lowered, {"lawsuit", "regulator", "probe", "antitrust"})) {
        return "legal";
    }
    if (contains_any(lowered, {"launch", "product", "chip", "platform"})) {
        return "product";
    }
    if (contains_any(lowered, {"acquire", "merger", "deal", "partnership"})) {
        return "corporate";
    }
    if (contains_any(lowered, {"ceo", "executive", "chief"})) {
        return "leadership";
    }
    return "general";
}

double event_intensity(const std::string& event_type) {
    if (event_type == "earnings") {
        return 1.0;
    }
    if (event_type == "legal") {
        return 0.9;
    }
    if (event_type == "product") {
        return 0.7;
    }
    if (event_type == "corporate") {
        return 0.8;
    }
    if (event_type == "leadership") {
        return 0.6;
    }
    return 0.4;
}

double safe_divide(double numerator, double denominator) {
    return std::abs(denominator) < 1e-12 ? 0.0 : numerator / denominator;
}

}  // namespace

std::vector<FeatureRow> build_feature_rows(
    const std::vector<NewsRecord>& news,
    const std::vector<PriceRecord>& prices
) {
    std::map<std::pair<std::string, std::string>, PriceSnapshot> price_by_ticker_date;
    std::map<std::string, std::vector<std::string>> dates_by_ticker;

    for (const auto& price : prices) {
        price_by_ticker_date[{price.ticker, price.date}] = PriceSnapshot {
            .open = price.open,
            .close = price.close,
            .benchmark_close = price.benchmark_close,
        };
        dates_by_ticker[price.ticker].push_back(price.date);
    }

    for (auto& [ticker, dates] : dates_by_ticker) {
        std::sort(dates.begin(), dates.end());
        dates.erase(std::unique(dates.begin(), dates.end()), dates.end());
    }

    std::map<std::string, std::vector<double>> historical_sentiment_by_ticker;
    std::map<std::string, int> daily_article_count;

    for (const auto& item : news) {
        daily_article_count[item.ticker + "|" + item.date] += 1;
    }

    std::vector<FeatureRow> features;
    for (const auto& item : news) {
        const auto price_it = price_by_ticker_date.find({item.ticker, item.date});
        if (price_it == price_by_ticker_date.end()) {
            continue;
        }

        const auto& ticker_dates = dates_by_ticker[item.ticker];
        const auto current_pos = std::find(ticker_dates.begin(), ticker_dates.end(), item.date);
        if (current_pos == ticker_dates.end() || current_pos + 1 == ticker_dates.end()) {
            continue;
        }

        const std::string next_day = *(current_pos + 1);
        const std::string five_day = (current_pos + 5 < ticker_dates.end())
            ? *(current_pos + 5)
            : ticker_dates.back();

        const auto next_it = price_by_ticker_date.find({item.ticker, next_day});
        const auto five_it = price_by_ticker_date.find({item.ticker, five_day});
        const auto benchmark_next_it = price_by_ticker_date.find({item.ticker, next_day});
        if (next_it == price_by_ticker_date.end() || five_it == price_by_ticker_date.end() || benchmark_next_it == price_by_ticker_date.end()) {
            continue;
        }

        const double sentiment = compute_sentiment_score(item.headline + " " + item.body);
        const double confidence = std::min(1.0, 0.45 + std::abs(sentiment) * 0.18);
        const auto& history = historical_sentiment_by_ticker[item.ticker];
        const double trailing_mean = history.empty()
            ? 0.0
            : std::accumulate(history.begin(), history.end(), 0.0) / static_cast<double>(history.size());
        const double surprise = sentiment - trailing_mean;

        const std::string event_type = classify_event_type(item.headline + " " + item.body);
        const double intensity = event_intensity(event_type);

        const double current_close = price_it->second.close;
        const double next_close = next_it->second.close;
        const double five_close = five_it->second.close;
        const double same_day_return = safe_divide(current_close - price_it->second.open, price_it->second.open);
        const double next_day_return = safe_divide(next_close - current_close, current_close);
        const double five_day_return = safe_divide(five_close - current_close, current_close);

        const double benchmark_current = price_it->second.benchmark_close;
        const double benchmark_next = benchmark_next_it->second.benchmark_close;
        const double benchmark_return = safe_divide(benchmark_next - benchmark_current, benchmark_current);
        const double abnormal_return = next_day_return - benchmark_return;

        const double attention = static_cast<double>(daily_article_count[item.ticker + "|" + item.date]);
        const double burst_score = history.empty() ? attention : attention / std::max(1.0, std::sqrt(static_cast<double>(history.size())));
        const double alpha_score = 0.45 * sentiment + 0.25 * surprise + 0.20 * intensity + 0.10 * burst_score;

        features.push_back(FeatureRow {
            .timestamp = item.timestamp,
            .date = item.date,
            .ticker = item.ticker,
            .source = item.source,
            .headline = item.headline,
            .sentiment_score = sentiment,
            .confidence_score = confidence,
            .sentiment_surprise = surprise,
            .attention_24h = attention,
            .burst_score = burst_score,
            .event_intensity = intensity,
            .event_type = event_type,
            .same_day_return = same_day_return,
            .next_day_return = next_day_return,
            .next_day_abnormal_return = abnormal_return,
            .five_day_return = five_day_return,
            .alpha_score = alpha_score,
        });

        historical_sentiment_by_ticker[item.ticker].push_back(sentiment);
    }

    return features;
}

std::string render_feature_table(const std::vector<FeatureRow>& features) {
    std::ostringstream output;
    output << "date,ticker,event_type,sentiment,confidence,surprise,attention,burst,alpha,next_day_return,next_day_abnormal,five_day_return\n";
    output << std::fixed << std::setprecision(4);
    for (const auto& feature : features) {
        output << feature.date << ','
               << feature.ticker << ','
               << feature.event_type << ','
               << feature.sentiment_score << ','
               << feature.confidence_score << ','
               << feature.sentiment_surprise << ','
               << feature.attention_24h << ','
               << feature.burst_score << ','
               << feature.alpha_score << ','
               << feature.next_day_return << ','
               << feature.next_day_abnormal_return << ','
               << feature.five_day_return << '\n';
    }
    return output.str();
}

}  // namespace news_alpha
