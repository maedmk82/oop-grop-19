#include "OpenProject.h"
#include "ProjectManager.h"
#include "TextRenderer.h"
#include "Page.h"
#include "EditorPage.h" // <--- ۱. اضافه کردن هدر ادیتور

#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <string>

#include <windows.h>
#include <commdlg.h>

extern int selectedProjectIndex;
extern Project currentProject;
extern Page CurrentPage;
extern EditorPage* editor; // <--- ۲. اضافه کردن اشاره‌گر به ادیتور

// ----------------------------------------------------
// تابع بومی ویندوز برای باز کردن پنجره انتخاب فایل
// ----------------------------------------------------
std::string OpenNativeOpenDialog()
{
    OPENFILENAMEA ofn;
    CHAR szFile[260] = {0};

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);

    ofn.lpstrFilter = "Proteus Clone Project (*.pro)\0*.pro\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = "pro";

    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }

    return "";
}

//--------------------------------------------------
// Draw Open Project
//--------------------------------------------------

void DrawOpenProject(SDL_Renderer* renderer)
{
    SDL_Color color = { 0, 0, 0, 255 };

    // Background
    SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);
    SDL_FRect background = { 0, 0, 950, 600 };
    SDL_RenderFillRect(renderer, &background);

    // Title
    SDL_Texture* title = TextRenderer::CreateText(renderer, "Open Project", color);
    if(title)
    {
        SDL_FRect pos = { 80, 50, 200, 30 };
        SDL_RenderTexture(renderer, title, NULL, &pos);
        SDL_DestroyTexture(title);
    }

    // Browse Button
    SDL_SetRenderDrawColor(renderer, 180, 210, 255, 255);
    SDL_FRect browseBtn = { 650, 50, 200, 40 };
    SDL_RenderFillRect(renderer, &browseBtn);

    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderRect(renderer, &browseBtn);

    SDL_Texture* browseTxt = TextRenderer::CreateText(renderer, "Browse Computer...", color);
    if(browseTxt)
    {
        SDL_FRect pos = { 665, 60, 170, 20 };
        SDL_RenderTexture(renderer, browseTxt, NULL, &pos);
        SDL_DestroyTexture(browseTxt);
    }

    // Recent Projects
    std::vector<Project> list = GetRecentProjects();

    for(int i = 0; i < (int)list.size() && i < 10; i++)
    {
        float y = 120 + i * 45;

        SDL_SetRenderDrawColor(renderer, 210, 210, 210, 255);
        SDL_FRect box = { 100, y, 500, 35 };
        SDL_RenderFillRect(renderer, &box);

        SDL_Texture* txt = TextRenderer::CreateText(renderer, list[i].name.c_str(), color);
        if(txt)
        {
            SDL_FRect pos = { 115, y + 7, 350, 22 };
            SDL_RenderTexture(renderer, txt, NULL, &pos);
            SDL_DestroyTexture(txt);
        }
    }
}


//--------------------------------------------------
// Click on Project
//--------------------------------------------------

void HandleOpenProjectClick(int x, int y)
{
    // ۱. بررسی کلیک روی دکمه Browse Computer
    if(x >= 650 && x <= 850 && y >= 50 && y <= 90)
    {
        std::string fullPath = OpenNativeOpenDialog();

        if(!fullPath.empty())
        {
            size_t lastSlash = fullPath.find_last_of("\\/");
            if(lastSlash != std::string::npos) {
                currentProject.name = fullPath.substr(lastSlash + 1);
            } else {
                currentProject.name = fullPath;
            }

            if(OpenProject(currentProject))
            {
                // *** پاک کردن صفحه قبل از ورود به ادیتور ***
                if (editor) {
                    editor->ClearWorkspace();
                }

                CurrentPage = EDITOR_PAGE;
                std::cout << "Opened From Computer: " << fullPath << std::endl;
            }
        }
        return;
    }

    // ۲. بررسی کلیک روی لیست پروژه‌های اخیر (Recent Projects)
    std::vector<Project> projects = GetRecentProjects();

    for(int i = 0; i < (int)projects.size() && i < 10; i++)
    {
        float yPos = 120 + i * 45;

        if(x >= 100 && x <= 600 && y >= yPos && y <= yPos + 35)
        {
            selectedProjectIndex = i;
            currentProject = projects[i];

            if(OpenProject(currentProject))
            {
                // *** پاک کردن صفحه قبل از ورود به ادیتور ***
                if (editor) {
                    editor->ClearWorkspace();
                }

                CurrentPage = EDITOR_PAGE;
            }

            std::cout << "Opening Recent Project : " << currentProject.name << std::endl;
            return;
        }
    }
}
