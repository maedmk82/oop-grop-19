#include "Menu.h"
#include <SDL3/SDL.h>
#include <iostream>
#include "openProject.h"
bool fileMenuOpen = false;

void DrawMenu(SDL_Renderer* renderer)
{
    // نوار بالا
    SDL_SetRenderDrawColor(renderer,200,200,200,255);

    SDL_FRect menuBar = {0,0,950,30};

    SDL_RenderFillRect(renderer,&menuBar);

    // دکمه File
    SDL_SetRenderDrawColor(renderer,170,170,170,255);

    SDL_FRect fileButton = {0,0,60,30};

    SDL_RenderFillRect(renderer,&fileButton);

    // متن File
    if(txtFile)
    {
        SDL_FRect dst = {10,5,40,18};

        SDL_RenderTexture(renderer,
                          txtFile,
                          NULL,
                          &dst);
    }

    // منوی باز شده
    if(fileMenuOpen)
    {
        SDL_FRect menu = {0,30,160,60};

        SDL_SetRenderDrawColor(renderer,245,245,245,255);

        SDL_RenderFillRect(renderer,&menu);

        SDL_SetRenderDrawColor(renderer,150,150,150,255);

        SDL_RenderLine(renderer,0,60,160,60);

        // New Project
        if(txtNew)
        {
            SDL_FRect dst1 = {10,35,130,18};

            SDL_RenderTexture(renderer,
                              txtNew,
                              NULL,
                              &dst1);
        }

        // Open Project
        if(txtOpen)
        {
            SDL_FRect dst2 = {10,65,130,18};

            SDL_RenderTexture(renderer,
                              txtOpen,
                              NULL,
                              &dst2);
        }
    }
}

void HandleMenuClick(int mx, int my)
{
    // File
    if(mx>=0 && mx<=60 &&
       my>=0 && my<=30)
    {
        std::cout << "Clicked" << std::endl;
        fileMenuOpen = !fileMenuOpen;
        return;
    }

    // New Project
    if(fileMenuOpen &&
       mx>=0 && mx<=160 &&
       my>=30 && my<=60)
    {
        std::cout << "Clicked" << std::endl;
        CurrentPage = NEW_PROJECT_PAGE;
        SDL_StartTextInput(window);
        fileMenuOpen = false;

        return;
    }

    // Open Project
    if(fileMenuOpen &&
       mx>=0 && mx<=160 &&
       my>=60 && my<=90)
    {
        std::cout << "Clicked" << std::endl;
        CurrentPage = OPEN_PROJECT_PAGE;

        LoadOpenProjects();

        fileMenuOpen=false;

        return;
    }
}
