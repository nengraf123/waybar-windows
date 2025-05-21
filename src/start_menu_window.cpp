#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <filesystem>

struct MenuItem {
    std::string name;
    std::string iconPath;
    std::string command;
    bool isSeparator;
};

std::vector<MenuItem> getMenuItems() {
    std::vector<MenuItem> items = {
        {"Profile", "/usr/share/icons/hicolor/48x48/apps/system-users.png", "notify-send 'Profile clicked'", false},
        {"Settings", "/usr/share/icons/hicolor/48x48/apps/preferences-system.png", "wofi --show run", false},
        {"Power", "/usr/share/icons/hicolor/48x48/apps/system-shutdown.png", "hyprctl dispatch exit", false},
        {"---", "", "", true}
    };

    // Добавляем установленные приложения
    for (const auto& entry : std::filesystem::directory_iterator("/usr/share/applications")) {
        if (entry.path().extension() == ".desktop") {
            std::string name = entry.path().stem().string();
            std::string icon = "/usr/share/icons/hicolor/48x48/apps/" + name + ".png";
            std::string command = "xdg-open \"" + entry.path().string() + "\"";
            items.push_back({name, icon, command, false});
        }
    }

    // Pinned apps
    items.push_back({"---", "", "", true});
    items.push_back({"Firefox", "/usr/share/icons/hicolor/48x48/apps/firefox.png", "firefox", false});
    items.push_back({"File Explorer", "img/111windows10.png", "/home/graf/.config/waybar/file_explorer", false});

    return items;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0 || IMG_Init(IMG_INIT_PNG) < 0) {
        std::cerr << "Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

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

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    int windowWidth = 300;
    int windowHeight = displayMode.h;
    SDL_Window* window = SDL_CreateWindow("Start Menu", 0, 0, windowWidth, windowHeight,
                                         SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    std::vector<MenuItem> items = getMenuItems();
    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                int itemHeight = 40;
                int index = y / itemHeight;
                if (index >= 0 && index < items.size() && !items[index].isSeparator) {
                    system(items[index].command.c_str());
                    running = false; // Закрываем после запуска
                }
            }
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 30, 30, 50, 230); // Полупрозрачный фон
        SDL_Rect bgRect = {0, 0, windowWidth, windowHeight};
        SDL_RenderFillRect(renderer, &bgRect);

        int y = 10;
        for (const auto& item : items) {
            if (item.isSeparator) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);
                SDL_Rect separator = {10, y + 15, windowWidth - 20, 1};
                SDL_RenderFillRect(renderer, &separator);
                y += 40;
                continue;
            }

            // Иконка
            SDL_Surface* iconSurface = IMG_Load(item.iconPath.c_str());
            if (iconSurface) {
                SDL_Texture* iconTexture = SDL_CreateTextureFromSurface(renderer, iconSurface);
                SDL_Rect iconRect = {10, y, 24, 24};
                SDL_RenderCopy(renderer, iconTexture, nullptr, &iconRect);
                SDL_DestroyTexture(iconTexture);
                SDL_FreeSurface(iconSurface);
            }

            // Текст
            SDL_Color textColor = {255, 255, 255, 255};
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, item.name.c_str(), textColor);
            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
            SDL_Rect textRect = {40, y + 4, textSurface->w, textSurface->h};
            SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
            SDL_FreeSurface(textSurface);
            SDL_DestroyTexture(textTexture);

            y += 40;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0;
}
