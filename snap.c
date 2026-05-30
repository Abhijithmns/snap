#include <X11/X.h>
#include <stdbool.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <sys/types.h>

void cap_sel(void);
void cap_win(void);
void draw_rect(void);

void overlay(Display *dpy, Window window,unsigned int width, unsigned int height) {
    XSetWindowAttributes overlayattr;
    overlayattr.override_redirect = true;
    unsigned long valuemask = CWOverrideRedirect;
    Window overlaywin = XCreateWindow(dpy, window, 0, 0, width, height, 0, CopyFromParent, InputOutput, CopyFromParent, valuemask, &overlayattr);
    XStoreName(dpy, overlaywin, "snap");
    XSelectInput(dpy, overlaywin, KeyPressMask | KeyReleaseMask);
    XMapWindow(dpy, overlaywin);
    XFlush(dpy);

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

void cap_fullscr(Display *dpy,int scr,Window window) {
    unsigned int width = DisplayWidth(dpy, scr);
    unsigned int height = DisplayHeight(dpy, scr);

    XImage *img = XGetImage(dpy,window, 0, 0, width, height, AllPlanes, ZPixmap);
    if(!img) printf("XGetImage failed");
    
    // TODO : save or cpy the image based on the key press
}



int main() {
    Display *display = XOpenDisplay(NULL);
    if(!display) printf("XOpenDisplay failed!");
    Window rootWindow = RootWindow(display,DefaultScreen(display));
    XWindowAttributes rootwinattr;
    XGetWindowAttributes(display, rootWindow,&rootwinattr);
    unsigned int width = rootwinattr.width;
    unsigned int height = rootwinattr.height;
    overlay(display, rootWindow, width, height);
    XCloseDisplay(display);
    return 0;

}

