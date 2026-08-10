#ifndef SAVEAS_H
#define SAVEAS_H

#include <SDL3/SDL.h>

void DrawSaveAsPage(SDL_Renderer* renderer);

void HandleSaveAsClick(int x, int y);

void HandleSaveAsKeyboard(SDL_Event& event);

#endif
