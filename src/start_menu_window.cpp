#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <algorithm>

struct MenuItem {
    std::string name;
    std::string iconPath;
    std::string command;
    bool isSeparator;
    bool isPinned; // Для плиток
};

std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "Error";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

std::vector<MenuItem> getMenuItems() {
    std::vector<MenuItem> items = {
        {"Profile", "/usr/share/icons/hicolor/48x48/apps/system-users.png", "notify-send 'Profile clicked'", false, false},
        {"Settings", "/usr/share/icons/hicolor/48x48/apps/preferences-system.png", "wofi --show run", false, false},
        {"Power", "/usr/share/icons/hicolor/48x48/apps/system-shutdown.png", "hyprctl dispatch exit", false, false},
        {"---", "", "", true, false}
    };

    // Добавляем установленные приложения
    std::vector<std::string> apps;
    for (const auto& entry : std::filesystem::directory_iterator("/usr/share/applications")) {
        if (entry.path().extension() == ".desktop") {
            apps.push_back(entry.path().stem().string());
        }
    }
    std::sort(apps.begin(), apps.end()); // Сортировка по алфавиту
    for (const auto& name : apps) {
        std::string icon = "/usr/share/icons/hicolor/48x48/apps/" + name + ".png";
        std::string command = "xdg-open \"/usr/share/applications/" + name + ".desktop\"";
        items.push_back({name, icon, command, false, false});
    }

    // Pinned apps (для плиток)
    items.push_back({"---", "", "", true, false});
    items.push_back({"Firefox", "/usr/share/icons/hicolor/48x48/apps/firefox.png", "firefox", false, true});
    items.push_back({"File Explorer", "img/111windows10.png", "/home/graf/.config/waybar/file_explorer", false, true});

    // Добавляем Task View (открытые окна)
    std::string windows = exec("hyprctl clients -j | jq -r '.[] | \"\\(.title) (\\(.address))\"'");
    std::istringstream iss(windows);
    std::string line;
    items.push_back({"---", "", "", true, false});
    while (std::getline(iss, line)) {
        std::string title = line.substr(0, line.find_last_of('(') - 1);
        std::string address = line.substr(line.find_last_of('(') + 1, line.find_last_of(')') - line.find_last_of('(') - 1);
        items.push_back({title, "", "hyprctl dispatch focuswindow address:" + address, false, false});
    }

    return items;
}

