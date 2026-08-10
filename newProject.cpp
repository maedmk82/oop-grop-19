#include "NewProject.h"
#include "ProjectManager.h"
#include "TextRenderer.h"
#include "Page.h"
#include "EditorPage.h" // <--- حتما این هدر اضافه شود

#include <iostream>
#include <string>

extern Page CurrentPage;
extern Project currentProject;
extern SDL_Window* window;
extern EditorPage* editor; // <--- برای دسترسی به صفحه ادیتور

//-----------------------------------------
// Variables
//-----------------------------------------
std::string projectName = "";
std::string projectPath = "Projects/";
int selectedPageSize = 0;

//-----------------------------------------
// Draw New Project
//-----------------------------------------

void DrawNewProject(SDL_Renderer* renderer)
{
    SDL_Color color =
    {
        0,
        0,
        0,
        255
    };


    //-----------------------------------------
    // Background
    //-----------------------------------------

    SDL_SetRenderDrawColor(
        renderer,
        230,
        230,
        230,
        255
    );

    SDL_FRect background =
    {
        50,
        60,
        850,
        450
    };

    SDL_RenderFillRect(
        renderer,
        &background
    );


    //-----------------------------------------
    // Project Name Box
    //-----------------------------------------

    SDL_SetRenderDrawColor(
        renderer,
        255,
        255,
        255,
        255
    );

    SDL_FRect nameBox =
    {
        220,
        80,
        450,
        40
    };

    SDL_RenderFillRect(
        renderer,
        &nameBox
    );


    //-----------------------------------------
    // Project Name Text
    //-----------------------------------------

    if(!projectName.empty())
    {
        SDL_Texture* nameTexture =
            TextRenderer::CreateText(
                renderer,
                projectName.c_str(),
                color
            );

        if(nameTexture)
        {
            SDL_FRect pos =
            {
                230,
                88,
                420,
                25
            };

            SDL_RenderTexture(
                renderer,
                nameTexture,
                NULL,
                &pos
            );

            SDL_DestroyTexture(nameTexture);
        }
    }


    //-----------------------------------------
    // اگر نام خالی است
    //-----------------------------------------

    if(projectName.empty())
    {
        SDL_Texture* placeholder =
            TextRenderer::CreateText(
                renderer,
                "Project Name",
                color
            );

        if(placeholder)
        {
            SDL_FRect pos =
            {
                230,
                88,
                200,
                25
            };

            SDL_RenderTexture(
                renderer,
                placeholder,
                NULL,
                &pos
            );

            SDL_DestroyTexture(placeholder);
        }
    }


    //-----------------------------------------
    // Path Box
    //-----------------------------------------

    SDL_SetRenderDrawColor(
        renderer,
        255,
        255,
        255,
        255
    );

    SDL_FRect pathBox =
    {
        220,
        140,
        450,
        40
    };

    SDL_RenderFillRect(
        renderer,
        &pathBox
    );


    //-----------------------------------------
    // Path Text
    //-----------------------------------------

    SDL_Texture* pathTexture =
        TextRenderer::CreateText(
            renderer,
            projectPath.c_str(),
            color
        );

    if(pathTexture)
    {
        SDL_FRect pos =
        {
            230,
            148,
            400,
            25
        };

        SDL_RenderTexture(
            renderer,
            pathTexture,
            NULL,
            &pos
        );

        SDL_DestroyTexture(pathTexture);
    }


    //-----------------------------------------
    // A3
    //-----------------------------------------

    if(selectedPageSize == 3)
    {
        SDL_SetRenderDrawColor(
            renderer,
            100,
            200,
            100,
            255
        );
    }
    else
    {
        SDL_SetRenderDrawColor(
            renderer,
            180,
            220,
            180,
            255
        );
    }

    SDL_FRect A3 =
    {
        120,
        220,
        220,
        120
    };

    SDL_RenderFillRect(
        renderer,
        &A3
    );


    //-----------------------------------------
    // A4
    //-----------------------------------------

    if(selectedPageSize == 4)
    {
        SDL_SetRenderDrawColor(
            renderer,
            100,
            200,
            100,
            255
        );
    }
    else
    {
        SDL_SetRenderDrawColor(
            renderer,
            180,
            180,
            230,
            255
        );
    }

    SDL_FRect A4 =
    {
        550,
        220,
        220,
        120
    };

    SDL_RenderFillRect(
        renderer,
        &A4
    );


    //-----------------------------------------
    // Create Button
    //-----------------------------------------

    SDL_SetRenderDrawColor(
        renderer,
        200,
        180,
        80,
        255
    );

    SDL_FRect create =
    {
        350,
        400,
        180,
        50
    };

    SDL_RenderFillRect(
        renderer,
        &create
    );


    //-----------------------------------------
    // A3 Text
    //-----------------------------------------

    SDL_Texture* txtA3 =
        TextRenderer::CreateText(
            renderer,
            "A3",
            color
        );

    if(txtA3)
    {
        SDL_FRect pos =
        {
            210,
            265,
            40,
            25
        };

        SDL_RenderTexture(
            renderer,
            txtA3,
            NULL,
            &pos
        );

        SDL_DestroyTexture(txtA3);
    }


    //-----------------------------------------
    // A4 Text
    //-----------------------------------------

    SDL_Texture* txtA4 =
        TextRenderer::CreateText(
            renderer,
            "A4",
            color
        );

    if(txtA4)
    {
        SDL_FRect pos =
        {
            640,
            265,
            40,
            25
        };

        SDL_RenderTexture(
            renderer,
            txtA4,
            NULL,
            &pos
        );

        SDL_DestroyTexture(txtA4);
    }


    //-----------------------------------------
    // Create Text
    //-----------------------------------------

    SDL_Texture* txtCreate =
        TextRenderer::CreateText(
            renderer,
            "Create",
            color
        );

    if(txtCreate)
    {
        SDL_FRect pos =
        {
            390,
            412,
            100,
            25
        };

        SDL_RenderTexture(
            renderer,
            txtCreate,
            NULL,
            &pos
        );

        SDL_DestroyTexture(txtCreate);
    }
}

