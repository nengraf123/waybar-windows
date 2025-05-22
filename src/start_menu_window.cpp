#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <algorithm>

// Структура для хранения информации о пункте меню
struct MenuItem {
    std::string name;        // Название пункта (например, "Profile")
    std::string iconPath;    // Путь к иконке
    std::string command;     // Команда, которая выполняется при клике
    bool isSeparator;        // Это разделительная линия?
    bool isPinned;           // Это закреплённое приложение (плитка)?
};

// Функция для выполнения команд в терминале и получения результата
std::string runCommand(const char* cmd) {
    char buffer[128];        // Место для хранения текста из команды
    std::string result = ""; // Пустая строка для результата
    FILE* pipe = popen(cmd, "r"); // Открываем команду
    if (!pipe) return "Error";    // Если ошибка, возвращаем "Error"
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer; // Добавляем текст в результат
    }
    pclose(pipe); // Закрываем команду
    return result; // Возвращаем результат
}

// Функция для получения списка всех пунктов меню
std::vector<MenuItem> getMenuList() {
    std::vector<MenuItem> menuItems = {
        {"Profile", "/usr/share/icons/hicolor/48x48/apps/system-users.png", "notify-send 'Profile clicked'", false, false},
        {"Settings", "/usr/share/icons/hicolor/48x48/apps/preferences-system.png", "wofi --show run", false, false},
        {"Power", "/usr/share/icons/hicolor/48x48/apps/system-shutdown.png", "hyprctl dispatch exit", false, false},
        {"---", "", "", true, false} // Разделительная линия
    };

    // Добавляем все приложения с .desktop файлами
    std::vector<std::string> appNames;
    for (const auto& file : std::filesystem::directory_iterator("/usr/share/applications")) {
        if (file.path().extension() == ".desktop") { // Ищем только .desktop файлы
            appNames.push_back(file.path().stem().string()); // Добавляем имя без .desktop
        }
    }
    std::sort(appNames.begin(), appNames.end()); // Сортируем по алфавиту
    for (const std::string& name : appNames) {
        std::string icon = "/usr/share/icons/hicolor/48x48/apps/" + name + ".png";
        std::string cmd = "xdg-open \"/usr/share/applications/" + name + ".desktop\"";
        menuItems.push_back({name, icon, cmd, false, false});
    }

    // Добавляем закреплённые приложения (плитки)
    menuItems.push_back({"---", "", "", true, false}); // Разделительная линия
    menuItems.push_back({"Firefox", "/usr/share/icons/hicolor/48x48/apps/firefox.png", "firefox", false, true});
    menuItems.push_back({"File Explorer", "img/111windows10.png", "/home/graf/.config/waybar/file_explorer", false, true});

    // Добавляем открытые окна (Task View)
    std::string windowList = runCommand("hyprctl clients -j | jq -r '.[] | \"\\(.title) (\\(.address))\"'");
    std::istringstream stream(windowList);
    std::string line;
    menuItems.push_back({"---", "", "", true, false}); // Разделительная линия
    while (std::getline(stream, line)) {
        std::string title = line.substr(0, line.find_last_of('(') - 1); // Берём название окна
        std::string address = line.substr(line.find_last_of('(') + 1, line.find_last_of(')') - line.find_last_of('(') - 1); // Берём адрес
        menuItems.push_back({title, "", "hyprctl dispatch focuswindow address:" + address, false, false});
    }

    return menuItems; // Возвращаем список пунктов
}

// Функция для отрисовки иконки
void drawIcon(SDL_Renderer* renderer, const std::string& iconPath, int x, int y) {
    SDL_Surface* iconSurface = IMG_Load(iconPath.c_str()); // Загружаем иконку
    if (iconSurface) {
        SDL_Texture* iconTexture = SDL_CreateTextureFromSurface(renderer, iconSurface); // Преобразуем в текстуру
        SDL_Rect rect = {x, y, 24, 24}; // Устанавливаем размер иконки
        SDL_RenderCopy(renderer, iconTexture, nullptr, &rect); // Рисуем иконку
        SDL_DestroyTexture(iconTexture); // Очищаем текстуру
        SDL_FreeSurface(iconSurface);    // Очищаем поверхность
    }
}

