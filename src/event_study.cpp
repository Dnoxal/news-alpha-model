#include "news_alpha/event_study.hpp"

#include <map>
#include <numeric>

namespace news_alpha {

std::vector<EventStudyBucket> run_event_study(const std::vector<FeatureRow>& features) {
    struct Aggregate {
        std::size_t count {};
        double abnormal_sum {};
        double five_day_sum {};
    };

    std::map<std::string, Aggregate> aggregates {
        {"positive", {}},
        {"neutral", {}},
        {"negative", {}},
    };

    for (const auto& row : features) {
        std::string label = "neutral";
        if (row.sentiment_score > 0.0) {
            label = "positive";
        } else if (row.sentiment_score < 0.0) {
            label = "negative";
        }

        auto& bucket = aggregates[label];
        bucket.count += 1;
        bucket.abnormal_sum += row.next_day_abnormal_return;
        bucket.five_day_sum += row.five_day_return;
    }

    std::vector<EventStudyBucket> result;
    for (const auto& [label, aggregate] : aggregates) {
        result.push_back(EventStudyBucket {
            .label = label,
            .count = aggregate.count,
            .avg_next_day_abnormal_return = aggregate.count == 0 ? 0.0 : aggregate.abnormal_sum / static_cast<double>(aggregate.count),
            .avg_five_day_return = aggregate.count == 0 ? 0.0 : aggregate.five_day_sum / static_cast<double>(aggregate.count),
        });
    }
    return result;
}

}  // namespace news_alpha
