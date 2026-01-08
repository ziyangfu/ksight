#!/bin/bash

# Ksight Orchestration Script
# This script handles building, installation, and setup of ksight tools and CLI.

set -e

INSTALL_DIR="/usr/local/bin/ksight"
KSIGHT_CLI_DIR="$(pwd)/ksightCli"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_sudo() {
    if [ "$EUID" -ne 0 ]; then
        log_error "Please run as root (use sudo)"
        exit 1
    fi
}

check_deps() {
    log_info "Checking dependencies..."
    
    deps=("cmake" "make" "python3" "pip3")
    for dep in "${deps[@]}"; do
        if ! command -v "$dep" &> /dev/null; then
            log_error "$dep is not installed. Please install it first."
            exit 1
        fi
    done
    
    # Check for click
    if ! python3 -c "import click" &> /dev/null; then
        log_warn "Python 'click' module not found. Attempting to install..."
        pip3 install click
    fi
    
    log_info "All dependencies satisfied."
}

build_and_install() {
    log_info "Building and installing ksight tools..."

    # 检查当前文件夹，确保在 ksight 根目录下运行 run.sh
    if [ ! -f "CMakeLists.txt" ] || [ ! -d "src" ] || [ ! -d "ksightCli" ]; then
        log_error "This script must be run from the ksight root directory."
        log_error "Please navigate to the ksight project root and try again."
        exit 1
    fi
    
    mkdir -p build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" ..
    make -j$(nproc)
    make install
    cd ..
    
    log_info "Tools built and installed to ${INSTALL_DIR}"
}

install_third_party() {
    log_info "Installing third-party tools..."
    # nettrace的安装   
    ARCH=$(uname -m)
    THIRD_PARTY_SRC="third_tools/binary/nettrace/${ARCH}"
    
    if [ ! -d "${THIRD_PARTY_SRC}" ]; then
        log_error "Unsupported architecture: ${ARCH} or third-party tools missing."
        exit 1
    fi
    
    # Create target directories for nettrace
    mkdir -p "${INSTALL_DIR}/net/nettrace/bin"
    mkdir -p "${INSTALL_DIR}/net/nettrace/config"
    mkdir -p "${INSTALL_DIR}/net/nettrace/scripts"
    
    # Copy nettrace binary and config
    cp "${THIRD_PARTY_SRC}/bin/nettrace" "${INSTALL_DIR}/net/nettrace/bin/"
    cp "${THIRD_PARTY_SRC}/config/brief.json" "${INSTALL_DIR}/net/nettrace/config/"
    cp "${THIRD_PARTY_SRC}/scripts/bash-complete.sh" "${INSTALL_DIR}/net/nettrace/scripts/"

    log_info "Third-party tools installed."
}

generate_metadata() {
    log_info "Generating command metadata for ksightCli..."    
    # Run gen_cmd_data.py to create commands_data.py
    # Now scanning the entire INSTALL_DIR for bash-complete.sh
    python3 "${KSIGHT_CLI_DIR}/gen_cmd_data.py" --scan-dir "${INSTALL_DIR}" --output "${KSIGHT_CLI_DIR}/commands_data.py"
    
    log_info "Metadata generated."
}

setup_ksight_cli() {
    log_info "Setting up ksightCli..."
    
    # Create ksightCli directory in INSTALL_DIR
    mkdir -p "${INSTALL_DIR}/ksightCli"
    
    # Copy ksightCli and generated commands_data.py
    cp "${KSIGHT_CLI_DIR}/ksightCli" "${INSTALL_DIR}/ksightCli/"
    cp "${KSIGHT_CLI_DIR}/commands_data.py" "${INSTALL_DIR}/ksightCli/"
    chmod +x "${INSTALL_DIR}/ksightCli/ksightCli"
    
    # Create symlink in /usr/local/bin
    ln -sf "${INSTALL_DIR}/ksightCli/ksightCli" /usr/local/bin/ksightCli
    
    log_info "ksightCli setup complete. You can now use 'ksightCli' command."
    log_info "To enable tab completion, run: eval \"\$(_KSIGHTCLI_COMPLETE=source ksightCli)\""

    # Add eval command to .bashrc if not already present
    # When running with sudo, we need to get the actual user's home directory
    if [ -n "${SUDO_USER}" ]; then
        # Running under sudo, get the actual user's home directory
        ACTUAL_USER="${SUDO_USER}"
        ACTUAL_HOME=$(eval echo ~${SUDO_USER})
    else
        # Not running under sudo
        ACTUAL_USER="${USER}"
        ACTUAL_HOME="${HOME}"
    fi
    
    BASHRC="${ACTUAL_HOME}/.bashrc"
    COMP_CMD="eval \"\$(_KSIGHTCLI_COMPLETE=source ksightCli)\""
    
    if ! grep -qF "${COMP_CMD}" "${BASHRC}"; then
        log_info "Adding auto-completion to ${BASHRC}..."
        echo -e "\n# ksight CLI auto-completion\n${COMP_CMD}" >> "${BASHRC}"
        source "${BASHRC}"
        log_info "Auto-completion added. Please restart shell to enable it"
    else
        log_info "Auto-completion already configured in ${BASHRC}"
    fi
}

main() {
    check_sudo
    check_deps
    build_and_install
    install_third_party
    generate_metadata
    setup_ksight_cli
    
    log_info "ksight installation finished successfully!"
}

main "$@"
