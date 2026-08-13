#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <vector>
#include "SaveAs.h"
#include "Page.h"
#include "ProjectManager.h"
#include "TextRenderer.h"
#include "Menu.h"
#include "NewProject.h"
#include "OpenProject.h"
#include "EditorPage.h"

Page CurrentPage = HOME_PAGE;
TTF_Font* font = nullptr;
SDL_Window* window = nullptr;
SDL_Texture* txtFile = nullptr;
SDL_Texture* txtNew = nullptr;
SDL_Texture* txtOpen = nullptr;
int selectedProjectIndex = -1;
Project currentProject;
EditorPage* editor = nullptr;

//----------------------
// توابع
//----------------------
void DrawHomePage(SDL_Renderer* renderer);

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }
    if(!TTF_Init())
    {
        std::cout << "TTF Error" << std::endl;
        return -1;
    }

    TextRenderer::Init("Fonts/arial.ttf", 18);
    LoadProjects();

    if(!font)
    {
        std::cout << "Font Error" << std::endl;
    }

    window = SDL_CreateWindow(
        "Proteus Clone",
        950,
        600,
        0
    );

    if(!window)
    {
        std::cout << "Window Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    editor = new EditorPage(window);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    if (!renderer)
    {
        std::cout << "Renderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_Color color = {0, 0, 0, 255};
    txtFile = TextRenderer::CreateText(renderer, "File", color);
    txtNew  = TextRenderer::CreateText(renderer, "New Project", color);
    txtOpen = TextRenderer::CreateText(renderer, "Open Project", color);

    bool running = true;
    SDL_Event event;

    while(running)
    {
        while(SDL_PollEvent(&event))
        {
            // --------------------------------
            // 1. خروج از برنامه و Auto-Save
            // --------------------------------
            if(event.type == SDL_EVENT_QUIT)
            {
                if (CurrentPage == EDITOR_PAGE && !currentProject.path.empty() && editor != nullptr)
                {
                    editor->SaveWorkspace(currentProject.path);
                    SaveProject(currentProject);
                    std::cout << "Project Auto-Saved before exiting!" << std::endl;
                }
                running = false;
            }

            // --------------------------------
            // 2. کلیک موس
            // --------------------------------
            else if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                int x = event.button.x;
                int y = event.button.y;

                // --- جدید: دکمه‌ی وسط موس برای شروع جابه‌جایی (Pan) بوم ---
                if (event.button.button == SDL_BUTTON_MIDDLE)
                {
                    if (CurrentPage == EDITOR_PAGE && editor != nullptr)
                    {
                        editor->StartPan(x, y);
                    }
                }
                // --- جدید: کلیک راست برای لغو حالت جای‌گذاری قطعه ---
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    if (CurrentPage == EDITOR_PAGE && editor != nullptr)
                    {
                        editor->CancelPlacing();
                    }
                }
                else if(CurrentPage == HOME_PAGE)
                {
                    HandleMenuClick(x, y);

                    std::vector<Project> list = GetRecentProjects();
                    for(int i = 0; i < (int)list.size() && i < 10; i++)
                    {
                        int yPos = 120 + i * 45;
                        if(x >= 100 && x <= 500 && y >= yPos && y <= yPos + 35)
                        {
                            if (editor != nullptr && !currentProject.path.empty()) {
                                editor->SaveWorkspace(currentProject.path);
                                SaveProject(currentProject);
                            }

                            selectedProjectIndex = i;
                            currentProject = list[i];

                            if(OpenProject(currentProject))
                            {
                                if (editor) {
                                    editor->pageSize = currentProject.pageSize;
                                    editor->LoadWorkspace(currentProject.path);
                                }

                                if (currentProject.pageSize.find("A3") != std::string::npos) {
                                    SDL_SetWindowSize(window, 1350, 800);
                                    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                                } else {
                                    SDL_SetWindowSize(window, 950, 600);
                                    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
                                }

                                CurrentPage = EDITOR_PAGE;
                            }
                            break;
                        }
                    }
                }
                else if(CurrentPage == NEW_PROJECT_PAGE)
                {
                    HandleNewProjectClick(x, y);
                }
                else if(CurrentPage == OPEN_PROJECT_PAGE)
                {
                    HandleOpenProjectClick(x, y);
                }
                else if(CurrentPage == EDITOR_PAGE)
                {
                    EditorMenuAction action = editor->HandleClick(x, y);

                    if(action == EDITOR_NEW_PROJECT)
                    {
                        CurrentPage = NEW_PROJECT_PAGE;
                        SDL_StartTextInput(window);
                    }
                    else if(action == EDITOR_OPEN_PROJECT)
                    {
                        CurrentPage = OPEN_PROJECT_PAGE;
                    }
                    else if(action == EDITOR_SAVE_PROJECT)
                    {
                        SaveProject(currentProject);
                        if (!currentProject.path.empty())
                        {
                            editor->SaveWorkspace(currentProject.path);
                            std::cout << "Project Overwritten & Saved: " << currentProject.path << std::endl;
                        }
                        else
                        {
                            CurrentPage = SAVE_AS_PAGE;
                        }
                    }
                    else if(action == EDITOR_SAVE_AS)
                    {
                        CurrentPage = SAVE_AS_PAGE;
                    }
                }
                else if(CurrentPage == SAVE_AS_PAGE)
                {
                   HandleSaveAsClick(x, y);
                }
            } // <--- پایان رویداد کلیک موس (DOWN)

            // --------------------------------
            // 3. حرکت موس (درگ قطعات، جابه‌جایی بوم، سایه‌ی قطعه)
            // --------------------------------
            else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if (CurrentPage == EDITOR_PAGE && editor != nullptr) {
                    editor->HandleMouseMove(event.motion.x, event.motion.y);
                }
            }

            // --------------------------------
            // 4. رها کردن کلیک موس (پایان درگ، پایان انتخاب گروهی، پایان Pan)
            // --------------------------------
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (CurrentPage == EDITOR_PAGE && editor != nullptr) {
                    editor->HandleMouseRelease(event.button.x, event.button.y);
                }
            }

            // --------------------------------
            // 5. اسکرول موس (Zoom In / Zoom Out نسبت به نشانگر موس) - جدید
            // --------------------------------
            else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                if (CurrentPage == EDITOR_PAGE && editor != nullptr) {
                    float mx = 0, my = 0;
                    SDL_GetMouseState(&mx, &my);
                    editor->HandleMouseWheel(event.wheel.y, (int)mx, (int)my);
                }
            }

            // --------------------------------
            // 6. کیبورد (تایپ، دکمه‌ها، میان‌برهای زوم)
            // --------------------------------
            else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_TEXT_INPUT)
            {
                if(CurrentPage == NEW_PROJECT_PAGE)
                {
                    HandleKeyboard(event);
                }
                else if(CurrentPage == EDITOR_PAGE)
                {
                    // میانبرهای کیبورد: Delete / Ctrl+Z / Ctrl+Y / Ctrl+S / Esc / زوم
                    EditorMenuAction kbAction = editor->HandleKeyboard(event);

                    if (kbAction == EDITOR_SAVE_PROJECT)
                    {
                        SaveProject(currentProject);
                        if (!currentProject.path.empty())
                        {
                            editor->SaveWorkspace(currentProject.path);
                            std::cout << "Project Overwritten & Saved (Ctrl+S): " << currentProject.path << std::endl;
                        }
                        else
                        {
                            CurrentPage = SAVE_AS_PAGE;
                        }
                    }
                    else if (kbAction == EDITOR_SAVE_AS)
                    {
                        CurrentPage = SAVE_AS_PAGE;
                    }
                }
                else if(CurrentPage == SAVE_AS_PAGE)
                {
                    HandleSaveAsKeyboard(event);
                }
            }
        } // پایان while(SDL_PollEvent)

        // رندر کردن صفحات
        SDL_SetRenderDrawColor(renderer, 225, 225, 225, 255);
        SDL_RenderClear(renderer);

        switch(CurrentPage)
        {
            case HOME_PAGE:
                DrawHomePage(renderer);
                break;
            case NEW_PROJECT_PAGE:
                DrawNewProject(renderer);
                break;
            case OPEN_PROJECT_PAGE:
                DrawOpenProject(renderer);
                break;
            case EDITOR_PAGE:
                editor->Draw(renderer);
                break;
            case SAVE_AS_PAGE:
                DrawSaveAsPage(renderer);
                break;
        }

        if(CurrentPage == HOME_PAGE)
        {
            DrawMenu(renderer);
        }

        SDL_RenderPresent(renderer);
    } // پایان حلقه بازی

    // آزادسازی حافظه
    SDL_DestroyTexture(txtFile);
    SDL_DestroyTexture(txtNew);
    SDL_DestroyTexture(txtOpen);

    TTF_CloseFont(font);
    TextRenderer::Close();

    if (editor) delete editor;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

