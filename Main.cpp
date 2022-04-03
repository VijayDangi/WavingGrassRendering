/*
 * Waving Grass Rendering Using OpenGL-OpenCL (Performance Analysis)
 *
 * Created By Vijaykumar Dangi
 */

//Header
#include "Main.h"
#include <chrono>

//OpenCL API
#include <CL/opencl.h>

#include "Camera.h"
#include "ArcCamera.h"
#include "Resource.h"
#include "FreeType2DText.h"

//Library
#pragma comment( lib, "User32.lib")
#pragma comment( lib, "Gdi32.lib")
#pragma comment( lib, "Winmm.lib")

#pragma comment( lib, "glew32.lib")
#pragma comment( lib, "OpenGL32.lib")
#pragma comment( lib, "OpenCL.lib")

//macro
#define  MAX_MESH_SIZE          1024
#define  MIN_MESH_SIZE          2
#define  MESH_MULTIPLICANT      0.1f
#define  MESH_AMPLITUDE         5.0f
#define  GRASS_BLADE_SEGMENTS   12
#define  MSAA_SAMPLES           4
#define  COLOR_CHANNELS         4

#define  USE_FREE_CAMERA  0
#define  USE_ARC_CAMERA   1
#define  DEBUG            1

//global function declarations
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM);

//global variable declaration
FILE *gpLogFile = NULL;
HWND  ghwnd;
HDC   ghdc;
HGLRC ghrc;

DWORD style;
WINDOWPLACEMENT wpPrev = { sizeof(WINDOWPLACEMENT) };

int g_windowWidth = 800;
int g_windowHeight = 600;

bool gbDone = false;
bool gbActiveWindow = false;
bool gbIsEscapeKeyPressed = false;
bool gbFullScreen = false;

//Keyboard state
unsigned char g_keyboardState[256];

//Shader Program
GLuint program_grass;
GLuint program_display;
GLuint program_hdr_display;
GLuint program_alpha;
GLuint program_light;

//Mesh data
typedef struct VERTEX
{
    float position[3];
    float normal[3];
    float tangent[3];
    float texcoord[2];
}VERTEX;

typedef struct GRASS_VERTEX
{
    float position[3];
    float normal[3];
    float texcoord[2];
}GRASS_VERTEX;

typedef struct GRASS_STATIC_PROPERTIES
{
    vmath::mat4 tangentToLocalMatrix;
    vmath::mat4 facingRotationMatrix;
    vmath::mat4 bendRotationMatrix;
    float width;
    float height;
    float forward;
} GRASS_STATIC_PROPERTIES;

VERTEX meshVertexData[MAX_MESH_SIZE * MAX_MESH_SIZE];
GRASS_STATIC_PROPERTIES *grassStaticProps_cpu = NULL;

GLuint vbo_element_common;

GLuint vao_grass_cpu;
GLuint vbo_grassBuffer_cpu;

GLuint vao_grass_opencl;
GLuint vbo_grassBuffer_opencl;

GLuint vao_quad;
GLuint vbo_quad;

GLuint vao_light;
GLuint vbo_light;

int grassVerticesCount = 0;
int grassIndicesCount = 0;

int currentMeshWidth = MIN_MESH_SIZE;
int currentMeshHeight = MIN_MESH_SIZE;

//wind distortion map (dudv map)
IMAGE_DATA windDistortion_map;

GLuint grassBladeTexture;
GLuint grassBladeAlphaTexture;
GLuint groundTexture;
GLuint groundAlphaTexture;
GLuint waterMarkTexture;

float deltaTime;

FRAMEBUFFER msaaFramebuffer;
FRAMEBUFFER sceneFramebuffer;

//Camera
#if USE_FREE_CAMERA
Camera *g_camera = new Camera( 1.0f, 0.3f, vmath::vec3( 0.0f, 0.0f, 0.0f), vmath::vec3( 0.0f, 1.0f, 0.0f));
#elif USE_ARC_CAMERA
ArcBallCamera g_arcCamera( vmath::vec3( 0.0f), vmath::vec3( 0.0f), 15.0f, 0.0f, 0.0f, 0.01f);
#endif

//==========================
vmath::mat4 projection_matrix;

//OpenCL Related Variables
cl_int            clResult;
cl_mem            cl_graphics_resource_mesh;
cl_device_id      oclComputeDeviceId;
cl_context        oclContext;
cl_command_queue  oclCommandQueue;
cl_program        oclGrassProgram;
cl_kernel         oclGrassKernel;

cl_mem meshVertexData_opencl_input = NULL;
cl_mem distortionMap_opencl_input = NULL;

const char grassOpenCLFileName[] = "Grass.cl";
const char grassKernelName[] = "grass_kernel";

bool bOnGPU = false;
bool bNeedToUpdateBuffers = true;

float x = 0.0f, y = 0.0f, z = 0.0f;
float step = 1.0f;

struct Light
{
    vmath::vec3 position;
    vmath::vec3 color;
    vmath::vec3 attenuation;
} gLights[] = {
    {
        vmath::vec3( 0.0f, 2.0f, 0.0f),
        vmath::vec3( 1.0f, 1.0f, 1.0f),
        vmath::vec3( 0.29f, -0.21f, 0.122f)
    },
    {
        vmath::vec3( 7.0f, 2.0f, 7.0f),
        vmath::vec3( 0.8f, 0.0f, 0.0f),
        // vmath::vec3( 0.01f, 0.01f, 0.042f)
        vmath::vec3( 0.17f, 0.01f, 0.042f)
    },
    {
        vmath::vec3( -7.0f, 2.0f, 7.0f),
        // vmath::vec3( 0.0f, 0.7f, 0.0f),
        vmath::vec3( 0.5f, 0.5f, 0.2f),
        vmath::vec3( 0.12f, 0.042f, 0.01f)
    },
    {
        vmath::vec3( 7.0f, 2.0f, -7.0f),
        // vmath::vec3( 0.0f, 0.0f, 1.0f),
        vmath::vec3( 0.24f, 0.3f, 0.88f),
        // vmath::vec3( 1.0f, 0.09f, 0.032f)
        vmath::vec3( 0.019f, 0.07f, 0.042f)
    },
    {
        vmath::vec3( -7.0f, 2.0f, -7.0f),
        vmath::vec3( 1.0f, 0.0f, 1.0f),
        vmath::vec3( 1.0f, 0.09f, 0.032f)
    },
};

int currentIndex = 0;

//for FPS calculation
int currentFPS = 0;
double timePerFrame = 0.0;
double deltaTime_fps = 0.0;
std::chrono::high_resolution_clock::time_point lastTime;
std::chrono::high_resolution_clock::time_point currentTime;
char stringMessage[512];

bool gbEnableMSAA = false;
bool gbEnableLight = false;

float lightAngle = 0.0f;
float lightRotateRadius = 2.0f;

enum SCENE
{
    INITIAL_SCENE,
    GRASS_SCENE,
    CREDIT_SCENE,
    END_SCENE
};

int currentScene = INITIAL_SCENE;
float alpha_dt = 0.0f;

bool fadeIn = true;
bool fadeOut = false;

FreeTypeFont *NotoSerifBoldFreeTypeFont = NULL;
FreeTypeFont *TahomaFreeTypeFont = NULL;

//
//WinMain()
//
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow)
{
    //function delcaration
    int Initialize( void);
    void Update( void);
    void Display( void);
    void Uninitialize( void);

    //variable declaration
    WNDCLASSEX wndclass;
    HWND       hwnd;
    MSG        msg;
    TCHAR      szClassName[] = TEXT("OpenGL_PP");

    //code
    if( fopen_s( &gpLogFile, "LogFile.log", "w") != 0)
    {
        MessageBox( NULL, TEXT("Error while creating Log file"), TEXT("Error"), MB_OK | MB_TOPMOST | MB_ICONSTOP);
        return( 1);
    }
    
    fprintf( gpLogFile, "Log File Opened\n");

    //initialize window attributes
    wndclass.cbSize        = sizeof( WNDCLASSEX);
    wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_VJD_ICON));
    wndclass.hIconSm       = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_VJD_ICON));
    wndclass.hCursor       = LoadCursor( NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) GetStockObject( BLACK_BRUSH);
    wndclass.lpszClassName = szClassName;
    wndclass.lpszMenuName  = NULL;
    wndclass.lpfnWndProc   = WndProc;

    if( !RegisterClassEx( &wndclass))
    {
        fprintf( gpLogFile, "Wndclass Cannot be Registred\n");
        fclose( gpLogFile);
        gpLogFile = NULL;
        return( 1);
    }

    hwnd = CreateWindowEx(
                WS_EX_APPWINDOW,
                szClassName,
                TEXT("OpenGL Programmable Pipeline :- Grass ( MSAA :- Disable)"),
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
                ( GetSystemMetrics( SM_CXSCREEN) - g_windowWidth) / 2,
                ( GetSystemMetrics( SM_CYSCREEN) - g_windowHeight) / 2,
                g_windowWidth, g_windowHeight,
                NULL, NULL, hInstance, NULL
            );

    ghwnd = hwnd;

    ShowWindow( hwnd, iCmdShow);
    SetForegroundWindow( hwnd);
    SetFocus( hwnd);

    if( Initialize() != 0)
    {
        DestroyWindow( hwnd);
    }
    
    lastTime = std::chrono::high_resolution_clock::now();
    int FPS = 0;

    //Game Loop
    while( gbDone == false)
    {
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE))
        {
            if( msg.message == WM_QUIT)
            {
                gbDone = true;
            }
            else
            {
                TranslateMessage( &msg);
                DispatchMessage( &msg);
            }
        }
        else
        {
            if( gbActiveWindow == true)
            {
                Update();
                Display();

                if( gbIsEscapeKeyPressed == true)
                {
                    gbDone = true;
                }
                
                //FPS calculation
                FPS++;
                currentTime = std::chrono::high_resolution_clock::now();
                deltaTime_fps = std::chrono::duration<double>(currentTime - lastTime).count();

                if(deltaTime_fps >= 1.0)    //1 second elapsed
                {
                    currentFPS = FPS;
                    timePerFrame = 1.0 / (double)currentFPS;

                    FPS = 0;
                    lastTime = currentTime;
                }
            }
        }
    }

    Uninitialize();

    return( (int) msg.wParam);
}

