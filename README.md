# waybar
это мой waybar похожий на виндоус для его работу нужно скачать 

sudo pacman -S hyprland jq wofi # команда для скачки всего необходимого

и после скачивания вы должны поместить waybar в путь {home/имя_пользователя/.config/waybar}

еще вы должны прописать в .config/hypr.hyprland.conf

windowrulev2 = float, title:^(Volume)$
windowrulev2 = size 20 150, title:^(Volume)$
windowrulev2 = move 1700 940, title:^(Volume)$
windowrulev2 = nofocus, title:^(Volume)$, floating:1

