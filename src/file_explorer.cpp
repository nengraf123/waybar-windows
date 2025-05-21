#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <dirent.h>
#include <string>
#include <vector>
#include <iostream>

struct FileItem {
    std::string name;
    bool isDir;
};

std::vector<FileItem> getDirectoryContents(const std::string& path) {
    std::vector<FileItem> items;
    DIR* dir = opendir(path.c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir))) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            FileItem item = {entry->d_name, entry->d_type == DT_DIR};
            items.push_back(item);
        }
        closedir(dir);
    }
    return items;
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    TTF_Font* font = TTF_OpenFont("src/DejaVuSans.ttf", 16);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("File Explorer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    std::string currentPath = ".";
    std::vector<FileItem> files = getDirectoryContents(currentPath);

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);
                // Простая логика клика (нужно доработать)
                int itemHeight = 20;
                int index = y / itemHeight;
                if (index < files.size()) {
                    if (files[index].isDir) {
                        currentPath += "/" + files[index].name;
                        files = getDirectoryContents(currentPath);
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255); // Светло-серый фон
        SDL_RenderClear(renderer);

        int y = 40; // Смещение для панели инструментов
        for (const auto& file : files) {
            SDL_Color color = file.isDir ? SDL_Color{0, 0, 255, 255} : SDL_Color{0, 0, 0, 255};
            SDL_Surface* surface = TTF_RenderText_Blended(font, file.name.c_str(), color);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect rect = {10, y, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, nullptr, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
            y += 20;
        }

        // Простая панель инструментов (можно улучшить)
        SDL_SetRenderDrawColor(renderer, 0, 120, 215, 255);
        SDL_Rect toolbar = {0, 0, 800, 40};
        SDL_RenderFillRect(renderer, &toolbar);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