//
//WndProc()
//
LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    //function declaration
    void ToggleFullscreen( void);
    void Resize( int, int);

    //variable declarations
    static int mousePosX, mousePosY;
	static int iAccumDelta, zDelta;

	int mouseX, mouseY;
	int mouseDx, mouseDy;

	int newX = -1;
	int newY = -1;

    int prevMeshWidth;
    int prevMeshHeight;
    
    POINT pt;
    float cameraStep = 1.0f;
    char str[256];

    //code
    switch( message)
    {
        case WM_CREATE:
        case WM_SETTINGCHANGE:
            SystemParametersInfo( SPI_GETWHEELSCROLLLINES, 0, &zDelta, 0);
            if( zDelta)
            {
                zDelta = WHEEL_DELTA / zDelta;
            }
            else
            {
                zDelta = 0;
            }
        break;

        case WM_SETFOCUS:
            gbActiveWindow = true;
        break;
        
        case WM_KILLFOCUS:
            gbActiveWindow = false;
        break;

        case WM_ERASEBKGND:
        return(0);

        case WM_SIZE:
            g_windowWidth = LOWORD( lParam);
            g_windowHeight = HIWORD( lParam);

            Resize( g_windowWidth, g_windowHeight);
        break;

        case WM_KEYDOWN:
            switch( wParam)
            {
                case VK_ESCAPE:
                    gbIsEscapeKeyPressed = true;
                    gbDone = true;
                break;

                case 0x46:  //'F'
                    ToggleFullscreen();
                    gbFullScreen = !gbFullScreen;
                break;

#if USE_FREE_CAMERA
                case VK_UP:
				case 'W':
					g_camera->moveForward(0.5f);// (deltaTime);
				break;

				case VK_DOWN:
				case 'S':
					g_camera->moveBackward(0.5f);// (deltaTime);
				break;

				case VK_RIGHT:
				case 'D':
					g_camera->moveRight(0.5f);// (deltaTime);
				break;

				case VK_LEFT:
				case 'A':
					g_camera->moveLeft(0.5f);// (deltaTime);
				break;
                
                case VK_HOME:
					g_camera->moveUp(0.5f);// (deltaTime);
				break;

				case VK_END:
					g_camera->moveDown(0.5f);// (deltaTime);
				break;
#endif


#if USE_ARC_CAMERA
                case VK_UP:
				case 'W':
					g_arcCamera.fPitch = fmodf( g_arcCamera.fPitch + cameraStep, 360.0f);
				break;

				case VK_DOWN:
				case 'S':
					g_arcCamera.fPitch = fmodf( g_arcCamera.fPitch - cameraStep, 360.0f);
				break;

				case VK_RIGHT:
				case 'D':
					g_arcCamera.fRoll = fmodf( g_arcCamera.fRoll + cameraStep, 360.0f);
				break;

				case VK_LEFT:
				case 'A':
					g_arcCamera.fRoll = fmodf( g_arcCamera.fRoll - cameraStep, 360.0f);
				break;
                
#endif

                case 'H':
                    bOnGPU = true;
                break;

                case 'P':
                    bOnGPU = false;
                break;

                case 'L':
                    gbEnableLight = !gbEnableLight;
                break;
                
                default:
                break;
            }
        break;

        case WM_CHAR:
            switch( wParam)
            {
                case 'n':
                case 'N':
                    currentIndex = (currentIndex + 1)% _ARRAYSIZE(gLights);
                break;

                case 'x':
                    gLights[currentIndex].position[0] += step;
                break;
                case 'X':
                    gLights[currentIndex].position[0] -= step;
                break;

                case 'y':
                    gLights[currentIndex].position[1] += step;
                break;
                case 'Y':
                    gLights[currentIndex].position[1] -= step;
                break;

                case 'z':
                    gLights[currentIndex].position[2] += step;
                break;
                case 'Z':
                    gLights[currentIndex].position[2] -= step;
                break;


                case 'r':
                    gLights[currentIndex].color[0] += step;
                break;
                case 'R':
                    gLights[currentIndex].color[0] -= step;
                    if(gLights[currentIndex].color[0] < 0.0f)
                    {
                        gLights[currentIndex].color[0] = 0.0f;
                    }
                break;

                case 'g':
                    gLights[currentIndex].color[1] += step;
                break;
                case 'G':
                    gLights[currentIndex].color[1] -= step;
                    if(gLights[currentIndex].color[1] < 0.0f)
                    {
                        gLights[currentIndex].color[1] = 0.0f;
                    }
                break;

                case 'b':
                    gLights[currentIndex].color[2] += step;
                break;
                case 'B':
                    gLights[currentIndex].color[2] -= step;
                    if(gLights[currentIndex].color[2] < 0.0f)
                    {
                        gLights[currentIndex].color[2] = 0.0f;
                    }
                break;


                case '1':
                    gLights[currentIndex].attenuation[0] += step;
                break;
                case '2':
                    gLights[currentIndex].attenuation[0] -= step;
                break;

                case '4':
                    gLights[currentIndex].attenuation[1] += step;
                break;
                case '5':
                    gLights[currentIndex].attenuation[1] -= step;
                break;

                case '7':
                    gLights[currentIndex].attenuation[2] += step;
                break;
                case '8':
                    gLights[currentIndex].attenuation[2] -= step;
                break;


                case '*':
                    step = step * 10.0f;
                break;
                case '/':
                    step = step / 10.0f;
                break;


                case 'I':
                case 'i':
                    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
                break;

                case 'C':
                case 'c':
                    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
                break;

                case 'K':
                case 'k':
                    glPolygonMode( GL_FRONT_AND_BACK, GL_POINT);
                break;

                case 'M':
                case 'm':
                    gbEnableMSAA = !gbEnableMSAA;
                break;

                case '+':
                    prevMeshWidth = currentMeshWidth;
                    prevMeshHeight = currentMeshHeight;

                    currentMeshWidth = currentMeshWidth * 2;
                    currentMeshHeight = currentMeshHeight * 2;

                    if( currentMeshWidth > MAX_MESH_SIZE)
                    {
                        currentMeshWidth = MAX_MESH_SIZE;
                    }

                    if( currentMeshHeight > MAX_MESH_SIZE)
                    {
                        currentMeshHeight = MAX_MESH_SIZE;
                    }

                    if( (prevMeshWidth != currentMeshWidth) || (prevMeshHeight != currentMeshHeight))
                    {
                        bNeedToUpdateBuffers = true;
                    }
                break;

                case '-':
                    prevMeshWidth = currentMeshWidth;
                    prevMeshHeight = currentMeshHeight;

                    currentMeshWidth = currentMeshWidth / 2;
                    currentMeshHeight = currentMeshHeight / 2;
                    
                    if( currentMeshWidth < MIN_MESH_SIZE)
                    {
                        currentMeshWidth = MIN_MESH_SIZE;
                    }

                    if( currentMeshHeight < MIN_MESH_SIZE)
                    {
                        currentMeshHeight = MIN_MESH_SIZE;
                    }

                    if( (prevMeshWidth != currentMeshWidth) || (prevMeshHeight != currentMeshHeight))
                    {
                        bNeedToUpdateBuffers = true;
                    }
                break;

                case 'q':
                case 'Q':
                    fadeOut = true;
                    fadeIn = false;
                    alpha_dt = 0.0f;
                break;
            }
            sprintf(
                str,
                "(%d) position=[%f, %f, %f], color=[%f, %f, %f], C=%f, L=%f, Q=%f step=%f",
                currentIndex,
                
                gLights[currentIndex].position[0],
                gLights[currentIndex].position[1],
                gLights[currentIndex].position[2],

                gLights[currentIndex].color[0],
                gLights[currentIndex].color[1],
                gLights[currentIndex].color[2],

                gLights[currentIndex].attenuation[0],
                gLights[currentIndex].attenuation[1],
                gLights[currentIndex].attenuation[2],

                step
            );
            SetWindowTextA( hwnd, str);
        break;

        case WM_MOUSEWHEEL:
		{
			if (zDelta == 0)
				break;

			mouseX = LOWORD(lParam);
			mouseY = HIWORD(lParam);

			iAccumDelta = iAccumDelta + (short)HIWORD(wParam);

#if USE_ARC_CAMERA
            g_arcCamera.changeZoomLevel( iAccumDelta);
#endif

			while (iAccumDelta >= zDelta)
				iAccumDelta = iAccumDelta - zDelta;

			while (iAccumDelta <= -zDelta)
				iAccumDelta = iAccumDelta + zDelta;
		}
        break;

		case WM_MOUSEMOVE:
		{
			mouseX = LOWORD(lParam);
			mouseY = HIWORD(lParam);

			mouseDx = mousePosX - mouseX;
			mouseDy = mousePosY - mouseY;

			//set current mouse position
			mousePosX = mouseX;
			mousePosY = mouseY;

			if (wParam & MK_LBUTTON)   //calculate camera pitch
			{
				float pitchChange = mouseDy * 0.1f;
				float yawChange = mouseDx * 0.1f;

#if USE_FREE_CAMERA
				g_camera->rotate(pitchChange, yawChange);
#endif

#if USE_ARC_CAMERA
                g_arcCamera.updateAngleAroundPoint( yawChange);
                g_arcCamera.updatePitchAngle( pitchChange);
#endif

			}

			if ((wParam & MK_LBUTTON) || (wParam & MK_RBUTTON) || (wParam & MK_MBUTTON))
			{
				//Wrap Mouse Horizontally
				if (mouseX > (g_windowWidth - 20))
					newX = 22;
				else if (mouseX < 20)
					newX = g_windowWidth - 22;

				//Wrap Mouse Vertically
				if (mouseY > (g_windowHeight - 20))
					newY = 22;
				else if (mouseY < 20)
					newY = g_windowHeight - 22;


				if ((newX != -1) && (newY != -1))
				{
					pt.x = newX;
					pt.y = newY;

					ClientToScreen(hwnd, &pt);
					SetCursorPos(pt.x, pt.y);

					mousePosX = newX;
					mousePosY = newY;
				}
				else if (newX != -1)
				{
					pt.x = newX;
					pt.y = mouseY;

					ClientToScreen(hwnd, &pt);
					SetCursorPos(pt.x, pt.y);

					mousePosX = newX;
					mousePosY = mouseY;
				}
				else if (newY != -1)
				{
					pt.x = mouseX;
					pt.y = newY;

					ClientToScreen(hwnd, &pt);
					SetCursorPos(pt.x, pt.y);

					mousePosX = mouseX;
					mousePosY = newY;
				}
			}
		}
        break;

        case WM_LBUTTONDOWN:
            SetCapture( hwnd);
        break;

        case WM_LBUTTONUP:
            ReleaseCapture();
        break;

        case WM_CLOSE:
        break;

        case WM_DESTROY:
            PostQuitMessage( 0);
        break;

        default:
        break;
    }

    return( DefWindowProc( hwnd, message, wParam, lParam));
}