//--------------------------------------------------
void DrawHomePage(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_FRect rect = { 0, 30, 950, 570 };
    SDL_RenderFillRect(renderer, &rect);

    SDL_Color color = {0, 0, 0, 255};
    SDL_Texture* title = TextRenderer::CreateText(renderer, "Recent Projects", color);

    if(title)
    {
        SDL_FRect pos = { 100, 70, 200, 25 };
        SDL_RenderTexture(renderer, title, NULL, &pos);
        SDL_DestroyTexture(title);
    }

    std::vector<Project> list = GetRecentProjects();

    for(int i = 0; i < (int)list.size(); i++)
    {
        SDL_SetRenderDrawColor(renderer, 210, 210, 210, 255);

        SDL_FRect box = { 100, (float)(120 + i * 45), 400, 35 };
        SDL_RenderFillRect(renderer, &box);

        SDL_Texture* txt = TextRenderer::CreateText(renderer, list[i].name.c_str(), color);

        if(txt)
        {
            SDL_FRect tpos = { 120, (float)(130 + i * 45), 250, 20 };
            SDL_RenderTexture(renderer, txt, NULL, &tpos);
            SDL_DestroyTexture(txt);
        }
    }
}
//--------------------------------------------------

void DrawOpenProjectPage(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer,180,180,230,255);

    SDL_FRect rect={20,50,910,520};

    SDL_RenderFillRect(renderer,&rect);
}
