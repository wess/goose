#!/bin/sh
set -eu

REPO="wess/goose"
INSTALL_DIR="${GOOSE_INSTALL_DIR:-/usr/local/bin}"

main() {
  os="$(detect_os)"
  arch="$(detect_arch)"

  echo "Detected: ${os} ${arch}"

  version="$(fetch_latest_version)"
  echo "Latest version: ${version}"

  artifact="goose-${os}-${arch}.tar.gz"
  url="https://github.com/${REPO}/releases/download/${version}/${artifact}"

  tmpdir="$(mktemp -d)"
  trap 'rm -rf "$tmpdir"' EXIT

  echo "Downloading ${url}..."
  curl -fsSL "$url" -o "${tmpdir}/${artifact}"

  echo "Installing to ${INSTALL_DIR}..."
  tar xzf "${tmpdir}/${artifact}" -C "$tmpdir"

  if [ -w "$INSTALL_DIR" ]; then
    install -m 755 "${tmpdir}/goose" "${INSTALL_DIR}/goose"
  else
    echo "Elevated permissions required to install to ${INSTALL_DIR}"
    sudo install -m 755 "${tmpdir}/goose" "${INSTALL_DIR}/goose"
  fi

  echo "Installed goose $(${INSTALL_DIR}/goose --version) to ${INSTALL_DIR}/goose"
}

detect_os() {
  case "$(uname -s)" in
    Linux*)  echo "linux" ;;
    Darwin*) echo "darwin" ;;
    *)       echo "Unsupported OS: $(uname -s)" >&2; exit 1 ;;
  esac
}

detect_arch() {
  case "$(uname -m)" in
    x86_64|amd64)  echo "amd64" ;;
    arm64|aarch64) echo "arm64" ;;
    *)             echo "Unsupported architecture: $(uname -m)" >&2; exit 1 ;;
  esac
}

fetch_latest_version() {
  curl -fsSL "https://api.github.com/repos/${REPO}/releases/latest" \
    | sed -n 's/.*"tag_name": *"\([^"]*\)".*/\1/p'
}

main
