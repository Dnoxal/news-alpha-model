#pragma once

#include <string>
#include <vector>

namespace news_alpha {

std::vector<std::vector<std::string>> read_csv_rows(const std::string& path);
void write_text_file(const std::string& path, const std::string& contents);

}  // namespace news_alpha