//
//ToggleFullscreen()
//
void ToggleFullscreen( void)
{
    //variable declaration
    MONITORINFO mi = { sizeof( MONITORINFO) };

    //code
    if( gbFullScreen == false)
    {
        style = GetWindowLong( ghwnd, GWL_STYLE);
        if( style & WS_OVERLAPPEDWINDOW)
        {
            if( GetWindowPlacement( ghwnd, &wpPrev) && GetMonitorInfo( MonitorFromWindow( ghwnd, MONITORINFOF_PRIMARY), &mi))
            {
                SetWindowLong( ghwnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(
                    ghwnd,
                    HWND_TOP,
                    mi.rcMonitor.left,
                    mi.rcMonitor.top,
                    mi.rcMonitor.right - mi.rcMonitor.left,
                    mi.rcMonitor.bottom - mi.rcMonitor.top,
                    SWP_NOZORDER | SWP_FRAMECHANGED
                );
            }
        }
    }
    else
    {
        SetWindowLong( ghwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement( ghwnd, &wpPrev);
        SetWindowPos(
            ghwnd,
            HWND_TOP,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED
        );
    }
}


//
//Initialize()
//
int Initialize( void)
{
    //function declaration
    void Resize( int, int);
    float HeightCalculate( float, float, float);
    void CreateMesh(
        int cx, int cz, int MeshWidth, int MeshHeight,
        float multiplicant, float amplitude,
        VERTEX **vertexData, int *vertexCount,
        float (*heightFunc)(float, float, float)    //height function
    );

    //variable declarations
    PIXELFORMATDESCRIPTOR pfd;
    FILE *fpOpenGLInfo = NULL;
    int iPixelFormatIndex;

    //code
    ZeroMemory( &pfd, sizeof( PIXELFORMATDESCRIPTOR));

    //Initialize PFD
    pfd.nSize      = sizeof( PIXELFORMATDESCRIPTOR);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cRedBits   = 8;
    pfd.cGreenBits = 8;
    pfd.cBlueBits  = 8;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 32;

    ghdc = GetDC( ghwnd);

    //choose pixel format
    iPixelFormatIndex = ChoosePixelFormat( ghdc, &pfd);
    if( iPixelFormatIndex == 0)
    {
        ReleaseDC( ghwnd, ghdc);
        ghdc = 0;
        return(-1);
    }

    //set pixel format
    if(SetPixelFormat( ghdc, iPixelFormatIndex, &pfd) == false)
    {
        ReleaseDC( ghwnd, ghdc);
        ghdc = 0;
        return(-1);
    }

    //get rendering context
    ghrc = wglCreateContext( ghdc);
    if( ghrc == NULL)
    {
        ReleaseDC( ghwnd, ghdc);
        ghdc = 0;
        return(-1);
    }

    if( wglMakeCurrent( ghdc, ghrc) == false)
    {
        wglDeleteContext( ghrc);
        ghrc = 0;

        ReleaseDC( ghwnd, ghdc);
        ghdc = 0;
        return(-1);
    }


    //Initialize GLEW
    GLenum glew_error = glewInit();
    if( glew_error != GLEW_OK)
    {
        wglDeleteContext( ghrc);
        ghrc = 0;
        
        ReleaseDC( ghwnd, ghdc);
        ghdc = 0;
        return(-1);
    }

    fprintf( gpLogFile, "%zd, %zd-------\n", sizeof( GRASS_VERTEX), sizeof(VERTEX));

    /* _________________________ OpenGL Information _______________________ */
    fopen_s( &fpOpenGLInfo, "OpenGLInformation.log", "w");
    if( fpOpenGLInfo == NULL)
    {
        MessageBox( NULL, TEXT("Error while create \"OpenGLInformation.log\""), TEXT("Error"), MB_OK | MB_ICONSTOP);
        return(-1);
    }

    fprintf( fpOpenGLInfo, "============================================================================================\n");
    fprintf( fpOpenGLInfo, "* OpenGL Vendor   : %s\n", glGetString( GL_VENDOR));
    fprintf( fpOpenGLInfo, "* OpenGL Renderer : %s\n", glGetString( GL_RENDERER));
    fprintf( fpOpenGLInfo, "* OpenGL Version  : %s\n", glGetString( GL_VERSION));
    fprintf( fpOpenGLInfo, "* OpenGL Shading Language (GLSL) Version : %s\n", glGetString( GL_SHADING_LANGUAGE_VERSION)); 
    fprintf( fpOpenGLInfo, "============================================================================================\n");

    //OpenGL Enable Extensions
    GLint numExtension;
    glGetIntegerv( GL_NUM_EXTENSIONS, &numExtension);

    fprintf( fpOpenGLInfo, "Number of enable Extensions : %d\n", numExtension);
    fprintf( fpOpenGLInfo, "Enable Extension: \n");
    for( int i = 0; i < numExtension; i++)
    {
        fprintf( fpOpenGLInfo, "\t%s\n", glGetStringi( GL_EXTENSIONS, i));
    }

    fprintf( fpOpenGLInfo, "============================================================================================\n");

    fclose( fpOpenGLInfo);
    fpOpenGLInfo = NULL;

    /* ___________________________________ OpenCL Context ___________________________________ */
    cl_platform_id  oclPlatformID;
    cl_device_id   *oclDeviceIDs;
    cl_uint         deviceCount;

        //get platform
    clResult = clGetPlatformIDs( 1, &oclPlatformID, NULL);
    if( CL_SUCCESS != clResult)
    {
        fprintf( gpLogFile, "OpenCL Error(%d): clGetPlatformIDs() Failed\n", __LINE__);
        return(-1);
    }

        //get supported device count
    clResult = clGetDeviceIDs( oclPlatformID, CL_DEVICE_TYPE_GPU, 0, NULL, &deviceCount);
    if( CL_SUCCESS != clResult)
    {
        fprintf( gpLogFile, "OpenCL Error(%d): clGetDeviceIDs() Failed\n", __LINE__);
        return(-1);
    }
    else if( deviceCount < 1)
    {
        fprintf( gpLogFile, "OpenCL Error(%d): No device which support OpenCL\n", __LINE__);
        return(-1);
    }

        //allocate memory
    oclDeviceIDs = ( cl_device_id *) calloc( deviceCount, sizeof( cl_device_id));

        //get devices
    clResult = clGetDeviceIDs( oclPlatformID, CL_DEVICE_TYPE_GPU, deviceCount, oclDeviceIDs, NULL);
    if( CL_SUCCESS != clResult)
    {
        fprintf( gpLogFile, "OpenCL Error(%d): clGetDeviceIDs() Failed\n", __LINE__);
        return(-1);
    }

        //get first device
    oclComputeDeviceId = oclDeviceIDs[0];

    free(oclDeviceIDs);
    oclDeviceIDs = NULL;


    //Create OpenCL Context which is compatible with OpenGL Context
    cl_context_properties context_properties[] =
    {
        CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
        CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(),
        CL_CONTEXT_PLATFORM, (cl_context_properties) oclPlatformID,
        0   //end of array
    };

    oclContext = clCreateContext( context_properties, 1, &oclComputeDeviceId, NULL, NULL, &clResult);
    if( CL_SUCCESS != clResult)
    {
        fprintf( gpLogFile, "OpenCL Error(%d): clCreateContext() Failed: %d\n", __LINE__, clResult);
        return(-1);
    }

    //create command queue
    oclCommandQueue = clCreateCommandQueueWithProperties( oclContext, oclComputeDeviceId, 0, &clResult);
    if( CL_SUCCESS != clResult)
    {
        fprintf( gpLogFile, "OpenCL Error(%d): clCreateCommandQueueWithProperties() Failed\n", __LINE__);
        return(-1);
    }

    //Read Kernel source code
    const char *openclGrassKernelSourceCode = ReadShaderFromFile( grassOpenCLFileName);
    if( openclGrassKernelSourceCode == NULL)
    {
        fprintf( gpLogFile, "OpenCL Error(%d): ReadShaderFromFile() Failed\n", __LINE__);
        return(-1);
    }

    size_t clSourceCodeSize = strlen( openclGrassKernelSourceCode) + 1;

    //create OpenCL program
    oclGrassProgram = clCreateProgramWithSource( oclContext, 1, (const char **)&openclGrassKernelSourceCode, &clSourceCodeSize, &clResult);
    if( CL_SUCCESS != clResult)
    {
        fprintf( gpLogFile, "OpenCL Error(%d): clCreateProgramWithSource() Failed\n", __LINE__);

        free( (void *)openclGrassKernelSourceCode);
        openclGrassKernelSourceCode = NULL;
        
        return(-1);
    }

    free( (void *)openclGrassKernelSourceCode);
    openclGrassKernelSourceCode = NULL;

    //build program
    clResult = clBuildProgram( oclGrassProgram, 0, NULL, "-cl-fast-relaxed-math", NULL, NULL);
    if( CL_SUCCESS != clResult)
    {
        fprintf( gpLogFile, "OpenCL Error(%d): clBuildProgram() Failed (%d)\n", __LINE__, clResult);

        char *buffer = NULL;
        size_t len;

            //Get Length of log
        clGetProgramBuildInfo( oclGrassProgram, oclComputeDeviceId, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);

        buffer = (char *) malloc( len);
        if( buffer)
        {
                //Get Log
            clGetProgramBuildInfo( oclGrassProgram, oclComputeDeviceId, CL_PROGRAM_BUILD_LOG, len, buffer, NULL);
            fprintf( gpLogFile, "OpenCL Program Build Log : %s\n", buffer);

            free( buffer);
            buffer = NULL;
        }

        return(-1);
    }

    //create kernel
    oclGrassKernel = clCreateKernel( oclGrassProgram, grassKernelName, &clResult);
    if( CL_SUCCESS != clResult)
    {
        fprintf( gpLogFile, "OpenCL Error(%d): clCreateKernel() Failed\n", __LINE__);
        return(-1);
    }


    /** _______________________________ SHADERS ____________________________ **/
    //Simple program
    GLchar *vertexShaderSource = 
        "#version 450 core      \n"
        ""
        "in vec4 vPosition;     \n"
        ""
        "uniform mat4 MMatrix; \n"
        "uniform mat4 VMatrix; \n"
        "uniform mat4 PMatrix; \n"
        ""
        "void main( void)   \n"
        "{"
        "   gl_Position = PMatrix * VMatrix * MMatrix * vPosition; \n"
        "}"
        "\n\n";

    GLchar *fragmentShaderSource =
        "#version 450 core      \n"
        ""
        "uniform vec4 color; \n"
        ""
        "out vec4 fragColor;    \n"
        ""
        "void main( void)"
        "{"
        "   fragColor = color; \n"
        "}"
        "\n\n";

    SHADERS_INFO shaderInfo[2];
    
    shaderInfo[0].shaderType = GL_VERTEX_SHADER;
	shaderInfo[0].shaderLoadAs = CREATE_PROGRAM_SHADER_LOAD_FROM_STRING;
	shaderInfo[0].shaderFileName = vertexShaderSource;
	shaderInfo[0].shaderID = 0;

	shaderInfo[1].shaderType = GL_FRAGMENT_SHADER;
	shaderInfo[1].shaderLoadAs = CREATE_PROGRAM_SHADER_LOAD_FROM_STRING;
	shaderInfo[1].shaderFileName = fragmentShaderSource;
	shaderInfo[1].shaderID = 0;

    BIND_ATTRIBUTES_INFO bindAttributeInfo[] =
	{
		{"vPosition", VJD_ATTRIBUTE_POSITION}
	};

    program_light = CreateProgram( shaderInfo, _ARRAYSIZE( shaderInfo), bindAttributeInfo, _ARRAYSIZE(bindAttributeInfo), __LINE__);
    if(program_light == 0)
    {
        return(-1);
    }


    //alpha
    shaderInfo[0].shaderType = GL_VERTEX_SHADER;
	shaderInfo[0].shaderLoadAs = CREATE_PROGRAM_SHADER_LOAD_FROM_FILE;
	shaderInfo[0].shaderFileName = "shaders/alpha_blending/vertex_shader.glsl";
	shaderInfo[0].shaderID = 0;

	shaderInfo[1].shaderType = GL_FRAGMENT_SHADER;
	shaderInfo[1].shaderLoadAs = CREATE_PROGRAM_SHADER_LOAD_FROM_FILE;
	shaderInfo[1].shaderFileName = "shaders/alpha_blending/fragment_shader.glsl";
	shaderInfo[1].shaderID = 0;

    BIND_ATTRIBUTES_INFO alpha_bindAttributeInfo[] =
	{
		{ "vPosition", VJD_ATTRIBUTE_POSITION },
		{ "vTexCoord", VJD_ATTRIBUTE_TEXTCOORD }
	};

    program_alpha = CreateProgram( shaderInfo, _ARRAYSIZE( shaderInfo), alpha_bindAttributeInfo, _ARRAYSIZE(alpha_bindAttributeInfo), __LINE__);
    if(program_alpha == 0)
    {
        return(-1);
    }

    //Grass
    shaderInfo[0].shaderType = GL_VERTEX_SHADER;
	shaderInfo[0].shaderLoadAs = CREATE_PROGRAM_SHADER_LOAD_FROM_FILE;
	shaderInfo[0].shaderFileName = "shaders/grass/vertex_shader.glsl";
	shaderInfo[0].shaderID = 0;

	shaderInfo[1].shaderType = GL_FRAGMENT_SHADER;
	shaderInfo[1].shaderLoadAs = CREATE_PROGRAM_SHADER_LOAD_FROM_FILE;
	shaderInfo[1].shaderFileName = "shaders/grass/fragment_shader.glsl";
	shaderInfo[1].shaderID = 0;

    BIND_ATTRIBUTES_INFO grass_bindAttributeInfo[] =
	{
		{"vPosition", VJD_ATTRIBUTE_POSITION},
		{"vNormal", VJD_ATTRIBUTE_NORMAL},
		{"vTangent", VJD_ATTRIBUTE_TANGENT},
		{"vTexCoord", VJD_ATTRIBUTE_TEXTCOORD}
	};

    program_grass = CreateProgram( shaderInfo, _ARRAYSIZE( shaderInfo), grass_bindAttributeInfo, _ARRAYSIZE(grass_bindAttributeInfo), __LINE__);
    if(program_grass == 0)
    {
        return(-1);
    }


    //Display
    shaderInfo[0].shaderType = GL_VERTEX_SHADER;
	shaderInfo[0].shaderLoadAs = CREATE_PROGRAM_SHADER_LOAD_FROM_FILE;
	shaderInfo[0].shaderFileName = "shaders/display/vertex_shader.glsl";
	shaderInfo[0].shaderID = 0;

	shaderInfo[1].shaderType = GL_FRAGMENT_SHADER;
	shaderInfo[1].shaderLoadAs = CREATE_PROGRAM_SHADER_LOAD_FROM_FILE;
	shaderInfo[1].shaderFileName = "shaders/display/fragment_shader.glsl";
	shaderInfo[1].shaderID = 0;

    BIND_ATTRIBUTES_INFO display_bindAttributeInfo[] =
	{
		{"vPosition", VJD_ATTRIBUTE_POSITION},
		{"vTexCoord", VJD_ATTRIBUTE_TEXTCOORD}
	};

    program_display = CreateProgram( shaderInfo, _ARRAYSIZE( shaderInfo), display_bindAttributeInfo, _ARRAYSIZE(display_bindAttributeInfo), __LINE__);
    if(program_display == 0)
    {
        return(-1);
    }


    //HDR Display
    shaderInfo[0].shaderType = GL_VERTEX_SHADER;
	shaderInfo[0].shaderLoadAs = CREATE_PROGRAM_SHADER_LOAD_FROM_FILE;
	shaderInfo[0].shaderFileName = "shaders/hdr_display/vertex_shader.glsl";
	shaderInfo[0].shaderID = 0;

	shaderInfo[1].shaderType = GL_FRAGMENT_SHADER;
	shaderInfo[1].shaderLoadAs = CREATE_PROGRAM_SHADER_LOAD_FROM_FILE;
	shaderInfo[1].shaderFileName = "shaders/hdr_display/fragment_shader.glsl";
	shaderInfo[1].shaderID = 0;

    BIND_ATTRIBUTES_INFO hdr_display_bindAttributeInfo[] =
	{
		{"vPosition", VJD_ATTRIBUTE_POSITION},
		{"vTexCoord", VJD_ATTRIBUTE_TEXTCOORD}
	};

    program_hdr_display = CreateProgram( shaderInfo, _ARRAYSIZE( shaderInfo), hdr_display_bindAttributeInfo, _ARRAYSIZE(hdr_display_bindAttributeInfo), __LINE__);
    if(program_hdr_display == 0)
    {
        return(-1);
    }

    /* _____________ Mesh Buffer ______________ */

    int maxGrassVerticesCount = MAX_MESH_SIZE * MAX_MESH_SIZE * 2 * GRASS_BLADE_SEGMENTS;

    int trianglePerGrassBlade = 2 * (GRASS_BLADE_SEGMENTS - 1);
    int verticesPerTriangle = 3;
    int maxGrassIndicesCount = MAX_MESH_SIZE * MAX_MESH_SIZE * trianglePerGrassBlade * verticesPerTriangle;

    grassStaticProps_cpu = ( GRASS_STATIC_PROPERTIES *) calloc( maxGrassVerticesCount, sizeof( GRASS_STATIC_PROPERTIES));


        //common buffer to both OpenCL and CPU vao
    glGenBuffers(1, &vbo_element_common);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_element_common);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxGrassIndicesCount * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


        //CPU VERTEX ARRAY AND BUFFER
    glGenVertexArrays(1, &vbo_grassBuffer_cpu);
	glBindVertexArray(vao_grass_cpu);
		glGenBuffers(1, &vbo_grassBuffer_cpu);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_grassBuffer_cpu);
			glBufferData(GL_ARRAY_BUFFER, maxGrassVerticesCount * sizeof(GRASS_VERTEX), NULL, GL_STATIC_DRAW);

			glVertexAttribPointer(VJD_ATTRIBUTE_POSITION,   3, GL_FLOAT, GL_FALSE, sizeof(GRASS_VERTEX), (void*)offsetof(GRASS_VERTEX, position));
			glVertexAttribPointer(VJD_ATTRIBUTE_NORMAL,     3, GL_FLOAT, GL_FALSE, sizeof(GRASS_VERTEX), (void*)offsetof(GRASS_VERTEX, normal));
			glVertexAttribPointer(VJD_ATTRIBUTE_TEXTCOORD,  2, GL_FLOAT, GL_FALSE, sizeof(GRASS_VERTEX), (void*)offsetof(GRASS_VERTEX, texcoord));

			glEnableVertexAttribArray(VJD_ATTRIBUTE_POSITION);
			glEnableVertexAttribArray(VJD_ATTRIBUTE_NORMAL);
			glEnableVertexAttribArray(VJD_ATTRIBUTE_TEXTCOORD);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

            //element buffer is common for both cpu and gpu
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_element_common);
	glBindVertexArray(0);


	    //OpenCL VERTEX ARRAY AND BUFFER
    glGenVertexArrays(1, &vao_grass_opencl);
	glBindVertexArray(vao_grass_opencl);
		glGenBuffers(1, &vbo_grassBuffer_opencl);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_grassBuffer_opencl);
			glBufferData(GL_ARRAY_BUFFER, maxGrassVerticesCount * sizeof(GRASS_VERTEX), NULL, GL_STATIC_DRAW);

			glVertexAttribPointer(VJD_ATTRIBUTE_POSITION,   3, GL_FLOAT, GL_FALSE, sizeof(GRASS_VERTEX), (void*)offsetof(GRASS_VERTEX, position));
			glVertexAttribPointer(VJD_ATTRIBUTE_NORMAL,     3, GL_FLOAT, GL_FALSE, sizeof(GRASS_VERTEX), (void*)offsetof(GRASS_VERTEX, normal));
			glVertexAttribPointer(VJD_ATTRIBUTE_TEXTCOORD,  2, GL_FLOAT, GL_FALSE, sizeof(GRASS_VERTEX), (void*)offsetof(GRASS_VERTEX, texcoord));

			glEnableVertexAttribArray(VJD_ATTRIBUTE_POSITION);
			glEnableVertexAttribArray(VJD_ATTRIBUTE_NORMAL);
			glEnableVertexAttribArray(VJD_ATTRIBUTE_TEXTCOORD);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

        //Create OpenCL graphics resource for OpenGL buffer
        cl_graphics_resource_mesh = clCreateFromGLBuffer( oclContext, CL_MEM_WRITE_ONLY, vbo_grassBuffer_opencl, &clResult);
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clCreateFromGLBuffer() Failed\n");
            return(-1);
        }

            //element buffer is common for both cpu and gpu
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_element_common);
	glBindVertexArray(0);



        //allocate memory device for kernel's 2nd parameter
    size_t grassBufferSize = maxGrassVerticesCount * sizeof( VERTEX);

    meshVertexData_opencl_input = clCreateBuffer( 
                                        oclContext,           //opencl context
                                        CL_MEM_READ_WRITE,    //flag
                                        grassBufferSize,           //buffer size in bytes
                                        NULL,                 //copy data from host to device
                                        &clResult             //return error if any
                                    );
    if( CL_SUCCESS != clResult)
    {
        fprintf( gpLogFile, "clCreateBuffer() Failed\n");
        return(-1);
    }

    //quad
    VERTEX quadVertexData[] = 
    {
        { {+1.0f, +1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}, {+1.0f, +1.0f} },
        { {-1.0f, +1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}, {+0.0f, +1.0f} },
        { {-1.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}, {+0.0f, +0.0f} },

        { {+1.0f, +1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}, {+1.0f, +1.0f} },
        { {-1.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}, {+0.0f, +0.0f} },
        { {+1.0f, -1.0f, 0.0f}, {0.0f, +1.0f, 0.0f}, {+1.0f, 0.0f, 0.0f}, {+1.0f, +0.0f} }
    };

    glCreateVertexArrays( 1, &vao_quad);
    glBindVertexArray( vao_quad);
        glCreateBuffers( 1, &vbo_quad);
        glBindBuffer( GL_ARRAY_BUFFER, vbo_quad);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertexData), quadVertexData, GL_STATIC_DRAW);

			glVertexAttribPointer(VJD_ATTRIBUTE_POSITION,   3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (void*)offsetof(VERTEX, position));
			glVertexAttribPointer(VJD_ATTRIBUTE_NORMAL,     3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (void*)offsetof(VERTEX, normal));
			glVertexAttribPointer(VJD_ATTRIBUTE_TANGENT,    3, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (void*)offsetof(VERTEX, tangent));
			glVertexAttribPointer(VJD_ATTRIBUTE_TEXTCOORD,  2, GL_FLOAT, GL_FALSE, sizeof(VERTEX), (void*)offsetof(VERTEX, texcoord));

			glEnableVertexAttribArray( VJD_ATTRIBUTE_POSITION);
			glEnableVertexAttribArray( VJD_ATTRIBUTE_NORMAL);
			glEnableVertexAttribArray( VJD_ATTRIBUTE_TANGENT);
			glEnableVertexAttribArray( VJD_ATTRIBUTE_TEXTCOORD);
        glBindBuffer( GL_ARRAY_BUFFER, 0);
    glBindVertexArray( 0);

    //---------------------------- Wind Distortion Map
    HBITMAP hBitmap_wind = (HBITMAP) LoadImage( GetModuleHandle( NULL), TEXT("texture/Wind.bmp"), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    BITMAP windBitmap;

    if( hBitmap_wind)
    {
        GetObject( hBitmap_wind, sizeof(BITMAP), &windBitmap);
        int byteCount = windBitmap.bmBitsPixel / 8;

        //initiaize IMAGE_DATA for windDistortion map and image data to normalize
        windDistortion_map.width = windBitmap.bmWidth;
        windDistortion_map.height = windBitmap.bmHeight;
        windDistortion_map.bytePerPixel = byteCount;
        fprintf( gpLogFile, "%d\n", windDistortion_map.bytePerPixel);


        size_t imageBufferByteSize = windBitmap.bmWidth * windBitmap.bmHeight * COLOR_CHANNELS * sizeof(float);

        windDistortion_map.normalizeImageData = (float *) malloc( imageBufferByteSize);
        if(windDistortion_map.normalizeImageData == NULL)
        {
            fprintf( gpLogFile, "%s(%d): malloc() Failed\n", __FUNCTION__, __LINE__);
            return(-1);
        }

        BYTE *pBits = (BYTE *) windBitmap.bmBits;

                //normalize image data
        for( int y = 0; y < windBitmap.bmHeight; y++)
        {
            for( int x = 0; x < windBitmap.bmWidth; x++)
            {
                switch( byteCount)
                {
                    case 1:
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 0) ) = (float) *(pBits + (y * windBitmap.bmWidthBytes + x)) / 255.0f;           //red
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 1) ) = 0.0f;                                                                    //green
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 2) ) = 0.0f;                                                                    //blue
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 3) ) = 1.0f;                                                                    //alpha
                    break;

                    case 2:
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 0) ) = (float) *(pBits + (y * windBitmap.bmWidthBytes + 2 * x + 0)) / 255.0f;   //red
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 1) ) = (float) *(pBits + (y * windBitmap.bmWidthBytes + 2 * x + 1)) / 255.0f;   //green
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 2) ) = 0.0f;                                                                    //blue
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 3) ) = 1.0f;                                                                    //alpha
                    break;

                    case 3:
                            //Windows BITMAP is not an RGB it's BGR
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 0) ) = (float) *(pBits + (y * windBitmap.bmWidthBytes + 3 * x + 2)) / 255.0f;   //red
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 1) ) = (float) *(pBits + (y * windBitmap.bmWidthBytes + 3 * x + 1)) / 255.0f;   //green
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 2) ) = (float) *(pBits + (y * windBitmap.bmWidthBytes + 3 * x + 0)) / 255.0f;   //blue
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 3) ) = 1.0f;                                                                    //alpha
                    break;
                    
                    case 4:
                        //Windows BITMAP is not an RGBA it's BGRA
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 0) )  = (float) *(pBits + (y * windBitmap.bmWidthBytes + 4 * x + 2)) / 255.0f;  //red
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 1) )  = (float) *(pBits + (y * windBitmap.bmWidthBytes + 4 * x + 1)) / 255.0f;  //green
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 2) )  = (float) *(pBits + (y * windBitmap.bmWidthBytes + 4 * x + 0)) / 255.0f;  //blue
                        *( windDistortion_map.normalizeImageData + ( COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 3) )  = (float) *(pBits + (y * windBitmap.bmWidthBytes + 4 * x + 3)) / 255.0f;  //alpha
                    break;

                    default:
                    break;
                }
            }
        }
        
        DeleteObject(hBitmap_wind);
        hBitmap_wind = NULL;
    }
    else
    {
        fprintf( gpLogFile, "%s(%d): LoadImage() Failed\n", __FUNCTION__, __LINE__);
        return (-1);
    }

    size_t imageBufferByteSize = windDistortion_map.width * windDistortion_map.height * COLOR_CHANNELS * sizeof(float);
    distortionMap_opencl_input = clCreateBuffer(
                                    oclContext,
                                    CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                    imageBufferByteSize,
                                    (void *)windDistortion_map.normalizeImageData,
                                    &clResult
                                );
    if( CL_SUCCESS != clResult)
    {
        fprintf( gpLogFile, "OpenCL Error( %d): clCreateBuffer() failed\n", __LINE__);
        return(-1);
    }


    glGenVertexArrays( 1, &vao_light);
    glGenBuffers( 1, &vbo_light);

    glBindVertexArray( vao_light);
        glBindBuffer( GL_ARRAY_BUFFER, vbo_light);
            glBufferData( GL_ARRAY_BUFFER, 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
            glVertexAttribPointer( VJD_ATTRIBUTE_POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray( VJD_ATTRIBUTE_POSITION);
        glBindBuffer( GL_ARRAY_BUFFER, 0);
    glBindVertexArray( 0);

    /* ____ TEXTURE ____ */
    grassBladeTexture = LoadTexture( "texture/grassBlade_new.png");
    grassBladeAlphaTexture = LoadTexture( "texture/grassBladeAlpha_new.png");
    groundTexture = LoadTexture( "texture/ground_diffuse.jpg");
    waterMarkTexture = LoadTexture( "assets/water_mark.png");

    glBindTexture( GL_TEXTURE_3D, grassBladeAlphaTexture);
        glTextureParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture( GL_TEXTURE_3D, 0);

    glBindTexture( GL_TEXTURE_3D, grassBladeTexture);
        glTextureParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture( GL_TEXTURE_3D, 0);

    glGenTextures( 1, &groundAlphaTexture);
    glBindTexture( GL_TEXTURE_2D, groundAlphaTexture);

        unsigned char alpha[] = { 255, 255, 255, 255};
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, alpha);
    glBindTexture( GL_TEXTURE_2D, 0);


    /* ___________ OTHER INITIALIZE _______________ */
    FontRendringInitialize_FreeType();

    NotoSerifBoldFreeTypeFont = FontCreate( "assets/NotoSerif-Bold.ttf", 28);
    TahomaFreeTypeFont = FontCreate( "assets/tahoma.ttf", 38);



    /* ____ FRAMEBUFFER ____ */
    glGenFramebuffers( 1, &msaaFramebuffer.framebuffer);
    glGenTextures( 1, &msaaFramebuffer.colorAttachment);
    glGenTextures( 1, &msaaFramebuffer.depthAttachment);

    glGenFramebuffers( 1, &sceneFramebuffer.framebuffer);
    glGenTextures( 1, &sceneFramebuffer.colorAttachment);
    glGenTextures( 1, &sceneFramebuffer.depthAttachment);


    //Camera Setup
#if USE_ARC_CAMERA
    g_arcCamera.distanceFromPoint = 10.0f;
    g_arcCamera.fPitch = 20.0f;
#endif

    /* _______________ OpenGL State Initialize __________________ */
    glClearDepth(1.0f);
    glDepthFunc( GL_LEQUAL);
    glEnable( GL_DEPTH_TEST);

    // glEnable( GL_CULL_FACE);
    glClearColor( 0.0f, 0.0f, 0.02f, 1.0f);

    //Warm up resize call
    RECT rc;
    GetClientRect( ghwnd, &rc);
    Resize( rc.right - rc.left, rc.bottom - rc.top);

    return(0);
}


