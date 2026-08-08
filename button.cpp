#include "Button.h"

void Button::Draw(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer,200,200,200,255);

    SDL_RenderFillRect(renderer,&rect);

    if(text)
        SDL_RenderTexture(renderer,text,NULL,&rect);
}

bool Button::IsClicked(int x,int y)
{
    return x>=rect.x &&
           x<=rect.x+rect.w &&
           y>=rect.y &&
           y<=rect.y+rect.h;
}
