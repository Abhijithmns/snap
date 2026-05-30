#include <X11/X.h>
#include <stdbool.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <sys/types.h>

void overlay(Display *dpy, Window root, unsigned int width, unsigned int height) {
    XStoreName(dpy, root, "snap");
    XSelectInput(dpy, root, KeyPressMask | KeyReleaseMask);
    XMapWindow(dpy, root);

    bool quit = false;
    while(!quit) {
      while(XPending(dpy) > 0) {
        XEvent event = {0};
        XNextEvent(dpy,&event);
        if(event.type == KeyPress) {
          quit = true;
        }
      }
      // Application work (event loop??)

    }

}

void select_scr(void);

int main() {
    Display *display = XOpenDisplay(NULL);
    if(!display) printf("XOpenDisplay failed!");
    Window rootWindow = RootWindow(display,DefaultScreen(display));
    int screen_num = DefaultScreen(display);
    unsigned int width = DisplayWidth(display, screen_num);
    unsigned int height = DisplayHeight(display, screen_num);

    XImage *img = XGetImage(display, rootWindow, 0, 0, width, height, AllPlanes, ZPixmap);
    if(!img) printf("XGetImage failed!");
    overlay(display,rootWindow,width, height);
    XDestroyImage(img);
    XCloseDisplay(display);
    return 0;

}

