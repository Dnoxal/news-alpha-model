# News Sentiment Alpha Model

`news-alpha` is a hybrid `C++ + Python` quantitative research pipeline that generates or ingests large-scale financial news, engineers cross-sectional alpha features with `pandas`, optionally loads datasets into `PostgreSQL`, and evaluates signals in a `C++20` analytics engine backed by `Eigen`.

## Technology stack

- `C++20`
- `Python 3`
- `Eigen`
- `pandas`
- `PostgreSQL`
- `CMake`

## What the project does

- Generates a research dataset with `300K+` timestamped news records across `500+` equities
- Builds `10+` sentiment, event, attention, and market-context features with `pandas`
- Produces `100K+` labeled event windows with next-day and 5-day abnormal-return targets
- Optionally writes raw and processed tables into `PostgreSQL`
- Runs event studies, rank-IC analysis, and long/short portfolio backtests in `C++`

## Project layout

```text
news-alpha-model/
├── data/
│   ├── generated/
│   ├── processed/
│   └── raw/
├── include/news_alpha/
├── scripts/
├── sql/
├── src/
├── tests/
└── CMakeLists.txt
```

## Python dependencies

```bash
python3 -m pip install pandas numpy pyarrow psycopg
```

## Build the C++ analytics engine

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## End-to-end workflow

Generate a scaled dataset:

```bash
python3 scripts/generate_research_dataset.py --news-count 300000 --ticker-count 500
```

Build engineered features with pandas:

```bash
python3 scripts/build_feature_store.py --input-dir data/generated --output-dir data/processed
```

Run C++ analytics over the processed feature store:

```bash
./build/news_alpha scaled-run
```

Optionally load the tables into PostgreSQL:

```bash
python3 scripts/load_postgres.py --postgres-url postgresql://user:pass@host:5432/dbname
```

## Output artifacts

- `data/generated/news.parquet`
- `data/generated/prices.parquet`
- `data/processed/features.parquet`
- `data/processed/features.csv`
- `data/processed/feature_summary.json`
- `data/postgres/load_manifest.json`

## Supported feature set

The pandas pipeline computes more than ten modeled signals and labels, including:

- headline sentiment
- body sentiment
- combined sentiment
- sentiment surprise
- sentiment momentum
- sentiment dispersion
- article count
- unique-source count
- burstiness
- event intensity
- prior 5-day return
- realized 5-day volatility
- sector-relative sentiment
- next-day return
- next-day abnormal return
- 5-day abnormal return