int main(int argc, char* argv[]) {
    // Инициализация
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0 || IMG_Init(IMG_INIT_PNG) < 0) {
        std::cerr << "Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Загрузка шрифта
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/TTF/SegoeUI.ttf", 16);
    if (!font) {
        font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", 16);
        if (!font) {
            std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
            TTF_Quit();
            SDL_Quit();
            return 1;
        }
    }

    // Создание окна
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    int windowWidth = 400; // Фиксированная ширина
    int windowHeight = 600; // Фиксированная высота, можно настроить по содержимому
    SDL_Window* window = SDL_CreateWindow("Start Menu", 0, displayMode.h - windowHeight, // Слева внизу
                                         windowWidth, windowHeight,
                                         SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Анимация появления
    SDL_SetWindowOpacity(window, 0.0f);
    for (float opacity = 0.0f; opacity <= 1.0f; opacity += 0.05f) {
        SDL_SetWindowOpacity(window, opacity);
        SDL_Delay(10);
    }

    // Загрузка меню
    std::vector<MenuItem> items = getMenuItems();
    int selectedIndex = -1; // Для подсветки при наведении
    bool running = true;
    SDL_Event event;

    while (running) {
        // Обработка событий
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (event.key.keysym.sym == SDLK_DOWN) {
                    selectedIndex = (selectedIndex + 1) % items.size();
                    while (items[selectedIndex].isSeparator) selectedIndex = (selectedIndex + 1) % items.size();
                } else if (event.key.keysym.sym == SDLK_UP) {
                    selectedIndex = (selectedIndex - 1 + items.size()) % items.size();
                    while (items[selectedIndex].isSeparator) selectedIndex = (selectedIndex - 1 + items.size()) % items.size();
                } else if (event.key.keysym.sym == SDLK_RETURN && selectedIndex >= 0 && !items[selectedIndex].isSeparator) {
                    system(items[selectedIndex].command.c_str());
                    running = false;
                }
            }
            if (event.type == SDL_MOUSEMOTION) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                selectedIndex = -1;
                int yOffset = 10;
                for (size_t i = 0; i < items.size(); ++i) {
                    if (!items[i].isPinned && !items[i].isSeparator && y >= yOffset && y < yOffset + 40) {
                        selectedIndex = i;
                        break;
                    }
                    if (!items[i].isPinned) yOffset += 40;
                }
                // Плитки
                if (y >= windowHeight - 120 && x >= windowWidth - 120) {
                    int tileX = (x - (windowWidth - 120)) / 60;
                    int tileY = (y - (windowHeight - 120)) / 60;
                    int tileIndex = tileX + tileY * 2;
                    for (size_t i = 0; i < items.size(); ++i) {
                        if (items[i].isPinned && tileIndex-- == 0) {
                            selectedIndex = i;
                            break;
                        }
                    }
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                int yOffset = 10;
                for (size_t i = 0; i < items.size(); ++i) {
                    if (!items[i].isPinned && !items[i].isSeparator && y >= yOffset && y < yOffset + 40) {
                        system(items[i].command.c_str());
                        running = false;
                        break;
                    }
                    if (!items[i].isPinned) yOffset += 40;
                }
                // Плитки
                if (y >= windowHeight - 120 && x >= windowWidth - 120) {
                    int tileX = (x - (windowWidth - 120)) / 60;
                    int tileY = (y - (windowHeight - 120)) / 60;
                    int tileIndex = tileX + tileY * 2;
                    for (size_t i = 0; i < items.size(); ++i) {
                        if (items[i].isPinned && tileIndex-- == 0) {
                            system(items[i].command.c_str());
                            running = false;
                            break;
                        }
                    }
                }
            }
        }

        // Рендеринг
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 30, 30, 50, 230); // Полупрозрачный тёмно-синий фон
        SDL_Rect bgRect = {0, 0, windowWidth, windowHeight};
        SDL_RenderFillRect(renderer, &bgRect);

        // Рамка
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
        SDL_Rect borderRect = {0, 0, windowWidth - 1, windowHeight - 1};
        SDL_RenderDrawRect(renderer, &borderRect);

        // Рендеринг пунктов меню
        int yOffset = 10;
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].isSeparator) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
                SDL_Rect separator = {10, yOffset + 15, windowWidth - 20, 1};
                SDL_RenderFillRect(renderer, &separator);
                yOffset += 40;
                continue;
            }
            if (!items[i].isPinned) {
                // Подсветка при наведении
                if (static_cast<int>(i) == selectedIndex) {
                    SDL_SetRenderDrawColor(renderer, 0, 120, 215, 100);
                    SDL_Rect highlight = {5, yOffset, windowWidth - 10, 40};
                    SDL_RenderFillRect(renderer, &highlight);
                }

                // Иконка
                SDL_Surface* iconSurface = IMG_Load(items[i].iconPath.c_str());
                if (iconSurface) {
                    SDL_Texture* iconTexture = SDL_CreateTextureFromSurface(renderer, iconSurface);
                    SDL_Rect iconRect = {10, yOffset + 8, 24, 24};
                    SDL_RenderCopy(renderer, iconTexture, nullptr, &iconRect);
                    SDL_DestroyTexture(iconTexture);
                    SDL_FreeSurface(iconSurface);
                }

                // Текст
                SDL_Color textColor = {255, 255, 255, 255};
                SDL_Surface* textSurface = TTF_RenderText_Blended(font, items[i].name.c_str(), textColor);
                SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                SDL_Rect textRect = {40, yOffset + 12, textSurface->w, textSurface->h};
                SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
                SDL_FreeSurface(textSurface);
                SDL_DestroyTexture(textTexture);

                yOffset += 40;
            }
        }

        // Рендеринг плиток (Pinned Apps)
        int tileY = windowHeight - 120;
        int tileX = windowWidth - 120;
        int tileIndex = 0;
        for (const auto& item : items) {
            if (item.isPinned) {
                int x = tileX + (tileIndex % 2) * 60;
                int y = tileY + (tileIndex / 2) * 60;

                // Подсветка при наведении
                if (static_cast<int>(&item - &items[0]) == selectedIndex) {
                    SDL_SetRenderDrawColor(renderer, 0, 120, 215, 100);
                    SDL_Rect highlight = {x, y, 56, 56};
                    SDL_RenderFillRect(renderer, &highlight);
                }

                // Иконка
                SDL_Surface* iconSurface = IMG_Load(item.iconPath.c_str());
                if (iconSurface) {
                    SDL_Texture* iconTexture = SDL_CreateTextureFromSurface(renderer, iconSurface);
                    SDL_Rect iconRect = {x + 4, y + 4, 48, 48};
                    SDL_RenderCopy(renderer, iconTexture, nullptr, &iconRect);
                    SDL_DestroyTexture(iconTexture);
                    SDL_FreeSurface(iconSurface);
                }

                tileIndex++;
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Анимация исчезания
    for (float opacity = 1.0f; opacity >= 0.0f; opacity -= 0.05f) {
        SDL_SetWindowOpacity(window, opacity);
        SDL_Delay(10);
    }

    // Очистка
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}
