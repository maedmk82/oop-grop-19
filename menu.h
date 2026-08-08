#ifndef MENU_H
#define MENU_H

#include <SDL3/SDL.h>
#include "Page.h"

extern bool fileMenuOpen;

extern SDL_Texture* txtFile;
extern SDL_Texture* txtNew;
extern SDL_Texture* txtOpen;

extern Page CurrentPage;

extern SDL_Window* window;


void DrawMenu(SDL_Renderer* renderer);

void HandleMenuClick(int mx,int my);


#endif
