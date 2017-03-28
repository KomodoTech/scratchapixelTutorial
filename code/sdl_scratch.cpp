#include <SDL2/SDL.h>
#include <stdio.h>

bool HandleEvent(SDL_Event *event)
{
    bool shouldQuit = false;

    switch(event->type)
    {
        case SDL_QUIT:
        {
            printf("SDL_QUIT\n");
            shouldQuit = true;
        }
        break;

        case SDL_WINDOWEVENT:
        {
            switch(event->window.event)
            {
                case SDL_WINDOWEVENT_RESIZED:
                {
                    printf("SDL_WINDOWEVENT_RESIZED (%d, %d)\n",
                        event->window.data1, // width 
                        event->window.data2); // height
                }
                break;

                case SDL_WINDOWEVENT_FOCUS_GAINED:
                {
                    printf("SDL_WINDOWEVENT_FOCUSED_GAIN\n");
                }
                break;

                /* Check if window needs to be redrawn */
                case SDL_WINDOWEVENT_EXPOSED:
                {
                    SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
                    SDL_Renderer *renderer = SDL_GetRenderer(window);

                    static bool isWhite = true;
                    if (isWhite == true)
                    {
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                        isWhite = false;
                    }
                    else
                    {
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                        isWhite = true;
                    }

                    SDL_RenderClear(renderer);
                    SDL_RenderPresent(renderer);
                }
                break;
            }
        }
        break;
    }
    return(shouldQuit);
}

int main(int argc, char *argv[])
{
    /*Initialize Graphics*/
    SDL_Init(SDL_INIT_VIDEO);

    /*Create window*/
    SDL_Window *window = SDL_CreateWindow("scratchapixel",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          640,
                                          480,
                                          SDL_WINDOW_RESIZABLE);
    
    if (window)
    {
        /*Create a renderer for our window*/
        SDL_Renderer *renderer = SDL_CreateRenderer(window,
                                                    -1, // index of underlying driver. -1 => autodetect
                                                    0); // SDL_RendererFlags

        if (renderer)
        {
            for(;;)
            {
                SDL_Event event;
                SDL_WaitEvent(&event);

                /* HandleEvent returns bool shouldQuit */
                if (HandleEvent(&event))
                {
                    /* Break out of game loop */
                    break;
                }
            }
        }
        else
        {
            //TODO: logging no renderer
        }
    }
    else
    {
        //TODO: logging no window
    }

    SDL_Quit();
    return(0);
}