//Simple noise function, http://answers.unity.com/answers/624136/view.html
float random( vmath::vec3 coord)
{
    return( mymath::fract( sin( vmath::dot( coord, vmath::vec3( 12.9898, 78.233, 53.539)) ) * 43758.5453));
}

//
//HeightCalculate
//
float HeightCalculate( float x, float z, float amplitude)
{
    //code
    return( 0.0f);
    //return( random( 0.1f* vmath::vec3( x/currentMeshWidth, 0.0f, z/currentMeshHeight)) * amplitude);
}

//
//CreateMesh()
//
void CreateMesh(
    int cx,
    int cz,
    int MeshWidth,
    int MeshHeight,
    float multiplicant,
    float amplitude,
    VERTEX *vertexData,
    float (*heightFunc)(float, float, float)    //height function
)
{
	//code
    if( vertexData == NULL)
        return;

	int vertexCount = MeshWidth * MeshHeight;

	int vertexPointer = 0;

	int xStart = cx * (MeshWidth-1);
	int zStart = cz * (MeshHeight-1);

	float topLeftX = xStart - (MeshWidth -1) / 2;
	float topLeftZ = zStart + (MeshHeight -1) / 2;

    fprintf( gpLogFile, "[%f, %f], [%d, %d]\n", topLeftX, topLeftZ, MeshWidth, MeshHeight);

    float increment = multiplicant;

	for (int z = 0; z < MeshHeight; z++)
	{
		for (int x = 0; x < MeshWidth; x++)
		{
			float posX = (topLeftX + x) * multiplicant;
			float posZ = (topLeftZ - z) * multiplicant;

			vertexData[vertexPointer].position[0] = (float)posX;
			vertexData[vertexPointer].position[1] = heightFunc( posX, posZ, amplitude);//0.0f;
			vertexData[vertexPointer].position[2] = (float)posZ;

            vertexData[vertexPointer].normal[0] = 0.0f;
            vertexData[vertexPointer].normal[1] = 1.0f;
            vertexData[vertexPointer].normal[2] = 0.0f;

            vertexData[vertexPointer].texcoord[0] = (float)x / (float)MeshWidth;
			vertexData[vertexPointer].texcoord[1] = (float)z / (float)MeshHeight;

            vertexData[vertexPointer].tangent[0] = 1.0f;
			vertexData[vertexPointer].tangent[1] = 0.0f;
			vertexData[vertexPointer].tangent[2] = 0.0f;

            //fprintf( gpLogFile, "[%f, %f, %f]\n", vertexData[vertexPointer].position[0], vertexData[vertexPointer].position[1], vertexData[vertexPointer].position[2]);

            vertexPointer++;
		}
	}
}

