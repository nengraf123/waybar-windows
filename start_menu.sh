#!/bin/bash

# Запускаем SDL2-программу
/home/graf/.config/waybar/start_menu_window &

# Даём небольшую задержку, чтобы окно успело создаться
sleep 0.1

# Получаем PID процесса
PID=$(pidof start_menu_window)
if [ -z "$PID" ]; then
    echo "Ошибка: не удалось найти PID start_menu_window" >&2
    exit 1
fi

# Преобразуем PID в address для Hyprland
ADDRESS=$(printf '%x' "$PID")

# Перемещаем окно в специальный слой
hyprctl dispatch movetoworkspace special:startmenu,address:0x$ADDRESS

# Фокусируем окно
hyprctl dispatch focuswindow address:0x$ADDRESS

# Отладка
echo "Запущено start_menu_window с PID: $PID, address: 0x$ADDRESS" >&2
