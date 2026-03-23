#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import pandas as pd


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Generate a scaled synthetic news + price research dataset.")
    parser.add_argument("--output-dir", default="data/generated", help="Directory for generated parquet/csv outputs.")
    parser.add_argument("--news-count", type=int, default=300_000, help="Number of synthetic news rows.")
    parser.add_argument("--ticker-count", type=int, default=500, help="Number of synthetic equities.")
    parser.add_argument("--seed", type=int, default=7, help="Random seed.")
    return parser


def generate_prices(tickers: list[str], sectors: dict[str, str], dates: pd.DatetimeIndex, rng: np.random.Generator) -> pd.DataFrame:
    frames: list[pd.DataFrame] = []
    benchmark_returns = rng.normal(0.0003, 0.011, len(dates))
    benchmark_close = 4000 * np.cumprod(1 + benchmark_returns)

    for idx, ticker in enumerate(tickers):
        base_price = rng.uniform(20, 350)
        beta = rng.uniform(0.8, 1.4)
        idio = rng.normal(0.0002, 0.018, len(dates))
        returns = beta * benchmark_returns + idio
        close = base_price * np.cumprod(1 + returns)
        open_ = close / (1 + rng.normal(0.0002, 0.005, len(dates)))
        high = np.maximum(open_, close) * (1 + rng.uniform(0.0005, 0.02, len(dates)))
        low = np.minimum(open_, close) * (1 - rng.uniform(0.0005, 0.02, len(dates)))
        volume = rng.integers(500_000, 12_000_000, len(dates))

        frames.append(
            pd.DataFrame(
                {
                    "date": dates,
                    "ticker": ticker,
                    "sector": sectors[ticker],
                    "open": open_,
                    "high": high,
                    "low": low,
                    "close": close,
                    "volume": volume,
                    "benchmark_close": benchmark_close,
                }
            )
        )

    prices = pd.concat(frames, ignore_index=True)
    prices["date"] = prices["date"].dt.strftime("%Y-%m-%d")
    return prices


def generate_news(
    tickers: list[str],
    sectors: dict[str, str],
    dates: pd.DatetimeIndex,
    news_count: int,
    rng: np.random.Generator,
) -> pd.DataFrame:
    sources = np.array(["Reuters", "Bloomberg", "CNBC", "WSJ", "MarketWatch", "Benzinga"])
    event_types = np.array(["earnings", "product", "legal", "guidance", "partnership", "leadership"])
    positive_terms = {
        "earnings": ("beats estimates", "raises guidance", "strong margin expansion"),
        "product": ("launches platform", "records strong demand", "wins customer traction"),
        "legal": ("resolves dispute", "clears review", "wins favorable ruling"),
        "guidance": ("improves outlook", "sees accelerating demand", "reaffirms forecast"),
        "partnership": ("expands partnership", "signs multi-year deal", "adds strategic customer"),
        "leadership": ("appoints veteran executive", "strengthens management team", "adds experienced CEO"),
    }
    negative_terms = {
        "earnings": ("misses estimates", "cuts guidance", "sees weaker margins"),
        "product": ("faces launch delay", "sees softer demand", "reports weak adoption"),
        "legal": ("faces probe", "escalates lawsuit", "draws antitrust scrutiny"),
        "guidance": ("warns on outlook", "cuts forecast", "flags demand slowdown"),
        "partnership": ("loses customer", "deal stalls", "partnership weakens"),
        "leadership": ("announces sudden departure", "faces management turmoil", "loses key executive"),
    }

    event_choices = rng.choice(event_types, size=news_count, p=[0.22, 0.20, 0.14, 0.18, 0.16, 0.10])
    sentiment_draw = rng.normal(0.1, 1.0, news_count)
    news_dates = rng.choice(dates, size=news_count)
    ticker_choices = rng.choice(tickers, size=news_count)
    source_choices = rng.choice(sources, size=news_count)
    hours = rng.integers(6, 20, size=news_count)
    minutes = rng.integers(0, 60, size=news_count)

    headlines: list[str] = []
    bodies: list[str] = []
    sentiment_bucket: list[int] = []
    sectors_out: list[str] = []
    for ticker, event_type, raw_sentiment in zip(ticker_choices, event_choices, sentiment_draw):
        positive = raw_sentiment >= 0
        phrase = rng.choice(positive_terms[event_type] if positive else negative_terms[event_type])
        sector = sectors[ticker]
        headlines.append(f"{ticker} {phrase} in {event_type} update")
        tone = "strong" if positive else "weak"
        bodies.append(
            f"{ticker} reported a {tone} {event_type} development. "
            f"Sector context for {sector} names remained active as investors repriced near-term expectations."
        )
        sentiment_bucket.append(1 if positive else -1)
        sectors_out.append(sector)

    timestamps = pd.to_datetime(news_dates) + pd.to_timedelta(hours, unit="h") + pd.to_timedelta(minutes, unit="m")
    news = pd.DataFrame(
        {
            "timestamp": timestamps.strftime("%Y-%m-%dT%H:%M:%SZ"),
            "date": pd.to_datetime(news_dates).strftime("%Y-%m-%d"),
            "source": source_choices,
            "ticker": ticker_choices,
            "sector": sectors_out,
            "event_type": event_choices,
            "headline": headlines,
            "body": bodies,
            "sentiment_seed": sentiment_draw,
            "sentiment_bucket": sentiment_bucket,
        }
    )
    return news.sort_values(["timestamp", "ticker"]).reset_index(drop=True)


def main() -> None:
    args = build_argument_parser().parse_args()
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    rng = np.random.default_rng(args.seed)
    dates = pd.bdate_range("2023-01-02", periods=252)
    tickers = [f"EQ{idx:03d}" for idx in range(args.ticker_count)]
    sector_names = ["Technology", "Healthcare", "Financials", "Industrials", "Energy", "Consumer"]
    sectors = {ticker: sector_names[idx % len(sector_names)] for idx, ticker in enumerate(tickers)}

    prices = generate_prices(tickers, sectors, dates, rng)
    news = generate_news(tickers, sectors, dates, args.news_count, rng)

    prices.to_parquet(output_dir / "prices.parquet", index=False)
    news.to_parquet(output_dir / "news.parquet", index=False)
    prices.head(20_000).to_csv(output_dir / "prices_sample.csv", index=False)
    news.head(20_000).to_csv(output_dir / "news_sample.csv", index=False)

    print(f"Generated {len(news):,} news records across {news['ticker'].nunique():,} equities.")
    print(f"Generated {len(prices):,} daily price rows across {prices['date'].nunique():,} trading days.")
    print(f"Wrote outputs to {output_dir}")


if __name__ == "__main__":
    main()
