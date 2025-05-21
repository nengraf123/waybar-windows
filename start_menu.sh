#!/bin/bash

# Пути к иконкам (используем Papirus или локальные изображения)
ICON_PROFILE="/usr/share/icons/Papirus/48x48/apps/system-users.png"
ICON_SETTINGS="/usr/share/icons/Papirus/48x48/apps/preferences-system.png"
ICON_POWER="/usr/share/icons/Papirus/48x48/apps/system-shutdown.png"
ICON_FIREFOX="/usr/share/icons/Papirus/48x48/apps/firefox.png"
ICON_FOLDER="/home/graf/.config/waybar/img/111windows10.png"
ICON_ANDROID="/usr/share/icons/Papirus/48x48/apps/android-studio.png"
ICON_ASSISTANT="/usr/share/icons/Papirus/48x48/apps/assistant.png"
ICON_AVAHI="/usr/share/icons/Papirus/48x48/apps/avahi-discover.png"
ICON_BLUETOOTH="/usr/share/icons/Papirus/48x48/apps/bluetooth.png"

# Читаем пользовательские приложения из custom_apps.txt
CUSTOM_APPS=$(cat ~/.config/waybar/custom_apps.txt 2>/dev/null | while read -r app; do
    [ -n "$app" ] && echo "$app\0icon\x1f/usr/share/icons/Papirus/48x48/apps/$app.png"
done)

# Формируем секции с префиксом header: для заголовков
LEFT_PANEL="header:System Actions\0icon\x1f\nProfile\0icon\x1f$ICON_PROFILE\0Settings\0icon\x1f$ICON_SETTINGS\0Power\0icon\x1f$ICON_POWER"
CENTER_PANEL="header:Applications\0icon\x1f\n$(ls /usr/share/applications | grep -E '\.desktop$' | sed 's/\.desktop//g' | head -n 5 | awk '{print $0 "\0icon\x1f/usr/share/icons/Papirus/48x48/apps/" $0 ".png"}')\n$CUSTOM_APPS"
RIGHT_PANEL="header:Pinned Apps\0icon\x1f\nFirefox\0icon\x1f$ICON_FIREFOX\nFile Explorer\0icon\x1f$ICON_FOLDER\nAndroid Studio\0icon\x1f$ICON_ANDROID\nAssistant\0icon\x1f$ICON_ASSISTANT\nAvahi\0icon\x1f$ICON_AVAHI\nBluetooth\0icon\x1f$ICON_BLUETOOTH"

# Объединяем секции
MENU_ITEMS="$LEFT_PANEL\0---\0$CENTER_PANEL\0---\0$RIGHT_PANEL"

# Запускаем rofi с поддержкой колонок и клавиш переключения
echo -en "$MENU_ITEMS" | rofi -dmenu -p "Start" -theme ~/.config/rofi/start_menu.rasi -columns 3 -show-icons -icon-theme "Papirus" -kb-custom-1 "Alt+1" -kb-custom-2 "Alt+2" -kb-custom-3 "Alt+3" | {
    read -r selection
    # Удаляем префикс header: из выбора, если он есть
    selection_cleaned=$(echo "$selection" | sed 's/^header://')
    case "$selection_cleaned" in
        "Profile")
            notify-send "Profile clicked"
            ;;
        "Settings")
            wofi --show run &
            ;;
        "Power")
            hyprctl dispatch exit
            ;;
        "Firefox")
            firefox &
            ;;
        "File Explorer")
            /home/graf/.config/waybar/file_explorer &
            ;;
        "Android Studio")
            android-studio &
            ;;
        "Assistant")
            assistant &
            ;;
        "Avahi")
            avahi-discover &
            ;;
        "Bluetooth")
            bluetoothctl &
            ;;
        "System Actions"|"Applications"|"Pinned Apps"|"---")
            # Игнорируем заголовки и разделители
            ;;
        *)
            if [[ -n "$selection_cleaned" ]]; then
                xdg-open "/usr/share/applications/$selection_cleaned.desktop" &
            fi
            ;;
    esac
}

# Обработка клавиш для переключения секций
case "$ROFI_RETV" in
    10) # Alt+1 - переключение на System Actions
        echo -en "$LEFT_PANEL" | rofi -dmenu -p "Start" -theme ~/.config/rofi/start_menu.rasi -show-icons -icon-theme "Papirus"
        ;;
    11) # Alt+2 - переключение на Applications
        echo -en "$CENTER_PANEL" | rofi -dmenu -p "Start" -theme ~/.config/rofi/start_menu.rasi -show-icons -icon-theme "Papirus"
        ;;
    12) # Alt+3 - переключение на Pinned Apps
        echo -en "$RIGHT_PANEL" | rofi -dmenu -p "Start" -theme ~/.config/rofi/start_menu.rasi -show-icons -icon-theme "Papirus"
        ;;
esac
