#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-${ROOT_DIR}/build-release/windows-x86_64}"
OUT_DIR="${2:-${ROOT_DIR}/dist/CryptEX_windows_x86_64_bundle}"
QT_ROOT="${QT6_ROOT_WIN_X86_64:-${HOME}/Qt/6.10.2/mingw_64}"

declare -A COPIED_PATHS=()
declare -A COPIED_DLLS=()

cache_value() {
  local key="$1"
  local cache="${BUILD_DIR}/CMakeCache.txt"
  if [[ ! -f "${cache}" ]]; then
    return 0
  fi
  sed -n "s/^${key}:[^=]*=//p" "${cache}" | head -n 1
}

OPENSSL_ROOT="${OPENSSL_ROOT_DIR_WIN_X86_64:-$(cache_value OPENSSL_ROOT_DIR)}"
OPUS_ROOT="${OPUS_ROOT_DIR_WIN_X86_64:-$(cache_value OPUS_ROOT_DIR)}"

detect_mingw_bin() {
  for candidate in \
    "$(command -v x86_64-w64-mingw32-g++ 2>/dev/null || true)" \
    "$(command -v x86_64-w64-mingw32-g++-posix 2>/dev/null || true)" \
    "$(command -v x86_64-w64-mingw32-gcc 2>/dev/null || true)" \
    "$(command -v x86_64-w64-mingw32-gcc-posix 2>/dev/null || true)"; do
    if [[ -n "${candidate}" ]]; then
      dirname "${candidate}"
      return 0
    fi
  done
  return 1
}

MINGW_BIN="${MINGW_W64_BIN_WIN_X86_64:-$(detect_mingw_bin || true)}"

if [[ ! -d "${BUILD_DIR}" ]]; then
  echo "[package-win] build directory not found: ${BUILD_DIR}" >&2
  exit 1
fi

if [[ ! -d "${QT_ROOT}" ]]; then
  echo "[package-win] Qt root not found: ${QT_ROOT}" >&2
  exit 1
fi

rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"

copy_required() {
  local src="$1"
  local dst="$2"
  if [[ ! -f "${src}" ]]; then
    echo "[package-win] missing required file: ${src}" >&2
    exit 1
  fi
  cp "${src}" "${dst}"
}

search_dirs=("${QT_ROOT}/bin")
if [[ -n "${MINGW_BIN}" && -d "${MINGW_BIN}" ]]; then
  search_dirs+=("${MINGW_BIN}")
fi
if [[ -n "${OPENSSL_ROOT}" && -d "${OPENSSL_ROOT}/bin" ]]; then
  search_dirs+=("${OPENSSL_ROOT}/bin")
fi
if [[ -n "${OPUS_ROOT}" && -d "${OPUS_ROOT}/bin" ]]; then
  search_dirs+=("${OPUS_ROOT}/bin")
fi

find_dep_file() {
  local name="$1"
  local dir
  for dir in "${search_dirs[@]}"; do
    if [[ -f "${dir}/${name}" ]]; then
      printf '%s\n' "${dir}/${name}"
      return 0
    fi
  done
  return 1
}

copy_pe_file() {
  local src="$1"
  local dst="$2"
  local key
  key="$(cd "$(dirname "${src}")" && pwd)/$(basename "${src}")"
  if [[ -n "${COPIED_PATHS[${key}]:-}" ]]; then
    return 0
  fi
  mkdir -p "$(dirname "${dst}")"
  cp "${src}" "${dst}"
  COPIED_PATHS["${key}"]=1
  COPIED_DLLS["$(basename "${src}" | tr '[:upper:]' '[:lower:]')"]=1
}

is_known_windows_runtime_dll() {
  local low="${1,,}"
  case "${low}" in
    kernel32.dll|user32.dll|gdi32.dll|gdi32full.dll|advapi32.dll|ws2_32.dll|mswsock.dll|crypt32.dll|ole32.dll|oleaut32.dll|shell32.dll|comdlg32.dll|comctl32.dll|winmm.dll|mpr.dll|imm32.dll|dwmapi.dll|setupapi.dll|version.dll|dnsapi.dll|netapi32.dll|bcrypt.dll|rpcrt4.dll|shlwapi.dll|secur32.dll|ntdll.dll|combase.dll|msvcrt.dll|cfgmgr32.dll|wtsapi32.dll|wevtapi.dll|oleacc.dll|userenv.dll|d3d11.dll|d3d12.dll|dxgi.dll|opengl32.dll|glu32.dll|winspool.drv|windows.storage.dll|powrprof.dll|dbghelp.dll|normaliz.dll|propsys.dll|authz.dll|dwrite.dll|avrt.dll|iphlpapi.dll|winhttp.dll|imagehlp.dll|d3d9.dll|dxva2.dll|evr.dll|mf.dll|mfplat.dll|mfreadwrite.dll|shcore.dll|ncrypt.dll|odbc32.dll|uxtheme.dll)
      return 0
      ;;
    api-ms-win-*.dll)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

resolve_and_copy_deps() {
  local pe_path="$1"
  local dest_dir="$2"
  local dll dep_src low
  while IFS= read -r dll; do
    [[ -n "${dll}" ]] || continue
    low="${dll,,}"
    if is_known_windows_runtime_dll "${low}"; then
      continue
    fi
    if [[ -n "${COPIED_DLLS[${low}]:-}" ]]; then
      continue
    fi
    dep_src="$(find_dep_file "${dll}" || true)"
    if [[ -z "${dep_src}" ]]; then
      echo "[package-win] warning: dependency not found locally, assuming provided by target system: ${dll}" >&2
      continue
    fi
    copy_pe_file "${dep_src}" "${dest_dir}/$(basename "${dep_src}")"
    resolve_and_copy_deps "${dep_src}" "${dest_dir}"
  done < <(x86_64-w64-mingw32-objdump -p "${pe_path}" | sed -n 's/.*DLL Name:[[:space:]]*//p')
}

