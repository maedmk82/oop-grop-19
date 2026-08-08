#include "EditorPage.h"

EditorPage::EditorPage(SDL_Window* window)
    : search(window)
{
}
void EditorPage::Draw(SDL_Renderer* renderer)
{
    // =====================================
    // پس زمینه
    // =====================================

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


    // =====================================
    // Editor Menu
    // =====================================

    menu.Draw(renderer);


    // =====================================
    // Search Box
    // =====================================

    search.Draw(renderer);
}


void EditorPage::HandleClick(int x, int y)
{
    menu.HandleClick(x, y);

    search.HandleClick(x, y);
}


void EditorPage::HandleKeyboard(SDL_Event event)
{
    search.HandleKeyboard(event);
}
