// Boyaroslav 2023

//thanks to https://stackoverflow.com/questions/8249669/how-do-take-a-screenshot-correctly-with-xlib

#define TGAP 10

#include<stdio.h>
#include<stdlib.h>
#include<X11/Xlib.h>
#include<X11/cursorfont.h>
#include<unistd.h>
#include<png.h>

int main(int argc, char **argv)
{
  int rx = 0, ry = 0, rw = 0, rh = 0;
  int rect_x = 0, rect_y = 0, rect_w = 0, rect_h = 0;
  int btn_pressed = 0, done = 0;

  XEvent ev;
  Display *disp = XOpenDisplay(NULL);

  if(!disp)
    return EXIT_FAILURE;

  Screen *scr = NULL;
  scr = ScreenOfDisplay(disp, DefaultScreen(disp));

  Window root = 0;
  root = RootWindow(disp, XScreenNumberOfScreen(scr));

  Cursor cursor, cursor2;
  cursor = XCreateFontCursor(disp, XC_left_ptr);
  cursor2 = XCreateFontCursor(disp, XC_lr_angle);

  XGCValues gcval;
  gcval.foreground = XWhitePixel(disp, 0);
  gcval.function = GXxor;
  gcval.background = XBlackPixel(disp, 0);
  gcval.plane_mask = gcval.background ^ gcval.foreground;
  gcval.subwindow_mode = IncludeInferiors;

  GC gc;
  gc = XCreateGC(disp, root,
                 GCFunction | GCForeground | GCBackground | GCSubwindowMode,
                 &gcval);

  /* this XGrab* stuff makes XPending true ? */
  if ((XGrabPointer
       (disp, root, False,
        ButtonMotionMask | ButtonPressMask | ButtonReleaseMask, GrabModeAsync,
        GrabModeAsync, root, cursor, CurrentTime) != GrabSuccess))
    printf("couldn't grab pointer:");

  if ((XGrabKeyboard
       (disp, root, False, GrabModeAsync, GrabModeAsync,
        CurrentTime) != GrabSuccess))
    printf("couldn't grab keyboard:");

  while (!done) {
    //~ while (!done && XPending(disp)) {
      //~ XNextEvent(disp, &ev);
    if (!XPending(disp)) { usleep(1000); continue; } // fixes the 100% CPU hog issue in original code
    if ( (XNextEvent(disp, &ev) >= 0) ) {
      switch (ev.type) {
        case MotionNotify:
        /* this case is purely for drawing rect on screen */
          if (btn_pressed) {
            if (rect_w) {
              /* re-draw the last rect to clear it */
              XDrawRectangle(disp, root, gc, rect_x, rect_y, rect_w, rect_h);
            } else {
              /* Change the cursor to show we're selecting a region */
              XChangeActivePointerGrab(disp,
                                       ButtonMotionMask | ButtonReleaseMask,
                                       cursor2, CurrentTime);
            }
            rect_x = rx;
            rect_y = ry;
            rect_w = ev.xmotion.x - rect_x;
            rect_h = ev.xmotion.y - rect_y;

            if (rect_w < 0) {
              rect_x += rect_w;
              rect_w = 0 - rect_w;
            }
            if (rect_h < 0) {
              rect_y += rect_h;
              rect_h = 0 - rect_h;
            }
            /* draw rectangle */
            XDrawRectangle(disp, root, gc, rect_x, rect_y, rect_w, rect_h);
            XFlush(disp);
          }
          break;
        case ButtonPress:
          btn_pressed = 1;
          rx = ev.xbutton.x;
          ry = ev.xbutton.y;
          break;
        case ButtonRelease:
          done = 1;
          break;
      }
    }
  }
  /* clear the drawn rectangle */
  if (rect_w) {
    XDrawRectangle(disp, root, gc, rect_x, rect_y, rect_w, rect_h);
    XFlush(disp);
  }
  rw = ev.xbutton.x - rx;
  rh = ev.xbutton.y - ry;
  /* cursor moves backwards */
  if (rw < 0) {
    rx += rw;
    rw = 0 - rw;
  }
  if (rh < 0) {
    ry += rh;
    rh = 0 - rh;
  }
  usleep(TGAP);
  XImage *image = XGetImage(disp,root, rx,ry , rw,rh,AllPlanes, ZPixmap);
     unsigned long red_mask = image->red_mask;
   unsigned long green_mask = image->green_mask;
   unsigned long blue_mask = image->blue_mask;

  int code = 0;
  FILE *fp;
  png_structp png_ptr;
  png_infop png_info_ptr;
  png_bytep png_row;

  if (rh == 0 || rw == 0){return -1;}

  if(argc < 2){
  fp = fopen("snip.png", "wb");
  }
  else{
    fp = fopen(argv[1], "wb");
  }
  if (fp == NULL){
    fprintf(stderr, "error\n");
    code = 1;
  }
  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL){
      fprintf (stderr, "Could not allocate write struct\n");
      code = 1;
  }
  png_info_ptr = png_create_info_struct (png_ptr);
  if (png_info_ptr == NULL){
    fprintf (stderr, "Could not allocate info struct\n");
    code = 1;
  }

  if (setjmp (png_jmpbuf (png_ptr))){
      fprintf(stderr, "Error during png creation\n");
      return 0;
    code = 1;
  }
  png_init_io (png_ptr, fp);
  png_set_IHDR (png_ptr, png_info_ptr, rw, rh,
     8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  char *title = "Screenshot";
  if (title != NULL){
      png_text title_text;
      title_text.compression = PNG_TEXT_COMPRESSION_NONE;
      title_text.key = "Title";
      title_text.text = title;
      png_set_text (png_ptr, png_info_ptr, &title_text, 1);
  }
  png_write_info (png_ptr, png_info_ptr);
  png_row = (png_bytep) malloc (3 * rw * sizeof (png_byte));

  int x, y;
  for( y = 0; y < rh; y++){
    for( x = 0; x < rw; x++){
        unsigned long pixel = XGetPixel (image, x, y);
        unsigned char blue = pixel & blue_mask;
        unsigned char green = (pixel & green_mask) >> 8; 
        unsigned char red = (pixel & red_mask) >> 16;
        png_byte *ptr = &(png_row[x*3]);
        ptr[0] = red;
        ptr[1] = green;
        ptr[2] = blue;
    }
    png_write_row (png_ptr, png_row);
  }
  png_write_end (png_ptr, NULL);
  fclose(fp);
   png_free_data (png_ptr, png_info_ptr, PNG_FREE_ALL, -1);
   png_destroy_write_struct (&png_ptr, (png_infopp)NULL);
   free (png_row);


  XCloseDisplay(disp);
  printf("%dx%d+%d+%d\n",rw,rh,rx,ry);


  return EXIT_SUCCESS;
}
