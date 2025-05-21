#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

// Цвета из твоего стиля Waybar
const SDL_Color COLOR_TEXT = {255, 255, 255, 255}; // Белый текст
const SDL_Color COLOR_BUTTON_BG = {0, 120, 215, 50}; // Полупрозрачный синий (rgba(0, 120, 215, 0.2))
const SDL_Color COLOR_BUTTON_HOVER = {0, 120, 215, 102}; // При наведении (rgba(0, 120, 215, 0.4))

// Класс кнопки
class Button {
public:
    SDL_Rect rect;
    std::string text;
    bool hovered;

    Button(int x, int y, int w, int h, const std::string& label) : text(label), hovered(false) {
        rect = {x, y, w, h};
    }

    void render(SDL_Renderer* renderer, TTF_Font* font) {
        // Фон кнопки
        if (hovered) {
            SDL_SetRenderDrawColor(renderer, COLOR_BUTTON_HOVER.r, COLOR_BUTTON_HOVER.g, COLOR_BUTTON_HOVER.b, COLOR_BUTTON_HOVER.a);
        } else {
            SDL_SetRenderDrawColor(renderer, COLOR_BUTTON_BG.r, COLOR_BUTTON_BG.g, COLOR_BUTTON_BG.b, COLOR_BUTTON_BG.a);
        }
        SDL_RenderFillRect(renderer, &rect);

        // Текст кнопки
        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), COLOR_TEXT);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        int texW = 0, texH = 0;
        SDL_QueryTexture(texture, nullptr, nullptr, &texW, &texH);
        SDL_Rect dstRect = {rect.x + (rect.w - texW) / 2, rect.y + (rect.h - texH) / 2, texW, texH};
        SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }

    bool isHovered(int mouseX, int mouseY) {
        return mouseX >= rect.x && mouseX <= rect.x + rect.w && mouseY >= rect.y && mouseY <= rect.y + rect.h;
    }
};

// Основной класс Проводника
class FileExplorer {
public:
    FileExplorer() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            return;
        }

        if (TTF_Init() < 0) {
            std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
            return;
        }

        window = SDL_CreateWindow("Custom File Explorer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
        if (!window) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            return;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
            return;
        }

        font = TTF_OpenFont("C:/Windows/Fonts/SegoeUI.ttf", 16); // Путь к шрифту Segoe UI (или укажи свой путь)
        if (!font) {
            std::cerr << "Font loading failed: " << TTF_GetError() << std::endl;
            return;
        }

        // Инициализация кнопок
        buttons.push_back(Button(10, 10, 40, 40, "←")); // Назад
        buttons.push_back(Button(60, 10, 40, 40, "→")); // Вперёд
        buttons.push_back(Button(110, 10, 40, 40, "↻")); // Обновить

        // Загрузка списка файлов (пример)
        loadFiles("/home/graf");
    }

    ~FileExplorer() {
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }

    void loadFiles(const std::string& path) {
        files.clear();
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                files.push_back(entry.path().filename().string());
            }
        } catch (const std::exception& e) {
            std::cerr << "Error reading directory: " << e.what() << std::endl;
        }
    }

    void run() {
        bool running = true;
        SDL_Event event;

        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                } else if (event.type == SDL_MOUSEMOTION) {
                    int mouseX = event.motion.x;
                    int mouseY = event.motion.y;
                    for (auto& button : buttons) {
                        button.hovered = button.isHovered(mouseX, mouseY);
                    }
                } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                    int mouseX = event.button.x;
                    int mouseY = event.button.y;
                    if (buttons[0].isHovered(mouseX, mouseY)) {
                        std::cout << "Back clicked\n";
                    } else if (buttons[1].isHovered(mouseX, mouseY)) {
                        std::cout << "Forward clicked\n";
                    } else if (buttons[2].isHovered(mouseX, mouseY)) {
                        loadFiles("/home/graf"); // Обновить
                    }
                }
            }

            // Отрисовка
            render();
            SDL_RenderPresent(renderer);
        }
    }

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    std::vector<Button> buttons;
    std::vector<std::string> files;

    void render() {
        // Градиентный фон
        for (int x = 0; x < 800; x++) {
            float t = static_cast<float>(x) / 800.0f;
            int r = 0 + t * (30 - 0);
            int g = 0 + t * (30 - 0);
            int b = 0 + t * (30 - 0);
            int a = 230; // 0.9 прозрачность
            SDL_SetRenderDrawColor(renderer, r, g, b, a);
            SDL_RenderDrawLine(renderer, x, 0, x, 600);
        }

        // Отрисовка кнопок
        for (auto& button : buttons) {
            button.render(renderer, font);
        }

        // Отрисовка списка файлов
        int y = 60;
        for (const auto& file : files) {
            SDL_Surface* surface = TTF_RenderText_Solid(font, file.c_str(), COLOR_TEXT);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            int texW = 0, texH = 0;
            SDL_QueryTexture(texture, nullptr, nullptr, &texW, &texH);
            SDL_Rect dstRect = {10, y, texW, texH};
            SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
            y += 20;
        }
    }
};

int main() {
    FileExplorer explorer;
    explorer.run();
    return 0;
}