//
//Resize()
//
void Resize( int width, int height)
{
    //code
    if( height == 0)
        height = 1;
    
    glViewport( 0, 0, (GLsizei) width, (GLsizei) height);

    projection_matrix = vmath::perspective( 45.0f, (GLfloat)width /(GLfloat)height, 0.1f, 200.0f);


    //framebuffer resize
    if( msaaFramebuffer.framebuffer)
    {
        msaaFramebuffer.width = width;
        msaaFramebuffer.height = height;

        glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, msaaFramebuffer.colorAttachment);
            glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, MSAA_SAMPLES, GL_R11F_G11F_B10F, msaaFramebuffer.width, msaaFramebuffer.height, GL_FALSE);
        glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, 0);

        glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, msaaFramebuffer.depthAttachment);
            glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, MSAA_SAMPLES, GL_DEPTH_COMPONENT32F, msaaFramebuffer.width, msaaFramebuffer.height, GL_FALSE);
        glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, 0);

        glBindFramebuffer( GL_FRAMEBUFFER, msaaFramebuffer.framebuffer); //bound framebuffer on read as well as draw
            glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msaaFramebuffer.colorAttachment, 0);
            glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, msaaFramebuffer.depthAttachment, 0);

            if( glCheckFramebufferStatus( GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                fprintf( gpLogFile, "MSAA Framebuffer is not complete\n");
                DestroyWindow( ghwnd);
                return;
            }
            else
            {
                fprintf( gpLogFile, "MSAA Framebuffer is complete\n");
            }
        glBindFramebuffer( GL_FRAMEBUFFER, 0);
    }

    if( sceneFramebuffer.framebuffer)
    {
        sceneFramebuffer.width = width;
        sceneFramebuffer.height = height;

        glBindTexture( GL_TEXTURE_2D, sceneFramebuffer.colorAttachment);
            glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, sceneFramebuffer.width, sceneFramebuffer.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture( GL_TEXTURE_2D, 0);

        glBindTexture( GL_TEXTURE_2D, sceneFramebuffer.depthAttachment);
            glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, sceneFramebuffer.width, sceneFramebuffer.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            // glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture( GL_TEXTURE_2D, 0);

        glBindFramebuffer( GL_FRAMEBUFFER, sceneFramebuffer.framebuffer); //bound framebuffer on read as well as draw
            glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneFramebuffer.colorAttachment, 0);
            glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneFramebuffer.depthAttachment, 0);

            if( glCheckFramebufferStatus( GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                fprintf( gpLogFile, "Scene Framebuffer is not complete\n");
                DestroyWindow( ghwnd);
                return;
            }
            else
            {
                fprintf( gpLogFile, "Scene Framebuffer is complete\n");
            }
        glBindFramebuffer( GL_FRAMEBUFFER, 0);
    }
}

