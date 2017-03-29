#include <SDL2/SDL.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdint.h>


/* Use these aliases instead of static for clarity */
#define internal static
#define local_persist static
#define global_variable static


/* MAP_ANONYMOUS does not exist on mac os and some other unix systems
 * On those systems MAP_ANON usually works
 */
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;


global_variable SDL_Texture *Texture;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

global_variable bool Running;

/*DISPLAY TEST GRADIENT FOR RENDERING*/
internal void
RenderWeirdGradient(int BlueOffset, int GreenOffset)
{
    int Width = BitmapWidth;
    int Height = BitmapHeight;

    int Pitch = Width * BytesPerPixel;
    uint8 *Row = (uint8 *)BitmapMemory; // Start at beginning of Bitmap

    for(int Y = 0; Y < BitmapHeight; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for(int X = 0; X < BitmapWidth; ++X)
        {
            uint8 Blue = (X + BlueOffset);
            uint8 Green = (Y + GreenOffset);

            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Pitch;
    }
}


internal void
SDLResizeTexture(SDL_Renderer *Renderer, int Width, int Height)
{
    /* 
     * Free up the memory being used by the texture before creating a 
     * new one with the proper size.
     */
    if (BitmapMemory)
    {
        munmap(BitmapMemory,
              BitmapWidth * BitmapHeight * BytesPerPixel);
    }
    if (Texture)
    {
        SDL_DestroyTexture(Texture);
    }
    Texture = SDL_CreateTexture(Renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                Width,
                                Height);
    BitmapWidth = Width;
    BitmapHeight = Height;
    /*
     * mmap arguments:
     *
     * start: address of beginning of memory block (0 means we don't care)
     *
     * length: this is how many bytes we wish to reserve in memory
     * 
     * prot: Memory protection. In our case we want RW permissions
     * 
     * flags: We are using anonymous mapping and we want our memory to be
     * accessible only to our process
     *
     * fd: file descriptor. Val -1 since we are not mapping a file into memory
     *
     * offset: Where in the file to begin mapping. 0 Since again we aren't
     * using mmap to map a chunk of a file into memory.
     */
    BitmapMemory = mmap(0,
                        Width * Height * BytesPerPixel,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1,
                        0);
}


internal void
SDLUpdateWindow(SDL_Window *Window, SDL_Renderer *Renderer)
{
    SDL_UpdateTexture(Texture,
                      0,                            // Pointer to SDL_Rect used for updating texture one rectangle at a time
                      BitmapMemory,                 // Pointer to Bitmap data
                      BitmapWidth * BytesPerPixel); // Pitch used for creating gaps in memory between horizontal lines of the bitmap 

    /*
     * NOTE: setting srcrect and dstrect to null pointers will stretch the
     * entire texture over the entire window 
     */
    SDL_RenderCopy(Renderer,
                    Texture,
                    0,             // (srcrect) Source Rectangle delimiting area of Texture we wish to stretch
                    0);            // (dstrect) Destination Rectangle delimiting area of Window the Texture selection will be stretched into

    /*NOTE: Without presenting the renderer, nothing gets rendered */
    SDL_RenderPresent(Renderer);
}


void HandleEvent(SDL_Event *Event)
{

    switch(Event->type)
    {
        case SDL_QUIT:
        {
            printf("SDL_QUIT\n");
            Running = false;
        }
        break;

        case SDL_WINDOWEVENT:
        {
            switch(Event->window.event)
            {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                {
                    SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
                    SDL_Renderer *Renderer = SDL_GetRenderer(Window);
                    printf("SDL_WINDOWEVENT_SIZE_CHANGED (%d, %d)\n",
                        Event->window.data1, // width 
                        Event->window.data2); // height
                    SDLResizeTexture(Renderer, Event->window.data1, Event->window.data2);
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
                    SDL_Window *Window = SDL_GetWindowFromID(Event->window.windowID);
                    SDL_Renderer *Renderer = SDL_GetRenderer(Window);
                    SDLUpdateWindow(Window, Renderer);
                }
                break;
            }
        }
        break;
    }
}

int main(int argc, char *argv[])
{
    /*Initialize Graphics*/
    SDL_Init(SDL_INIT_VIDEO);

    /*Create window*/
    SDL_Window *Window = SDL_CreateWindow("scratchapixel",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          640,
                                          480,
                                          SDL_WINDOW_RESIZABLE);
    
    if (Window)
    {
        /*Create a renderer for our window*/
        SDL_Renderer *Renderer = SDL_CreateRenderer(Window,
                                                    -1, // index of underlying driver. -1 => autodetect
                                                    0); // SDL_RendererFlags

        if (Renderer)
        {
            Running = true;
            int Width, Height;
            SDL_GetWindowSize(Window, &Width, &Height);
            SDLResizeTexture(Renderer, Width, Height);
            int XOffset = 0;
            int YOffset = 0;

            while(Running)
            {
                SDL_Event Event;
                //NOTE: SDL_WaitEvent blocks so instead we use PollEvent
                while(SDL_PollEvent(&Event))
                {
                    HandleEvent(&Event);
                }
                
                RenderWeirdGradient(XOffset, YOffset);
                SDLUpdateWindow(Window, Renderer);

                ++XOffset;
                YOffset += 2;
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