copy_plugin_with_deps() {
  local rel_path="$1"
  local src="${QT_ROOT}/plugins/${rel_path}"
  local dst="${OUT_DIR}/plugins/${rel_path}"
  if [[ ! -f "${src}" ]]; then
    echo "[package-win] warning: Qt plugin not found, skipping: ${src}" >&2
    return 0
  fi
  copy_pe_file "${src}" "${dst}"
  resolve_and_copy_deps "${src}" "${OUT_DIR}"
}

verify_bundle_dependencies() {
  local bundle_root="$1"
  local tmp="${bundle_root}/.missing-dll-report"
  : > "${tmp}"
  while IFS= read -r pe; do
    while IFS= read -r dll; do
      [[ -n "${dll}" ]] || continue
      local low="${dll,,}"
      if is_known_windows_runtime_dll "${low}"; then
        continue
      fi
      if [[ ! -f "${bundle_root}/${dll}" ]]; then
        printf '%s -> %s\n' "$(basename "${pe}")" "${dll}" >> "${tmp}"
      fi
    done < <(x86_64-w64-mingw32-objdump -p "${pe}" | sed -n 's/.*DLL Name:[[:space:]]*//p')
  done < <(find "${bundle_root}" -type f \( -iname '*.exe' -o -iname '*.dll' \) | sort)

  if [[ -s "${tmp}" ]]; then
    sort -u "${tmp}" >&2
    rm -f "${tmp}"
    echo "[package-win] unresolved non-system DLL references remain in bundle" >&2
    exit 1
  fi
  rm -f "${tmp}"
}

copy_required "${BUILD_DIR}/cryptexqt_win32.exe" "${OUT_DIR}/cryptexqt_win32.exe"
copy_required "${BUILD_DIR}/cryptexd_win32.exe" "${OUT_DIR}/cryptexd_win32.exe"
copy_required "${BUILD_DIR}/cryptex_tests.exe" "${OUT_DIR}/cryptex_tests.exe"
copy_required "${BUILD_DIR}/cryptex_powminer_win32.exe" "${OUT_DIR}/cryptex_powminer_win32.exe"

COPIED_DLLS["cryptexqt_win32.exe"]=1
COPIED_DLLS["cryptexd_win32.exe"]=1
COPIED_DLLS["cryptex_tests.exe"]=1
COPIED_DLLS["cryptex_powminer_win32.exe"]=1

resolve_and_copy_deps "${OUT_DIR}/cryptexqt_win32.exe" "${OUT_DIR}"
resolve_and_copy_deps "${OUT_DIR}/cryptexd_win32.exe" "${OUT_DIR}"
resolve_and_copy_deps "${OUT_DIR}/cryptex_tests.exe" "${OUT_DIR}"
resolve_and_copy_deps "${OUT_DIR}/cryptex_powminer_win32.exe" "${OUT_DIR}"

# Deploy only the plugin set the desktop app actually uses.
copy_plugin_with_deps "platforms/qwindows.dll"
copy_plugin_with_deps "imageformats/qico.dll"
copy_plugin_with_deps "imageformats/qjpeg.dll"
copy_plugin_with_deps "imageformats/qgif.dll"
copy_plugin_with_deps "imageformats/qsvg.dll"
copy_plugin_with_deps "iconengines/qsvgicon.dll"
copy_plugin_with_deps "styles/qmodernwindowsstyle.dll"
copy_plugin_with_deps "networkinformation/qnetworklistmanager.dll"
copy_plugin_with_deps "sqldrivers/qsqlite.dll"
copy_plugin_with_deps "tls/qcertonlybackend.dll"
copy_plugin_with_deps "tls/qschannelbackend.dll"
copy_plugin_with_deps "tls/qopensslbackend.dll"
copy_plugin_with_deps "multimedia/windowsmediaplugin.dll"
copy_plugin_with_deps "multimedia/ffmpegmediaplugin.dll"

cat > "${OUT_DIR}/qt.conf" <<'EOF'
[Paths]
Plugins = plugins
EOF

cat > "${OUT_DIR}/README.txt" <<'EOF'
CryptEX Windows Runtime Bundle
==============================

Contents
- cryptexqt_win32.exe : Qt GUI client
- cryptexd_win32.exe  : backend / node / RPC daemon
- cryptex_tests.exe   : test binary
- cryptex_powminer_win32.exe : external SHA3-512 PoW worker

How to launch
1. Start cryptexqt_win32.exe
2. The GUI will auto-discover cryptexd_win32.exe in the same folder
3. The GUI can launch the backend for you
4. Mining uses cryptex_powminer_win32.exe from the same folder

Notes
- Keep the plugins directory and DLLs beside the executables
- Use this bundle zip on Windows; the standalone .exe release assets are not self-contained
- On older Windows installs, the Microsoft Universal CRT may still need to be present system-wide
EOF

verify_bundle_dependencies "${OUT_DIR}"

(
  cd "$(dirname "${OUT_DIR}")"
  rm -f "$(basename "${OUT_DIR}").zip"
  zip -qry "$(basename "${OUT_DIR}").zip" "$(basename "${OUT_DIR}")"
)

echo "[package-win] created bundle: ${OUT_DIR}"
echo "[package-win] created archive: ${OUT_DIR}.zip"