//
//Display()
//
void Display( void)
{
    //function declaration
    void RenderWaterMark( void);

    //variable declarations
    static unsigned int Time = GetTickCount();

    unsigned int timeNow = GetTickCount();
    deltaTime = float( timeNow - Time) / 1000.0f;
    //deltaTime += 0.01f;

    vmath::mat4 model_matrix = vmath::mat4::identity();
    vmath::mat4 view_matrix = vmath::mat4::identity();

    char str[128];
    int stringLength;
    float fontSize;

    int polygonMode;

    //code
    if( currentScene == INITIAL_SCENE)
    {
        glViewport( 0, 0, g_windowWidth, g_windowHeight);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        vmath::vec4 color = vmath::vec4( 0.0f, 0.8f, 0.0f, 1.0f ) * alpha_dt;

        FontResetCursor_FreeType( TahomaFreeTypeFont);
        FontSetScale_FreeType( TahomaFreeTypeFont, 2.0f, 1.77f);

        FontSetColor_FreeType( TahomaFreeTypeFont, color[0], color[1], color[2], color[3]);

        sprintf( stringMessage, "Waving Grass Rendering");
        stringLength = FontGetLength_FreeType( TahomaFreeTypeFont, stringMessage);

        FontSetCursor_FreeType( TahomaFreeTypeFont, (g_windowWidth - stringLength) * 0.5f, g_windowHeight/2 + 11.54f + 30.0f);    
        FontCursorPrintText2D_FreeType( TahomaFreeTypeFont, stringMessage, g_windowWidth, g_windowHeight);


        sprintf( stringMessage, "Using OpenCL");
        stringLength = FontGetLength_FreeType( TahomaFreeTypeFont, stringMessage);

        FontSetCursor_FreeType( TahomaFreeTypeFont, (g_windowWidth - stringLength) * 0.5f, g_windowHeight/2 - 83.0f + 30.0f);    
        FontCursorPrintText2D_FreeType( TahomaFreeTypeFont, stringMessage, g_windowWidth, g_windowHeight);

        if( fadeIn)
        {
            alpha_dt = alpha_dt + 0.0001f;

            if( alpha_dt >= 1.0f)
            {
                fadeIn = false;
                fadeOut = true;
            }
        }
        else if( fadeOut)
        {
            alpha_dt = alpha_dt - 0.0001f;

            if( alpha_dt <= 0.0f)
            {
                fadeOut = false;
                currentScene = GRASS_SCENE;
                alpha_dt = 1.0f;
                PlaySoundA( "assets/Renaissance.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
            }
        }

    }
    else if( currentScene == GRASS_SCENE)
    {

#if USE_FREE_CAMERA
        view_matrix = g_camera->getViewMatrix();
#endif

#if USE_ARC_CAMERA
        view_matrix = g_arcCamera.getViewMatrix();
#endif

        // sprintf(str, "%f, %f, %f", view_matrix[3][0], view_matrix[3][1], view_matrix[3][2]);
        // SetWindowTextA( ghwnd, str);

        if( gbEnableMSAA)
        {
            glBindFramebuffer( GL_FRAMEBUFFER, msaaFramebuffer.framebuffer);
            glViewport( 0, 0, msaaFramebuffer.width, msaaFramebuffer.height);
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        else
        {
            glBindFramebuffer( GL_FRAMEBUFFER, sceneFramebuffer.framebuffer);
            glViewport( 0, 0, sceneFramebuffer.width, sceneFramebuffer.height);
            glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }


        if( gbEnableLight == true)
        {
            glUseProgram( program_light);
                
                glUniformMatrix4fv( glGetUniformLocation( program_light, "PMatrix"), 1, GL_FALSE, projection_matrix);
                glUniformMatrix4fv( glGetUniformLocation( program_light, "VMatrix"), 1, GL_FALSE, view_matrix);
                glUniformMatrix4fv( glGetUniformLocation( program_light, "MMatrix"), 1, GL_FALSE, vmath::mat4::identity());

                float pointSize;
                glGetFloatv( GL_POINT_SIZE, &pointSize);
                glPointSize( 8.0f);

                glBindVertexArray( vao_light);
                    glBindBuffer( GL_ARRAY_BUFFER, vbo_light);

                    float position[3];
                    for( int i = 0; i < _ARRAYSIZE( gLights); i++)
                    {
                        position[0] = gLights[i].position[0];// + lightRotateRadius * cosf( lightAngle);
                        position[1] = gLights[i].position[1];
                        position[2] = gLights[i].position[2];// + lightRotateRadius * sinf( lightAngle);
                        glBufferData( GL_ARRAY_BUFFER, sizeof(position), position, GL_DYNAMIC_DRAW);
                        glUniform4f( glGetUniformLocation( program_light, "color"), gLights[i].color[0], gLights[i].color[1], gLights[i].color[2], 1.0f);

                        glEnable( GL_POINT_SMOOTH);
                        glHint( GL_POINT_SMOOTH_HINT, GL_NICEST);
                        glDrawArrays( GL_POINTS, 0, 1);
                    }
                    glBindBuffer( GL_ARRAY_BUFFER, 0);
                glBindVertexArray( 0);

                glPointSize( pointSize);

            glUseProgram( 0);
        }

        glUseProgram( program_grass);

            if( gbEnableLight)
            {
                glUniform1i( glGetUniformLocation( program_grass, "useLight"), 1);
            
                for( int i = 0; i < _ARRAYSIZE( gLights); i++)
                {
                    glUniform3f(
                        glGetUniformLocation( program_grass, "LightPosition") + i,
                        gLights[i].position[0], // + lightRotateRadius * cosf( lightAngle),
                        gLights[i].position[1],
                        gLights[i].position[2] // + lightRotateRadius * sinf( lightAngle)
                    );
                    glUniform3f(
                        glGetUniformLocation( program_grass, "LightColor") + i,
                        gLights[i].color[0],
                        gLights[i].color[1],
                        gLights[i].color[2]
                    );

                    glUniform1f( glGetUniformLocation( program_grass, "LightConstant") + i, gLights[i].attenuation[0]); 
                    glUniform1f( glGetUniformLocation( program_grass, "LightLinear") + i, gLights[i].attenuation[1]);
                    glUniform1f( glGetUniformLocation( program_grass, "LightQuadratic") + i, gLights[i].attenuation[2]);
                }
            }
            else
            {
                glUniform1i( glGetUniformLocation( program_grass, "useLight"), 0);
            }


                //GROUND
            glUniformMatrix4fv( glGetUniformLocation( program_grass, "PMatrix"), 1, GL_FALSE, projection_matrix);
            glUniformMatrix4fv( glGetUniformLocation( program_grass, "VMatrix"), 1, GL_FALSE, view_matrix);
            glUniformMatrix4fv( glGetUniformLocation( program_grass, "MMatrix"), 1, GL_FALSE, vmath::rotate(90.0f, 1.0f, 0.0f, 0.0f) * vmath::scale(currentMeshWidth/2 * MESH_MULTIPLICANT));

            // glUniform3f( glGetUniformLocation( program_grass, "_TopColor"), 0.26f, 0.13f, 0.04f);
            // glUniform3f( glGetUniformLocation( program_grass, "_BottomColor"), 0.56f, 0.29f, 0.08f);

            glUniform1f( glGetUniformLocation( program_grass, "TranslucentGain"), 0.0f);

            glActiveTexture( GL_TEXTURE0);
            glBindTexture( GL_TEXTURE_2D, groundTexture);
            glUniform1i( glGetUniformLocation( program_grass, "GrassBladeSample"), 0);

            glActiveTexture( GL_TEXTURE1);
            glBindTexture( GL_TEXTURE_2D, groundAlphaTexture);
            glUniform1i( glGetUniformLocation( program_grass, "GrassBladeAlphaSample"), 1);

            glBindVertexArray( vao_quad);
                glDrawArrays( GL_TRIANGLES, 0, 6);
            glBindVertexArray( 0);

            glActiveTexture( GL_TEXTURE0);
            glBindTexture( GL_TEXTURE_2D, 0);

            glActiveTexture( GL_TEXTURE1);
            glBindTexture( GL_TEXTURE_2D, 0);


                //GRASS
            glUniformMatrix4fv( glGetUniformLocation( program_grass, "PMatrix"), 1, GL_FALSE, projection_matrix);
            glUniformMatrix4fv( glGetUniformLocation( program_grass, "VMatrix"), 1, GL_FALSE, view_matrix);
            glUniformMatrix4fv( glGetUniformLocation( program_grass, "MMatrix"), 1, GL_FALSE, model_matrix);

            glUniform1f( glGetUniformLocation( program_grass, "TranslucentGain"), 0.504f);

            // glUniform3f( glGetUniformLocation( program_grass, "_TopColor"), 0.5792569, 0.84599996, 0.3297231);
            // glUniform3f( glGetUniformLocation( program_grass, "_BottomColor"), 0.061297283, 0.378, 0.07151349);

            glActiveTexture( GL_TEXTURE0);
            glBindTexture( GL_TEXTURE_2D, grassBladeTexture);
            glUniform1i( glGetUniformLocation( program_grass, "GrassBladeSample"), 0);

            glActiveTexture( GL_TEXTURE1);
            glBindTexture( GL_TEXTURE_2D, grassBladeAlphaTexture);
            glUniform1i( glGetUniformLocation( program_grass, "GrassBladeAlphaSample"), 1);


            if( bOnGPU)
            {
                glBindVertexArray( vao_grass_opencl);
                    glDrawElements( GL_TRIANGLES, grassIndicesCount, GL_UNSIGNED_INT, 0);
                glBindVertexArray( 0);
            }
            else
            {
                glBindVertexArray( vao_grass_cpu);
                    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbo_element_common);
                    glDrawElements( GL_TRIANGLES, grassIndicesCount, GL_UNSIGNED_INT, 0);
                    // glDrawArrays( GL_LINES, 0, grassVerticesCount);
                glBindVertexArray( 0);
            }

            glActiveTexture( GL_TEXTURE0);
            glBindTexture( GL_TEXTURE_2D, 0);

            glActiveTexture( GL_TEXTURE1);
            glBindTexture( GL_TEXTURE_2D, 0);
        glUseProgram( 0);

        if( gbEnableMSAA)
        {
            glBindFramebuffer( GL_FRAMEBUFFER, 0);

                //copy (filter) data from MSAA frambuffer to default framebuffer
            glBindFramebuffer( GL_READ_FRAMEBUFFER, msaaFramebuffer.framebuffer);   //read from MSAA framebuffer
            glBindFramebuffer( GL_DRAW_FRAMEBUFFER, sceneFramebuffer.framebuffer); //draw to default framebuffer

                glBlitFramebuffer( 
                    0, 0, msaaFramebuffer.width, msaaFramebuffer.height,
                    0, 0, sceneFramebuffer.width, sceneFramebuffer.height,
                    GL_COLOR_BUFFER_BIT ,
                    GL_NEAREST
                );
        }

        glBindFramebuffer( GL_FRAMEBUFFER, 0);


            //Default Framebuffer
        glViewport( 0, 0, g_windowWidth, g_windowHeight);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        
        glGetIntegerv( GL_POLYGON_MODE, &polygonMode);
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);

        //HDR display
        glUseProgram( program_hdr_display);
            glUniform1f( glGetUniformLocation( program_hdr_display, "exposure"), 0.5f);

            glActiveTexture( GL_TEXTURE0);
            glBindTexture( GL_TEXTURE_2D, sceneFramebuffer.colorAttachment);
            glUniform1i( glGetUniformLocation( program_hdr_display, "hdrBuffer"), 0);

            glBindVertexArray( vao_quad);
                glDrawArrays( GL_TRIANGLES, 0, 6);
            glBindVertexArray( 0);

            glBindTexture( GL_TEXTURE_2D, 0);
        glUseProgram( 0);


        glPolygonMode( GL_FRONT_AND_BACK, polygonMode);



        //--------------------------------------------------------------------------------------------------------------------------------------------------------------//

        glDisable( GL_DEPTH_TEST);
            fontSize = FontGetSize( NotoSerifBoldFreeTypeFont);

            FontResetCursor_FreeType( NotoSerifBoldFreeTypeFont);
            FontSetScale_FreeType( NotoSerifBoldFreeTypeFont, 1.2f, 1.2f);

            FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 1.0f, 1.0f, 1.0f, 1.0f);

            FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, 50.0f, g_windowHeight - 2.0 * fontSize);

            if( bOnGPU)
            {
                sprintf( stringMessage, "(GPU) %s (OpenCL)", glGetString( GL_RENDERER));
            }
            else
            {
                sprintf( stringMessage, "(CPU) %s", "Intel(R) Xeon(R) Silver 4116 CPU @ 2.10GHz");
            }
            FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, stringMessage, g_windowWidth, g_windowHeight);


                //If FPS count is less than 23 FPS than show FPS in red color else show in green color
            ( currentFPS < 23) ?  FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 0.7f, 0.0f, 0.0f, 1.0f) : FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 0.0f, 0.7f, 0.0f, 1.0f);

            FontSetScale_FreeType( NotoSerifBoldFreeTypeFont, 0.9f, 0.9f);
            FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, 50.0f, g_windowHeight - 3.8 * fontSize * 1.0f);    
            sprintf( stringMessage, "FPS: %d", currentFPS);
            FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, stringMessage, g_windowWidth, g_windowHeight);

            FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, 50.0f, g_windowHeight - 5.0 * fontSize * 1.0f);    
            sprintf( stringMessage, "Time per Frame(sec.):  %g ", timePerFrame);
            FontCursorPrintSingleLineText2D_FreeType( NotoSerifBoldFreeTypeFont, stringMessage, g_windowWidth, g_windowHeight);



            FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 1.0f, 1.0f, 1.0f, 1.0f);

            FontSetScale_FreeType( NotoSerifBoldFreeTypeFont, 0.7f, 0.7f);
            FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, 50.0f, g_windowHeight - 8 * fontSize * 0.8f);    
            sprintf( stringMessage, "Grid Size:   %d X %d", currentMeshWidth, currentMeshHeight);
            FontCursorPrintSingleLineText2D_FreeType( NotoSerifBoldFreeTypeFont, stringMessage, g_windowWidth, g_windowHeight);

            FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, 50.0f, g_windowHeight - 9.2 * fontSize * 0.8f);    
            sprintf( stringMessage, "Vertices Per Blade:  %d", GRASS_BLADE_SEGMENTS * 2);
            FontCursorPrintSingleLineText2D_FreeType( NotoSerifBoldFreeTypeFont, stringMessage, g_windowWidth, g_windowHeight);

            FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, 50.0f, g_windowHeight - 10.4 * fontSize * 0.8f);    
            sprintf( stringMessage, "Vertices Count:  %d", currentMeshWidth * currentMeshHeight * GRASS_BLADE_SEGMENTS * 2);
            FontCursorPrintSingleLineText2D_FreeType( NotoSerifBoldFreeTypeFont, stringMessage, g_windowWidth, g_windowHeight);

            FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, 50.0f, g_windowHeight - 11.6 * fontSize * 0.8f);    
            sprintf( stringMessage, "Triangle Count:  %d", currentMeshWidth * currentMeshHeight * ( GRASS_BLADE_SEGMENTS - 1) * 2);
            FontCursorPrintSingleLineText2D_FreeType( NotoSerifBoldFreeTypeFont, stringMessage, g_windowWidth, g_windowHeight);


            glGetIntegerv( GL_POLYGON_MODE, &polygonMode);
            switch( polygonMode)
            {
                case GL_FILL:
                    sprintf( stringMessage, "Polygon Mode:  Fill");
                break;

                case GL_LINE:
                    sprintf( stringMessage, "Polygon Mode:  Line");
                break;

                case GL_POINT:
                    sprintf( stringMessage, "Polygon Mode:  Point");
                break;
            }
            FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, 50.0f, g_windowHeight - 12.8 * fontSize * 0.8f);    
            FontCursorPrintSingleLineText2D_FreeType( NotoSerifBoldFreeTypeFont, stringMessage, g_windowWidth, g_windowHeight);


            FontSetScale_FreeType( NotoSerifBoldFreeTypeFont, 0.9f, 0.9f);
                //If MSAA is enable than show text in green color else in red color
            gbEnableMSAA ?  FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 0.0f, 0.7f, 0.0f, 1.0f) :  FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 0.7f, 0.0f, 0.0f, 1.0f);

            FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth / 1.23f, g_windowHeight - 2.0 * fontSize);    
            sprintf( stringMessage, "MSAA (4x) :   %s", gbEnableMSAA ? "Enabled" : "Disable");
            FontCursorPrintSingleLineText2D_FreeType( NotoSerifBoldFreeTypeFont, stringMessage, g_windowWidth, g_windowHeight);


            gbEnableLight ?  FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 0.0f, 0.7f, 0.0f, 1.0f) :  FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 0.7f, 0.0f, 0.0f, 1.0f);

            FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth / 1.23f, g_windowHeight - 4.0 * fontSize);
            sprintf( stringMessage, "Lighting :  %s", gbEnableLight ? "ON" : "OFF");
            FontCursorPrintSingleLineText2D_FreeType( NotoSerifBoldFreeTypeFont, stringMessage, g_windowWidth, g_windowHeight);

        glEnable( GL_DEPTH_TEST);



        if( fadeIn || fadeOut)
        {
            glViewport( 0, 0, g_windowWidth, g_windowHeight);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glUseProgram( program_alpha);
                    glUniformMatrix4fv( glGetUniformLocation( program_alpha, "MVPMatrix"), 1, GL_FALSE, vmath::mat4::identity());

                    glUniform1f( glGetUniformLocation( program_alpha, "Alpha"), alpha_dt);
                    glUniform1i( glGetUniformLocation( program_alpha, "useTexture"), 0);
                    glUniform3f( glGetUniformLocation( program_alpha, "Color"), 0.0f, 0.0f, 0.0);

                    glBindVertexArray(vao_quad);
                        glDrawArrays( GL_TRIANGLE_FAN, 0, 6);
                    glBindVertexArray(0);

                glUseProgram(0);

            glDisable(GL_BLEND);

            if( fadeIn)
            {
                alpha_dt = alpha_dt - 0.001f;
                if( alpha_dt <= 0.0f)
                {
                    alpha_dt = 0.0f;
                    fadeIn = false;
                }
            }

            if( fadeOut)
            {
                alpha_dt = alpha_dt + 0.003f;
                if( alpha_dt >= 1.0f)
                {
                    alpha_dt = 1.0f;
                    fadeOut = false;
                    fadeIn = true;
                    currentScene = CREDIT_SCENE;
                }
            }
        }
    }
    else if( currentScene == CREDIT_SCENE)
    {
        glViewport( 0, 0, g_windowWidth, g_windowHeight);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        fontSize = FontGetSize( NotoSerifBoldFreeTypeFont);

        FontResetCursor_FreeType( NotoSerifBoldFreeTypeFont);
        FontSetScale_FreeType( NotoSerifBoldFreeTypeFont, 0.8f, 0.8f);

        stringLength = FontGetLength_FreeType( NotoSerifBoldFreeTypeFont, "Ignited By");
        FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 1.0f, 1.0f, 1.0f, 1.0f);
        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f - 170.0f - g_windowWidth * 0.032f, g_windowHeight - 10.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, "Ignited By", g_windowWidth, g_windowHeight);

        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f - 25.0f - g_windowWidth * 0.032f, g_windowHeight - 10.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, " : ", g_windowWidth, g_windowHeight);

        stringLength = FontGetLength_FreeType( NotoSerifBoldFreeTypeFont, "Dr. Vijay D. Gokhale");
        FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 0.5f, 0.5f, 0.5f, 1.0f);
        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f + 15.0f - g_windowWidth * 0.032f, g_windowHeight - 10.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, "Dr. Vijay D. Gokhale", g_windowWidth, g_windowHeight);


        
        FontSetScale_FreeType( NotoSerifBoldFreeTypeFont, 0.7f, 0.7f);
        FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 1.0f, 1.0f, 1.0f, 1.0f);
        //FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, gLights[0].position[0], g_windowHeight - gLights[0].color[1] * fontSize);    
        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f - 170.0f - g_windowWidth * 0.032f, g_windowHeight - 14.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, "Platform ", g_windowWidth, g_windowHeight);

        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f - 25.0f - g_windowWidth * 0.032f, g_windowHeight - 14.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, " : ", g_windowWidth, g_windowHeight);

        FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 0.5f, 0.5f, 0.5f, 1.0f);
        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f + 15.0f - g_windowWidth * 0.032f, g_windowHeight - 14.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, "Windows 10", g_windowWidth, g_windowHeight);



        FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 1.0f, 1.0f, 1.0f, 1.0f);
        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f - 170.0f - g_windowWidth * 0.032f, g_windowHeight - 16.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, "Reference", g_windowWidth, g_windowHeight);

        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f - 25.0f - g_windowWidth * 0.032f, g_windowHeight - 16.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, " : ", g_windowWidth, g_windowHeight);

        FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 0.5f, 0.5f, 0.5f, 1.0f);
        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f + 15.0f - g_windowWidth * 0.032f, g_windowHeight - 16.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, "RTR Assigment, HPP 2020", g_windowWidth, g_windowHeight);



        FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 1.0f, 1.0f, 1.0f, 1.0f);
        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f - 170.0f - g_windowWidth * 0.032f, g_windowHeight - 18.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, "Technologies", g_windowWidth, g_windowHeight);

        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f - 25.0f - g_windowWidth * 0.032f, g_windowHeight - 18.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, " : ", g_windowWidth, g_windowHeight);

        FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 0.5f, 0.5f, 0.5f, 1.0f);
        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f + 15.0f - g_windowWidth * 0.032f, g_windowHeight - 18.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, "OpenGL, OpenCL", g_windowWidth, g_windowHeight);



        FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 1.0f, 1.0f, 1.0f, 1.0f);
        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f - 170.0f - g_windowWidth * 0.032f, g_windowHeight - 20.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, "Music", g_windowWidth, g_windowHeight);

        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f - 25.0f - g_windowWidth * 0.032f, g_windowHeight - 20.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, " : ", g_windowWidth, g_windowHeight);

        FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 0.5f, 0.5f, 0.5f, 1.0f);
        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f + 15.0f - g_windowWidth * 0.032f, g_windowHeight - 20.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, "Renaissance by Audionautix", g_windowWidth, g_windowHeight);



        FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 1.0f, 1.0f, 1.0f, 1.0f);
        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f - 170.0f - g_windowWidth * 0.032f, g_windowHeight - 22.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, "Presented By", g_windowWidth, g_windowHeight);

        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f - 25.0f - g_windowWidth * 0.032f, g_windowHeight - 22.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, " : ", g_windowWidth, g_windowHeight);

        FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 0.5f, 0.5f, 0.5f, 1.0f);
        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, g_windowWidth * 0.5f + 15.0f - g_windowWidth * 0.032f, g_windowHeight - 22.0f * fontSize - g_windowHeight * 0.084f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, "Vijaykumar J. Dangi (RTR2021 Shader Group Leader)", g_windowWidth, g_windowHeight);


        if( fadeOut)
        {
            glViewport( 0, 0, g_windowWidth, g_windowHeight);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glUseProgram( program_alpha);
                    glUniformMatrix4fv( glGetUniformLocation( program_alpha, "MVPMatrix"), 1, GL_FALSE, vmath::mat4::identity());

                    glUniform1f( glGetUniformLocation( program_alpha, "Alpha"), alpha_dt);
                    glUniform1i( glGetUniformLocation( program_alpha, "useTexture"), 0);
                    glUniform3f( glGetUniformLocation( program_alpha, "Color"), 0.0f, 0.0f, 0.0f);

                    glBindVertexArray(vao_quad);
                        glDrawArrays( GL_TRIANGLE_FAN, 0, 6);
                    glBindVertexArray(0);

                glUseProgram(0);

            glDisable(GL_BLEND);

            if( fadeOut)
            {
                alpha_dt = alpha_dt + 0.001f;
                if( alpha_dt >= 1.0f)
                {
                    alpha_dt = 1.0f;
                    fadeOut = false;

                    currentScene = END_SCENE;
                }
            }
        }
    }
    else if( currentScene == END_SCENE)
    {
        glViewport( 0, 0, g_windowWidth, g_windowHeight);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        fontSize = FontGetSize( NotoSerifBoldFreeTypeFont);

        FontResetCursor_FreeType( NotoSerifBoldFreeTypeFont);
        FontSetScale_FreeType( NotoSerifBoldFreeTypeFont, 1.4f, 1.4f);

        stringLength = FontGetLength_FreeType( NotoSerifBoldFreeTypeFont, "Thank You");
        FontSetColor_FreeType( NotoSerifBoldFreeTypeFont, 1.0f, 1.0f, 1.0f, 1.0f);
        FontSetCursor_FreeType( NotoSerifBoldFreeTypeFont, ( g_windowWidth - stringLength) * 0.5f, ( g_windowHeight - fontSize) * 0.5f);
        FontCursorPrintText2D_FreeType( NotoSerifBoldFreeTypeFont, "Thank You", g_windowWidth, g_windowHeight);

        if( fadeOut)
        {
            glViewport( 0, 0, g_windowWidth, g_windowHeight);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glUseProgram( program_alpha);
                    glUniformMatrix4fv( glGetUniformLocation( program_alpha, "MVPMatrix"), 1, GL_FALSE, vmath::mat4::identity());

                    glUniform1f( glGetUniformLocation( program_alpha, "Alpha"), alpha_dt);
                    glUniform1i( glGetUniformLocation( program_alpha, "useTexture"), 0);
                    glUniform3f( glGetUniformLocation( program_alpha, "Color"), 0.0f, 0.0f, 0.0f);

                    glBindVertexArray(vao_quad);
                        glDrawArrays( GL_TRIANGLE_FAN, 0, 6);
                    glBindVertexArray(0);

                glUseProgram(0);

            glDisable(GL_BLEND);

            if( fadeOut)
            {
                alpha_dt = alpha_dt + 0.001f;
                if( alpha_dt >= 1.0f)
                {
                    alpha_dt = 1.0f;
                    fadeOut = false;

                    currentScene = END_SCENE + 1;
                }
            }
        }
    }
    //--------------------------------------------------------------------------------------------------------------------------------------------------------------//

    RenderWaterMark();

    SwapBuffers( ghdc);
}


