#include "TextRenderer.h"
#include <iostream>


TTF_Font* TextRenderer::font = nullptr;



bool TextRenderer::Init(const char* fontPath,
                        int fontSize)
{

    if(!TTF_Init())
    {
        std::cout<<"TTF Init Error"<<std::endl;
        return false;
    }



    font = TTF_OpenFont(fontPath,fontSize);



    if(font==nullptr)
    {
        std::cout<<"Font Load Error"<<std::endl;
        return false;
    }


    return true;
}



//----------------------------


SDL_Texture* TextRenderer::CreateText(SDL_Renderer* renderer,
                                      const char* text,
                                      SDL_Color color)
{

    SDL_Surface* surface =
        TTF_RenderText_Blended(font,
                               text,
                               0,
                               color);



    if(surface==nullptr)
        return nullptr;



    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(renderer,
                                     surface);



    SDL_DestroySurface(surface);



    return texture;

}



//----------------------------


void TextRenderer::Close()
{

    if(font)
    {
        TTF_CloseFont(font);
        font=nullptr;
    }


    TTF_Quit();

}
