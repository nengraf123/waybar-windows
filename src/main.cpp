#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdio.h> // Для popen

#ifdef _WIN32
    #define popen _popen
    #define pclose _pclose
#endif

// Функция для выполнения shell-команд и получения вывода
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

// Структура для хранения данных системы
struct SystemData {
    std::string cpu, ram, gpu;
    std::vector<std::string> disks; // Список дисков
};

SystemData getSystemData() {
    SystemData data;

    // CPU usage
    data.cpu = exec("top -bn1 | grep \"Cpu(s)\" | awk '{print $2}' | cut -d. -f1");

    // RAM usage
    data.ram = exec("free -h | grep \"Mem:\" | awk '{print $3\"/\"$2}'");

    // GPU usage
    std::string gpuIntel = exec("intel_gpu_top -s 1 -o - | grep \"GPU\" | awk '{print $2}' | cut -d% -f1");
    std::string gpuAmd = exec("radeontop -l 1 | grep -A 1 \"GPU\" | tail -n 1 | awk '{print $2}' | cut -d% -f1");
    std::string gpuNvidia = exec("nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader | awk '{print $1}' | cut -d% -f1");
    data.gpu = "";
    if (!gpuIntel.empty()) data.gpu += "GPU Intel: " + gpuIntel + "%\n";
    if (!gpuAmd.empty()) data.gpu += "GPU AMD: " + gpuAmd + "%\n";
    if (!gpuNvidia.empty()) data.gpu += "GPU NVIDIA: " + gpuNvidia + "%\n";
    if (data.gpu.empty()) data.gpu = "No GPU data\n";

    // Данные о дисках
    std::string dfOutput = exec("df -h");
    std::istringstream dfStream(dfOutput);
    std::string line;
    std::getline(dfStream, line); // Пропускаем заголовок
    while (std::getline(dfStream, line)) {
        std::istringstream lineStream(line);
        std::string filesystem, size, used, avail, usePercent, mountPoint;
        if (lineStream >> filesystem >> size >> used >> avail >> usePercent >> mountPoint) {
            // Пропускаем корень и системные точки, если нужно (например, /boot или /run)
            if (mountPoint != "/" && mountPoint != "none" && !mountPoint.empty()) {
                data.disks.push_back(mountPoint + ": " + used + "/" + size);
            }
        }
    }

    return data;
}

int main(int argc, char* argv[]) {
    // Инициализация SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Инициализация SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Загружаем шрифт (попробуем Roboto, если нет, то Noto Sans или DejaVuSans)
    TTF_Font* font = nullptr;
    font = TTF_OpenFont("/usr/share/fonts/TTF/Roboto-Regular.ttf", 16);
    if (!font) {
        font = TTF_OpenFont("/usr/share/fonts/noto/NotoSans-Regular.ttf", 16);
        if (!font) {
            font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", 16);
            if (!font) {
                std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
                TTF_Quit();
                SDL_Quit();
                return 1;
            }
        }
    }

    // Получаем размеры экрана
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    int screenWidth = displayMode.w;
    int screenHeight = displayMode.h;

    // Создаем окно в центре экрана
    int windowWidth = 300;
    int windowHeight = 200;
    int windowX = (screenWidth - windowWidth) / 2;
    int windowY = (screenHeight - windowHeight) / 2;
    SDL_Window* window = SDL_CreateWindow(
        "System Info",
        windowX, windowY,
        windowWidth, windowHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Перемещаем мышь в центр окна
    SDL_WarpMouseInWindow(window, windowWidth / 2, windowHeight / 2);

    bool running = true;
    SDL_Event event;
    SystemData data = getSystemData();

    SDL_Surface* textSurface = nullptr;
    SDL_Texture* textTexture = nullptr;
    SDL_Surface* shadowSurface = nullptr;
    SDL_Texture* shadowTexture = nullptr;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            // Закрываем окно при потере фокуса
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                running = false;
            }
            // Проверяем позицию мыши
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            if (mouseX < 0 || mouseX >= windowWidth || mouseY < 0 || mouseY >= windowHeight) {
                running = false; // Закрываем, если мышь покидает окно
            }
        }

        // Очищаем экран
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        // Рисуем полупрозрачный фон
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 30, 30, 50, 230); // Темно-синий с прозрачностью
        SDL_Rect bgRect = {5, 5, windowWidth - 10, windowHeight - 10};
        SDL_RenderFillRect(renderer, &bgRect);

        // Рисуем рамку
        SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255); // Светло-синяя рамка
        SDL_Rect borderRect = {5, 5, windowWidth - 10, windowHeight - 10};
        SDL_RenderDrawRect(renderer, &borderRect);

        // Рендерим текст и тень
        std::stringstream ss;
        ss << "CPU: " << data.cpu << "\n";
        if (!data.gpu.empty()) {
            ss << data.gpu;
        }
        ss << "RAM: " << data.ram << "\n";
        for (const auto& disk : data.disks) {
            ss << disk << "\n";
        }
        std::string text = ss.str();
        SDL_Color textColor = {255, 255, 255, 255};
        SDL_Color shadowColor = {50, 50, 50, 255};

        if (textSurface) SDL_FreeSurface(textSurface);
        if (textTexture) SDL_DestroyTexture(textTexture);
        if (shadowSurface) SDL_FreeSurface(shadowSurface);
        if (shadowTexture) SDL_DestroyTexture(shadowTexture);

        // Тень
        shadowSurface = TTF_RenderText_Blended_Wrapped(font, text.c_str(), shadowColor, windowWidth);
        shadowTexture = SDL_CreateTextureFromSurface(renderer, shadowSurface);
        SDL_Rect shadowRect = {12, 12, shadowSurface->w, shadowSurface->h};
        SDL_RenderCopy(renderer, shadowTexture, nullptr, &shadowRect);

        // Основной текст
        textSurface = TTF_RenderText_Blended_Wrapped(font, text.c_str(), textColor, windowWidth);
        textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_Rect textRect = {10, 10, textSurface->w, textSurface->h};
        SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);

        // Обновляем данные каждые 2 секунды
        static Uint32 lastUpdate = 0;
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastUpdate >= 2000) {
            data = getSystemData();
            lastUpdate = currentTime;
        }

        // Рендерим
        SDL_RenderPresent(renderer);

        // Задержка для экономии ресурсов
        SDL_Delay(16); // ~60 FPS
    }

    // Очистка
    if (textTexture) SDL_DestroyTexture(textTexture);
    if (textSurface) SDL_FreeSurface(textSurface);
    if (shadowTexture) SDL_DestroyTexture(shadowTexture);
    if (shadowSurface) SDL_FreeSurface(shadowSurface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