//
//RenderWaterMark()
//
void RenderWaterMark( void)
{
    //variable declarations
    int polygonMode;

    //code
    glGetIntegerv( GL_POLYGON_MODE, &polygonMode);
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);

    glViewport( g_windowWidth - 120, 20, 100, 100);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram( program_alpha);
            glUniformMatrix4fv( glGetUniformLocation( program_alpha, "MVPMatrix"), 1, GL_FALSE, vmath::mat4::identity());

            glUniform1f( glGetUniformLocation( program_alpha, "Alpha"), 0.3f);
            glUniform1i( glGetUniformLocation( program_alpha, "useTexture"), 1);

            glActiveTexture( GL_TEXTURE0);
            glBindTexture( GL_TEXTURE_2D, waterMarkTexture);
            glUniform1i( glGetUniformLocation( program_alpha, "tex"), 0);

            glBindVertexArray(vao_quad);
                glDrawArrays( GL_TRIANGLE_FAN, 0, 6);
            glBindVertexArray(0);

            glBindTexture( GL_TEXTURE_2D, 0);
        glUseProgram(0);

    glDisable(GL_BLEND);

    glPolygonMode( GL_FRONT_AND_BACK, polygonMode);
}


//
//RotationMatrix()
//
vmath::mat4 RotationMatrix( float angleInRadians, float x, float y, float z)
{
    //code
    float s = sinf( angleInRadians);
    float c = cosf( angleInRadians);
    float t = 1.0 - c;

    return( vmath::mat4(
                vmath::vec4( x*x*t + c  , y*x*t + z*s, x*z*t - y*s, 0.0f),
                vmath::vec4( x*y*t - z*s, y*y*t + c  , y*z*t + x*s, 0.0f),
                vmath::vec4( x*z*t + y*s, y*z*t - x*s, z*z*t + c  , 0.0f),
                vmath::vec4( 0.0f, 0.0f, 0.0f, 1.0f)
            )
    );
}


//
//getTexel() :- Return color from specified texcoord location
//
vmath::vec4 getTexel( vmath::vec2 uv, IMAGE_DATA *texture)
{
    //code
    int x = floor( mymath::fract(uv[0]) * ( texture->width ));
    int y = floor( mymath::fract(uv[1]) * ( texture->height));

    vmath::vec4 sample = vmath::vec4( 0.0f);

    sample[0] = texture->normalizeImageData[ COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 0]; //red
    sample[1] = texture->normalizeImageData[ COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 1]; //green
    sample[2] = texture->normalizeImageData[ COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 2]; //blue
    sample[3] = texture->normalizeImageData[ COLOR_CHANNELS * ( y * windDistortion_map.width + x) + 3]; //alpha

    return( sample);
}


//
//Update()
//
void Update( void)
{
    //function declaration
    void UpdateKeyboardState( void);
    void UpdateGrassData( void);

    //code
#if USE_ARC_CAMERA
    g_arcCamera.updateAngleAroundPoint( 0.1f);
#endif

    //UpdateKeyboardState();
    if( currentScene == GRASS_SCENE)
    {
        UpdateGrassData();
    }

    if( gbEnableLight)
    {
        lightAngle = (lightAngle + 1.0f) * mymath::PI / 180.0f;
        if( lightAngle >= mymath::TWO_PI)
        {
            lightAngle = lightAngle - mymath::TWO_PI;
        }
    }
}

//
//updateKeyboardState()
//
void UpdateKeyboardState( void)
{
    //variable declarations
    static unsigned char keyState[256];
    const unsigned char HIGH_BIT = 1 << 7;
    bool isDown, wasDown;
    unsigned char newState = 0;

    //code
    ZeroMemory( keyState, sizeof( keyState));

    GetKeyboardState( keyState);

    for( int i = VK_LBUTTON; i <= VK_OEM_CLEAR; i++)
    {
        isDown = (keyState[i] & HIGH_BIT) != 0;
        wasDown = (g_keyboardState[i] & KeyState::KEY_STATE_HELD) != 0;

        newState = KeyState::KEY_STATE_NONE;

        if( isDown)
        {
            newState = newState | KeyState::KEY_STATE_HELD;
        }
        if( isDown && !wasDown)
        {
            newState = newState | KeyState::KEY_STATE_PRESSED;
        }
        if( !isDown && wasDown)
        {
            newState = newState | KeyState::KEY_STATE_RELEASED;
        }

        g_keyboardState[i] = newState;
    }
}

