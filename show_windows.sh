#!/bin/bash

# Получаем список окон через hyprctl
WINDOWS=$(hyprctl clients -j | jq -r '.[] | "\(.title) (\(.address))"')
echo "Окна: $WINDOWS" >&2

# Показываем окна через wofi и получаем выбранное окно
SELECTED=$(echo "$WINDOWS" | wofi --show dmenu --prompt "Active Windows" --allow-markup)
echo "Выбрано: $SELECTED" >&2

# Проверяем, было ли что-то выбрано
if [ -z "$SELECTED" ]; then
    echo "Ничего не выбрано, выход" >&2
    exit 0
fi

# Извлекаем address из выбранной строки (формат: "Title (address)")
ADDRESS=$(echo "$SELECTED" | grep -oP '\(0x[0-9a-f]+\)$' | tr -d '()')
echo "Address: $ADDRESS" >&2

# Проверяем, удалось ли извлечь address
if [ -z "$ADDRESS" ]; then
    echo "Не удалось извлечь address" >&2
    exit 1
fi

# Переключаемся на окно (фокус)
hyprctl dispatch focuswindow address:$ADDRESS
echo "Переключение на окно с address: $ADDRESS" >&2
