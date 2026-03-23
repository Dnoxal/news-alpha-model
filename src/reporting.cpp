#include "news_alpha/reporting.hpp"

#include <iomanip>
#include <sstream>

namespace news_alpha {

std::string render_event_study_table(const std::vector<EventStudyBucket>& buckets) {
    std::ostringstream output;
    output << "Event Study Results\n";
    output << "bucket,count,avg_next_day_abnormal_return,avg_five_day_return\n";
    output << std::fixed << std::setprecision(4);
    for (const auto& bucket : buckets) {
        output << bucket.label << ','
               << bucket.count << ','
               << bucket.avg_next_day_abnormal_return << ','
               << bucket.avg_five_day_return << '\n';
    }
    return output.str();
}

std::string render_backtest_summary(const BacktestSummary& summary) {
    std::ostringstream output;
    output << "Backtest Summary\n";
    output << std::fixed << std::setprecision(4);
    output << "cumulative_return=" << summary.cumulative_return << '\n';
    output << "average_spread_return=" << summary.average_spread_return << '\n';
    output << "win_rate=" << summary.win_rate << '\n';
    output << "turnover_proxy=" << summary.turnover_proxy << '\n';
    output << "\nDaily spreads\n";
    output << "date,long_avg,short_avg,spread\n";
    for (const auto& day : summary.daily_returns) {
        output << day.date << ','
               << day.long_average_return << ','
               << day.short_average_return << ','
               << day.spread_return << '\n';
    }
    return output.str();
}

}  // namespace news_alpha
