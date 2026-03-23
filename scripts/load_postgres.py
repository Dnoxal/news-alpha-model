#!/usr/bin/env python3

from __future__ import annotations

import argparse
import io
import json
from pathlib import Path

import pandas as pd
import psycopg


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Load generated research tables into PostgreSQL.")
    parser.add_argument("--postgres-url", required=True, help="PostgreSQL connection URL.")
    parser.add_argument("--input-dir", default="data/generated", help="Directory containing raw parquet files.")
    parser.add_argument("--processed-dir", default="data/processed", help="Directory containing engineered feature outputs.")
    parser.add_argument("--schema", default="news_alpha", help="Destination PostgreSQL schema.")
    return parser


def copy_frame(conn: psycopg.Connection, frame: pd.DataFrame, qualified_table: str) -> None:
    columns = list(frame.columns)
    buffer = io.StringIO()
    frame.to_csv(buffer, index=False, header=False)
    buffer.seek(0)
    with conn.cursor() as cur:
        with cur.copy(f"COPY {qualified_table} ({', '.join(columns)}) FROM STDIN WITH (FORMAT CSV)") as copy:
            copy.write(buffer.read())


def main() -> None:
    args = build_argument_parser().parse_args()
    input_dir = Path(args.input_dir)
    processed_dir = Path(args.processed_dir)
    manifest_dir = Path("data/postgres")
    manifest_dir.mkdir(parents=True, exist_ok=True)

    news = pd.read_parquet(input_dir / "news.parquet")
    prices = pd.read_parquet(input_dir / "prices.parquet")
    features = pd.read_parquet(processed_dir / "features.parquet")

    schema_sql = Path("sql/schema.sql").read_text(encoding="utf-8").replace("news_alpha", args.schema)

    with psycopg.connect(args.postgres_url) as conn:
        with conn.cursor() as cur:
            cur.execute(schema_sql)
            cur.execute(f"TRUNCATE TABLE {args.schema}.news_raw, {args.schema}.prices_raw, {args.schema}.feature_store")
        copy_frame(conn, news, f"{args.schema}.news_raw")
        copy_frame(conn, prices, f"{args.schema}.prices_raw")
        copy_frame(conn, features, f"{args.schema}.feature_store")
        conn.commit()

    manifest = {
        "schema": args.schema,
        "news_rows": int(len(news)),
        "price_rows": int(len(prices)),
        "feature_rows": int(len(features)),
    }
    with open(manifest_dir / "load_manifest.json", "w", encoding="utf-8") as handle:
        json.dump(manifest, handle, indent=2)
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
