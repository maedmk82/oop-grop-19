#include "SaveAs.h"

#include "TextRenderer.h"
#include "ProjectManager.h"
#include "Page.h"

#include <iostream>
#include <string>
extern SDL_Window* window;
extern Page CurrentPage;

extern Project currentProject;

static std::string saveAsName = "";

static bool saveAsTyping = false;


//----------------------------------------
// Draw Save As Page
//----------------------------------------

void DrawSaveAsPage(SDL_Renderer* renderer)
{
    SDL_Color black =
    {
        0,
        0,
        0,
        255
    };


    // Background

    SDL_SetRenderDrawColor(
        renderer,
        235,
        235,
        235,
        255
    );

    SDL_FRect background =
    {
        0,
        0,
        950,
        600
    };

    SDL_RenderFillRect(
        renderer,
        &background
    );


    //--------------------------------
    // Title
    //--------------------------------

    SDL_Texture* title =
        TextRenderer::CreateText(
            renderer,
            "Save As",
            black
        );

    if(title)
    {
        SDL_FRect pos =
        {
            100,
            70,
            200,
            30
        };

        SDL_RenderTexture(
            renderer,
            title,
            NULL,
            &pos
        );

        SDL_DestroyTexture(title);
    }


    //--------------------------------
    // Name Box
    //--------------------------------

    SDL_SetRenderDrawColor(
        renderer,
        255,
        255,
        255,
        255
    );

    SDL_FRect nameBox =
    {
        100,
        130,
        500,
        45
    };

    SDL_RenderFillRect(
        renderer,
        &nameBox
    );


    SDL_SetRenderDrawColor(
        renderer,
        100,
        100,
        100,
        255
    );

    SDL_RenderRect(
        renderer,
        &nameBox
    );


    //--------------------------------
    // Project Name
    //--------------------------------

    std::string text = saveAsName;

    if(text.empty())
    {
        text = "Project Name";
    }


    SDL_Texture* nameText =
        TextRenderer::CreateText(
            renderer,
            text.c_str(),
            black
        );

    if(nameText)
    {
        SDL_FRect pos =
        {
            115,
            142,
            450,
            25
        };

        SDL_RenderTexture(
            renderer,
            nameText,
            NULL,
            &pos
        );

        SDL_DestroyTexture(nameText);
    }


    //--------------------------------
    // Save Button
    //--------------------------------

    SDL_SetRenderDrawColor(
        renderer,
        180,
        210,
        180,
        255
    );

    SDL_FRect saveButton =
    {
        100,
        210,
        150,
        45
    };

    SDL_RenderFillRect(
        renderer,
        &saveButton
    );


    SDL_Texture* saveText =
        TextRenderer::CreateText(
            renderer,
            "Save",
            black
        );

    if(saveText)
    {
        SDL_FRect pos =
        {
            150,
            220,
            70,
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


    //--------------------------------
    // Cancel Button
    //--------------------------------

    SDL_SetRenderDrawColor(
        renderer,
        210,
        180,
        180,
        255
    );

    SDL_FRect cancelButton =
    {
        270,
        210,
        150,
        45
    };

    SDL_RenderFillRect(
        renderer,
        &cancelButton
    );


    SDL_Texture* cancelText =
        TextRenderer::CreateText(
            renderer,
            "Cancel",
            black
        );

    if(cancelText)
    {
        SDL_FRect pos =
        {
            305,
            220,
            90,
            25
        };

        SDL_RenderTexture(
            renderer,
            cancelText,
            NULL,
            &pos
        );

        SDL_DestroyTexture(cancelText);
    }
}


//----------------------------------------
// Mouse
//----------------------------------------

void HandleSaveAsClick(int x, int y)
{
    //--------------------------------
    // Name Box
    //--------------------------------

    if(x >= 100 &&
       x <= 600 &&
       y >= 130 &&
       y <= 175)
    {
        saveAsTyping = true;

        SDL_StartTextInput(window);

        return;
    }


    //--------------------------------
    // Save
    //--------------------------------

    if(x >= 100 &&
       x <= 250 &&
       y >= 210 &&
       y <= 255)
    {
        if(saveAsName.empty())
        {
            return;
        }


        currentProject.name =
            saveAsName;


        SaveProjectAs(
            currentProject
        );


        std::cout
            << "Save As: "
            << currentProject.name
            << std::endl;


        saveAsTyping = false;

        SDL_StopTextInput(window);


        CurrentPage =
            EDITOR_PAGE;


        return;
    }


    //--------------------------------
    // Cancel
    //--------------------------------

    if(x >= 270 &&
       x <= 420 &&
       y >= 210 &&
       y <= 255)
    {
        saveAsTyping = false;

        SDL_StopTextInput(window);

        CurrentPage =
            EDITOR_PAGE;
    }
}


//----------------------------------------
// Keyboard
//----------------------------------------

void HandleSaveAsKeyboard(SDL_Event& event)
{
    if(!saveAsTyping)
        return;


    if(event.type == SDL_EVENT_TEXT_INPUT)
    {
        saveAsName +=
            event.text.text;
    }


    if(event.type == SDL_EVENT_KEY_DOWN)
    {
        if(event.key.key ==
           SDLK_BACKSPACE)
        {
            if(!saveAsName.empty())
            {
                saveAsName.pop_back();
            }
        }


        if(event.key.key ==
           SDLK_ESCAPE)
        {
            saveAsTyping = false;

            SDL_StopTextInput(window);

            CurrentPage =
                EDITOR_PAGE;
        }
    }
}
