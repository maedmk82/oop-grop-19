#include "NewProject.h"
#include "ProjectManager.h"
#include "TextRenderer.h"

#include <iostream>
#include <string>


std::string projectName="";

std::string projectPath="Projects/";

int selectedPageSize=0;


//---------------------------------

void DrawNewProject(SDL_Renderer* renderer)
{

    SDL_Color color={0,0,0,255};


    //------------------------
    // Background
    //------------------------

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



    //------------------------
    // Name Box
    //------------------------

    SDL_SetRenderDrawColor(renderer,
                           255,
                           255,
                           255,
                           255);


    SDL_FRect nameBox =
    {
        250,
        100,
        450,
        40
    };


    SDL_RenderFillRect(renderer,&nameBox);



    SDL_Texture* nameTexture =
        TextRenderer::CreateText(renderer,
                                 projectName.c_str(),
                                 color);


    if(nameTexture)
    {

        SDL_FRect pos =
        {
            260,
            110,
            300,
            25
        };


        SDL_RenderTexture(renderer,
                          nameTexture,
                          NULL,
                          &pos);


        SDL_DestroyTexture(nameTexture);
    }



    //------------------------
    // Path Box
    //------------------------


    SDL_FRect pathBox =
    {
        250,
        150,
        450,
        40
    };


    SDL_RenderFillRect(renderer,&pathBox);



    SDL_Texture* pathTexture =
        TextRenderer::CreateText(renderer,
                                 projectPath.c_str(),
                                 color);


    if(pathTexture)
    {

        SDL_FRect pos =
        {
            260,
            160,
            300,
            25
        };


        SDL_RenderTexture(renderer,
                          pathTexture,
                          NULL,
                          &pos);


        SDL_DestroyTexture(pathTexture);
    }




    //------------------------
    // A3 Button
    //------------------------

    if(selectedPageSize==3)
        SDL_SetRenderDrawColor(renderer,
                               100,
                               200,
                               100,
                               255);
    else
        SDL_SetRenderDrawColor(renderer,
                               180,
                               220,
                               180,
                               255);


    SDL_FRect A3 =
    {
        150,
        220,
        220,
        120
    };


    SDL_RenderFillRect(renderer,&A3);



    //------------------------
    // A4 Button
    //------------------------


    if(selectedPageSize==4)
        SDL_SetRenderDrawColor(renderer,
                               100,
                               200,
                               100,
                               255);
    else
        SDL_SetRenderDrawColor(renderer,
                               180,
                               180,
                               230,
                               255);



    SDL_FRect A4 =
    {
        580,
        220,
        220,
        120
    };


    SDL_RenderFillRect(renderer,&A4);




    //------------------------
    // Create Button
    //------------------------

    SDL_SetRenderDrawColor(renderer,
                           200,
                           180,
                           80,
                           255);


    SDL_FRect create =
    {
        380,
        400,
        180,
        50
    };


    SDL_RenderFillRect(renderer,&create);




    //------------------------
    // Text
    //------------------------


    SDL_Texture* txtA3 =
        TextRenderer::CreateText(renderer,
                                 "A3",
                                 color);


    SDL_Texture* txtA4 =
        TextRenderer::CreateText(renderer,
                                 "A4",
                                 color);



    SDL_Texture* txtCreate =
        TextRenderer::CreateText(renderer,
                                 "Create",
                                 color);



    if(txtA3)
    {

        SDL_FRect pos =
        {
            240,
            260,
            40,
            25
        };


        SDL_RenderTexture(renderer,
                          txtA3,
                          NULL,
                          &pos);


        SDL_DestroyTexture(txtA3);
    }



    if(txtA4)
    {

        SDL_FRect pos =
        {
            670,
            260,
            40,
            25
        };


        SDL_RenderTexture(renderer,
                          txtA4,
                          NULL,
                          &pos);


        SDL_DestroyTexture(txtA4);
    }



    if(txtCreate)
    {

        SDL_FRect pos =
        {
            430,
            415,
            90,
            25
        };


        SDL_RenderTexture(renderer,
                          txtCreate,
                          NULL,
                          &pos);


        SDL_DestroyTexture(txtCreate);
    }

}


//---------------------------------

void HandleNewProjectClick(int x,int y)
{


    // A3

    if(x>=150 && x<=370 &&
       y>=220 && y<=340)
    {

        selectedPageSize=3;

        std::cout<<"A3 Selected\n";
    }



    // A4

    if(x>=580 && x<=800 &&
       y>=220 && y<=340)
    {

        selectedPageSize=4;

        std::cout<<"A4 Selected\n";
    }



    // Create

    if(x>=380 && x<=560 &&
       y>=400 && y<=450)
    {


        if(selectedPageSize==0)
        {

            std::cout<<"Select Page Size\n";

        }

        else if(projectName=="")
        {

            std::cout<<"Enter Project Name\n";

        }

        else
        {

            Project p;


            p.name=projectName;

            p.path=projectPath;

            p.pageSize=selectedPageSize;



            SaveProject(p);



            std::cout<<"Project Saved : "
                     <<p.name
                     <<std::endl;

        }

    }

}


//---------------------------------

void HandleKeyboard(SDL_Event event)
{


    if(event.type==SDL_EVENT_TEXT_INPUT)
    {

        projectName += event.text.text;

    }



    if(event.type==SDL_EVENT_KEY_DOWN)
    {

        if(event.key.key==SDLK_BACKSPACE)
        {

            if(projectName.length()>0)
                projectName.pop_back();

        }

    }

}
