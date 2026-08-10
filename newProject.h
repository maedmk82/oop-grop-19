#ifndef NEWPROJECT_H
#define NEWPROJECT_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

void DrawNewProject(SDL_Renderer* renderer);

void HandleNewProjectClick(int x, int y);

void HandleKeyboard(SDL_Event event);

extern TTF_Font* font;

#endif
