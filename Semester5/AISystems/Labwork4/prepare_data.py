"""Воспроизведение региональной выборки из Russia Real Estate, версия 3.

Запуск: python prepare_data.py [--archive /путь/к/архиву.zip].
Без --archive архив загружается в игнорируемый каталог .runtime.
Используется только стандартная библиотека Python.
"""

import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import urllib.request
from zipfile import ZipFile


ROOT = Path(__file__).resolve().parent
REGION = "4189"
URL = (
    "https://www.kaggle.com/api/v1/datasets/download/"
    "mrdaniilak/russia-real-estate-20182021?datasetVersionNumber=3"
)
COLUMNS = [
    "price", "date", "time", "geo_lat", "geo_lon", "region", "building_type",
    "level", "levels", "rooms", "area", "kitchen_area", "object_type",
]


def extract(archive):
    destination = ROOT / "dataset.csv"
    temporary = destination.with_suffix(".csv.tmp")
    total = selected = 0
    try:
        with ZipFile(archive) as zipped, zipped.open("all_v2.csv") as binary:
            reader = csv.DictReader(io.TextIOWrapper(binary, encoding="utf-8-sig"))
            if reader.fieldnames != COLUMNS:
                raise ValueError(f"Неожиданная схема: {reader.fieldnames}")
            with temporary.open("w", encoding="utf-8", newline="") as output:
                writer = csv.DictWriter(output, fieldnames=COLUMNS, lineterminator="\n")
                writer.writeheader()
                for row in reader:
                    total += 1
                    if row["region"] == REGION:
                        writer.writerow(row)
                        selected += 1
        if selected < 100:
            raise ValueError(f"Недостаточно данных региона: {selected}")
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)
    metadata = {
        "dataset": "mrdaniilak/russia-real-estate-20182021",
        "dataset_page": "https://www.kaggle.com/datasets/mrdaniilak/russia-real-estate-20182021",
        "version": 3,
        "download_url": URL,
        "archive_member": "all_v2.csv",
        "license": "GNU Affero General Public License 3.0 (Kaggle metadata)",
        "region_code": int(REGION),
        "region_name": "Костромская область",
        "selection": "Все строки region == 4189, без очистки и случайной подвыборки",
        "source_rows": total,
        "selected_rows": selected,
        "sha256": hashlib.sha256(destination.read_bytes()).hexdigest(),
    }
    (ROOT / "dataset_source.json").write_text(
        json.dumps(metadata, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    print(json.dumps(metadata, ensure_ascii=False, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, help="Уже скачанный архив датасета")
    args = parser.parse_args()
    archive = args.archive or ROOT / ".runtime" / "real_estate.zip"
    if not archive.exists():
        if args.archive:
            parser.error(f"Архив не найден: {archive}")
        archive.parent.mkdir(parents=True, exist_ok=True)
        partial = archive.with_suffix(".zip.part")
        try:
            urllib.request.urlretrieve(URL, partial)
            partial.replace(archive)
        finally:
            partial.unlink(missing_ok=True)
    extract(archive)


if __name__ == "__main__":
    main()
