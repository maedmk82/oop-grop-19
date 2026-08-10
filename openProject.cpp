#include "OpenProject.h"
#include "TextRenderer.h"
#include <iostream>


std::vector<Project> openProjects;


//--------------------------------
// خواندن پروژه های ذخیره شده
//--------------------------------

void LoadOpenProjects()
{
    openProjects = LoadProjects();
}



//--------------------------------
// رسم صفحه Open Project
//--------------------------------

void DrawOpenProject(SDL_Renderer* renderer)
{

    // پس زمینه

    SDL_SetRenderDrawColor(renderer,
                           230,
                           230,
                           230,
                           255);


    SDL_FRect background =
    {
        50,
        60,
        850,
        450
    };


    SDL_RenderFillRect(renderer,&background);



    SDL_Color color={0,0,0,255};



    // عنوان

    SDL_Texture* title =
        TextRenderer::CreateText(renderer,
                                 "Open Project",
                                 color);



    if(title)
    {
        SDL_FRect pos =
        {
            80,
            80,
            180,
            30
        };


        SDL_RenderTexture(renderer,
                          title,
                          NULL,
                          &pos);


        SDL_DestroyTexture(title);
    }



    //--------------------------------
    // نمایش لیست پروژه ها
    //--------------------------------


    int y = 140;


    for(int i=0;i<openProjects.size();i++)
    {

        // کادر پروژه

        SDL_SetRenderDrawColor(renderer,
                               255,
                               255,
                               255,
                               255);


        SDL_FRect box =
        {
            100,
            (float)y,
            700,
            50
        };


        SDL_RenderFillRect(renderer,&box);



        // متن نام پروژه

        SDL_Texture* name =
            TextRenderer::CreateText(renderer,
                                     openProjects[i].name.c_str(),
                                     color);



        if(name)
        {

            SDL_FRect textPos =
            {
                120,
                (float)y+15,
                250,
                25
            };


            SDL_RenderTexture(renderer,
                              name,
                              NULL,
                              &textPos);


            SDL_DestroyTexture(name);

        }



        // سایز صفحه

        std::string size;


        if(openProjects[i].pageSize==3)
            size="A3";
        else
            size="A4";



        SDL_Texture* sizeText =
            TextRenderer::CreateText(renderer,
                                     size.c_str(),
                                     color);



        if(sizeText)
        {

            SDL_FRect sizePos =
            {
                450,
                (float)y+15,
                80,
                25
            };


            SDL_RenderTexture(renderer,
                              sizeText,
                              NULL,
                              &sizePos);


            SDL_DestroyTexture(sizeText);

        }



        y+=60;

    }

}



//--------------------------------
// کلیک روی پروژه
//--------------------------------

void HandleOpenProjectClick(int x,int y)
{

    int startY=140;


    for(int i=0;i<openProjects.size();i++)
    {

        if(x>=100 && x<=800 &&
           y>=startY &&
           y<=startY+50)
        {

            std::cout<<"Open Project : "
                     <<openProjects[i].name
                     <<std::endl;


            // اینجا بعداً صفحه شماتیک باز می‌شود


            return;

        }


        startY+=60;

    }

}
