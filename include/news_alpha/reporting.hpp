#pragma once

#include "news_alpha/types.hpp"

#include <string>
#include <vector>

namespace news_alpha {

std::string render_event_study_table(const std::vector<EventStudyBucket>& buckets);
std::string render_backtest_summary(const BacktestSummary& summary);

}  // namespace news_alpha
