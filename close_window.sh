#!/bin/bash

echo "Скрипт запущен из: $0" >&2
echo "Текущий PID: $$" >&2
echo "Начало выполнения: $(date)" >&2

touch /tmp/debug_close_temp.log 2>/dev/null
if [ $? -ne 0 ]; then
    echo "Ошибка: нет прав для создания временного файла" >&2
    exit 1
fi
echo "Временный файл создан" >&2

WINDOWS=$(hyprctl clients -j | jq -r '.[] | "\(.title) (\(.address))"')
echo "Окна: $WINDOWS" >&2

SELECTED=$(echo "$WINDOWS" | wofi --show dmenu --prompt "Close Window" --allow-markup)
echo "Выбрано: $SELECTED" >&2

if [ -z "$SELECTED" ]; then
    echo "Ничего не выбрано, выход" >&2
    exit 0
fi

ADDRESS=$(echo "$SELECTED" | grep -oP '\(0x[0-9a-f]+\)$' | tr -d '()')
echo "Address: $ADDRESS" >&2

if [ -z "$ADDRESS" ]; then
    echo "Не удалось извлечь address" >&2
    exit 1
fi

# Поиск PID с отладкой
PID=$(hyprctl clients -j | jq -r ".[] | select(.address == \"0x$ADDRESS\") | .pid" | head -n 1)
echo "Найденный PID: $PID" >&2

if [ -n "$PID" ] && kill -0 $PID 2>/dev/null; then
    kill -15 $PID 2>/dev/null
    echo "Попытка завершения PID: $PID" >&2
    if [ $? -eq 0 ]; then
        echo "Процесс с PID: $PID успешно убит" >&2
    else
        echo "Ошибка при убийстве процесса с PID: $PID" >&2
    fi
else
    echo "PID $PID не активен или не найден для address: $ADDRESS" >&2
    # Попытка закрытия через Hyprland
    hyprctl dispatch closewindow address:$ADDRESS
    echo "Попытка закрытия окна через Hyprland с address: $ADDRESS" >&2
fi

echo "Завершение: $(date)" >&2
