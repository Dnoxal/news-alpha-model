#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
import pandas as pd


POSITIVE_WORDS = {
    "beats",
    "raises",
    "strong",
    "expands",
    "wins",
    "improves",
    "reaffirms",
    "launches",
    "accelerating",
    "favorable",
}
NEGATIVE_WORDS = {
    "misses",
    "cuts",
    "weak",
    "delay",
    "probe",
    "lawsuit",
    "slowdown",
    "scrutiny",
    "loses",
    "stalls",
}
SOURCE_WEIGHTS = {
    "Reuters": 1.00,
    "Bloomberg": 1.05,
    "WSJ": 1.03,
    "CNBC": 0.96,
    "MarketWatch": 0.94,
    "Benzinga": 0.90,
}
EVENT_INTENSITY = {
    "earnings": 1.00,
    "guidance": 0.95,
    "legal": 0.85,
    "partnership": 0.75,
    "product": 0.70,
    "leadership": 0.60,
}


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Build a pandas feature store for news alpha research.")
    parser.add_argument("--input-dir", default="data/generated", help="Directory containing raw parquet files.")
    parser.add_argument("--output-dir", default="data/processed", help="Directory for engineered features.")
    return parser


def text_sentiment(series: pd.Series) -> pd.Series:
    lowered = series.str.lower()
    positive_hits = sum(lowered.str.contains(word, regex=False).astype(int) for word in POSITIVE_WORDS)
    negative_hits = sum(lowered.str.contains(word, regex=False).astype(int) for word in NEGATIVE_WORDS)
    return positive_hits - negative_hits


def main() -> None:
    args = build_argument_parser().parse_args()
    input_dir = Path(args.input_dir)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    news = pd.read_parquet(input_dir / "news.parquet")
    prices = pd.read_parquet(input_dir / "prices.parquet")

    news["timestamp"] = pd.to_datetime(news["timestamp"], utc=True)
    news["date"] = pd.to_datetime(news["date"])
    prices["date"] = pd.to_datetime(prices["date"])

    news["headline_sentiment"] = text_sentiment(news["headline"])
    news["body_sentiment"] = text_sentiment(news["body"])
    news["combined_sentiment"] = 0.65 * news["headline_sentiment"] + 0.35 * news["body_sentiment"] + 0.25 * news["sentiment_seed"]
    news["source_weight"] = news["source"].map(SOURCE_WEIGHTS).fillna(0.9)
    news["weighted_sentiment"] = news["combined_sentiment"] * news["source_weight"]
    news["event_intensity"] = news["event_type"].map(EVENT_INTENSITY).fillna(0.5)

    news = news.sort_values(["ticker", "timestamp"]).reset_index(drop=True)

    daily_agg = (
        news.groupby(["date", "ticker", "sector"], as_index=False)
        .agg(
            article_count=("headline", "size"),
            unique_sources=("source", "nunique"),
            avg_sentiment=("combined_sentiment", "mean"),
            sentiment_dispersion=("combined_sentiment", "std"),
            weighted_sentiment=("weighted_sentiment", "mean"),
            event_intensity=("event_intensity", "mean"),
        )
        .fillna({"sentiment_dispersion": 0.0})
    )

    daily_agg["sentiment_surprise"] = (
        daily_agg.groupby("ticker")["avg_sentiment"].transform(lambda s: s - s.shift(1).rolling(20, min_periods=1).mean())
    ).fillna(daily_agg["avg_sentiment"])
    daily_agg["sentiment_momentum_5d"] = (
        daily_agg.groupby("ticker")["avg_sentiment"].transform(lambda s: s.rolling(5, min_periods=1).mean())
    )
    daily_agg["burstiness"] = (
        daily_agg["article_count"]
        / daily_agg.groupby("ticker")["article_count"].transform(lambda s: s.rolling(20, min_periods=1).mean())
    ).fillna(1.0)

    sector_daily = (
        daily_agg.groupby(["date", "sector"], as_index=False)
        .agg(sector_avg_sentiment=("avg_sentiment", "mean"))
    )
    daily_agg = daily_agg.merge(sector_daily, on=["date", "sector"], how="left")
    daily_agg["sector_relative_sentiment"] = daily_agg["avg_sentiment"] - daily_agg["sector_avg_sentiment"]

    prices = prices.sort_values(["ticker", "date"]).reset_index(drop=True)
    prices["daily_return"] = prices.groupby("ticker")["close"].pct_change().fillna(0.0)
    prices["benchmark_return"] = prices.groupby("ticker")["benchmark_close"].pct_change().fillna(0.0)
    prices["prior_5d_return"] = prices.groupby("ticker")["close"].pct_change(5).fillna(0.0)
    prices["realized_vol_5d"] = prices.groupby("ticker")["daily_return"].transform(lambda s: s.rolling(5, min_periods=2).std()).fillna(0.0)
    prices["next_day_return"] = prices.groupby("ticker")["close"].shift(-1) / prices["close"] - 1.0
    prices["next_day_benchmark_return"] = prices.groupby("ticker")["benchmark_close"].shift(-1) / prices["benchmark_close"] - 1.0
    prices["next_day_abnormal_return"] = prices["next_day_return"] - prices["next_day_benchmark_return"]
    prices["five_day_return"] = prices.groupby("ticker")["close"].shift(-5) / prices["close"] - 1.0
    prices["five_day_benchmark_return"] = prices.groupby("ticker")["benchmark_close"].shift(-5) / prices["benchmark_close"] - 1.0
    prices["five_day_abnormal_return"] = prices["five_day_return"] - prices["five_day_benchmark_return"]

    features = daily_agg.merge(
        prices[
            [
                "date",
                "ticker",
                "daily_return",
                "prior_5d_return",
                "realized_vol_5d",
                "next_day_return",
                "next_day_abnormal_return",
                "five_day_return",
                "five_day_abnormal_return",
            ]
        ],
        on=["date", "ticker"],
        how="inner",
    )

    features = features.dropna(subset=["next_day_return", "next_day_abnormal_return", "five_day_return", "five_day_abnormal_return"])
    features["alpha_score"] = (
        0.26 * features["weighted_sentiment"]
        + 0.18 * features["sentiment_surprise"]
        + 0.14 * features["sentiment_momentum_5d"]
        + 0.12 * features["burstiness"]
        + 0.10 * features["event_intensity"]
        + 0.08 * features["sector_relative_sentiment"]
        - 0.07 * features["realized_vol_5d"]
        - 0.05 * features["prior_5d_return"]
    )
    features["label_up"] = (features["next_day_abnormal_return"] > 0).astype(int)

    summary = {
        "news_rows": int(len(news)),
        "price_rows": int(len(prices)),
        "feature_rows": int(len(features)),
        "equities": int(features["ticker"].nunique()),
        "trading_days": int(features["date"].nunique()),
        "feature_columns": int(len(features.columns)),
    }

    features = features.sort_values(["date", "ticker"]).reset_index(drop=True)
    features.to_parquet(output_dir / "features.parquet", index=False)
    features.to_csv(output_dir / "features.csv", index=False)
    with open(output_dir / "feature_summary.json", "w", encoding="utf-8") as handle:
        json.dump(summary, handle, indent=2)

    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
