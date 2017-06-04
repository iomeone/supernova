#ifndef engine_h
#define engine_h


#define S_GLES2 1

#define S_SCALING_FITWIDTH 1
#define S_SCALING_FITHEIGHT 2
#define S_SCALING_LETTERBOX 3
#define S_SCALING_CROP 4
#define S_SCALING_STRETCH 5

#define S_ANDROID 1
#define S_IOS 2
#define S_WEB 3

typedef struct lua_State lua_State;

class Scene;

class Engine {
    
private:
    static lua_State *luastate;
    static Scene *mainScene;
    
    static int screenWidth;
    static int screenHeight;
    
    static int canvasWidth;
    static int canvasHeight;
    
    static int preferedCanvasWidth;
    static int preferedCanvasHeight;
    
    static int renderAPI;
    static bool mouseAsTouch;
    static bool useDegrees;
    static int scalingMode;

public:
    
    Engine();
    virtual ~Engine();
    
    //-----Supernova config-----
    static void setLuaState(lua_State*);
    static lua_State* getLuaState();
    
    static void setScene(Scene *mainScene);
    static Scene* getScene();
    
    static int getScreenWidth();
    static int getScreenHeight();
    static void setScreenSize(int screenWidth, int screenHeight);
    
    static int getCanvasWidth();
    static int getCanvasHeight();
    static void setCanvasSize(int canvasWidth, int canvasHeight);
    
    static int getPreferedCanvasWidth();
    static int getPreferedCanvasHeight();
    static void setPreferedCanvasSize(int preferedCanvasWidth, int preferedCanvasHeight);
    
    static void setRenderAPI(int renderAPI);
    static int getRenderAPI();
    
    static void setScalingMode(int scalingMode);
    static int getScalingMode();
    
    static void setMouseAsTouch(bool mouseAsTouch);
    static bool isMouseAsTouch();
    
    static void setUseDegrees(bool useDegrees);
    static bool isUseDegrees();
    
    static int getPlatform();
    
    //-----Supernova API events-----
    static void onStart();
	static void onStart(int width, int height);
	static void onSurfaceCreated();
	static void onSurfaceChanged(int width, int height);
	static void onDrawFrame();

	static void onPause();
	static void onResume();

    static void onTouchPress(float x, float y);
    static void onTouchUp(float x, float y);
    static void onTouchDrag(float x, float y);

    static void onMousePress(int button, float x, float y);
    static void onMouseUp(int button, float x, float y);
    static void onMouseDrag(int button, float x, float y);
    static void onMouseMove(float x, float y);

    static void onKeyPress(int inputKey);
    static void onKeyUp(int inputKey);

    static void onTextInput(const char* text);

};

#endif /* engine_h */
