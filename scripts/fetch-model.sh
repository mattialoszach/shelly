#!/usr/bin/env bash
set -euo pipefail

# usage: ./scripts/fetch-model.sh            # lädt base.en
#        ./scripts/fetch-model.sh small.en   # lädt small.en
#        ./scripts/fetch-model.sh small      # multilingual small

MODEL="${1:-base.en}"

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WHISPER_DIR="${REPO_ROOT}/third_party/whisper.cpp"
DL_SCRIPT="${WHISPER_DIR}/models/download-ggml-model.sh"
MODELS_OUT="${REPO_ROOT}/models"

# 1) whisper.cpp submodule
if [ ! -d "${WHISPER_DIR}" ]; then
  echo "→ whisper.cpp submodule not found. Initializing…"
  git submodule update --init --recursive third_party/whisper.cpp
fi

# 2) Download using official script from whisper.cpp
echo "→ Downloading Whisper model '${MODEL}' via whisper.cpp script…"
bash "${DL_SCRIPT}" "${MODEL}"

# 3) Derive destination name
SRC="${WHISPER_DIR}/models/ggml-${MODEL}.bin"
mkdir -p "${MODELS_OUT}"
DST="${MODELS_OUT}/ggml-${MODEL}.bin"

# 4) Copy to models folder
cp -f "${SRC}" "${DST}"

# 5) Quick Sanity-Check
BYTES=$(wc -c < "${DST}")
if [ "${BYTES}" -lt 1000000 ]; then
  echo "✗ Model seems too small (${BYTES} bytes) – did the download fail?"
  echo "  File: ${DST}"
  exit 1
fi

echo "✓ Model ready: ${DST}"
ls -lh "${DST}"