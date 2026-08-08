#ifndef OPENPROJECT_H
#define OPENPROJECT_H

#include <SDL3/SDL.h>
#include <vector>
#include "ProjectManager.h"


void DrawOpenProject(SDL_Renderer* renderer);

void HandleOpenProjectClick(int x,int y);

void LoadOpenProjects();


#endif
