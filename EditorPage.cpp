#include "EditorPage.h"


EditorPage::EditorPage(SDL_Window* window)
    : search(window)
{
}


//-----------------------------------------
// Draw
//-----------------------------------------

void EditorPage::Draw(SDL_Renderer* renderer)
{
    //-----------------------------------------
    // Background
    //-----------------------------------------

    SDL_SetRenderDrawColor(
        renderer,
        230,
        230,
        230,
        255
    );

    SDL_FRect area =
    {
        0,
        0,
        950,
        600
    };

    SDL_RenderFillRect(
        renderer,
        &area
    );


    //-----------------------------------------
    // Editor Menu
    //-----------------------------------------

    menu.Draw(renderer);


    //-----------------------------------------
    // Search
    //-----------------------------------------

    search.Draw(renderer);
}


//-----------------------------------------
// Mouse
//-----------------------------------------

EditorMenuAction EditorPage::HandleClick(int x, int y)
{
    EditorMenuAction action =
        menu.HandleClick(x,y);


    // Search
    search.HandleClick(x,y);


    return action;
}


//-----------------------------------------
// Keyboard
//-----------------------------------------

void EditorPage::HandleKeyboard(SDL_Event event)
{
    search.HandleKeyboard(event);
}
