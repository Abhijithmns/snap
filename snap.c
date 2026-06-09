#include <X11/X.h>
#include <stdbool.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

Display *dpy = NULL;
Window root = None;
Window overlaywin = None;
GC sel_gc = 0;
int scr = -1;
unsigned int w = 0;
unsigned int h = 0;

#define MIN(x,y) (x > y ? y : x)
#define MAX(x,y) (x > y ? x : y)

void setup(void);
void cap_sel(void);
void cap_win(void);
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
    if(dpy == NULL) printf("failed to open display\n");
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
    gcv.line_width = 2;
    gcv.subwindow_mode = IncludeInferiors;

    sel_gc = XCreateGC(dpy, root,GCFunction | GCForeground | GCLineStyle | GCLineWidth | GCSubwindowMode, &gcv);
}

void draw_rect(int x0,int y0, int x1, int y1) {
    int rx = MIN(x0,x1);
    int ry = MIN(x1,y1);
    unsigned int rw = MAX(x0,x1) - rx;
    unsigned int rh = MAX(y0,y1) - ry;

    if(rw > 0 && rh > 0) {
        XDrawRectangle(dpy, overlaywin, sel_gc, rx, ry, rw, rh);
    }
}


void overlay(void) {
    XSetWindowAttributes overlayattr;
    struct selection slct = {0};
    overlayattr.override_redirect = true;
    unsigned long valuemask = CWOverrideRedirect;
    overlaywin = XCreateWindow(dpy, root, 0, 0, w, h, 0, CopyFromParent, InputOutput, CopyFromParent, valuemask, &overlayattr);
    XStoreName(dpy, overlaywin, "snap");
    XSelectInput(dpy, overlaywin, KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
    XMapWindow(dpy, overlaywin);
    XFlush(dpy);

    bool quit = false;
    while(!quit) {
        XEvent event = {0};
        XNextEvent(dpy,&event);

        if(event.type == ButtonPress) {
            slct.active = true;
            slct.x0 = event.xbutton.x_root;
            slct.y0 = event.xbutton.y_root;
            slct.x1 = event.xbutton.x_root;
            slct.y1 = event.xbutton.y_root;

            printf("initial coords : (%d,%d)", slct.x0,slct.y0);
        }
        
        if(event.type == MotionNotify) {
            slct.active = true;
            
            slct.x1 = event.xmotion.x_root;
            slct.y1 = event.xmotion.y_root;
            draw_rect(slct.x0, slct.y0, slct.x1, slct.y1);

        }
        

        if(event.type == ButtonRelease) {
            slct.active = false;

            unsigned int x = MIN(slct.x0,slct.x1);
            unsigned int y = MIN(slct.y0,slct.y1);

            unsigned int w = abs(slct.x1 - slct.x0);
            unsigned int h = abs(slct.y1 -slct.y0);

            printf("final coords : (%d,%d)\n", slct.x1,slct.y1);
            printf("height and width : %d x %d", h, w);

            // quit 
            quit = true;
        }
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

void snap_close(Bool ex) {
    XCloseDisplay(dpy);
    if(ex) {
        exit(EXIT_SUCCESS);
    }
}



int main() {
    setup();
    overlay();
    snap_close(True);

    return EXIT_SUCCESS;
}

