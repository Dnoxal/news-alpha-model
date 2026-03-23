CREATE SCHEMA IF NOT EXISTS news_alpha;

CREATE TABLE IF NOT EXISTS news_alpha.news_raw (
    timestamp TIMESTAMPTZ,
    date DATE,
    source TEXT,
    ticker TEXT,
    sector TEXT,
    event_type TEXT,
    headline TEXT,
    body TEXT,
    sentiment_seed DOUBLE PRECISION,
    sentiment_bucket INTEGER
);

CREATE TABLE IF NOT EXISTS news_alpha.prices_raw (
    date DATE,
    ticker TEXT,
    sector TEXT,
    open DOUBLE PRECISION,
    high DOUBLE PRECISION,
    low DOUBLE PRECISION,
    close DOUBLE PRECISION,
    volume DOUBLE PRECISION,
    benchmark_close DOUBLE PRECISION
);

CREATE TABLE IF NOT EXISTS news_alpha.feature_store (
    date DATE,
    ticker TEXT,
    sector TEXT,
    article_count INTEGER,
    unique_sources INTEGER,
    avg_sentiment DOUBLE PRECISION,
    sentiment_dispersion DOUBLE PRECISION,
    weighted_sentiment DOUBLE PRECISION,
    event_intensity DOUBLE PRECISION,
    sentiment_surprise DOUBLE PRECISION,
    sentiment_momentum_5d DOUBLE PRECISION,
    burstiness DOUBLE PRECISION,
    sector_avg_sentiment DOUBLE PRECISION,
    sector_relative_sentiment DOUBLE PRECISION,
    daily_return DOUBLE PRECISION,
    prior_5d_return DOUBLE PRECISION,
    realized_vol_5d DOUBLE PRECISION,
    next_day_return DOUBLE PRECISION,
    next_day_abnormal_return DOUBLE PRECISION,
    five_day_return DOUBLE PRECISION,
    five_day_abnormal_return DOUBLE PRECISION,
    alpha_score DOUBLE PRECISION,
    label_up INTEGER
);
