/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 * Parts Copyright (c) 2004 by Chris Moller
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and documentation
 * files (the "Software"), including without limitation the rights to use,
 * copy, modify, merge, publish distribute, sublicense and/or sell copies of
 * the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that the above copyright
 * and this permission notice remain intact.
 *
 */

#ifndef W_SNAP_H
#define W_SNAP_H

extern void snap_hold(Widget w, XtPointer closure, XtPointer call_data);
extern void snap_release(Widget w, XtPointer closure, XtPointer call_data);
extern void snap_endpoint(Widget w, XtPointer closure, XtPointer call_data);
extern void snap_midpoint(Widget w, XtPointer closure, XtPointer call_data);
extern void snap_nearest(Widget w, XtPointer closure, XtPointer call_data);
extern void snap_focus(Widget w, XtPointer closure, XtPointer call_data);
extern void snap_diameter(Widget w, XtPointer closure, XtPointer call_data);
extern void snap_normal(Widget w, XtPointer closure, XtPointer call_data);
extern void snap_tangent(Widget w, XtPointer closure, XtPointer call_data);
extern void snap_intersect(Widget w, XtPointer closure, XtPointer call_data);
extern Boolean snap_process(int * px, int *py, unsigned int state);
extern void get_line_from_points(double * c, struct f_point * s1, struct f_point * s2);
extern void snap_rotate_vector(double * dx, double * dy, double x, double y, double theta);
extern Boolean is_point_on_arc(F_arc * a, int x, int y);
extern void snap_polyline_focus_handler(F_line * a, int x, int y);
extern void init_snap_panel(Widget parent);

typedef enum {
  SNAP_MODE_NONE,
  SNAP_MODE_ENDPOINT,
  SNAP_MODE_MIDPOINT,
  SNAP_MODE_NEAREST,
  SNAP_MODE_FOCUS,
  SNAP_MODE_DIAMETER,
  SNAP_MODE_NORMAL,
  SNAP_MODE_TANGENT,
  SNAP_MODE_INTERSECT
} snap_mode_e;

typedef enum {
  INTERSECT_INITIAL,
  INTERSECT_FIRST_FOUND
} intersect_state_e;

extern intersect_state_e intersect_state;

extern snap_mode_e snap_mode;
extern Boolean snap_msg_set;
extern int snap_gx;
extern int snap_gy;
extern Boolean snap_found;
extern Widget snap_indicator_panel;  
extern Widget snap_indicator_label;

#  define signbit(x) \
    ((0.0 >  (x)) ? 1 : 0)


#endif
