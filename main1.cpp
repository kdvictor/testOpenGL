#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

// 定义我们需要的函数指针类型
typedef void (*PFNGLGETSTRINGPROC)(int name);
typedef void (*PFNGLGETINTEGERVPROC)(int pname, int *params);
typedef void (*PFNGLGETSTRINGIPROC)(unsigned int name, unsigned int index);

// 创建OpenGL上下文
GLXContext createGLContext(Display* dpy, Window window) {
    // 1. 选择帧缓冲配置
    int attribs[] = {
        GLX_X_RENDERABLE,  True,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,   GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
        GLX_RED_SIZE,      8,
        GLX_GREEN_SIZE,    8,
        GLX_BLUE_SIZE,     8,
        GLX_ALPHA_SIZE,    8,
        GLX_DEPTH_SIZE,    24,
        GLX_STENCIL_SIZE,  8,
        GLX_DOUBLEBUFFER,  True,
        None
    };

    int fbcount = 0;
    GLXFBConfig* fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), attribs, &fbcount);
    if (!fbc || fbcount == 0) {
        fprintf(stderr, "没有找到合适的帧缓冲配置\n");
        return NULL;
    }

    // 2. 获取对应的X Visual
    XVisualInfo* vi = glXGetVisualFromFBConfig(dpy, fbc[0]);
    if (!vi) {
        fprintf(stderr, "无法获取X Visual\n");
        XFree(fbc);
        return NULL;
    }

    // 3. 创建OpenGL上下文
    GLXContext ctx = glXCreateContext(dpy, vi, NULL, True);
    if (!ctx) {
        fprintf(stderr, "无法创建OpenGL上下文\n");
    }

    // 4. 清理
    XFree(vi);
    XFree(fbc);
    
    return ctx;
}

