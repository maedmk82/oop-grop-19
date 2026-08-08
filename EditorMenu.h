#ifndef EDITORMENU_H
#define EDITORMENU_H

#include <SDL3/SDL.h>

class EditorMenu
{
private:
    bool fileOpen;

public:
    EditorMenu();

    void Draw(SDL_Renderer* renderer);

    void HandleClick(int x, int y);
};

#endif
