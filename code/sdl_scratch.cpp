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

#define MAX_CONTROLLERS 4

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct sdl_offscreen_buffer
{
    SDL_Texture *Texture;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct sdl_window_dimension
{
    int Width;
    int Height;
};

global_variable sdl_offscreen_buffer GlobalBackbuffer;

SDL_GameController *ControllerHandles[MAX_CONTROLLERS];
SDL_Haptic *RumbleHandles[MAX_CONTROLLERS];


sdl_window_dimension
SDLGetWindowDimension(SDL_Window *Window)
{
    sdl_window_dimension Result;
    SDL_GetWindowSize(Window, &Result.Width, &Result.Height);

    return(Result);
}

/*DISPLAY TEST GRADIENT FOR RENDERING*/
internal void
RenderWeirdGradient(sdl_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
    /* NOTE(Alex):
     * Typeset to make pointer arithmetic simple: C multiplies pointer by size of thing being pointed to
     * This way, when we say Row += Pitch, it is equivalent to saying
     * 
     * Row += Width * BytesPerPixel * SizeOf(uint8 *)
     * 
     * Since uint8 has a size of 8 bits(aka 1 byte), and we only need 1 byte
     * per pixel component (4 bytes for one RGBX pixel),we end up with a 
     * nice intuitive for-loop.
     */
    uint8 *Row = (uint8 *)Buffer->Memory;
    for(int Y = 0; Y < Buffer->Height; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row; // One Pixel is 4x8 bits large
        for(int X = 0; X < Buffer->Width; ++X)
        {
            uint8 Blue = (X + BlueOffset);
            uint8 Green = (Y + GreenOffset);
            uint8 Red = 0;

            /*NOTE (Alex): 
             * x << y means shift all bits in x left by y bits 
             * x | y  means each bit in x OR each bit in y
             *
             * In our case:
             * 
             * 32-bit Pixel
             *
             * MEMORY ORDER WE EXPECT:                RR  GG  BB  XX
             * LOADED IN (LITTLE ENDIAN):             XX  BB  GG  RR
             * WINDOWS WANTED:                        XX  RR  GG  BB
             * MEMORY ORDER WINDOWS:                  BB  GG  RR  XX
             * WINDOWS ORDER LOADED (LITTLE ENDIAN):  XX  RR  GG  BB
             * 
             * RESULT: Windows got what they wanted and now I'm sad
             */

            *Pixel = ((Red << 16) | (Green << 8) | Blue);
            Pixel++; //NOTE(Alex): I separated the pointer increment from value assignment to make it easier to debug this with GDB
        }

        Row += Buffer->Pitch; // NOTE(Alex): We do this in case Pitch does not end up lining up    
    }
}

/* NOTE(Alex):
 * Here we pass Buffer as a pointer, because we will need to modify its
 * contents
 */
internal void
SDLResizeTexture(sdl_offscreen_buffer *Buffer, SDL_Renderer *Renderer, int Width, int Height)
{
    int BytesPerPixel = 4;

    /* 
     * Free up the memory being used by the texture before creating a 
     * new one with the proper size.
     */
    if (Buffer->Memory)
    {
        munmap(Buffer->Memory,
              Buffer->Width * Buffer->Height * BytesPerPixel);
    }
    if (Buffer->Texture)
    {
        SDL_DestroyTexture(Buffer->Texture);
    }
    Buffer->Texture = SDL_CreateTexture(Renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                Width,
                                Height);
    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->Pitch = Width * BytesPerPixel;
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
    Buffer->Memory = mmap(0,
                        Width * Height * BytesPerPixel,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1,
                        0);
}


internal void
SDLUpdateWindow(SDL_Window *Window, SDL_Renderer *Renderer, sdl_offscreen_buffer *Buffer)
{
    SDL_UpdateTexture(Buffer->Texture,
                      0,                  // Pointer to SDL_Rect used for updating texture one rectangle at a time
                      Buffer->Memory,     // Pointer to Bitmap data
                      Buffer->Pitch);     // Pitch used for creating gaps in memory between horizontal lines of the bitmap 

    /*
     * NOTE (Alex): setting srcrect and dstrect to null pointers will stretch the
     * entire texture over the entire window 
     */
    SDL_RenderCopy(Renderer,
                    Buffer->Texture,
                    0,             // (srcrect) Source Rectangle delimiting area of Texture we wish to stretch
                    0);            // (dstrect) Destination Rectangle delimiting area of Window the Texture selection will be stretched into

    /*NOTE (Alex): Without presenting the renderer, nothing gets rendered */
    SDL_RenderPresent(Renderer);
}


bool HandleEvent(SDL_Event *Event)
{
    bool ShouldQuit = false;
    
    switch(Event->type)
    {
        case SDL_QUIT:
        {
            printf("SDL_QUIT\n");
            ShouldQuit = true;
        }
        break;
        
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            SDL_Keycode KeyCode = Event->key.keysym.sym;
            /*
             * Figure out if the key was pressed, released, or repeated:
             * 
             * NOTE(ALEX):
             * - If Event->key.state == SDL_RELEASED then the key was down
             * - If Event->type == SDL_KEYUP then the key was down
             * - If Event->key.state == SDL_PRESSED and Event->key.repeat != 0,
             *   then the key was also down
             * - Otherwise, the key was not down
             */
            bool Pressed = (Event->key.state == SDL_PRESSED);
            bool Released = (Event->key.state == SDL_RELEASED);
            bool Repeat = (Event->key.repeat);
            
            bool IsDown = Pressed;
            bool WasDown = Released || (Pressed && Repeat);

            if (!Repeat)
            {
                if (KeyCode == SDLK_w)
                {
                }
                else if (KeyCode == SDLK_a)
                {
                }
                else if (KeyCode == SDLK_s)
                {
                }
                else if (KeyCode == SDLK_d)
                {
                }
                else if (KeyCode == SDLK_q)
                {
                }
                else if (KeyCode == SDLK_e)
                {
                }
                else if (KeyCode == SDLK_UP)
                {
                }
                else if (KeyCode == SDLK_DOWN)
                {
                }
                else if (KeyCode == SDLK_LEFT)
                {
                }
                else if (KeyCode == SDLK_RIGHT)
                {
                }
                else if (KeyCode == SDLK_ESCAPE)
                {
                    printf("ESCAPE: ");
                    if (IsDown)
                    {
                        printf("IsDown ");
                    }
                    if (WasDown)
                    {
                        printf("WasDown");
                    }
                    printf("\n");
                }

                else if (KeyCode == SDLK_SPACE)
                {
                }
            }
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
                    SDLResizeTexture(&GlobalBackbuffer, Renderer, Event->window.data1, Event->window.data2);
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
                    SDLUpdateWindow(Window, Renderer, &GlobalBackbuffer);
                }
                break;
            }
        }
        break;
    }
    return(ShouldQuit);
}

