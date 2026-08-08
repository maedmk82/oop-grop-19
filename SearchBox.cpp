#include "SearchBox.h"
#include "TextRenderer.h"


SearchBox::SearchBox(SDL_Window* window)
{
    text = "";
    active = false;
    this->window = window;
}


// =====================================
// Draw
// =====================================

void SearchBox::Draw(SDL_Renderer* renderer)
{
    SDL_Color black = {0, 0, 0, 255};


    // -------------------------
    // Search Box
    // -------------------------

    SDL_SetRenderDrawColor(
        renderer,
        255,
        255,
        255,
        255
    );


    SDL_FRect box =
    {
        570,
        5,
        300,
        30
    };


    SDL_RenderFillRect(
        renderer,
        &box
    );


    // کادر دور Search

    SDL_SetRenderDrawColor(
        renderer,
        100,
        100,
        100,
        255
    );


    SDL_RenderRect(
        renderer,
        &box
    );


    // -------------------------
    // متن داخل Search
    // -------------------------

    std::string displayText;


    if(text.empty())
    {
        displayText = "Search...";
    }
    else
    {
        displayText = text;
    }


    SDL_Texture* searchText =
        TextRenderer::CreateText(
            renderer,
            displayText.c_str(),
            black
        );


    if(searchText)
    {
        SDL_FRect pos =
        {
            580,
            10,
            270,
            20
        };


        SDL_RenderTexture(
            renderer,
            searchText,
            NULL,
            &pos
        );


        SDL_DestroyTexture(searchText);
    }
}


// =====================================
// Mouse
// =====================================

void SearchBox::HandleClick(int x, int y)
{
    if(x >= 570 && x <= 870 &&
       y >= 5 && y <= 35)
    {
        active = true;

        SDL_StartTextInput(window);
    }
    else
    {
        active = false;

        SDL_StopTextInput(window);
    }
}


// =====================================
// Keyboard
// =====================================

void SearchBox::HandleKeyboard(SDL_Event event)
{
    if(!active)
        return;


    if(event.type == SDL_EVENT_TEXT_INPUT)
    {
        text += event.text.text;
    }


    if(event.type == SDL_EVENT_KEY_DOWN)
    {
        if(event.key.key == SDLK_BACKSPACE)
        {
            if(!text.empty())
            {
                text.pop_back();
            }
        }
    }
}


// =====================================

std::string SearchBox::GetText()
{
    return text;
}
