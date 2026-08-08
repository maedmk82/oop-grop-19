#ifndef BUTTON_H
#define BUTTON_H

#include <SDL3/SDL.h>

class Button
{
public:

    SDL_FRect rect;

    SDL_Texture* text;

    void Draw(SDL_Renderer* renderer);

    bool IsClicked(int x,int y);
};

#endif
