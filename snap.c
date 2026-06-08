#include <X11/X.h>
#include <stdbool.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <sys/types.h>

#define MIN(x,y) (x > y ? y : x)
#define MAX(x,y) (x > y ? x : y)

void cap_sel(void);
void cap_win(void);
void draw_rect(void);

enum mode {
    MODE_SEL,
    MODE_FULL,
    MODE_SCR,
    MODE_WIN
};


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


unsigned char channel(unsigned long pxl, Mask m) {
    if(m == 0) return 0;

    int shift = 0;
    //shift down until we find a channel
    while((m & 1UL) == 0) {
        m >>= 1UL;
        shift++;
    }

    // no of bits in channel
    int bits = 0;
    while(m & 1UL) {
        m >>= 1UL;
        bits++;
    }

    //raw channel value
    // shift pxl right to bring the channel to bit number '0' , then cut off upper bits
    unsigned long rcv = (pxl >> shift) & ( (1UL << bits) - 1UL );

    // scale from [0, 2^bits - 1] -> [0, 255] to meet standard
    // so this maps 0->0 and 31 -> 255 
    if(bits == 8) return rcv; // already 8-bits wide
    return ((rcv * 255UL) / ((1UL << bits) - 1UL));
}


void mkppm(XImage *img) {
    FILE *out = stdout;

    // write ppm header metadata
    fprintf(out, "P6\n%d %d\n255\n", img->width, img->height);

    for(int i = 0; i < img->height; i++) {
        for(int j = 0; j < img->width; j++) {
            unsigned long pxl = XGetPixel(img, j, i);
            fputc(channel(pxl, img->red_mask),out);
            fputc(channel(pxl, img->green_mask),out);
            fputc(channel(pxl, img->blue_mask),out);
        }
    }

    fflush(stdout);
}

void cap_fullscr(Display *dpy,int scr,Window window) {
    unsigned int width = DisplayWidth(dpy, scr);
    unsigned int height = DisplayHeight(dpy, scr);

    XImage *img = XGetImage(dpy,window, 0, 0, width, height, AllPlanes, ZPixmap);
    if(!img) {
        printf("XGetImage failed");
        return;
    }
    
    mkppm(img);
}



int main() {
    Display *display = XOpenDisplay(NULL);
    if(!display) printf("XOpenDisplay failed!");
    Window rootWindow = RootWindow(display,DefaultScreen(display));
    XWindowAttributes rootwinattr;
    XGetWindowAttributes(display, rootWindow,&rootwinattr);
    unsigned int width = rootwinattr.width;
    unsigned int height = rootwinattr.height;
    cap_fullscr(display,DefaultScreen(display), rootWindow);
    XCloseDisplay(display);
    return 0;

}

