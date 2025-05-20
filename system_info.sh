#!/bin/bash

# CPU usage
CPU_USAGE=$(top -bn1 | grep "Cpu(s)" | awk '{print $2}' | cut -d. -f1)
CPU_INFO="CPU: $CPU_USAGE%"

# RAM usage
RAM_TOTAL=$(free -h | grep "Mem:" | awk '{print $2}')
RAM_USED=$(free -h | grep "Mem:" | awk '{print $3}')
RAM_INFO="RAM: $RAM_USED / $RAM_TOTAL"

# Disk usage
DISK_TOTAL=$(df -h / | tail -1 | awk '{print $2}')
DISK_USED=$(df -h / | tail -1 | awk '{print $3}')
DISK_INFO="Disk: $DISK_USED / $DISK_TOTAL"

# GPU usage (проверка для Intel, AMD, NVIDIA)
GPU_INFO=""

# Проверка Intel GPU
if command -v intel_gpu_top &> /dev/null; then
    INTEL_USAGE=$(intel_gpu_top -s 1 -o - | grep "GPU" | awk '{print $2}' | cut -d% -f1)
    if [ -n "$INTEL_USAGE" ]; then
        GPU_INFO="$GPU_INFO\n🎮 Intel: $INTEL_USAGE%"
    fi
fi

# Проверка AMD GPU
if command -v radeontop &> /dev/null; then
    AMD_USAGE=$(radeontop -l 1 | grep -A 1 "GPU" | tail -n 1 | awk '{print $2}' | cut -d% -f1)
    if [ -n "$AMD_USAGE" ]; then
        GPU_INFO="$GPU_INFO\n🎮 AMD: $AMD_USAGE%"
    fi
fi

# Проверка NVIDIA GPU
if command -v nvidia-smi &> /dev/null; then
    NVIDIA_USAGE=$(nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader | awk '{print $1}' | cut -d% -f1)
    if [ -n "$NVIDIA_USAGE" ]; then
        GPU_INFO="$GPU_INFO\n🎮 NVIDIA: $NVIDIA_USAGE%"
    fi
fi

# Если GPU не найдено, добавляем сообщение
if [ -z "$GPU_INFO" ]; then
    GPU_INFO="\n🎮 No GPU data available"
fi

# Собираем все в одну строку с разделителями
SYSTEM_INFO="$CPU_INFO\n$RAM_INFO\n$DISK_INFO$GPU_INFO"

# Выводим через wofi
echo -e "$SYSTEM_INFO" | wofi --show dmenu --prompt "System Info" --allow-markup
