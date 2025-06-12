#include "glad.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 全局变量
Display* display = NULL;
Window win;
GLXContext ctx;

// 创建X窗口和OpenGL上下文
static int createWindow(int width, int height) {
    // 1. 打开X display
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "无法打开X Display\n");
        return 0;
    }

    // 2. 获取根窗口
    Window root = DefaultRootWindow(display);

    // 3. 选择帧缓冲配置
    static int visualAttribs[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        None
    };
    
    int fbcount = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display), visualAttribs, &fbcount);
    if (!fbc || fbcount == 0) {
        fprintf(stderr, "没有找到合适的帧缓冲配置\n");
        return 0;
    }

    // 4. 创建X Window
    XVisualInfo* vi = glXGetVisualFromFBConfig(display, fbc[0]);
    XSetWindowAttributes swa;
    swa.colormap = XCreateColormap(display, root, vi->visual, AllocNone);
    swa.event_mask = ExposureMask | KeyPressMask | StructureNotifyMask;
    swa.background_pixmap = None;
    swa.border_pixel = 0;
    swa.override_redirect = True;

    win = XCreateWindow(display, root, 0, 0, width, height, 0, vi->depth, InputOutput, 
                       vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);
    if (!win) {
        fprintf(stderr, "无法创建窗口\n");
        return 0;
    }

    // 5. 设置窗口标题
    XStoreName(display, win, "GLAD Loader Test (X11)");

    // 6. 创建OpenGL上下文
    ctx = glXCreateNewContext(display, fbc[0], GLX_RGBA_TYPE, NULL, True);
    if (!ctx) {
        fprintf(stderr, "无法创建OpenGL上下文\n");
        return 0;
    }

    // 7. 映射窗口并刷新
    XMapWindow(display, win);
    XFlush(display);

    return 1;
}

static void destroyWindow() {
    if (ctx) {
        glXDestroyContext(display, ctx);
        ctx = NULL;
    }
    if (win) {
        XDestroyWindow(display, win);
        win = 0;
    }
    if (display) {
        XCloseDisplay(display);
        display = NULL;
    }
}

int main() {
    // 创建隐藏窗口（大小1x1，但实际会被映射到屏幕上，但我们可以不显示它）
    if (!createWindow(1, 1)) {
        fprintf(stderr, "创建窗口失败\n");
        return EXIT_FAILURE;
    }

    // 将OpenGL上下文设为当前
    if (!glXMakeCurrent(display, win, ctx)) {
        fprintf(stderr, "无法将上下文设为当前\n");
        destroyWindow();
        return EXIT_FAILURE;
    }

    // 加载OpenGL函数
    int version = gladLoadGL();
    printf("=== 开始GLAD加载测试（使用X11） ===\n");
    if (!version) {
        fprintf(stderr, "[严重错误] gladLoadGL完全失败\n\n");
    } else {
        printf("[成功] 加载OpenGL函数 (API %d.%d)\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
    }

    // 检查核心函数是否加载成功
    int coreFuncLoaded = (glad_glGetString != NULL);
    printf("核心函数（glGetString）加载状态: %s\n", coreFuncLoaded ? "成功" : "失败");

    // 如果核心函数加载成功，则输出OpenGL信息
    if (coreFuncLoaded) {
        const GLubyte *vendor = glGetString(GL_VENDOR);
        const GLubyte *renderer = glGetString(GL_RENDERER);
        const GLubyte *version_str = glGetString(GL_VERSION);
        printf("GL_VENDOR: %s\n", vendor);
        printf("GL_RENDERER: %s\n", renderer);
        printf("GL_VERSION: %s\n", version_str);

        // 检查扩展（示例）
        printf("\n=== 支持的扩展（示例） ===\n");
        if (glad_glGetStringi) {
            // 使用glGetStringi获取扩展列表（需要OpenGL 3.0+）
            GLint num_extensions = 0;
            glad_glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
            for (int i = 0; i < 5 && i < num_extensions; i++) { // 只打印前5个扩展
                const GLubyte *ext = glad_glGetStringi(GL_EXTENSIONS, i);
                printf("扩展 %d: %s\n", i, ext);
            }
        } else {
            printf("glGetStringi不可用，无法枚举扩展。\n");
        }
    }

    // 清理
    destroyWindow();

    printf("\n测试完成。\n");
    return EXIT_SUCCESS;
}