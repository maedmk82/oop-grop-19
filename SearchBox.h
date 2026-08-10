#ifndef SEARCHBOX_H
#define SEARCHBOX_H

#include <SDL3/SDL.h>
#include <string>

class SearchBox
{
private:
    std::string text;
    bool active;
    SDL_Window* window;

public:
    SearchBox(SDL_Window* window);

    void Draw(SDL_Renderer* renderer);
    void HandleClick(int x, int y);
    void HandleKeyboard(SDL_Event event);

    // اضافه شدن کلمه const برای امنیت و یکپارچگی کد
    std::string GetText() const;
};

#endif
