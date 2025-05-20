#!/bin/bash

# Временный файл для хранения данных
TEMP_FILE="/tmp/system_info_$$.txt"

# Создаем временный файл
touch "$TEMP_FILE" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "Ошибка: нет прав для создания временного файла" >&2
    exit 1
fi

# Функция для сбора данных
update_system_info() {
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

    # Если GPU не найдено
    if [ -z "$GPU_INFO" ]; then
        GPU_INFO="\n🎮 No GPU data available"
    fi

    # Собираем все в одну строку с разделителями
    SYSTEM_INFO="$CPU_INFO\n$RAM_INFO\n$DISK_INFO$GPU_INFO"

    # Записываем в файл
    echo -e "$SYSTEM_INFO" > "$TEMP_FILE"
}

# Запускаем фоновый процесс для обновления данных
(
    while true; do
        update_system_info
        sleep 2  # Обновляем каждые 2 секунды
    done
) &

# Сохраняем PID фонового процесса
UPDATE_PID=$!

# Запускаем wofi для отображения данных
kitty -e bash -c "watch -n 2 cat $TEMP_FILE"

# Ожидаем завершения wofi и убиваем фоновый процесс
kill $UPDATE_PID

# Удаляем временный файл
rm "$TEMP_FILE"
