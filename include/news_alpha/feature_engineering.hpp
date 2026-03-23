#pragma once

#include "news_alpha/types.hpp"

#include <string>
#include <vector>

namespace news_alpha {

std::vector<FeatureRow> build_feature_rows(
    const std::vector<NewsRecord>& news,
    const std::vector<PriceRecord>& prices
);

std::string render_feature_table(const std::vector<FeatureRow>& features);

}  // namespace news_alpha
