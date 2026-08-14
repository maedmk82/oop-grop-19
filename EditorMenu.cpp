#include "EditorMenu.h"
#include "TextRenderer.h"

EditorMenu::EditorMenu()
{
    fileOpen = false;
}

//-----------------------------------------
// Draw
//-----------------------------------------

void EditorMenu::Draw(SDL_Renderer* renderer)
{
    SDL_Color black = {0,0,0,255};

    //-----------------------------------------
    // نوار بالای Editor
    //-----------------------------------------

    SDL_SetRenderDrawColor(
        renderer,
        200,
        200,
        200,
        255
    );

    SDL_FRect bar =
    {
        0,
        0,
        1200,
        45
    };

    SDL_RenderFillRect(renderer, &bar);


    //-----------------------------------------
    // File
    //-----------------------------------------

    SDL_Texture* fileText =
        TextRenderer::CreateText(
            renderer,
            "File",
            black
        );

    if(fileText)
    {
        SDL_FRect pos =
        {
            15,
            8,
            50,
            24
        };

        SDL_RenderTexture(
            renderer,
            fileText,
            NULL,
            &pos
        );

        SDL_DestroyTexture(fileText);
    }


    //-----------------------------------------
    // File Menu
    //-----------------------------------------

    if(fileOpen)
    {
        SDL_SetRenderDrawColor(
            renderer,
            245,
            245,
            245,
            255
        );

        SDL_FRect menu =
        {
            0,
            40,
            180,
            160
        };

        SDL_RenderFillRect(
            renderer,
            &menu
        );


        //-----------------------------------------
        // Border
        //-----------------------------------------

        SDL_SetRenderDrawColor(
            renderer,
            150,
            150,
            150,
            255
        );

        SDL_RenderRect(
            renderer,
            &menu
        );


        //-----------------------------------------
        // New Project
        //-----------------------------------------

        SDL_Texture* newText =
            TextRenderer::CreateText(
                renderer,
                "New Project",
                black
            );

        if(newText)
        {
            SDL_FRect pos =
            {
                15,
                48,
                140,
                25
            };

            SDL_RenderTexture(
                renderer,
                newText,
                NULL,
                &pos
            );

            SDL_DestroyTexture(newText);
        }


        //-----------------------------------------
        // Open Project
        //-----------------------------------------

        SDL_Texture* openText =
            TextRenderer::CreateText(
                renderer,
                "Open Project",
                black
            );

        if(openText)
        {
            SDL_FRect pos =
            {
                15,
                88,
                140,
                25
            };

            SDL_RenderTexture(
                renderer,
                openText,
                NULL,
                &pos
            );

            SDL_DestroyTexture(openText);
        }


        //-----------------------------------------
        // Save Project
        //-----------------------------------------

        SDL_Texture* saveText =
            TextRenderer::CreateText(
                renderer,
                "Save Project",
                black
            );

        if(saveText)
        {
            SDL_FRect pos =
            {
                15,
                128,
                140,
                25
            };

            SDL_RenderTexture(
                renderer,
                saveText,
                NULL,
                &pos
            );

            SDL_DestroyTexture(saveText);
        }


        //-----------------------------------------
        // Save As
        //-----------------------------------------

        SDL_Texture* saveAsText =
            TextRenderer::CreateText(
                renderer,
                "Save As",
                black
            );

        if(saveAsText)
        {
            SDL_FRect pos =
            {
                15,
                168,
                140,
                25
            };

            SDL_RenderTexture(
                renderer,
                saveAsText,
                NULL,
                &pos
            );

            SDL_DestroyTexture(saveAsText);
        }
    }
}


//-----------------------------------------
// Handle Click
//-----------------------------------------

EditorMenuAction EditorMenu::HandleClick(int x, int y)
{
    //-----------------------------------------
    // کلیک روی File
    //-----------------------------------------

    if(x >= 0 &&
       x <= 80 &&
       y >= 0 &&
       y <= 40)
    {
        fileOpen = !fileOpen;

        return EDITOR_NO_ACTION;
    }


    //-----------------------------------------
    // اگر File باز نیست
    //-----------------------------------------

    if(!fileOpen)
    {
        return EDITOR_NO_ACTION;
    }


    //-----------------------------------------
    // New Project
    //-----------------------------------------

    if(x >= 0 &&
       x <= 180 &&
       y >= 40 &&
       y < 80)
    {
        fileOpen = false;

        return EDITOR_NEW_PROJECT;
    }


    //-----------------------------------------
    // Open Project
    //-----------------------------------------

    if(x >= 0 &&
       x <= 180 &&
       y >= 80 &&
       y < 120)
    {
        fileOpen = false;

        return EDITOR_OPEN_PROJECT;
    }


    //-----------------------------------------
    // Save Project
    //-----------------------------------------

    if(x >= 0 &&
       x <= 180 &&
       y >= 120 &&
       y < 160)
    {
        fileOpen = false;

        return EDITOR_SAVE_PROJECT;
    }


    //-----------------------------------------
    // Save As
    //-----------------------------------------

    if(x >= 0 &&
       x <= 180 &&
       y >= 160 &&
       y < 200)
    {
        fileOpen = false;

        return EDITOR_SAVE_AS;
    }


    return EDITOR_NO_ACTION;
}