internal void
SDLOpenGameControllers()
{
    int MaxJoysticks = SDL_NumJoysticks();
    int ControllerIndex = 0;

    for (int JoystickIndex = 0; JoystickIndex < MaxJoysticks; ++JoystickIndex)
    {
        if (!SDL_IsGameController(JoystickIndex))
        {
            continue;
        }
        if (ControllerIndex >= MAX_CONTROLLERS)
        {
            break;
        }

        ControllerHandles[ControllerIndex] = SDL_GameControllerOpen(JoystickIndex);
        SDL_Joystick *JoystickHandle = SDL_GameControllerGetJoystick(ControllerHandles[ControllerIndex]);
        RumbleHandles[ControllerIndex] = SDL_HapticOpenFromJoystick(JoystickHandle);
        
        if (SDL_HapticRumbleInit(RumbleHandles[ControllerIndex]) != 0)
        {
            SDL_HapticClose(RumbleHandles[ControllerIndex]);
            RumbleHandles[ControllerIndex] = 0;
        }

        ControllerIndex++;
    }
}

internal void
SDLCloseGameControllers()
{
    for (int ControllerIndex = 0; ControllerIndex < MAX_CONTROLLERS; ++ControllerIndex)
    {
        if (ControllerHandles[ControllerIndex])
        {
            if (RumbleHandles[ControllerIndex])
            {
                SDL_HapticClose(RumbleHandles[ControllerIndex]);
            }
            SDL_GameControllerClose(ControllerHandles[ControllerIndex]);
        }
    }
}


int main(int argc, char *argv[])
{
    /*Initialize Graphics and Controllers*/
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
    
    SDLOpenGameControllers();

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
            bool Running = true;
            sdl_window_dimension Dimension = SDLGetWindowDimension(Window);
            SDLResizeTexture(&GlobalBackbuffer, Renderer, Dimension.Width, Dimension.Height);
            int XOffset = 0;
            int YOffset = 0;

            while (Running)
            {
                SDL_Event Event;
                //NOTE: SDL_WaitEvent blocks so instead we use PollEvent
                while(SDL_PollEvent(&Event))
                {
                   bool ShouldQuit = HandleEvent(&Event);
                   if (ShouldQuit)
                   {
                       Running = false;
                   }
                }

                // Poll Controllers for input
              	for (int ControllerIndex = 0;
                		ControllerIndex < MAX_CONTROLLERS;
                    ++ControllerIndex)
                {
                		if(ControllerHandles[ControllerIndex] != 0 && SDL_GameControllerGetAttached(ControllerHandles[ControllerIndex]))
                    {
                        // NOTE: We have a controller with index ControllerIndex.
                        bool Up = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_UP);
                        bool Down = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_DOWN);
                        bool Left = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_LEFT);
                        bool Right = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
                        bool Start = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_START);
                        bool Back = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_BACK);
                        bool LeftShoulder = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
                        bool RightShoulder = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
                        bool AButton = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_A);
                        bool BButton = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_B);
                        bool XButton = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_X);
                        bool YButton = SDL_GameControllerGetButton(ControllerHandles[ControllerIndex], SDL_CONTROLLER_BUTTON_Y);

                        int16 StickX = SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_LEFTX);
                        int16 StickY = SDL_GameControllerGetAxis(ControllerHandles[ControllerIndex], SDL_CONTROLLER_AXIS_LEFTY);
                       	
												XOffset = StickX;
												YOffset = StickY;
 
                        if (AButton)
                        {
                            YOffset += 2;
                        }
                        
												if (BButton)
                        {
                            if (RumbleHandles[ControllerIndex])
                            {
                                SDL_HapticRumblePlay(RumbleHandles[ControllerIndex], 0.5f, 2000);
                            }
                        }

                    }
                    else
                    {
                        // TODO: This controller is not plugged in.
                    }
                } 
                
                RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);
                SDLUpdateWindow(Window, Renderer, &GlobalBackbuffer);

                ++XOffset;
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
		
		SDLCloseGameControllers();
    SDL_Quit();
    return(0);
}
