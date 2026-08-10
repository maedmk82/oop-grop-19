#include "SaveAs.h"
#include "TextRenderer.h"
#include "ProjectManager.h"
#include "Page.h"
#include "EditorPage.h"
#include <iostream>
#include <string>

// اضافه کردن کتابخانه‌های ویندوز در بالاترین نقطه
#include <windows.h>
#include <commdlg.h>

extern SDL_Window* window;
extern Page CurrentPage;
extern Project currentProject;

static std::string saveAsName = "";
static bool saveAsTyping = false;

// ----------------------------------------------------
// تابع بومی ویندوز (باید حتماً اینجا و قبل از بقیه توابع باشد)
// ----------------------------------------------------
std::string OpenNativeSaveAsDialog()
{
    OPENFILENAMEA ofn;
    CHAR szFile[260] = {0};

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);

    // فیلتر فرمت فایل‌ها
    ofn.lpstrFilter = "Proteus Clone Project (*.pro)\0*.pro\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;

    // تنظیمات پنجره
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = "pro";

    if (GetSaveFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }

    return "";
}
//----------------------------------------
// Draw Save As Page
//----------------------------------------

void DrawSaveAsPage(SDL_Renderer* renderer)
{
    SDL_Color black = { 0, 0, 0, 255 };

    //--------------------------------
    // Background
    //--------------------------------
    SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);
    SDL_FRect background = { 0, 0, 950, 600 };
    SDL_RenderFillRect(renderer, &background);

    //--------------------------------
    // Title
    //--------------------------------
    SDL_Texture* title = TextRenderer::CreateText(renderer, "Save As", black);
    if(title)
    {
        SDL_FRect pos = { 100, 70, 200, 30 };
        SDL_RenderTexture(renderer, title, NULL, &pos);
        SDL_DestroyTexture(title);
    }

    //--------------------------------
    // نمایش آدرس محل ذخیره‌سازی (بخش جدید)
    //--------------------------------
    SDL_Texture* pathLabel = TextRenderer::CreateText(renderer, "Location: D:\\", black);
    if(pathLabel)
    {
        SDL_FRect pos = { 100, 110, 120, 18 }; // ابعاد تقریبی متن
        SDL_RenderTexture(renderer, pathLabel, NULL, &pos);
        SDL_DestroyTexture(pathLabel);
    }

    //--------------------------------
    // Name Box
    //--------------------------------
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_FRect nameBox = { 100, 130, 500, 45 };
    SDL_RenderFillRect(renderer, &nameBox);

    // تغییر رنگ حاشیه در زمان تایپ شدن
    if(saveAsTyping) {
        SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255); // رنگ آبی هنگام فکوس
    } else {
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    }
    SDL_RenderRect(renderer, &nameBox);

    //--------------------------------
    // Project Name
    //--------------------------------
    std::string text = saveAsName;
    if(text.empty())
    {
        text = "Project Name";
    }

    SDL_Texture* nameText = TextRenderer::CreateText(renderer, text.c_str(), black);
    if(nameText)
    {
        float texW, texH;
        SDL_GetTextureSize(nameText, &texW, &texH);

        if (texW > 480) texW = 480; // جلوگیری از بیرون زدن متن

        SDL_FRect pos = { 115, 142, texW, 25 };
        SDL_RenderTexture(renderer, nameText, NULL, &pos);
        SDL_DestroyTexture(nameText);
    }

    //--------------------------------
    // Save Button
    //--------------------------------
    SDL_SetRenderDrawColor(renderer, 180, 210, 180, 255);
    SDL_FRect saveButton = { 100, 210, 150, 45 };
    SDL_RenderFillRect(renderer, &saveButton);

    SDL_Texture* saveText = TextRenderer::CreateText(renderer, "Save", black);
    if(saveText)
    {
        SDL_FRect pos = { 150, 220, 70, 25 };
        SDL_RenderTexture(renderer, saveText, NULL, &pos);
        SDL_DestroyTexture(saveText);
    }

    //--------------------------------
    // Cancel Button
    //--------------------------------
    SDL_SetRenderDrawColor(renderer, 210, 180, 180, 255);
    SDL_FRect cancelButton = { 270, 210, 150, 45 };
    SDL_RenderFillRect(renderer, &cancelButton);

    SDL_Texture* cancelText = TextRenderer::CreateText(renderer, "Cancel", black);
    if(cancelText)
    {
        SDL_FRect pos = { 305, 220, 90, 25 };
        SDL_RenderTexture(renderer, cancelText, NULL, &pos);
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
    if(x >= 100 && x <= 600 && y >= 130 && y <= 175)
    {
        saveAsTyping = true;
        SDL_StartTextInput(window);
        return;
    }

    //--------------------------------
    // Save Button
    //--------------------------------
    if(x >= 100 && x <= 250 && y >= 210 && y <= 255) // دکمه Save
    {
        std::string fullPath = OpenNativeSaveAsDialog();
        if (!fullPath.empty())
        {
            size_t lastSlash = fullPath.find_last_of("\\/");
            if(lastSlash != std::string::npos) {
                currentProject.name = fullPath.substr(lastSlash + 1);
            } else {
                currentProject.name = fullPath;
            }

            // *** تغییر بسیار مهم: ذخیره مسیر کامل ***
            currentProject.path = fullPath;

            SaveProjectAs(currentProject);

            extern EditorPage* editor;
            if (editor) editor->SaveWorkspace(currentProject.path); // حتما از path استفاده شود

            saveAsTyping = false;
            CurrentPage = EDITOR_PAGE;
        }
        return;
    }

    //--------------------------------
    // Cancel
    //--------------------------------
    if(x >= 270 && x <= 420 && y >= 210 && y <= 255)
    {
        saveAsTyping = false;
        SDL_StopTextInput(window);
        CurrentPage = EDITOR_PAGE;
    }

    // اگر کاربر جای دیگری کلیک کرد، حالت تایپ غیرفعال شود
    saveAsTyping = false;
    SDL_StopTextInput(window);
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
        // در ویندوز نمی‌توان از کاراکترهای خاص در نام فایل استفاده کرد
        std::string input = event.text.text;
        if (input.find_first_of("\\/:*?\"<>|") == std::string::npos) {
            saveAsName += input;
        }
    }

    if(event.type == SDL_EVENT_KEY_DOWN)
    {
        if(event.key.key == SDLK_BACKSPACE)
        {
            if(!saveAsName.empty())
            {
                saveAsName.pop_back();
            }
        }

        if(event.key.key == SDLK_ESCAPE)
        {
            saveAsTyping = false;
            SDL_StopTextInput(window);
            CurrentPage = EDITOR_PAGE;
        }

        // پشتیبانی از کلید Enter برای ذخیره سریع
        if(event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER)
        {
            if(!saveAsName.empty())
            {
                currentProject.name = saveAsName;
                // currentProject.path = "D:\\" + saveAsName + ".pro";
                SaveProjectAs(currentProject);

                saveAsTyping = false;
                SDL_StopTextInput(window);
                CurrentPage = EDITOR_PAGE;
            }
        }
    }
}
// ----------------------------------------------------
// تابع بومی ویندوز برای گرفتن مسیر ذخیره‌سازی
// ----------------------------------------------------

