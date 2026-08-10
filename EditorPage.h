#ifndef EDITORPAGE_H
#define EDITORPAGE_H

#include <SDL3/SDL.h>

#include "EditorMenu.h"
#include "SearchBox.h"


class EditorPage
{
private:

    EditorMenu menu;
    SearchBox search;


public:

    EditorPage(SDL_Window* window);

    void Draw(SDL_Renderer* renderer);

    EditorMenuAction HandleClick(int x, int y);

    void HandleKeyboard(SDL_Event event);
};

#endif