// Функция для отрисовки текста
void drawText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y) {
    SDL_Color color = {255, 255, 255, 255}; // Белый цвет текста
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), color); // Создаём текст
    if (textSurface) {
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface); // Преобразуем в текстуру
        SDL_Rect rect = {x, y, textSurface->w, textSurface->h}; // Устанавливаем размер текста
        SDL_RenderCopy(renderer, textTexture, nullptr, &rect); // Рисуем текст
        SDL_FreeSurface(textSurface); // Очищаем поверхность
        SDL_DestroyTexture(textTexture); // Очищаем текстуру
    }
}

int main(int argc, char* argv[]) {
    // Проверяем, запустились ли библиотеки SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0 || IMG_Init(IMG_INIT_PNG) < 0) {
        std::cout << "Ошибка запуска библиотек: " << SDL_GetError() << std::endl;
        return 1; // Выходим с ошибкой
    }

    // Пробуем загрузить шрифт
    TTF_Font* myFont = TTF_OpenFont("/usr/share/fonts/TTF/SegoeUI.ttf", 16);
    if (!myFont) {
        myFont = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", 16); // Пробуем другой шрифт
        if (!myFont) {
            std::cout << "Ошибка загрузки шрифта: " << TTF_GetError() << std::endl;
            TTF_Quit(); // Закрываем TTF
            SDL_Quit(); // Закрываем SDL
            return 1;   // Выходим с ошибкой
        }
    }

    // Создаём окно
    SDL_DisplayMode screenInfo;
    SDL_GetCurrentDisplayMode(0, &screenInfo); // Получаем размер экрана
    int windowWidth = 400;    // Ширина окна
    int windowHeight = 600;   // Высота окна
    SDL_Window* myWindow = SDL_CreateWindow("Start Menu", 0, screenInfo.h - windowHeight, // Слева внизу
                                           windowWidth, windowHeight,
                                           SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);
    SDL_Renderer* myRenderer = SDL_CreateRenderer(myWindow, -1, SDL_RENDERER_ACCELERATED);

    // Анимация появления окна
    SDL_SetWindowOpacity(myWindow, 0.0f); // Начинаем с прозрачности
    for (float opacity = 0.0f; opacity <= 1.0f; opacity += 0.05f) {
        SDL_SetWindowOpacity(myWindow, opacity); // Увеличиваем прозрачность
        SDL_Delay(10); // Ждём 10 миллисекунд
    }

    // Получаем список меню
    std::vector<MenuItem> menuItems = getMenuList();
    int selectedItem = -1; // Номер выбранного пункта
    bool isRunning = true; // Программа работает?
    SDL_Event event;       // Событие от мыши или клавиатуры

    while (isRunning) {
        // Проверяем события
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)) {
                isRunning = false; // Закрываем, если потеряли фокус
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    isRunning = false; // Закрываем по Escape
                } else if (event.key.keysym.sym == SDLK_DOWN) {
                    selectedItem = (selectedItem + 1) % menuItems.size(); // Вниз
                    while (menuItems[selectedItem].isSeparator) selectedItem = (selectedItem + 1) % menuItems.size();
                } else if (event.key.keysym.sym == SDLK_UP) {
                    selectedItem = (selectedItem - 1 + menuItems.size()) % menuItems.size(); // Вверх
                    while (menuItems[selectedItem].isSeparator) selectedItem = (selectedItem - 1 + menuItems.size()) % menuItems.size();
                } else if (event.key.keysym.sym == SDLK_RETURN && selectedItem >= 0 && !menuItems[selectedItem].isSeparator) {
                    system(menuItems[selectedItem].command.c_str()); // Выполняем команду
                    isRunning = false; // Закрываем после выполнения
                }
            }
            if (event.type == SDL_MOUSEMOTION) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY); // Получаем позицию мыши
                selectedItem = -1;
                int yPos = 10; // Начальная позиция по Y
                for (size_t i = 0; i < menuItems.size(); ++i) {
                    if (!menuItems[i].isPinned && !menuItems[i].isSeparator && mouseY >= yPos && mouseY < yPos + 40) {
                        selectedItem = i; // Выбираем пункт под курсором
                        break;
                    }
                    if (!menuItems[i].isPinned) yPos += 40;
                }
                // Проверяем плитки
                if (mouseY >= windowHeight - 120 && mouseX >= windowWidth - 120) {
                    int tileX = (mouseX - (windowWidth - 120)) / 60;
                    int tileY = (mouseY - (windowHeight - 120)) / 60;
                    int tileNum = tileX + tileY * 2;
                    for (size_t i = 0; i < menuItems.size(); ++i) {
                        if (menuItems[i].isPinned && tileNum-- == 0) {
                            selectedItem = i;
                            break;
                        }
                    }
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY); // Получаем позицию клика
                int yPos = 10;
                for (size_t i = 0; i < menuItems.size(); ++i) {
                    if (!menuItems[i].isPinned && !menuItems[i].isSeparator && mouseY >= yPos && mouseY < yPos + 40) {
                        system(menuItems[i].command.c_str()); // Выполняем команду
                        isRunning = false; // Закрываем после клика
                        break;
                    }
                    if (!menuItems[i].isPinned) yPos += 40;
                }
                // Проверяем клик по плиткам
                if (mouseY >= windowHeight - 120 && mouseX >= windowWidth - 120) {
                    int tileX = (mouseX - (windowWidth - 120)) / 60;
                    int tileY = (mouseY - (windowHeight - 120)) / 60;
                    int tileNum = tileX + tileY * 2;
                    for (size_t i = 0; i < menuItems.size(); ++i) {
                        if (menuItems[i].isPinned && tileNum-- == 0) {
                            system(menuItems[i].command.c_str()); // Выполняем команду
                            isRunning = false; // Закрываем после клика
                            break;
                        }
                    }
                }
            }
        }

        // Рисуем фон
        SDL_SetRenderDrawBlendMode(myRenderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(myRenderer, 30, 30, 50, 230); // Тёмно-синий с прозрачностью
        SDL_Rect bg = {0, 0, windowWidth, windowHeight};
        SDL_RenderFillRect(myRenderer, &bg);

        // Рисуем рамку
        SDL_SetRenderDrawColor(myRenderer, 255, 255, 255, 100);
        SDL_Rect border = {0, 0, windowWidth - 1, windowHeight - 1};
        SDL_RenderDrawRect(myRenderer, &border);

        // Рисуем пункты меню
        int yPos = 10;
        for (size_t i = 0; i < menuItems.size(); ++i) {
            if (menuItems[i].isSeparator) {
                SDL_SetRenderDrawColor(myRenderer, 255, 255, 255, 100);
                SDL_Rect line = {10, yPos + 15, windowWidth - 20, 1};
                SDL_RenderFillRect(myRenderer, &line);
                yPos += 40;
                continue;
            }
            if (!menuItems[i].isPinned) {
                // Подсвечиваем при наведении
                if (static_cast<int>(i) == selectedItem) {
                    SDL_SetRenderDrawColor(myRenderer, 0, 120, 215, 100);
                    SDL_Rect highlight = {5, yPos, windowWidth - 10, 40};
                    SDL_RenderFillRect(myRenderer, &highlight);
                }

                // Рисуем иконку
                drawIcon(myRenderer, menuItems[i].iconPath, 10, yPos + 8);

                // Рисуем текст
                drawText(myRenderer, myFont, menuItems[i].name, 40, yPos + 12);

                yPos += 40; // Переходим к следующему пункту
            }
        }

        // Рисуем плитки
        int tileY = windowHeight - 120;
        int tileX = windowWidth - 120;
        int tileNum = 0;
        for (const auto& item : menuItems) {
            if (item.isPinned) {
                int x = tileX + (tileNum % 2) * 60;
                int y = tileY + (tileNum / 2) * 60;

                // Подсвечиваем при наведении
                if (static_cast<int>(&item - &menuItems[0]) == selectedItem) {
                    SDL_SetRenderDrawColor(myRenderer, 0, 120, 215, 100);
                    SDL_Rect highlight = {x, y, 56, 56};
                    SDL_RenderFillRect(myRenderer, &highlight);
                }

                // Рисуем иконку плитки
                drawIcon(myRenderer, item.iconPath, x + 4, y + 4);

                tileNum++; // Переходим к следующей плитке
            }
        }

        SDL_RenderPresent(myRenderer); // Показываем всё на экране
        SDL_Delay(16); // Ждём 16 миллисекунд для плавности
    }

    // Анимация исчезания
    for (float opacity = 1.0f; opacity >= 0.0f; opacity -= 0.05f) {
        SDL_SetWindowOpacity(myWindow, opacity); // Уменьшаем прозрачность
        SDL_Delay(10); // Ждём 10 миллисекунд
    }

    // Очищаем всё
    SDL_DestroyRenderer(myRenderer);
    SDL_DestroyWindow(myWindow);
    TTF_CloseFont(myFont);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    return 0; // Всё завершено успешно
}