int main() {
    printf("=== 开始GLAD LoadGL测试 ===\n");
    printf("测试环境: Linux, X11, GLX\n");
    
    // 1. 打开与X服务器的连接
    Display* dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "无法打开X显示\n");
        return EXIT_FAILURE;
    }
    printf("X显示打开成功: %s\n", DisplayString(dpy));
    
    // 2. 创建隐藏窗口
    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);
    
    // 创建窗口属性
    XSetWindowAttributes swa;
    swa.event_mask = ExposureMask | KeyPressMask;
    swa.background_pixel = BlackPixel(dpy, screen);
    swa.border_pixel = BlackPixel(dpy, screen);
    swa.colormap = XCreateColormap(dpy, root, DefaultVisual(dpy, screen), AllocNone);
    
    // 创建窗口（不可见）
    Window win = XCreateWindow(dpy, root, 
                               0, 0, 1, 1, // 1x1像素窗口（不可见）
                               0, 
                               DefaultDepth(dpy, screen), 
                               InputOutput,
                               DefaultVisual(dpy, screen),
                               CWBackPixel | CWBorderPixel | CWColormap | CWEventMask, 
                               &swa);
    if (!win) {
        fprintf(stderr, "无法创建窗口\n");
        XCloseDisplay(dpy);
        return EXIT_FAILURE;
    }
    
    // 3. 创建OpenGL上下文
    GLXContext ctx = createGLContext(dpy, win);
    if (!ctx) {
        fprintf(stderr, "无法创建OpenGL上下文\n");
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
        return EXIT_FAILURE;
    }
    
    // 4. 将上下文绑定到窗口
    if (!glXMakeCurrent(dpy, win, ctx)) {
        fprintf(stderr, "无法设置当前上下文\n");
        glXDestroyContext(dpy, ctx);
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
        return EXIT_FAILURE;
    }
    
    // 5. 加载GLAD
    printf("\n尝试加载gladLoadGL...\n");
    int version = gladLoadGL();
    
    // 6. 检查加载结果
    printf("\n=== GLAD加载结果 ===\n");
    printf("gladLoadGL返回值: %d\n", version);
    
    if (!version) {
        fprintf(stderr, "\n严重错误: gladLoadGL完全失败\n");
    } else {
        printf("加载OpenGL函数成功 (API %d.%d)\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
        printf("核心函数glGetString地址: %p\n", (void*)glGetString);
    }
    
    // 7. 检查关键函数指针
    int coreFunctionsLoaded = 1;
    
    printf("\n关键函数指针状态:\n");
    if (glad_glGetString == NULL) {
        fprintf(stderr, "  glGetString: 未加载\n");
        coreFunctionsLoaded = 0;
    } else {
        printf("  glGetString: 加载成功 (%p)\n", (void*)glad_glGetString);
    }
    
    if (glad_glGetIntegerv == NULL) {
        fprintf(stderr, "  glGetIntegerv: 未加载\n");
        coreFunctionsLoaded = 0;
    } else {
        printf("  glGetIntegerv: 加载成功 (%p)\n", (void*)glad_glGetIntegerv);
    }
    
    if (glad_glViewport == NULL) {
        fprintf(stderr, "  glViewport: 未加载\n");
        coreFunctionsLoaded = 0;
    } else {
        printf("  glViewport: 加载成功 (%p)\n", (void*)glad_glViewport);
    }
    
    // 8. 尝试使用加载的函数获取信息
    if (coreFunctionsLoaded) {
        printf("\nOpenGL基本信息:\n");
        printf("GL_VENDOR:   %s\n", glGetString(GL_VENDOR));
        printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
        printf("GL_VERSION:  %s\n", glGetString(GL_VERSION));
        
        GLint major, minor;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        printf("GL上下文版本: %d.%d\n", major, minor);
        
        GLint numExtensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        printf("\n扩展数量: %d\n", numExtensions);
        
        if (glad_glGetStringi) {
            printf("前5个扩展:\n");
            for (int i = 0; i < 5 && i < numExtensions; i++) {
                printf("  %s\n", glGetStringi(GL_EXTENSIONS, i));
            }
        } else {
            fprintf(stderr, "glGetStringi扩展不可用\n");
        }
    } else {
        fprintf(stderr, "\n关键函数未加载，无法获取OpenGL信息\n");
    }
    
    // 9. 手动加载函数测试
    printf("\n=== 替代加载方法测试 ===\n");
    PFNGLGETSTRINGPROC myGetString = (PFNGLGETSTRINGPROC)glXGetProcAddressARB((const GLubyte*)"glGetString");
    PFNGLGETINTEGERVPROC myGetIntegerv = (PFNGLGETINTEGERVPROC)glXGetProcAddressARB((const GLubyte*)"glGetIntegerv");
    PFNGLGETSTRINGIPROC myGetStringi = (PFNGLGETSTRINGIPROC)glXGetProcAddressARB((const GLubyte*)"glGetStringi");
    
    printf("直接通过glXGetProcAddressARB加载:\n");
    printf("  glGetString地址: %p\n", (void*)myGetString);
    printf("  glGetIntegerv地址: %p\n", (void*)myGetIntegerv);
    printf("  glGetStringi地址: %p\n", (void*)myGetStringi);
    
    // 10. 清理资源
    glXMakeCurrent(dpy, None, NULL);
    glXDestroyContext(dpy, ctx);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    
    // 11. 诊断建议
    printf("\n=== 诊断建议 ===\n");
    if (!version) {
        printf("针对gladLoadGL失败的建议:\n");
        printf("1. 检查显卡驱动是否正确安装:\n");
        printf("   glxinfo | grep 'OpenGL vendor'\n");
        printf("2. 检查MESA实现是否可用:\n");
        printf("   ls /usr/lib/x86_64-linux-gnu/libGL* | grep mesa\n");
        printf("3. 尝试使用替代加载器:\n");
        printf("   int version = gladLoadGL((GLADloadproc)glXGetProcAddressARB);\n");
        printf("4. 验证glad头文件是否正确:\n");
        printf("   #include <glad/glad.h> 必须在使用任何OpenGL函数前包含\n");
        printf("5. 使用符号表检查工具定位缺失函数:\n");
        printf("   objdump -T your_program | grep UND\n");
        printf("6. 检查GLX版本:\n");
        printf("   glxinfo -B | grep 'OpenGL version'\n");
    } else if (!coreFunctionsLoaded) {
        printf("gladLoadGL报告成功但关键函数未加载:\n");
        printf("1. glad实现可能有bug，尝试更新glad\n");
        printf("2. 使用gladSetGLGetProcAddressProc设置自定义加载函数\n");
        printf("3. 检查glad生成的C文件是否正确\n");
    } else {
        printf("gladLoadGL测试通过，所有功能正常！\n");
    }
    
    return EXIT_SUCCESS;
}