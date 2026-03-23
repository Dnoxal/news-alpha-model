#pragma once

#include "news_alpha/types.hpp"

#include <vector>

namespace news_alpha {

std::vector<EventStudyBucket> run_event_study(const std::vector<FeatureRow>& features);

}  // namespace news_alpha
