#include "news_alpha/csv_reader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace news_alpha {

namespace {

std::vector<std::string> parse_csv_line(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool in_quotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (ch == '"') {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                current.push_back('"');
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
        } else if (ch == ',' && !in_quotes) {
            fields.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    fields.push_back(current);
    return fields;
}

}  // namespace

std::vector<std::vector<std::string>> read_csv_rows(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to open CSV file: " + path);
    }

    std::vector<std::vector<std::string>> rows;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            rows.push_back(parse_csv_line(line));
        }
    }
    return rows;
}

void write_text_file(const std::string& path, const std::string& contents) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Unable to write file: " + path);
    }
    output << contents;
}

}  // namespace news_alpha
