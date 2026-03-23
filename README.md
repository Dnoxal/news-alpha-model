# News Sentiment Alpha Model

`news-alpha` is a C++20 MVP for a quant-style research pipeline that links financial news sentiment to short-horizon equity returns, computes event-study statistics, and runs a simple long/short backtest.

## What is included

- CSV ingestion for news and daily prices
- Rule-based sentiment and event tagging for headlines
- Feature engineering for attention, burstiness, and sentiment surprise
- Market alignment for next-day and 5-day forward returns
- Event-study analysis by positive and negative sentiment buckets
- Cross-sectional daily long/short backtest on a sentiment alpha score
- Unit-style tests over the fixture dataset

## Project layout

```text
news-alpha-model/
├── data/
│   ├── processed/
│   └── raw/
├── include/news_alpha/
├── src/
├── tests/
└── CMakeLists.txt
```

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run

From the repository root:

```bash
./build/news_alpha full-run
```

Other commands:

```bash
./build/news_alpha features
./build/news_alpha event-study
./build/news_alpha backtest
```

## Data assumptions

This MVP ships with small deterministic fixture datasets in `data/raw/` so you can test the full flow immediately. The default universe includes `AAPL`, `MSFT`, and `NVDA`.

News schema:

```text
timestamp,source,ticker,headline,body
```

Price schema:

```text
date,ticker,open,high,low,close,volume,benchmark_close
```

## Resume framing

- Built a C++20 quant research pipeline linking financial news to market reactions through event studies and long/short backtesting.
- Engineered text-derived sentiment, attention, and event-intensity features and aligned them to forward abnormal returns.
- Evaluated signal efficacy with cross-sectional ranking metrics, hit rate, spread returns, and cumulative PnL.
