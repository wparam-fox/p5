/* Copyright 2006 wParam, licensed under the GPL */

#ifndef _WP_LAYOUT_H_
#define _WP_LAYOUT_H_





typedef struct layman_t layman_t;



layman_t *LmCreate (void);
void LmDestroy (layman_t *lm);

//Single group.  This group takes exactly 1 child, and makes the child the
//same size as the rect passed in.  It is not generally useful, it's in the
//code because it's the starting point for the recursion.
int LmAddGroupSingle (layman_t **lm);

//A window.  This "group" isn't really a group, it is only a window, it
//has no children
int LmAddHwnd (layman_t **lm, HWND hwnd);

//Border group.  Takes 1 child.  This group makes its child width pixels
//smaller than the rectangle it began with.
int LmAddGroupBorder (layman_t **lm, int width);

//Multi group.  Takes as many children as the number parameter.  Sets all
//children to be the full size of the input
int LmAddGroupMulti (layman_t **lm, int number);

//Nothing.  This will fill the slot as a group child but does not do anything.
//Use it when a group requires x number of children but you only want to have
//x-n of them be actual windows.
int LmAddNothing (layman_t **lm);

//Side delta groups.  They all take 1 child.  Each one changes the given side
//by the (signed) value you give
int LmAddGroupTopDelta (layman_t **lm, int delta);
int LmAddGroupRightDelta (layman_t **lm, int delta);
int LmAddGroupBottomDelta (layman_t **lm, int delta);
int LmAddGroupLeftDelta (layman_t **lm, int delta);

//Vertical split group.  Takes 2 children.  The parameter is the width of the
//left pane.  If it is positive, it is the absolute width in pixels, if it is
//negative, it is a percentage of the whole window.
int LmAddGroupVerticalSplit (layman_t **lm, int leftwidth);

//Horizontal split group.  Takes 2 children.  The parameter is the height of the
//top pane.  If it is positive, it is the absolute width in pixels, if it is
//negative, it is a percentage of the whole window.
int LmAddGroupHorizontalSplit (layman_t **lm, int topheight);




int LmApplyLayout (layman_t *lm, HWND hwnd);




















#endif