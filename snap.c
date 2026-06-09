#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include "config.h"

Display *dpy = NULL;
Window root = None;
Window overlaywin = None;
GC sel_gc = 0; // stores all the params : width, color...
int scr = -1;
unsigned int w = 0;
unsigned int h = 0;

#define MIN(x,y) (x > y ? y : x)
#define MAX(x,y) (x > y ? x : y)

void setup(void);
void cap_win(void);
void cap_sel_core(int x0,int y0, int x1, int y1);
void cap_sel(void);
void cap_fullscr(void);
void draw_rect(int x0,int y0, int x1, int y1);
void mkppm(XImage *img);
void die(const char *s);
void snap_close(Bool ex);

enum mode {
    MODE_SEL,
    MODE_FULL,
    MODE_WIN
};

struct selection {
    bool active;

    // start
    int x0;
    int y0;

    //end 
    int x1;
    int y1;
};

void setup(void) {
    dpy = XOpenDisplay(NULL);
    if(dpy == NULL) die("failed to open display\n");
    scr = DefaultScreen(dpy);
    root = RootWindow(dpy, scr);
    XWindowAttributes rootattr;
    XGetWindowAttributes(dpy, root, &rootattr);
    w = rootattr.width;
    h = rootattr.height;

    //rectangle
    XGCValues gcv;
    gcv.function = GXxor;
    gcv.foreground = WhitePixel(dpy, scr) ^ BlackPixel(dpy, scr);
    gcv.line_style = LineSolid;
    gcv.line_width = rect_line_width;
    gcv.subwindow_mode = IncludeInferiors;

    sel_gc = XCreateGC(dpy, root,GCFunction | GCForeground | GCLineStyle | GCLineWidth | GCSubwindowMode, &gcv);
}

void draw_rect(int x0,int y0, int x1, int y1) {
    int rx = MIN(x0,x1);
    int ry = MIN(y0,y1);
    unsigned int rw = MAX(x0,x1) - rx;
    unsigned int rh = MAX(y0,y1) - ry;

    if(rw > 0 && rh > 0) {
        XDrawRectangle(dpy, overlaywin, sel_gc, rx, ry, rw, rh);
    }
}


void spawnoverlay(void) {
    XSetWindowAttributes overlayattr;
    overlayattr.override_redirect = true;
    unsigned long valuemask = CWOverrideRedirect;
    overlaywin = XCreateWindow(dpy, root, 0, 0, w, h, 0, CopyFromParent, InputOutput, CopyFromParent, valuemask, &overlayattr);
    Cursor cur = XCreateFontCursor(dpy,cursor_font); 
    XDefineCursor(dpy, overlaywin, cur);
    XStoreName(dpy, overlaywin, "snap");
    XSelectInput(dpy, overlaywin, KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    XMapWindow(dpy, overlaywin);
    XFlush(dpy);

    XGrabPointer(dpy, overlaywin, False, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, overlaywin, None, CurrentTime);
    XGrabKeyboard(dpy, overlaywin, False, GrabModeAsync, GrabModeAsync, CurrentTime);
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

void cap_fullscr(void) {
    unsigned int width = DisplayWidth(dpy, scr);
    unsigned int height = DisplayHeight(dpy, scr);

    XImage *img = XGetImage(dpy,root, 0, 0, width, height, AllPlanes, ZPixmap);
    if(!img) {
        printf("XGetImage failed");
        return;
    }
    
    mkppm(img);
    XDestroyImage(img);
}

void die(const char *s) {
    fprintf(stderr, "snap: %s\n" , s);
    exit(EXIT_FAILURE);
}

void cap_sel(void) {
    spawnoverlay();
    struct selection sel = {0};
    bool quit = false;
    while(!quit) {
        XEvent event = {0};
        XNextEvent(dpy,&event);

        if(event.type == ButtonPress) {
            sel.active = true;
            sel.x0 = event.xbutton.x_root;
            sel.y0 = event.xbutton.y_root;
            sel.x1 = event.xbutton.x_root;
            sel.y1 = event.xbutton.y_root;

        }
        
        if(event.type == MotionNotify && sel.active) {
            draw_rect(sel.x0, sel.y0, sel.x1, sel.y1); // erase old

            sel.x1 = event.xmotion.x_root;
            sel.y1 = event.xmotion.y_root;

            draw_rect(sel.x0, sel.y0, sel.x1, sel.y1); //draw new

            XFlush(dpy);
        }
        

        if(event.type == ButtonRelease) {
            sel.active = false;
            XUngrabPointer(dpy, CurrentTime);
            XUngrabKeyboard(dpy, CurrentTime);
            XDestroyWindow(dpy,overlaywin);
            XSync(dpy, False);
            cap_sel_core(sel.x0,sel.y0,sel.x1,sel.y1);
            // exit looop 
            quit = true;
        }
        if(event.type == KeyPress) {
            KeySym key = XLookupKeysym(&event.xkey,0);
            if(key == XK_Escape) {
                XUngrabPointer(dpy, CurrentTime);
                XUngrabKeyboard(dpy, CurrentTime);
                XDestroyWindow(dpy, overlaywin);
                snap_close(True);
            }
        }
      }
}

void cap_sel_core(int x0,int y0, int x1, int y1) {

    int rx = MIN(x0,x1);
    int ry = MIN(y0,y1);
    int rw = MAX(x0,x1) - rx;
    int rh = MAX(y0,y1) - ry;
    
    if(rw <= 0 || rh <= 0) die("empty selection");

    XImage *img = XGetImage(dpy,root, rx, ry, rw, rh, AllPlanes, ZPixmap);
    if(!img) {
        printf("XGetImage failed\n");
        return;
    }
    mkppm(img);
    
    XDestroyImage(img);

}

void cap_win(void) {
    Window rooot; // root_return
    Window child;
    int rx;
    int ry;
    int wx;
    int wy;
    unsigned int wr;
    unsigned int hr;
    unsigned int bwr;
    unsigned int dr;
    unsigned int mr;

    // find window under the pointer
    XQueryPointer(dpy, root, &rooot,&child,&rx,&ry,&wx,&wy,&mr);
    if(child == None) child = root;

    XGetGeometry(dpy,child, &rooot, &rx, &ry, &wr, &hr, &bwr, &dr);
    XTranslateCoordinates(dpy, child, root, 0, 0, &rx, &ry, &rooot);

    XImage *img = XGetImage(dpy, root, rx, ry, wr, hr, AllPlanes, ZPixmap);
     if(!img) {
        printf("XGetImage failed\n");
        return;
    }

    mkppm(img);
    XDestroyImage(img);
}

void snap_close(Bool ex) {
    XCloseDisplay(dpy);
    if(ex) {
        exit(EXIT_SUCCESS);
    }
}



int main() {
    setup();
    cap_sel();
    snap_close(True);

    return EXIT_SUCCESS;
}

