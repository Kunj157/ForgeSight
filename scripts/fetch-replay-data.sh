#!/usr/bin/env bash
# Fetch NASA C-MAPSS and UCI SECOM into gitignored data/. Nothing from
# these archives is committed — the simulator reads whatever path you pass
# to --replay-file.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DATA="${FORGESIGHT_DATA:-$ROOT/data}"
mkdir -p "$DATA/cmapss" "$DATA/secom"

# NASA PCoE C-MAPSS zip (PHM 2008). The ti.arc.nasa.gov/c/6 short-link has
# been flaky; this NASA data-portal download is the same archive.
CMAPSS_URL="${CMAPSS_URL:-https://data.nasa.gov/download/xaut-bemq/application%2Fzip}"
# UCI ML repository, dataset 390.
SECOM_URL="${SECOM_URL:-https://archive.ics.uci.edu/static/public/390/secom.zip}"

fetch() {
  local url="$1" dest="$2"
  echo "GET $url -> $dest"
  curl -fL --retry 3 --retry-delay 2 -o "$dest" "$url"
}

if [[ ! -f "$DATA/cmapss/train_FD001.txt" ]]; then
  zip="$DATA/cmapss/CMAPSSData.zip"
  if ! fetch "$CMAPSS_URL" "$zip"; then
    echo "C-MAPSS download failed. Place train_FD001.txt in $DATA/cmapss/ by hand." >&2
  else
    unzip -o -q "$zip" -d "$DATA/cmapss"
    # Some archives nest a CMAPSSData/ folder.
    if [[ ! -f "$DATA/cmapss/train_FD001.txt" ]]; then
      find "$DATA/cmapss" -name 'train_FD001.txt' -exec cp {} "$DATA/cmapss/train_FD001.txt" \;
    fi
  fi
else
  echo "C-MAPSS already present: $DATA/cmapss/train_FD001.txt"
fi

if [[ ! -f "$DATA/secom/secom.data" ]]; then
  zip="$DATA/secom/secom.zip"
  if ! fetch "$SECOM_URL" "$zip"; then
    echo "SECOM download failed. Place secom.data in $DATA/secom/ by hand." >&2
  else
    unzip -o -q "$zip" -d "$DATA/secom"
    if [[ ! -f "$DATA/secom/secom.data" ]]; then
      find "$DATA/secom" -iname 'secom.data' -exec cp {} "$DATA/secom/secom.data" \;
      find "$DATA/secom" -iname 'secom_labels.data' -exec cp {} "$DATA/secom/secom_labels.data" \;
    fi
  fi
else
  echo "SECOM already present: $DATA/secom/secom.data"
fi

echo
echo "C-MAPSS: $DATA/cmapss/train_FD001.txt"
echo "SECOM:   $DATA/secom/secom.data"
echo
echo "Replay:"
echo "  PYTHONPATH=$ROOT python3 -m simulators.run -c $ROOT/simulators/config.yaml \\"
echo "    --replay cmapss --replay-file $DATA/cmapss/train_FD001.txt"
echo "  PYTHONPATH=$ROOT python3 -m simulators.run -c $ROOT/simulators/config.yaml \\"
echo "    --replay secom --replay-file $DATA/secom/secom.data \\"
echo "    --replay-labels $DATA/secom/secom_labels.data"
