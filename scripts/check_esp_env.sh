#!/bin/bash
set -euo pipefail

MIN_ESPTOOL_VER="5.0.0"

echo "========================================"
echo "  PlatformIO / esptool / Python Report"
echo "  Host: $(uname -srm)  ($(date -u))"
echo "========================================"
echo

# Compare two versions; returns 0 if $1 >= $2
version_ge() {
  # sort -V is GNU, so fallback to printf trick:
  # Use sort -V if available, else lex compare (not perfect)
  if command -v sort >/dev/null 2>&1 && sort --version >/dev/null 2>&1; then
    [ "$(printf '%s\n%s\n' "$1" "$2" | sort -V | head -1)" = "$2" ]
  else
    # Fallback: string comparison (not perfect for versions)
    [ "$1" \> "$2" ] || [ "$1" = "$2" ]
  fi
}

check_executable_versions() {
  exe_name="$1"
  min_version="$2"

  echo "----- CHECK: $exe_name -----"

  # Find all executables in PATH
  IFS=':' read -r -a pathdirs <<< "$PATH"
  found_paths=()
  for d in "${pathdirs[@]}"; do
    if [ -x "$d/$exe_name" ]; then
      found_paths+=("$d/$exe_name")
    fi
  done

  if [ ${#found_paths[@]} -eq 0 ]; then
    echo "Not found in PATH"
    echo
    return
  fi

  default_path=$(command -v "$exe_name" || echo "")

  echo "Found ${#found_paths[@]} location(s)"
  for p in "${found_paths[@]}"; do
    if [ "$p" = "$default_path" ]; then
      prefix="*DEFAULT*"
    else
      prefix="          "
    fi

    echo "  - $p"
    # Symlink info
    if [ -L "$p" ]; then
      target=$(readlink "$p")
      echo "    symlink → $target"
    else
      echo "    (not a symlink)"
    fi

    # Version info
    if [[ "$exe_name" == "python" || "$exe_name" == "python3" ]]; then
      version=$("$p" --version 2>&1)
    elif [[ "$exe_name" == "esptool" ]]; then
      # esptool.py --version invalid, use 'version' command instead
      version=$("$p" version 2>&1 || echo "unknown")
    elif [[ "$exe_name" == "pio" ]]; then
      version=$("$p" --version 2>&1 | head -1)
    else
      version=$("$p" --version 2>&1 || echo "unknown")
    fi
    echo "    version: $version"

    # Check version compatibility for esptool only
    if [[ "$exe_name" == "esptool" ]]; then
      ver_num=$(echo "$version" | grep -oE '[0-9]+(\.[0-9]+)+' | head -1 || echo "")
      if [ -n "$ver_num" ]; then
        if version_ge "$ver_num" "$min_version"; then
          echo "    [OK] Version is compatible"
        else
          echo "    [WARN] Version is outdated; minimum required is $min_version"
        fi
      else
        echo "    [WARN] Could not parse version number"
      fi
    fi
  done
  echo
}

check_platformio_esptool() {
  echo "----- CHECK: PlatformIO tool-esptoolpy -----"
  pio_path="$HOME/.platformio/packages/tool-esptoolpy"
  if [ ! -d "$pio_path" ]; then
    echo "Not found at $pio_path"
    echo
    return
  fi

  echo "Found at $pio_path"

  if [ -f "$pio_path/package.json" ]; then
    ver=$(grep -o '"version"[[:space:]]*:[[:space:]]*"[^"]*"' "$pio_path/package.json" | head -1 | cut -d'"' -f4)
    echo "  Version: $ver"
    if version_ge "$ver" "$MIN_ESPTOOL_VER"; then
      echo "  [OK] Version is compatible"
    else
      echo "  [WARN] Version is outdated; may cause build failures"
    fi
  else
    echo "  package.json missing; cannot determine version"
  fi
  echo
}

# Run all checks
check_executable_versions python3 ""
check_executable_versions python ""
check_executable_versions esptool "$MIN_ESPTOOL_VER"
check_executable_versions pio ""

check_platformio_esptool

echo "----- PATH -----"
for d in "${pathdirs[@]}"; do
  echo "  $d"
done
echo

echo "===== END OF REPORT ====="
