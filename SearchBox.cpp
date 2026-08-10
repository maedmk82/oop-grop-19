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
    // پس‌زمینه کادر جستجو
    // -------------------------
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_FRect box = {
        570,
        5,
        300,
        30
    };

    SDL_RenderFillRect(renderer, &box);

    // -------------------------
    // کادر دور Search (تغییر رنگ هنگام فعال بودن)
    // -------------------------
    if (active) {
        // رنگ آبی روشن برای نشان دادن حالت فعال (Focus)
        SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
    } else {
        // رنگ خاکستری برای حالت غیرفعال
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
        // رنگ متن پیش‌فرض را کمی کم‌رنگ‌تر می‌کنیم (اختیاری)
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
        // گرفتن سایز واقعی متن برای جلوگیری از کشیده شدن (Stretch)
        float texW = 0, texH = 0;
        SDL_GetTextureSize(searchText, &texW, &texH);

        // جلوگیری از بیرون زدن متن از کادر اگر طولانی شد
        if (texW > 280) {
            texW = 280;
        }

        SDL_FRect pos = {
            580,
            10,
            texW,  // استفاده از عرض واقعی کلمه به جای عدد ثابت 270
            20     // ارتفاع فونت شما تقریبا 20 مناسب است
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
// Get Text
// =====================================

std::string SearchBox::GetText() const
{
    return text;
}