//-----------------------------------------
// Mouse Click
//-----------------------------------------
void HandleNewProjectClick(int x, int y)
{
    //-----------------------------------------
    // A3
    //-----------------------------------------
    if(x >= 120 && x <= 340 && y >= 220 && y <= 340) {
        selectedPageSize = 3;
        std::cout << "A3 Selected" << std::endl;
        return;
    }

    //-----------------------------------------
    // A4
    //-----------------------------------------
    if(x >= 550 && x <= 770 && y >= 220 && y <= 340) {
        selectedPageSize = 4;
        std::cout << "A4 Selected" << std::endl;
        return;
    }

    //-----------------------------------------
    // Create Button
    //-----------------------------------------
    if(x >= 350 && x <= 530 && y >= 400 && y <= 450)
    {
        if(selectedPageSize == 0) {
            std::cout << "Select Page Size" << std::endl;
            return;
        }

        if(projectName.empty()) {
            std::cout << "Enter Project Name" << std::endl;
            return;
        }

        //-------------------------------------
        // Create Project
        //-------------------------------------
        Project p;
        p.name = projectName;
        p.path = projectPath + projectName + "/";

        if(selectedPageSize == 3)
            p.pageSize = "A3";
        else
            p.pageSize = "A4";

        // ذخیره پروژه در فایل‌ها
        SaveProject(p);

        // تنظیم به عنوان پروژه فعلی
        currentProject = p;

        // *** قسمت مهم: پاک کردن صفحه ادیتور از قطعات پروژه قبلی ***
        if (editor) {
            editor->ClearWorkspace();
        }

        // توقف تایپ کیبورد
        SDL_StopTextInput(window);

        // نام پروژه بعد از ساخت پاک شود تا دفعه بعد خالی باشد (اختیاری)
        projectName = "";

        // رفتن به Editor
        CurrentPage = EDITOR_PAGE;

        std::cout << "Project Created & Saved : " << p.name << std::endl;
    }
}
//-----------------------------------------
// Keyboard
//-----------------------------------------

void HandleKeyboard(SDL_Event event)
{
    //-----------------------------------------
    // Text Input
    //-----------------------------------------

    if(event.type == SDL_EVENT_TEXT_INPUT)
    {
        projectName += event.text.text;

        std::cout
            << "Project Name : "
            << projectName
            << std::endl;
    }


    //-----------------------------------------
    // Backspace
    //-----------------------------------------

    if(event.type == SDL_EVENT_KEY_DOWN)
    {
        if(event.key.key == SDLK_BACKSPACE)
        {
            if(!projectName.empty())
            {
                projectName.pop_back();
            }
        }
    }
}
