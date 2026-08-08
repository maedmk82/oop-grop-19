#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

class TextRenderer
{

public:

    static bool Init(const char* fontPath,int fontSize);

    static SDL_Texture* CreateText(
        SDL_Renderer* renderer,
        const char* text,
        SDL_Color color);


    static void Close();


private:

    static TTF_Font* font;

};

#endif
