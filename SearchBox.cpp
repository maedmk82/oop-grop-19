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

// =====================================
// Draw
// =====================================

void SearchBox::Draw(SDL_Renderer* renderer)
{
    SDL_Color black = {0, 0, 0, 255};

    // -------------------------
    // پس‌زمینه کادر جستجو (منتقل شد به سمت چپ)
    // -------------------------
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_FRect box = {
        120,   // تغییر یافته به سمت چپ
        5,
        250,   // عرض کمی جمع‌وجورتر شد تا با بقیه دکمه‌ها تداخل نکند
        30
    };

    SDL_RenderFillRect(renderer, &box);

    // -------------------------
    // کادر دور Search (تغییر رنگ هنگام فعال بودن)
    // -------------------------
    if (active) {
        SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    }

    SDL_RenderRect(renderer, &box);

    // -------------------------
    // متن داخل Search
    // -------------------------
    std::string displayText;

    if(text.empty())
    {
        displayText = "Search...";
        black = {150, 150, 150, 255};
    }
    else
    {
        displayText = text;
    }

    SDL_Texture* searchText = TextRenderer::CreateText(
        renderer,
        displayText.c_str(),
        black
    );

    if(searchText)
    {
        float texW = 0, texH = 0;
        SDL_GetTextureSize(searchText, &texW, &texH);

        if (texW > 230) { // محدودیت عرض متن هم متناسب با عرض جدید کادر تغییر کرد
            texW = 230;
        }

        SDL_FRect pos = {
            130,   // متن هم به سمت چپ شیفت پیدا کرد
            10,
            texW,
            20
        };

        SDL_RenderTexture(renderer, searchText, NULL, &pos);
        SDL_DestroyTexture(searchText);
    }
}

// =====================================
// Mouse
// =====================================

void SearchBox::HandleClick(int x, int y)
{
    // مختصات کلیک هم با کادر جدیدِ سمت چپ تنظیم شد
    if(x >= 120 && x <= 370 &&
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
// Get Text
// =====================================

std::string SearchBox::GetText() const
{
    return text;
}
