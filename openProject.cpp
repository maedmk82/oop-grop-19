#include "OpenProject.h"
#include "ProjectManager.h"
#include "TextRenderer.h"
#include "Page.h"

#include <SDL3/SDL.h>
#include <iostream>
#include <vector>

// این متغیرها در main.cpp تعریف شده‌اند
extern int selectedProjectIndex;
extern Project currentProject;
extern Page CurrentPage;


//--------------------------------------------------
// Draw Open Project
//--------------------------------------------------

void DrawOpenProject(SDL_Renderer* renderer)
{
    SDL_Color color =
    {
        0,
        0,
        0,
        255
    };


    //--------------------------------
    // Background
    //--------------------------------

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
            "Open Project",
            color
        );

    if(title)
    {
        SDL_FRect pos =
        {
            80,
            50,
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
    // Recent Projects
    //--------------------------------

    std::vector<Project> list =
        GetRecentProjects();


    //--------------------------------
    // نمایش حداکثر 10 پروژه
    //--------------------------------

    for(int i = 0;
        i < (int)list.size() && i < 10;
        i++)
    {
        float y = 120 + i * 45;


        //--------------------------------
        // Project Box
        //--------------------------------

        SDL_SetRenderDrawColor(
            renderer,
            210,
            210,
            210,
            255
        );

        SDL_FRect box =
        {
            100,
            y,
            500,
            35
        };

        SDL_RenderFillRect(
            renderer,
            &box
        );


        //--------------------------------
        // Project Name
        //--------------------------------

        SDL_Texture* txt =
            TextRenderer::CreateText(
                renderer,
                list[i].name.c_str(),
                color
            );

        if(txt)
        {
            SDL_FRect pos =
            {
                115,
                y + 7,
                350,
                22
            };

            SDL_RenderTexture(
                renderer,
                txt,
                NULL,
                &pos
            );

            SDL_DestroyTexture(txt);
        }
    }
}


//--------------------------------------------------
// Click on Project
//--------------------------------------------------

void HandleOpenProjectClick(int x, int y)
{
    std::vector<Project> projects =
        GetRecentProjects();


    for(int i = 0;
        i < (int)projects.size() && i < 10;
        i++)
    {
        // باید دقیقاً با DrawOpenProject یکی باشد
        float yPos =
            120 + i * 45;


        //--------------------------------
        // آیا روی پروژه کلیک شده؟
        //--------------------------------

        if(x >= 100 &&
           x <= 600 &&
           y >= yPos &&
           y <= yPos + 35)
        {
            //--------------------------------
            // ذخیره پروژه انتخاب شده
            //--------------------------------

            selectedProjectIndex = i;

            currentProject = projects[i];


            //--------------------------------
            // باز کردن پروژه
            //--------------------------------

            if(OpenProject(currentProject))
            {
                CurrentPage = EDITOR_PAGE;
            }


            std::cout
                << "Opening Project : "
                << currentProject.name
                << std::endl;


            return;
        }
    }
}