//
//UpdateGrassData()
//
void UpdateGrassData( void)
{
    //variable declarations
    static const float grassBladeHeight = 0.83f;
    static const float grassBladeHeightRandom = 0.26f;
    static const float grassBladeWidth = 0.03f;
    static const float grassBladeWidthRandom = 0.01f;
    static const float grassBendRotationRandom = 0.4f;
    static const float grassBladeForwardAmount = 0.515f;
    static const float grassBladeCurvatureAmount = 1.18f;

    static const vmath::vec2 windFrequency = { 0.05f, 0.05f};
    static const vmath::vec2 windScale = { 0.009f, 0.009f};
    static const vmath::vec2 windOffset = { 0.0f, 0.0f};
    static const float windStrength = 0.345f;

    int verticesPerBlade = 2 * GRASS_BLADE_SEGMENTS;

    //code

    /****
     *   This initial update of vertices buffer and index buffer require whenever mesh size changes.
     ****/
    if( bNeedToUpdateBuffers)
    {
        grassVerticesCount = currentMeshWidth * currentMeshHeight;

        CreateMesh( 0, 0, currentMeshWidth, currentMeshHeight, MESH_MULTIPLICANT, MESH_AMPLITUDE, meshVertexData, HeightCalculate);

        //fill opencl buffer
        int bufferSize = grassVerticesCount * sizeof(VERTEX);
        clResult = clEnqueueWriteBuffer(
            oclCommandQueue,
            meshVertexData_opencl_input,
            CL_FALSE,
            0,
            bufferSize,
            meshVertexData,
            0,
            NULL, NULL
        );
        if( clResult != CL_SUCCESS)
        {
            fprintf( gpLogFile, "OpenCL Error: clEnqueueWriteBuffer() Failed: %d\n", clResult);
            DestroyWindow( ghwnd);
            return;
        }
        
        //Update Index Buffer
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grassMeshBuffer.vbo_element);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_element_common);
        GLuint *indexBufferPtr = (GLuint *) glMapBuffer( GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

        int indexPointer = 0;
        for( int i = 0; i < grassVerticesCount; i++)
        {
            for( int j = 0; j < GRASS_BLADE_SEGMENTS - 1; j++)
            {
                indexBufferPtr[indexPointer++] = (i * verticesPerBlade) + (2 * j + 0);
                indexBufferPtr[indexPointer++] = (i * verticesPerBlade) + (2 * j + 2);
                indexBufferPtr[indexPointer++] = (i * verticesPerBlade) + (2 * j + 3);

                indexBufferPtr[indexPointer++] = (i * verticesPerBlade) + (2 * j + 0);
                indexBufferPtr[indexPointer++] = (i * verticesPerBlade) + (2 * j + 3);
                indexBufferPtr[indexPointer++] = (i * verticesPerBlade) + (2 * j + 1);
            }
        }
        grassIndicesCount = indexPointer;


        glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER);
        indexBufferPtr = NULL;
            //glBufferData(GL_ELEMENT_ARRAY_BUFFER, grassMeshBuffer.elementCount * sizeof(GLuint), grassIndexBuffer, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);



        //Update grass static properties        //static means the properties which are not changing
        Vector3f pos, normal, tangent;
        float angle;
        for (int i = 0; i < grassVerticesCount; i++)
        {
            pos = *((Vector3f *) &meshVertexData[i].position);
            normal = *((Vector3f *) &meshVertexData[i].normal);
            tangent = *((Vector3f *) &meshVertexData[i].tangent);
            
            Vector3f biNormal = normal.cross(tangent);

            grassStaticProps_cpu[i].tangentToLocalMatrix = vmath::mat4(
                vmath::vec4( tangent.x,  tangent.y,  tangent.z,  0.0f),
                vmath::vec4( biNormal.x, biNormal.y, biNormal.z, 0.0f),
                vmath::vec4( normal.x,   normal.y,   normal.z,   0.0f),
                vmath::vec4( 0.0f, 0.0f, 0.0f, 1.0f)
            );

            //random rotation of vertex but consistent between frames
            angle = random( vmath::vec3( pos.x, pos.y, pos.z)) * mymath::TWO_PI;
            grassStaticProps_cpu[i].facingRotationMatrix = RotationMatrix( angle, 0.0f, 0.0f, 1.0f);//vmath::rotate( vmath::degrees( angle), 0.0f, 0.0f, 1.0f);

            //rotate grass along X-axis
            angle = random( vmath::vec3( pos.z, pos.z, pos.x)) * grassBendRotationRandom * mymath::PI * 0.5f;
            grassStaticProps_cpu[i].bendRotationMatrix = RotationMatrix( angle, -1.0f, 0.0f, 0.0f);
            
            //blade width and height
            grassStaticProps_cpu[i].width  = ( random( vmath::vec3( pos.x, pos.z, pos.y)) * 2.0f - 1.0f) * grassBladeWidthRandom + grassBladeWidth;
            grassStaticProps_cpu[i].height = ( random( vmath::vec3( pos.z, pos.y, pos.x)) * 2.0f - 1.0f) * grassBladeHeightRandom + grassBladeHeight;

            //for curvature of grass we add Y-offset in each vertex. ( Y-offset in tangent space)
            grassStaticProps_cpu[i].forward = random( vmath::vec3( pos.y, pos.y, pos.z)) * grassBladeForwardAmount;

        }

        bNeedToUpdateBuffers = false;
    }



    if( bOnGPU )
    {
        unsigned int mesh_width = currentMeshWidth;
        unsigned int mesh_height = currentMeshHeight;
        unsigned int grassBladeSegment = GRASS_BLADE_SEGMENTS;
        
        //Set Parameter of kernel
        clResult = clSetKernelArg( oclGrassKernel, 0, sizeof( cl_mem), (void *) &cl_graphics_resource_mesh);
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clSetKernelArg() for 0 failed\n");
            DestroyWindow( ghwnd);
        }

        clResult = clSetKernelArg( oclGrassKernel, 1, sizeof( cl_mem), (void *)&meshVertexData_opencl_input);
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clSetKernelArg() for 1 failed\n");
            DestroyWindow( ghwnd);
        }

        clResult = clSetKernelArg( oclGrassKernel, 2, sizeof( cl_uint), (void *)&mesh_width);
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clSetKernelArg() for 2 failed\n");
            DestroyWindow( ghwnd);
        }

        clResult = clSetKernelArg( oclGrassKernel, 3, sizeof( cl_uint), (void *)&mesh_height);
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clSetKernelArg() for 3 failed\n");
            DestroyWindow( ghwnd);
        }

        clResult = clSetKernelArg( oclGrassKernel, 4, sizeof( cl_uint), (void *)&grassBladeSegment);
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clSetKernelArg() for 4 failed\n");
            DestroyWindow( ghwnd);
        }

        clResult = clSetKernelArg( oclGrassKernel, 5, sizeof( cl_mem), (void *)&distortionMap_opencl_input);
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clSetKernelArg() for 5 failed\n");
            DestroyWindow( ghwnd);
        }

        clResult = clSetKernelArg( oclGrassKernel, 6, sizeof( cl_int), (void *)&windDistortion_map.width);
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clSetKernelArg() for 6 failed\n");
            DestroyWindow( ghwnd);
        }

        clResult = clSetKernelArg( oclGrassKernel, 7, sizeof( cl_int), (void *)&windDistortion_map.height);
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clSetKernelArg() for 7 failed\n");
            DestroyWindow( ghwnd);
        }

        float t = 0.8f * deltaTime;
        clResult = clSetKernelArg( oclGrassKernel, 8, sizeof( cl_float), (void *)&t);
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clSetKernelArg() for 8 failed\n");
            DestroyWindow( ghwnd);
        }

        //map resource
        clResult = clEnqueueAcquireGLObjects( oclCommandQueue, 1, &cl_graphics_resource_mesh, 0, NULL, NULL);
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clEnqueueAcquireGLObjects() Failed\n");
            DestroyWindow( ghwnd);
        }

        //run kernel
        size_t globalWorkSize[2];
        globalWorkSize[0] = mesh_width;
        globalWorkSize[1] = mesh_height;

        clResult = clEnqueueNDRangeKernel(
            oclCommandQueue,
            oclGrassKernel,
            2,              //Work Dimension
            NULL,           //global_work_offset
            globalWorkSize, //global work size
            NULL,           //local work size
            0,
            NULL,
            NULL
        );
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clEnqueueNDRangeKernel() failed\n");
            DestroyWindow( ghwnd);
        }

        //unmape / release resource
        clResult = clEnqueueReleaseGLObjects( oclCommandQueue, 1, &cl_graphics_resource_mesh, 0, NULL, NULL);
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clEnqueueReleaseGLObjects() failed\n");
            DestroyWindow( ghwnd);
        }

        clResult = clFinish( oclCommandQueue);
        if( CL_SUCCESS != clResult)
        {
            fprintf( gpLogFile, "clFinish() failed\n");
            DestroyWindow( ghwnd);
        }


        // glBindBuffer(GL_ARRAY_BUFFER, vbo_grassBuffer_opencl);
        // GRASS_VERTEX *grassVertex = (GRASS_VERTEX *) glMapBuffer( GL_ARRAY_BUFFER, GL_READ_ONLY);  //get pointer from buffer so we can update data into it
        //     fprintf( gpLogFile, "\n\n--------------- GPU --------------------\n");
        //     // for( int i = 0; i <meshVertexCount; i++)
        //     // {
        //     //     for( int j = 0; j < GRASS_BLADE_SEGMENTS; j++)
        //     //     {
        //     //         fprintf(
        //     //             gpLogFile,
        //     //             "[%2.6f, %2.6f, %2.6f], [%2.6f, %2.6f, %2.6f]\n",
        //     //             grassVertex[(verticesPerBlade*i + 2*j + 0)].position[0], grassVertex[(verticesPerBlade*i + 2*j + 0)].position[1], grassVertex[(verticesPerBlade*i + 2*j + 0)].position[2],
        //     //             grassVertex[(verticesPerBlade*i + 2*j + 1)].position[0], grassVertex[(verticesPerBlade*i + 2*j + 1)].position[1], grassVertex[(verticesPerBlade*i + 2*j + 1)].position[2]
        //     //         );
        //     //     }
        //     // }
        //     for( int i = 0; i < grassMeshBuffer.vertexCount; i++)
        //     {
        //         fprintf(
        //             gpLogFile,
        //             "[%2.6f, %2.6f, %2.6f], ",
        //             grassVertex[i].position[0], grassVertex[i].position[1], grassVertex[i].position[2]
        //         );
        //         if( i%2 == 1)
        //         {
        //             fprintf( gpLogFile, "\n");
        //         }
        //     }
        //     fprintf( gpLogFile, "<<<<<<<<<<<<<<<<<<<<<<<< GPU >>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
        // glUnmapBuffer( GL_ARRAY_BUFFER);
        // glBindBuffer(GL_ARRAY_BUFFER, 0);

    }
    else
    {
        int i, j;
        float t;
        float segmentHeight, segmentWidth, segmentForward;
        Vector3f pos;

        vmath::vec4 tangentPoint;
        vmath::vec4 localPosition;
        vmath::vec4 tangentNormal;
        vmath::vec4 localNormal;

        vmath::mat4 transformationMatrix;
        vmath::mat4 baseTransformationMatrix;
        vmath::mat4 windRotationMatrix;

        vmath::vec2 uv;
        vmath::vec4 color;
        vmath::vec2 windSample;
        vmath::vec3 windDirection;

        vmath::vec2 windParam = windOffset + windFrequency * deltaTime;
        int index;

        glBindBuffer(GL_ARRAY_BUFFER, vbo_grassBuffer_cpu);
        GRASS_VERTEX *grassVertex = (GRASS_VERTEX *) glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY);  //get pointer from buffer so we can update data into it

            for (i = 0; i < grassVerticesCount; i++)        // grass position
            {
                pos = *((Vector3f *) &meshVertexData[i].position);

                // //ADD WIND
                uv = vmath::vec2( pos.x, pos.z) * windScale + windParam;
                color = getTexel( uv, &windDistortion_map);

                windSample = ( (vmath::vec2( color[0], color[1]) * 2.0f) - 1.0f) * windStrength;
                    //normalize vector representing direction
                // vmath::vec3 wind = vmath::normalize( vmath::vec3( 0.0f, windSample[0], 0.0f));
                windDirection = vmath::normalize( vmath::vec3( windSample[0], windSample[1], 0.0f));
                    //construct matrix to rotate above vector
                windRotationMatrix = RotationMatrix( mymath::PI * windSample[0], windDirection[0], windDirection[1], windDirection[2]);

                    //for base vertices, we don't want to bend or rotate base vertices
                baseTransformationMatrix = grassStaticProps_cpu[i].tangentToLocalMatrix * grassStaticProps_cpu[i].facingRotationMatrix;

                    //for vertices other than base  vertices, as we want them to move with wind
                transformationMatrix = grassStaticProps_cpu[i].tangentToLocalMatrix * windRotationMatrix * grassStaticProps_cpu[i].facingRotationMatrix * grassStaticProps_cpu[i].bendRotationMatrix;


                for( j = 0; j < GRASS_BLADE_SEGMENTS; j++)      //vertices of single grass blade
                {
                    index = verticesPerBlade * i + 2*j;

                    t = (float)j / (float)(GRASS_BLADE_SEGMENTS - 1);

                    segmentWidth = grassStaticProps_cpu[i].width * ( 1 - t);
                    segmentHeight = grassStaticProps_cpu[i].height * t;
                    // segmentForward = grassStaticProps[i].forward * t;
                    segmentForward = powf( t, 2.0f * grassBladeCurvatureAmount) * grassStaticProps_cpu[i].forward;

                    tangentNormal = vmath::vec4( 0.0f, -1.0f, segmentForward, 0.0f);
                    localNormal = ((j == 0) ? baseTransformationMatrix : transformationMatrix) * tangentNormal;

                        //////////////////////////////////////////
                    tangentPoint = vmath::vec4( segmentWidth, segmentForward, segmentHeight, 0.0f);
                    localPosition = ((j == 0) ? baseTransformationMatrix : transformationMatrix) * tangentPoint;    //don't bend base vertices
                    localPosition[0] += pos.x;
                    localPosition[1] += pos.y;
                    localPosition[2] += pos.z;

                    //memcpy( grassVertex[index + 0].position, &localPosition[0], sizeof(float) * 3);
                    grassVertex[index + 0].position[0] = localPosition[0];
                    grassVertex[index + 0].position[1] = localPosition[1];
                    grassVertex[index + 0].position[2] = localPosition[2];

                    // memcpy( grassVertex[index + 0].normal, &localNormal[0], sizeof(float) * 3);
                    grassVertex[index + 0].normal[0] = localNormal[0];
                    grassVertex[index + 0].normal[1] = localNormal[1];
                    grassVertex[index + 0].normal[2] = localNormal[2];

                    grassVertex[index + 0].texcoord[0] = 0.0f;
                    grassVertex[index + 0].texcoord[1] = t;


                        //////////////////////////////////////////
                    tangentPoint = vmath::vec4( -segmentWidth, segmentForward, segmentHeight, 0.0f);
                    localPosition = ((j == 0) ? baseTransformationMatrix : transformationMatrix) * tangentPoint;    //don't bend base vertices
                    localPosition[0] += pos.x;
                    localPosition[1] += pos.y;
                    localPosition[2] += pos.z;

                    // memcpy( grassVertex[index + 1].position, &localPosition[0], sizeof(float) * 3);
                    grassVertex[index + 1].position[0] = localPosition[0];
                    grassVertex[index + 1].position[1] = localPosition[1];
                    grassVertex[index + 1].position[2] = localPosition[2];

                    // memcpy( grassVertex[index + 1].normal, &localNormal[0], sizeof(float) * 3);
                    grassVertex[index + 1].normal[0] = localNormal[0];
                    grassVertex[index + 1].normal[1] = localNormal[1];
                    grassVertex[index + 1].normal[2] = localNormal[2];

                    grassVertex[index + 1].texcoord[0] = 1.0f;
                    grassVertex[index + 1].texcoord[1] = t;
                }
            }

        glUnmapBuffer( GL_ARRAY_BUFFER);
        grassVertex = NULL;
        glBindBuffer(GL_ARRAY_BUFFER, 0);


        // glBindBuffer(GL_ARRAY_BUFFER, vbo_grassBuffer_cpu);
        // grassVertex = (GRASS_VERTEX *) glMapBuffer( GL_ARRAY_BUFFER, GL_READ_ONLY);  //get pointer from buffer so we can update data into it
        //     fprintf( gpLogFile, "\n\n--------------- CPU --------------------\n");
        //     for( int i = 0; i < grassVerticesCount; i++)
        //     {
        //         fprintf(
        //             gpLogFile,
        //             "[%2.6f, %2.6f, %2.6f], ",
        //             grassVertex[i].position[0], grassVertex[i].position[1], grassVertex[i].position[2]
        //         );
        //         if(i%2 == 1)
        //         {
        //             fprintf( gpLogFile, "\n");
        //         }
        //     }
        //     fprintf( gpLogFile, ">>>>>>>>>>>>>>>>>>>>>>>>>>>> CPU <<<<<<<<<<<<<<<<<<<<<<<<\n");
        // glUnmapBuffer( GL_ARRAY_BUFFER);
        // glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

//
//Uninitialize()
//
void Uninitialize( void)
{
    //code
    PlaySoundA( NULL, NULL, NULL);

    if( gbFullScreen == true)
    {
        SetWindowLong( ghwnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement( ghwnd, &wpPrev);
        SetWindowPos(
            ghwnd, HWND_TOP,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED
        );
    }

    FontRenderingUninitialize_FreeType();

    /* _________________________ OpenCL Uninitialize ________________________ */
    if( distortionMap_opencl_input)
    {
        clReleaseMemObject( distortionMap_opencl_input);
        distortionMap_opencl_input = NULL;
    }

    if( meshVertexData_opencl_input)
    {
        clReleaseMemObject( meshVertexData_opencl_input);
        meshVertexData_opencl_input = NULL;
    }

    if( cl_graphics_resource_mesh)
    {
        clReleaseMemObject( cl_graphics_resource_mesh);
        cl_graphics_resource_mesh = NULL;
    }

    if( oclGrassKernel)
    {
        clReleaseKernel( oclGrassKernel);
        oclGrassKernel = NULL;
    }

    if( oclGrassProgram)
    {
        clReleaseProgram( oclGrassProgram);
        oclGrassProgram = NULL;
    }

    if( oclCommandQueue)
    {
        clReleaseCommandQueue( oclCommandQueue);
        oclCommandQueue = NULL;
    }

    if( oclContext)
    {
        clReleaseContext( oclContext);
        oclContext = NULL;
    }

    /* ______________________________________________________________________ */

    //Shader Program
    if( program_light)
    {
        DeleteProgram( program_light);
        program_light = 0;
    }

    if( program_alpha)
    {
        DeleteProgram( program_alpha);
        program_alpha = 0;
    }

    if( program_grass)
    {
        DeleteProgram( program_grass);
        program_grass = 0;
    }

    if( program_display)
    {
        DeleteProgram( program_display);
        program_display = 0;
    }

    if( program_hdr_display)
    {
        DeleteProgram( program_hdr_display);
        program_hdr_display = 0;
    }

    //Buffers
    if( grassStaticProps_cpu)
    {
        free( grassStaticProps_cpu);
        grassStaticProps_cpu = NULL;
    }

    DELETE_VERTEX_ARRAY( vao_light);
    DELETE_BUFFER( vbo_light);

    DELETE_VERTEX_ARRAY(vao_grass_opencl)
    DELETE_BUFFER(vbo_grassBuffer_opencl);

    DELETE_VERTEX_ARRAY( vao_grass_cpu);
    DELETE_BUFFER( vbo_grassBuffer_cpu);

    DELETE_BUFFER( vbo_element_common);

    DELETE_VERTEX_ARRAY( vao_quad);
    DELETE_BUFFER( vbo_quad);

    DELETE_TEXTURE( grassBladeTexture);
    DELETE_TEXTURE( grassBladeAlphaTexture);
    DELETE_TEXTURE( groundTexture);
    DELETE_TEXTURE( groundAlphaTexture);
    DELETE_TEXTURE( waterMarkTexture);

    //Texture
    windDistortion_map.Delete();

    //Framebuffer
    sceneFramebuffer.Delete();
    msaaFramebuffer.Delete();

    //Free Font
    if( NotoSerifBoldFreeTypeFont)
    {
        FontDelete( &NotoSerifBoldFreeTypeFont);
        NotoSerifBoldFreeTypeFont = NULL;
    }

    if( TahomaFreeTypeFont)
    {
        FontDelete( &TahomaFreeTypeFont);
        TahomaFreeTypeFont = NULL;
    }

    //Camera

#if USE_FREE_CAMERA
    if(g_camera)
    {
        delete(g_camera);
        g_camera = NULL;
    }
#endif

    wglMakeCurrent( NULL, NULL);

    if( ghrc)
    {
        wglDeleteContext( ghrc);
        ghrc = NULL;
    }

    if( ghdc)
    {
        ReleaseDC( ghwnd, ghdc);
        ghdc = NULL;
    }

    if( gpLogFile)
    {
        fprintf( gpLogFile, "Log File Closed\n");
        fclose( gpLogFile);
        gpLogFile = NULL;
    }
}

