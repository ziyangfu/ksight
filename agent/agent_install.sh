#!/bin/bash

# ksight Agent Installation Script
# This script handles the installation and dependency management for the ksight AI agent.

set -e

# Target installation path (passed from run.sh)
INSTALL_DIR="${1:-/usr/local/bin/ksight}"
AGENT_SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="${INSTALL_DIR}/venv"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_err() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check for root
if [ "$EUID" -ne 0 ] && [ "$EUID" -ne 1000 ]; then
    # Some environments allow non-root install to home dir, but for /usr/local/bin we need sudo
    if [[ "${INSTALL_DIR}" == /usr/local/* ]]; then
        log_err "Please run as root (use sudo) to install to ${INSTALL_DIR}"
        exit 1
    fi
fi

# Ensure target directories exist
mkdir -p "${INSTALL_DIR}/agent"

log_info "Installing agent source to ${INSTALL_DIR}/agent..."
# Copy all files except venv and __pycache__
cp -r "${AGENT_SRC_DIR}/"* "${INSTALL_DIR}/agent/"
rm -rf "${INSTALL_DIR}/agent/venv"
find "${INSTALL_DIR}/agent" -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true

# Setup virtual environment
if [ ! -d "${VENV_DIR}" ]; then
    log_info "Creating virtual environment in ${VENV_DIR}..."
    python3 -m venv "${VENV_DIR}"
else
    log_info "Virtual environment already exists in ${VENV_DIR}."
fi

# Install dependencies
log_info "Installing/Updating agent dependencies..."
"${VENV_DIR}/bin/pip" install --upgrade pip
"${VENV_DIR}/bin/pip" install -r "${INSTALL_DIR}/agent/requirements.txt"

# Handle .env file
# Try to find .env in project root (AGENT_SRC_DIR/..)
PROJECT_ROOT="$(cd "${AGENT_SRC_DIR}/.." && pwd)"
DOTENV_TARGET="${INSTALL_DIR}/agent/.env"

if [ -f "${PROJECT_ROOT}/.env" ]; then
    log_info "Copying .env from project root to ${DOTENV_TARGET}..."
    cp "${PROJECT_ROOT}/.env" "${DOTENV_TARGET}"
    chmod 600 "${DOTENV_TARGET}"
elif [ -f "$(pwd)/.env" ]; then
    log_info "Copying .env from current directory to ${DOTENV_TARGET}..."
    cp "$(pwd)/.env" "${DOTENV_TARGET}"
    chmod 600 "${DOTENV_TARGET}"
fi

if [ ! -f "${DOTENV_TARGET}" ]; then
    log_warn "No .env file found. Creating an empty one in ${DOTENV_TARGET}."
    touch "${DOTENV_TARGET}"
    chmod 600 "${DOTENV_TARGET}"
fi

log_info "Agent installation finished successfully."
