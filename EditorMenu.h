#ifndef EDITORMENU_H
#define EDITORMENU_H

#include <SDL3/SDL.h>

enum EditorMenuAction
{
    EDITOR_NO_ACTION,

    EDITOR_NEW_PROJECT,

    EDITOR_OPEN_PROJECT,

    EDITOR_SAVE_PROJECT,

    EDITOR_SAVE_AS
};

class EditorMenu
{
private:
    bool fileOpen;

public:
    EditorMenu();

    void Draw(SDL_Renderer* renderer);

    EditorMenuAction HandleClick(int x, int y);
};

#endif
