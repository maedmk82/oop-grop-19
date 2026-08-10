#include "EditorMenu.h"
#include "TextRenderer.h"


EditorMenu::EditorMenu()
{
    fileOpen = false;
}


void EditorMenu::Draw(SDL_Renderer* renderer)
{
    SDL_Color black = {0, 0, 0, 255};


    // =====================================
    // نوار بالای Editor
    // =====================================

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
        950,
        40
    };

    SDL_RenderFillRect(renderer, &bar);


    // =====================================
    // متن File
    // =====================================

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
            40,
            25
        };

        SDL_RenderTexture(
            renderer,
            fileText,
            NULL,
            &pos
        );

        SDL_DestroyTexture(fileText);
    }


    // =====================================
    // متن Search
    // =====================================

    SDL_Texture* searchText =
        TextRenderer::CreateText(
            renderer,
            "Search",
            black
        );

    if(searchText)
    {
        SDL_FRect pos =
        {
            500,
            8,
            60,
            25
        };

        SDL_RenderTexture(
            renderer,
            searchText,
            NULL,
            &pos
        );

        SDL_DestroyTexture(searchText);
    }


    // =====================================
    // منوی File
    // =====================================

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
            120
        };

        SDL_RenderFillRect(
            renderer,
            &menu
        );


        // New Project

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
                50,
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


        // Open Project

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
                90,
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


        // Save Project

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
                130,
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
    }
}


// =====================================
// Mouse
// =====================================

void EditorMenu::HandleClick(int x, int y)
{
    // کلیک روی File

    if(x >= 0 && x <= 80 &&
       y >= 0 && y <= 40)
    {
        fileOpen = !fileOpen;

        return;
    }


    // اگر منوی File باز است

    if(fileOpen)
    {
        if(x >= 0 && x <= 180 &&
           y >= 40 && y <= 160)
        {
            // فعلاً فقط منو را نگه می‌داریم

            return;
        }
    }
}
