#include "news_alpha/data_loader.hpp"

#include "news_alpha/csv_reader.hpp"

#include <stdexcept>

namespace news_alpha {

std::vector<NewsRecord> load_news(const std::string& path) {
    const auto rows = read_csv_rows(path);
    if (rows.size() < 2) {
        throw std::runtime_error("News file does not contain data rows.");
    }

    std::vector<NewsRecord> records;
    for (std::size_t i = 1; i < rows.size(); ++i) {
        const auto& row = rows[i];
        if (row.size() < 5) {
            throw std::runtime_error("Malformed news row at line " + std::to_string(i + 1));
        }
        records.push_back(NewsRecord {
            .timestamp = row[0],
            .date = row[0].substr(0, 10),
            .source = row[1],
            .ticker = row[2],
            .headline = row[3],
            .body = row[4],
        });
    }
    return records;
}

std::vector<PriceRecord> load_prices(const std::string& path) {
    const auto rows = read_csv_rows(path);
    if (rows.size() < 2) {
        throw std::runtime_error("Price file does not contain data rows.");
    }

    std::vector<PriceRecord> records;
    for (std::size_t i = 1; i < rows.size(); ++i) {
        const auto& row = rows[i];
        if (row.size() < 8) {
            throw std::runtime_error("Malformed price row at line " + std::to_string(i + 1));
        }
        records.push_back(PriceRecord {
            .date = row[0],
            .ticker = row[1],
            .open = std::stod(row[2]),
            .high = std::stod(row[3]),
            .low = std::stod(row[4]),
            .close = std::stod(row[5]),
            .volume = std::stod(row[6]),
            .benchmark_close = std::stod(row[7]),
        });
    }
    return records;
}

}  // namespace news_alpha
