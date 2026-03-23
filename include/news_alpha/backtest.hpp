#pragma once

#include "news_alpha/types.hpp"

#include <vector>

namespace news_alpha {

BacktestSummary run_backtest(const std::vector<FeatureRow>& features);

}  // namespace news_alpha
