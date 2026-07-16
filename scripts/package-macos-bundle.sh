#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-${ROOT_DIR}/build-release/macOS-ARM64}"
OUT_APP="${2:-${ROOT_DIR}/dist/cryptexqt_macos_arm64.app}"
OUT_ZIP="${3:-${ROOT_DIR}/dist/CryptEX_macos_arm64_bundle.zip}"
SRC_APP="${BUILD_DIR}/cryptexqt_osx.app"

if [[ ! -d "${BUILD_DIR}" ]]; then
  echo "[package-mac] build directory not found: ${BUILD_DIR}" >&2
  exit 1
fi

if [[ ! -d "${SRC_APP}" ]]; then
  echo "[package-mac] app bundle not found: ${SRC_APP}" >&2
  exit 1
fi

if [[ ! -d "${SRC_APP}/Contents/Frameworks" || ! -d "${SRC_APP}/Contents/PlugIns" ]]; then
  echo "[package-mac] source app does not contain bundled frameworks/plugins" >&2
  exit 1
fi

rm -rf "${OUT_APP}"
rm -f "${OUT_ZIP}"
mkdir -p "$(dirname "${OUT_APP}")"
ditto "${SRC_APP}" "${OUT_APP}"

rewrite_dep_if_bundled() {
  local binary="$1"
  local old="$2"
  local base new_path

  if [[ "${old}" == *".framework/"* ]]; then
    local framework_name
    framework_name="$(echo "${old}" | sed -E 's#^.*/([^/]+)\.framework/Versions/[^/]+/([^/]+)$#\1#')"
    new_path="@loader_path/../Frameworks/${framework_name}.framework/Versions/A/${framework_name}"
  else
    base="$(basename "${old}")"
    new_path="@loader_path/../Frameworks/${base}"
  fi

  if [[ "${binary}" == *"/PlugIns/"* ]]; then
    new_path="${new_path/@loader_path/@executable_path}"
  fi

  install_name_tool -change "${old}" "${new_path}" "${binary}"
}

rewrite_binary() {
  local binary="$1"
  while IFS= read -r dep; do
    [[ -n "${dep}" ]] || continue
    case "${dep}" in
      /opt/homebrew/*|/opt/local/*|/Users/*)
        rewrite_dep_if_bundled "${binary}" "${dep}"
        ;;
    esac
  done < <(otool -L "${binary}" | awk 'NR > 1 { print $1 }')

  while IFS= read -r rpath; do
    [[ -n "${rpath}" ]] || continue
    case "${rpath}" in
      /opt/homebrew/*|/opt/local/*|/Users/*)
        install_name_tool -delete_rpath "${rpath}" "${binary}" || true
        ;;
    esac
  done < <(otool -l "${binary}" | awk '
    $1 == "cmd" && $2 == "LC_RPATH" { seen = 1; next }
    seen && $1 == "path" { print $2; seen = 0 }
  ')
}

verify_no_external_refs() {
  local found=0
  while IFS= read -r path; do
    [[ -n "${path}" ]] || continue
    local current_name
    current_name="$(basename "${path}")"
    if otool -L "${path}" 2>/dev/null | awk 'NR > 1 { print $1 }' | \
      awk -v current="${current_name}" '
        {
          n = split($0, parts, "/");
          base = parts[n];
          if (base == current) {
            next;
          }
          if ($0 ~ /^\/opt\/homebrew/ || $0 ~ /^\/opt\/local/ || $0 ~ /^\/Users\//) {
            print $0;
          }
        }
      ' | grep . >/dev/null; then
      echo "[package-mac] unresolved external library reference in ${path}" >&2
      otool -L "${path}" >&2
      found=1
    fi
  done < <(find "${OUT_APP}" \( -perm -111 -o -name '*.dylib' \) -type f | sort)

  if [[ ${found} -ne 0 ]]; then
    exit 1
  fi
}

rewrite_binary "${OUT_APP}/Contents/MacOS/cryptexqt_osx"
if [[ -x "${OUT_APP}/Contents/MacOS/cryptexd_osx" ]]; then
  rewrite_binary "${OUT_APP}/Contents/MacOS/cryptexd_osx"
fi
if [[ -x "${OUT_APP}/Contents/MacOS/cryptex_powminer_osx" ]]; then
  rewrite_binary "${OUT_APP}/Contents/MacOS/cryptex_powminer_osx"
fi

verify_no_external_refs

codesign --force --deep --sign - "${OUT_APP}"
codesign --verify --deep --strict --verbose=2 "${OUT_APP}" >/dev/null

ditto -c -k --sequesterRsrc --keepParent "${OUT_APP}" "${OUT_ZIP}"

echo "[package-mac] created bundle: ${OUT_APP}"
echo "[package-mac] created archive: ${OUT_ZIP}"
