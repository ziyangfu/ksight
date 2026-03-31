#!/bin/bash

# ksight src_gtools 统一安装脚本
# 该脚本自动扫描 src_gtools 下的子目录，并将 Python 工具按照 ksight 标准结构安装。

set -e

INSTALL_DIR=$1

if [ -z "${INSTALL_DIR}" ]; then
    echo "Usage: $0 <INSTALL_DIR>"
    exit 1
fi

LOG_INFO() {
    echo -e "\033[0;32m[GTOOLS-INFO]\033[0m $1"
}

LOG_WARN() {
    echo -e "\033[1;33m[GTOOLS-WARN]\033[0m $1"
}

# 模块列表
MODULES=("net" "memory" "ipc")

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

for module in "${MODULES[@]}"; do
    module_dir="${SCRIPT_DIR}/${module}"
    if [ ! -d "${module_dir}" ]; then
        continue
    fi

    for tool_path in "${module_dir}"/*; do
        if [ ! -d "${tool_path}" ]; then
            continue
        fi

        tool_name=$(basename "${tool_path}")
        target_base="${INSTALL_DIR}/${module}/${tool_name}"
        
        LOG_INFO "Installing tool: ${module}/${tool_name}..."

        # 创建标准安装结构
        mkdir -p "${target_base}/bin"
        mkdir -p "${target_base}/config"
        mkdir -p "${target_base}/scripts"

        # 1. 安装配置和补全脚本 (用于 ksightCli 元数据扫描)
        if [ -d "${tool_path}/config" ]; then
            cp -r "${tool_path}/config/"* "${target_base}/config/"
        fi
        if [ -d "${tool_path}/scripts" ]; then
            cp -r "${tool_path}/scripts/"* "${target_base}/scripts/"
        fi

        # 2. 安装 Python 源码和支持文件
        # 我们将目录下所有的 .py 和 .json (非 config/scripts 下的) 复制到 bin/
        find "${tool_path}" -maxdepth 1 -name "*.py" -o -name "*.json" | while read -r file; do
            cp "$file" "${target_base}/bin/"
        done

        # 3. 处理主入口脚本并重命名为工具名
        # 约定优于配置：如果存在与文件夹同名的 .py 文件，或者是 nsp.py (针对 netstack_probe)
        entry_point=""
        if [ -f "${tool_path}/${tool_name}.py" ]; then
            entry_point="${tool_name}.py"
        elif [ -f "${tool_path}/nsp.py" ]; then
            entry_point="nsp.py"
        else
            # 找找看有没有唯一的 .py 文件
            py_files=($(find "${tool_path}" -maxdepth 1 -name "*.py"))
            if [ ${#py_files[@]} -eq 1 ]; then
                entry_point=$(basename "${py_files[0]}")
            fi
        fi

        if [ -n "${entry_point}" ]; then
            cp "${tool_path}/${entry_point}" "${target_base}/bin/${tool_name}"
            chmod +x "${target_base}/bin/${tool_name}"
            LOG_INFO "Set entry point: ${entry_point} -> bin/${tool_name}"
        else
            LOG_WARN "Could not identify entry point for ${tool_name}"
        fi
    done
done

LOG_INFO "All src_gtools installed to ${INSTALL_DIR}"
