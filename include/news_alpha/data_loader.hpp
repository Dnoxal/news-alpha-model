#pragma once

#include "news_alpha/types.hpp"

#include <string>
#include <vector>

namespace news_alpha {

std::vector<NewsRecord> load_news(const std::string& path);
std::vector<PriceRecord> load_prices(const std::string& path);

}  // namespace news_alpha
