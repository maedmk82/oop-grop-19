#include <SDL3/SDL.h>
#include "Page.h"
#include <iostream>
#include <SDL3_ttf/SDL_ttf.h>
#include "TextRenderer.h"
#include "Menu.h"
#include "NewProject.h"
#include "OpenProject.h"
Page CurrentPage = HOME_PAGE;
TTF_Font* font = nullptr;
SDL_Window* window = nullptr;
SDL_Texture* txtFile = nullptr;
SDL_Texture* txtNew = nullptr;
SDL_Texture* txtOpen = nullptr;
int selectedProjectIndex = -1;


//----------------------
// توابع
//----------------------
void DrawHomePage(SDL_Renderer* renderer);



int main(int argc,char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }
    if(!TTF_Init())
    {
        std::cout<<"TTF Error"<<std::endl;
        return -1;
    }
    TextRenderer::Init("Fonts/arial.ttf",18);
    LoadProjects();
    if(!font)
    {
        std::cout<<"Font Error"<<std::endl;
    }
    window = SDL_CreateWindow(
    "Proteus Clone",
    950,
    600,
    0);

    if (!window)
    {
        std::cout << "Window Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    if (!renderer)
    {
        std::cout << "Renderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    SDL_Color color = {0,0,0,255};
    txtFile = TextRenderer::CreateText(renderer,"File",color);
    txtNew  = TextRenderer::CreateText(renderer,"New Project",color);
    txtOpen = TextRenderer::CreateText(renderer,"Open Project",color);
    bool running=true;

    SDL_Event event;

    while(running)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_EVENT_QUIT)
                running=false;


            // کلیک موس
            if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                int x = event.button.x;
                int y = event.button.y;


                if(CurrentPage == HOME_PAGE)
                {
                    HandleMenuClick(x,y);


                    // کلیک روی لیست پروژه های اخیر

                    std::vector<Project> list = GetRecentProjects();


                    for(int i=0;i<list.size();i++)
                    {
                        int yPos = 120 + i*45;


                        if(x>=100 && x<=500 &&
                           y>=yPos && y<=yPos+35)
                        {

                            selectedProjectIndex=i;

                            CurrentPage=OPEN_PROJECT_PAGE;


                            std::cout<<"Opening Project : "
                                     <<list[i].name
                                     <<std::endl;

                            break;
                        }
                    }
                }


                else if(CurrentPage == NEW_PROJECT_PAGE)
                {
                    HandleNewProjectClick(x,y);
                }
                else if(CurrentPage == OPEN_PROJECT_PAGE)
                {
                    HandleOpenProjectClick(x,y);
                }
            }


            // تایپ کیبورد
            if(CurrentPage == NEW_PROJECT_PAGE)
            {
                HandleKeyboard(event);
            }
        }
        SDL_SetRenderDrawColor(renderer,225,225,225,255);
        SDL_RenderClear(renderer);



        // سپس صفحه جاری را رسم کن
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
    }

    // منو روی صفحه
    DrawMenu(renderer);


    SDL_RenderPresent(renderer);
        }
    SDL_DestroyTexture(txtFile);
    SDL_DestroyTexture(txtNew);
    SDL_DestroyTexture(txtOpen);

    TTF_CloseFont(font);

    TextRenderer::Close();

    SDL_DestroyRenderer(renderer);

    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}

//--------------------------------------------------

void DrawHomePage(SDL_Renderer* renderer)
{

    SDL_SetRenderDrawColor(renderer,
                           240,
                           240,
                           240,
                           255);


    SDL_FRect rect =
    {
        0,
        30,
        950,
        570
    };

    SDL_RenderFillRect(renderer,&rect);



    // عنوان

    SDL_Color color={0,0,0,255};


    SDL_Texture* title =
    TextRenderer::CreateText(renderer,
                             "Recent Projects",
                             color);



    if(title)
    {
        SDL_FRect pos=
        {
            100,
            70,
            200,
            25
        };


        SDL_RenderTexture(renderer,
                          title,
                          NULL,
                          &pos);


        SDL_DestroyTexture(title);
    }



    //-------------------------
    // نمایش 5 پروژه آخر
    //-------------------------


    std::vector<Project> list =
        GetRecentProjects();



    for(int i=0;i<list.size();i++)
    {


        SDL_SetRenderDrawColor(renderer,
                               210,
                               210,
                               210,
                               255);



        SDL_FRect box=
        {
            100,
            120+i*45,
            400,
            35
        };


        SDL_RenderFillRect(renderer,&box);



        SDL_Texture* txt =
        TextRenderer::CreateText(renderer,
                                 list[i].name.c_str(),
                                 color);



        if(txt)
        {

            SDL_FRect tpos=
            {
                120,
                130+i*45,
                250,
                20
            };


            SDL_RenderTexture(renderer,
                              txt,
                              NULL,
                              &tpos);



            SDL_DestroyTexture(txt);
        }

    }

}
void DrawOpenProjectPage(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer,180,180,230,255);

    SDL_FRect rect={20,50,910,520};

    SDL_RenderFillRect(renderer,&rect);
}
