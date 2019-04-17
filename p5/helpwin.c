/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>
#include "p5.h"
#include "..\common\wp_layout.h"
#include <commctrl.h>
#include "resource.h"

#define HELPWIN_COMMCTRL_INIT_FAILED	DERRCODE (HELPWIN_DERR_BASE, 0x01)
#define HELPWIN_CREATE_DIALOG_FAILED	DERRCODE (HELPWIN_DERR_BASE, 0x02)
#define HELPWIN_SHOW_WINDOW_FAILED		DERRCODE (HELPWIN_DERR_BASE, 0x03)
#define HELPWIN_SET_WINDOW_POS_FAILED	DERRCODE (HELPWIN_DERR_BASE, 0x04)
#define HELPWIN_INSANE_DIALOG			DERRCODE (HELPWIN_DERR_BASE, 0x05)
#define HELPWIN_OUT_OF_MEMORY			DERRCODE (HELPWIN_DERR_BASE, 0x06)
#define HELPWIN_LAYOUT_CREATE_ERROR		DERRCODE (HELPWIN_DERR_BASE, 0x07)
#define HELPWIN_SET_WINDOW_LONG_FAILED	DERRCODE (HELPWIN_DERR_BASE, 0x08)
#define HELPWIN_LAYOUT_APPLY_ERROR		DERRCODE (HELPWIN_DERR_BASE, 0x09)
#define HELPWIN_IMAGELIST_FAILURE		DERRCODE (HELPWIN_DERR_BASE, 0x0A)
#define HELPWIN_LOADIMAGE_FAILED		DERRCODE (HELPWIN_DERR_BASE, 0x0B)
#define HELPWIN_IMAGELIST_ADD_FAILED	DERRCODE (HELPWIN_DERR_BASE, 0x0C)
#define HELPWIN_SET_IMAGELIST_FAILED	DERRCODE (HELPWIN_DERR_BASE, 0x0D)
#define HELPWIN_TREE_INSERT_FAILED		DERRCODE (HELPWIN_DERR_BASE, 0x0E)
#define HELPWIN_EMPTY_TREE_FAILED		DERRCODE (HELPWIN_DERR_BASE, 0x0F)
#define HELPWIN_GET_ITEM_FAILED			DERRCODE (HELPWIN_DERR_BASE, 0x10)
#define HELPWIN_INVALID_HELPWIN_TYPE	DERRCODE (HELPWIN_DERR_BASE, 0x11)
#define HELPWIN_ITEM_NOT_FOUND			DERRCODE (HELPWIN_DERR_BASE, 0x12)
#define HELPWIN_SETWINDOWTEXT_FAILED	DERRCODE (HELPWIN_DERR_BASE, 0x13)
#define HELPWIN_FONT_FAILURE			DERRCODE (HELPWIN_DERR_BASE, 0x14)








char *HwErrors[] = 
{
	NULL,
	"HELPWIN_COMMCTRL_INIT_FAILED",
	"HELPWIN_CREATE_DIALOG_FAILED",
	"HELPWIN_SHOW_WINDOW_FAILED",
	"HELPWIN_SET_WINDOW_POS_FAILED",
	"HELPWIN_INSANE_DIALOG",		
	"HELPWIN_OUT_OF_MEMORY",		
	"HELPWIN_LAYOUT_CREATE_ERROR",	
	"HELPWIN_SET_WINDOW_LONG_FAILED",
	"HELPWIN_LAYOUT_APPLY_ERROR",
	"HELPWIN_IMAGELIST_FAILURE",
	"HELPWIN_LOADIMAGE_FAILED",
	"HELPWIN_IMAGELIST_ADD_FAILED",
	"HELPWIN_SET_IMAGELIST_FAILED",
	"HELPWIN_TREE_INSERT_FAILED",
	"HELPWIN_EMPTY_TREE_FAILED",
	"HELPWIN_GET_ITEM_FAILED",
	"HELPWIN_INVALID_HELPWIN_TYPE",
	"HELPWIN_ITEM_NOT_FOUND",
	"HELPWIN_SETWINDOWTEXT_FAILED",
	"HELPWIN_FONT_FAILURE",

};

char *HwGetErrorString (int errindex)
{
	int numerrors = sizeof (HwErrors) / sizeof (char *);

	if (errindex >= numerrors || !HwErrors[errindex])
		return "HELPWIN_UNKNOWN_ERROR_CODE";

	return HwErrors[errindex];
}

typedef struct
{
	DERR st;

	helpblock_t *helps;
} hwcreate_t;

typedef struct
{
	helpblock_t *hb;

	int layout; //0 for ->lm, 1 for ->infolm
	layman_t *lm;
	layman_t *infolm;

	HWND tree; //just to speed things up
	HIMAGELIST him;

	HICON bigicon; //helpwin icon, since the resource editor doesn't seem to be able to set one
	HICON smallicon;
	HFONT font; //fixed width font for the edit boxes

} helpwin_t;

#define ICON_ROOT 0
#define ICON_DEFINE 1
#define ICON_VARIABLE 2
#define ICON_ALIAS 3
#define ICON_INFO 4
#define ICON_FUNCTION 5
#define ICON_FOLDER 6


HTREEITEM HwAddNode (HWND tree, HTREEITEM parent, HTREEITEM after, char *text, char *proto, int icon)
{
	TVINSERTSTRUCT tvi = {0};
	char buffer[256];

	//clamp name lengths to max 255
	if (strlen (text) > 255)
	{
		memcpy (buffer, text, 255);
		buffer[255] = '\0';
	}
	else
	{
		strcpy (buffer, text);
	}

	tvi.hParent = parent;
	tvi.hInsertAfter = after;

	tvi.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
	tvi.item.pszText = buffer;
	tvi.item.iImage = icon;
	tvi.item.iSelectedImage = icon;
	tvi.item.lParam = (LPARAM)proto;

	return TreeView_InsertItem (tree, &tvi);

}

HTREEITEM HwFindNode (HWND tree, HTREEITEM parent, char *title)
{
	HTREEITEM walk;
	char buffer[256];
	TVITEM tvi;
	DERR st;

	walk = TreeView_GetChild (tree, parent);

	while (walk)
	{
		tvi.hItem = walk;
		tvi.mask = TVIF_TEXT;
		tvi.pszText = buffer;
		tvi.cchTextMax = 255;

		st = TreeView_GetItem (tree, &tvi);
		if (st)
		{
			buffer[255] = '\0'; //not sure if it definitely does this or not.
			if (strcmp (title, buffer) == 0)
				return walk;
		}
		//else just ignore it and continue.

		walk = TreeView_GetNextSibling (tree, walk);
		//returns NULL on failure, so that will work fine.
	}

	return NULL;
}


HTREEITEM HwAddFunction (HWND tree, HTREEITEM parent, char *name, char *proto, int icon)
{
	char namebuffer[256];
	char buffer[256];
	char *start, *walk;
	HTREEITEM node;

	//clamp max length
	if (strlen (name) > 255)
	{
		memcpy (namebuffer, name, 255);
		namebuffer[255] = '\0';
	}
	else
	{
		strcpy (namebuffer, name);
	}

	//buffer is the actual working buffer, so copy the name there.
	strcpy (buffer, namebuffer);

	//now go.
	start = buffer;
	while (1)
	{
		//go forth and find a period.
		walk = start;

		while (*walk)
		{
			if (*walk == '.')
			{
				//periods at the end don't count.
				if (*(walk + 1) == '\0')
				{
					walk++;
					continue;
				}

				//neither do ones at the start of the string
				if (walk == start)
				{
					walk++;
					continue;
				}

				//ok, found a breaking period
				break;
			}

			walk++;
		}

		if (!*walk)
		{
			//end of the string, add the whole thing and we're done.
			return HwAddNode (tree, parent, TVI_SORT, name, proto, icon);
		}

		//found a period.  Null out the period, and search for a node named start.
		//(after the null, start will be a string contining just the current valid
		//part of the name.

		*walk = '\0';

		node = HwFindNode (tree, parent, start);
		if (!node)
		{
			node = HwAddNode (tree, parent, TVI_SORT, start, NULL, ICON_FOLDER);
			if (!node)
				return NULL;
		}

		//the parent is now the node we just added (or found)
		parent = node;

		//go past the period (currently '\0') and continue looking.
		walk++;
		start = walk;
	}

	return NULL;
}


int HwStristr (char *str, char *substr)
{
	int len;

	if (!str || !substr)
		return 0;

	len = strlen (substr);

	while (*str)
	{
		if (_strnicmp (str, substr, len) == 0)
			return 1;
		str++;
	}

	return 0;
}

int HwTestHelpBlockFilter (helpblock_t *h, char *filter)
{
	//the search is slightly different based on the type
	if (h->type == FUNCTYPE_NATIVE || h->type == FUNCTYPE_DEFINE || h->type == FUNCTYPE_ALIAS)
	{
		if (HwStristr (h->name, filter))
			return 1;
		if (HwStristr (h->description, filter))
			return 1;
		if (HwStristr (h->usage, filter))
			return 1;
		if (HwStristr (h->more, filter))
			return 1;
		//don't search the prototype
	}
	else if (h->type == FUNCTYPE_VARIABLE)
	{
		if (HwStristr (h->name, filter))
			return 1;
		if (HwStristr (h->proto, filter))
			return 1;
	}

	return 0;
}

#define INFO_PROTOTYPE ((char *)234)


DERR HwPopulateTreeViewFiltered (helpwin_t *hw, char *filter)
{
	DERR st;
	HTREEITEM root, temp;
	helpblock_t *h;
	char **infolist = NULL, *info = NULL;
	int x;

	st = TreeView_DeleteAllItems(hw->tree);
	fail_if (!st, HELPWIN_EMPTY_TREE_FAILED, 0);

	root = HwAddNode (hw->tree, TVI_ROOT, TVI_SORT, filter, NULL, ICON_ROOT);
	fail_if (!root, HELPWIN_TREE_INSERT_FAILED, 0);

	//go through and search and add.what matches
	h = hw->hb;
	while (h && h->name)
	{
		if (HwTestHelpBlockFilter (h, filter))
		{
			switch (h->type)
			{
			case FUNCTYPE_NATIVE:
			case FUNCTYPE_DEFINE:
				temp = HwAddNode (hw->tree, root, TVI_SORT, h->name, h->proto, h->type == FUNCTYPE_NATIVE ? ICON_FUNCTION : ICON_DEFINE);
				fail_if (!temp, HELPWIN_TREE_INSERT_FAILED, 0);
				break;

			case FUNCTYPE_VARIABLE:
				temp = HwAddNode (hw->tree, root, TVI_SORT, h->name, h->proto, ICON_VARIABLE);
				fail_if (!temp, HELPWIN_TREE_INSERT_FAILED, 0);
				break;

			case FUNCTYPE_ALIAS:
				temp = HwAddNode (hw->tree, root, TVI_SORT, h->name, h->proto, ICON_ALIAS);
				fail_if (!temp, HELPWIN_TREE_INSERT_FAILED, 0);
				break;
			}
		}

		h++;
	}

	//do the same for the infos.  (This is somewhat more irritating.)
	st = InfGetInfoNameList (&infolist);
	fail_if (!DERR_OK (st), st, 0);

	for (x=0;infolist[x];x++)
	{
		st = InfGetInfoText (infolist[x], &info);
		fail_if (!DERR_OK (st), st, 0); //should this really be a fail?

		if (HwStristr (info, filter))
		{
			temp = HwAddNode (hw->tree, root, TVI_SORT, infolist[x], INFO_PROTOTYPE, ICON_INFO);
			fail_if (!temp, HELPWIN_TREE_INSERT_FAILED, 0);
		}

		PfFree (info);
		info = NULL;
	}

	PfFree (infolist);
	infolist = NULL;


	st = TreeView_Expand (hw->tree, root, TVE_EXPAND);
	fail_if (!st, 0, 0);

	return PF_SUCCESS;
failure:

	if (infolist)
		PfFree (infolist);

	if (info)
		PfFree (info);

	return st;
}



DERR HwPopulateTreeView (helpwin_t *hw)
{
	DERR st;
	HTREEITEM root; //the real root
	HTREEITEM vars, funcs, aliases, infos, temp;
	helpblock_t *h;
	char **infolist = NULL;
	int x;

	st = TreeView_DeleteAllItems(hw->tree);
	fail_if (!st, HELPWIN_EMPTY_TREE_FAILED, 0);

	root = HwAddNode (hw->tree, TVI_ROOT, TVI_SORT, "Parser", NULL, ICON_ROOT);
	fail_if (!root, HELPWIN_TREE_INSERT_FAILED, 0);

	funcs = HwAddNode (hw->tree, root, TVI_LAST, "Functions", NULL, ICON_FUNCTION);
	fail_if (!funcs, HELPWIN_TREE_INSERT_FAILED, 0);

	aliases = HwAddNode (hw->tree, root, TVI_LAST, "Aliases", NULL, ICON_ALIAS);
	fail_if (!aliases, HELPWIN_TREE_INSERT_FAILED, 0);

	vars = HwAddNode (hw->tree, root, TVI_LAST, "Variables", NULL, ICON_VARIABLE);
	fail_if (!vars, HELPWIN_TREE_INSERT_FAILED, 0);

	infos = HwAddNode (hw->tree, root, TVI_LAST, "Info", NULL, ICON_INFO);
	fail_if (!infos, HELPWIN_TREE_INSERT_FAILED, 0);

	st = TreeView_Expand (hw->tree, root, TVE_EXPAND);
	fail_if (!st, 0, 0);

	//now add the stuff.
	h = hw->hb;
	while (h && h->name)
	{
		switch (h->type)
		{
		case FUNCTYPE_NATIVE:
		case FUNCTYPE_DEFINE:
			temp = HwAddFunction (hw->tree, funcs, h->name, h->proto, h->type == FUNCTYPE_NATIVE ? ICON_FUNCTION : ICON_DEFINE);
			fail_if (!temp, HELPWIN_TREE_INSERT_FAILED, 0);
			break;

		case FUNCTYPE_VARIABLE:
			temp = HwAddNode (hw->tree, vars, TVI_SORT, h->name, h->proto, ICON_VARIABLE);
			fail_if (!temp, HELPWIN_TREE_INSERT_FAILED, 0);
			break;

		case FUNCTYPE_ALIAS:
			temp = HwAddNode (hw->tree, aliases, TVI_SORT, h->name, h->proto, ICON_ALIAS);
			fail_if (!temp, HELPWIN_TREE_INSERT_FAILED, 0);
			break;
		}

		h++;
	}

	//query for the info list, and add it in
	st = InfGetInfoNameList (&infolist);
	fail_if (!DERR_OK (st), st, 0);

	for (x=0;infolist[x];x++)
	{
		temp = HwAddNode (hw->tree, infos, TVI_SORT, infolist[x], INFO_PROTOTYPE, ICON_INFO);
		fail_if (!temp, HELPWIN_TREE_INSERT_FAILED, 0);
	}

	PfFree (infolist);
	infolist = NULL;



	return PF_SUCCESS;

failure:

	if (infolist)
		PfFree (infolist);

	return st;

}






DERR HwCreateImageList (HIMAGELIST *him)
{
	HIMAGELIST out = NULL;
	HICON icon = NULL;
	DERR st;

	out = ImageList_Create (16, 16, ILC_COLOR | ILC_MASK , 7, 7);
	fail_if (!out, HELPWIN_IMAGELIST_FAILURE, 0);

	icon = LoadImage (PfInstance, MAKEINTRESOURCE (IDI_APPICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	fail_if (!icon, HELPWIN_LOADIMAGE_FAILED, DIGLE);
	st = ImageList_AddIcon (out, icon);
	fail_if (st == -1, HELPWIN_IMAGELIST_ADD_FAILED, 0);
	DestroyIcon (icon);

	icon = LoadImage (PfInstance, MAKEINTRESOURCE (IDI_DEFINE), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	fail_if (!icon, HELPWIN_LOADIMAGE_FAILED, DIGLE);
	st = ImageList_AddIcon (out, icon);
	fail_if (st == -1, HELPWIN_IMAGELIST_ADD_FAILED, 0);
	DestroyIcon (icon);

	icon = LoadImage (PfInstance, MAKEINTRESOURCE (IDI_VARIABLE), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	fail_if (!icon, HELPWIN_LOADIMAGE_FAILED, DIGLE);
	st = ImageList_AddIcon (out, icon);
	fail_if (st == -1, HELPWIN_IMAGELIST_ADD_FAILED, 0);
	DestroyIcon (icon);

	icon = LoadImage (PfInstance, MAKEINTRESOURCE (IDI_ALIAS), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	fail_if (!icon, HELPWIN_LOADIMAGE_FAILED, DIGLE);
	st = ImageList_AddIcon (out, icon);
	fail_if (st == -1, HELPWIN_IMAGELIST_ADD_FAILED, 0);
	DestroyIcon (icon);

	icon = LoadImage (PfInstance, MAKEINTRESOURCE (IDI_INFO), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	fail_if (!icon, HELPWIN_LOADIMAGE_FAILED, DIGLE);
	st = ImageList_AddIcon (out, icon);
	fail_if (st == -1, HELPWIN_IMAGELIST_ADD_FAILED, 0);
	DestroyIcon (icon);

	icon = LoadImage (PfInstance, MAKEINTRESOURCE (IDI_FUNCTION), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	fail_if (!icon, HELPWIN_LOADIMAGE_FAILED, DIGLE);
	st = ImageList_AddIcon (out, icon);
	fail_if (st == -1, HELPWIN_IMAGELIST_ADD_FAILED, 0);
	DestroyIcon (icon);

	icon = LoadImage (PfInstance, MAKEINTRESOURCE (IDI_FOLDER), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
	fail_if (!icon, HELPWIN_LOADIMAGE_FAILED, DIGLE);
	st = ImageList_AddIcon (out, icon);
	fail_if (st == -1, HELPWIN_IMAGELIST_ADD_FAILED, 0);
	DestroyIcon (icon);

	*him = out;
	return PF_SUCCESS;

failure:
	if (out)
		ImageList_Destroy (out);

	if (icon)
		DestroyIcon (icon);

	return st;
}





int HwAddEditBox (HWND border, HWND edit, layman_t **lm)
{
	DERR st;

	fail_if (!border || !edit, HELPWIN_INSANE_DIALOG, 0);

	st = LmAddGroupMulti (lm, 2); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
	{
		st = LmAddHwnd (lm, border); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
		st = LmAddGroupBorder (lm, 6); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
		{
			st = LmAddGroupTopDelta (lm, 9); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
			{
				st = LmAddHwnd (lm, edit); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
			}
		}
	}
	
	return 1;

failure:
	return 0;
}

void HwDestroyHelpwinType (helpwin_t *hw)
{
	//Yes there is a reason we set all this stuff to NULL.  This function is called
	//from WM_DESTROY.  In WM_DESTROY, there is a call to SetWindowLong, which in
	//theory will prevent any future spurious calls to WM_DESTROY from seeing this
	//type again and trying to free it.  In case that SetWindowLong fails, though,
	//and we somehow end up getting two WM_DESTROYS, I want to avoid as much double
	//freeing as possible, hence the NULLing

	if (hw->him)
		ImageList_Destroy (hw->him);
	hw->him = NULL;

	if (hw->hb)
		ParFree (hw->hb);
	hw->hb = NULL;

	if (hw->lm)
		LmDestroy (hw->lm);
	hw->lm = NULL;

	if (hw->infolm)
		LmDestroy (hw->infolm);
	hw->infolm = NULL;

	if (hw->font)
		DeleteObject (hw->font);
	hw->font = NULL;

	if (hw->bigicon)
		DestroyIcon (hw->bigicon);
	hw->bigicon = NULL;

	if (hw->smallicon)
		DestroyIcon (hw->smallicon);
	hw->smallicon = NULL;

	PfFree (hw);
}

void HwCreateProc (HWND hwnd, hwcreate_t *hwc)
{
	helpwin_t *hw = NULL;
	DERR st;
	HIMAGELIST him;
	HICON oldicon;

	

	hw = PfMalloc (sizeof (helpwin_t));
	fail_if (!hw, HELPWIN_OUT_OF_MEMORY, 0);
	memset (hw, 0, sizeof (helpwin_t));

	hw->hb = hwc->helps;
	hwc->helps = NULL;

	hw->tree = GetDlgItem (hwnd, IDC_TREE);
	fail_if (!hw->tree, HELPWIN_INSANE_DIALOG, 0);

	hw->font = CreateFont (15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Courier");
	fail_if (!hw->font, HELPWIN_FONT_FAILURE, DIGLE);
	//don't set it until the rest of this succeeds.

	hw->bigicon = LoadImage (PfInstance, MAKEINTRESOURCE (IDI_HELPWIN), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
	fail_if (!hw->bigicon, HELPWIN_LOADIMAGE_FAILED, DIGLE);

	hw->smallicon = LoadImage (PfInstance, MAKEINTRESOURCE (IDI_HELPWIN), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	fail_if (!hw->smallicon, HELPWIN_LOADIMAGE_FAILED, DIGLE);

	hw->lm = LmCreate ();
	fail_if (!hw->lm, HELPWIN_LAYOUT_CREATE_ERROR, 0);

	st = LmAddGroupVerticalSplit (&hw->lm, 200); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
	{
		st = LmAddGroupBorder (&hw->lm, 10); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
		{	
			st = LmAddGroupHorizontalSplit (&hw->lm, -100); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
			{
				st = LmAddGroupBottomDelta (&hw->lm, -25); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
				{
					st = LmAddHwnd (&hw->lm, hw->tree); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
				}
				st = LmAddGroupTopDelta (&hw->lm, -20); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
				{
					st = LmAddGroupVerticalSplit (&hw->lm, -70); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
					{
						HWND temp;

						temp = GetDlgItem (hwnd, IDC_SEARCHBOX);
						fail_if (!temp, HELPWIN_INSANE_DIALOG, 0);
						st = LmAddHwnd (&hw->lm, temp); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);

						temp = GetDlgItem (hwnd, IDC_FILTER);
						fail_if (!temp, HELPWIN_INSANE_DIALOG, 0);
						st = LmAddHwnd (&hw->lm, temp); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
					}
				}
			}
		}
		st = LmAddGroupBorder (&hw->lm, 10); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
		{
			st = LmAddGroupHorizontalSplit (&hw->lm, -33); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
			{
				st = HwAddEditBox (GetDlgItem (hwnd, IDC_DESCRIPTION_BORDER), GetDlgItem (hwnd, IDC_DESCRIPTION), &hw->lm); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
				st = LmAddGroupHorizontalSplit (&hw->lm, -40); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
				{
					st = HwAddEditBox (GetDlgItem (hwnd, IDC_USAGE_BORDER), GetDlgItem (hwnd, IDC_USAGE), &hw->lm); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
					st = HwAddEditBox (GetDlgItem (hwnd, IDC_MORE_BORDER), GetDlgItem (hwnd, IDC_MORE), &hw->lm); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
				}
			}
		}
	}

	st = LmApplyLayout (hw->lm, hwnd);
	fail_if (!st, HELPWIN_LAYOUT_APPLY_ERROR, 0);


	hw->infolm = LmCreate ();
	fail_if (!hw->infolm, HELPWIN_LAYOUT_CREATE_ERROR, 0);

	st = LmAddGroupVerticalSplit (&hw->infolm, 200); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
	{
		st = LmAddGroupBorder (&hw->infolm, 10); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
		{	
			st = LmAddGroupHorizontalSplit (&hw->infolm, -100); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
			{
				st = LmAddGroupBottomDelta (&hw->infolm, -25); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
				{
					st = LmAddHwnd (&hw->infolm, hw->tree); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
				}
				st = LmAddGroupTopDelta (&hw->infolm, -20); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
				{
					st = LmAddGroupVerticalSplit (&hw->infolm, -70); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
					{
						HWND temp;

						temp = GetDlgItem (hwnd, IDC_SEARCHBOX);
						fail_if (!temp, HELPWIN_INSANE_DIALOG, 0);
						st = LmAddHwnd (&hw->infolm, temp); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);

						temp = GetDlgItem (hwnd, IDC_FILTER);
						fail_if (!temp, HELPWIN_INSANE_DIALOG, 0);
						st = LmAddHwnd (&hw->infolm, temp); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
					}
				}
			}
		}
		st = LmAddGroupBorder (&hw->infolm, 10); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
		{
			st = HwAddEditBox (GetDlgItem (hwnd, IDC_MORE_BORDER), GetDlgItem (hwnd, IDC_MORE), &hw->infolm); fail_if (!st, HELPWIN_LAYOUT_CREATE_ERROR, 0);
		}
	}


	st = HwCreateImageList (&hw->him);
	fail_if (!DERR_OK (st), st, 0);

	him = TreeView_SetImageList (hw->tree, hw->him, TVSIL_NORMAL);
	if (him) //it SHOULD be null, but just in case
		ImageList_Destroy (him);
	him = TreeView_GetImageList (hw->tree, TVSIL_NORMAL);
	fail_if (him != hw->him, HELPWIN_SET_IMAGELIST_FAILED, 0);

	st = HwPopulateTreeView (hw);
	fail_if (!DERR_OK (st), st, 0);

	SetLastError (0);
	st = SetWindowLong (hwnd, GWL_USERDATA, (LONG)hw);
	fail_if (!st && GetLastError () != ERROR_SUCCESS, HELPWIN_SET_WINDOW_LONG_FAILED, DIGLE);
	//ok, once we pass SetWindowLong we need to be successful.


	//WM_SETFONT doesn't report failure, so I'm sticking it down here.  We can't hit failure
	//and thus destroy the font) once we've set it.
	SendDlgItemMessage (hwnd, IDC_DESCRIPTION, WM_SETFONT, (WPARAM)hw->font, 0);
	SendDlgItemMessage (hwnd, IDC_USAGE, WM_SETFONT, (WPARAM)hw->font, 0);
	SendDlgItemMessage (hwnd, IDC_MORE, WM_SETFONT, (WPARAM)hw->font, 0);

	oldicon = (HICON)SendMessage (hwnd, WM_SETICON, ICON_BIG, (LPARAM)hw->bigicon);
	if (oldicon)
		DestroyIcon (oldicon);

	oldicon = (HICON)SendMessage (hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hw->smallicon);
	if (oldicon)
		DestroyIcon (oldicon);
	
	return;

failure:

	if (hw)
		HwDestroyHelpwinType (hw);

	//do this just in case, as an extra precation to make sure WM_DESTROY gets NULL
	//for the helpwin_t parameter.
	SetWindowLong (hwnd, GWL_USERDATA, 0);

	hwc->st = st;

	DestroyWindow (hwnd);
}

DERR HwSetLayout (HWND hwnd, helpwin_t *hw, int infolayout)
{
	DERR st;
	HWND temp;

	if (hw->layout == infolayout) //if there's no change,
		return PF_SUCCESS;	//do nothing

	if (infolayout)
	{
		temp = GetDlgItem (hwnd, IDC_DESCRIPTION);
		fail_if (!temp, HELPWIN_INSANE_DIALOG, 0);
		ShowWindow (temp, SW_HIDE);

		temp = GetDlgItem (hwnd, IDC_DESCRIPTION_BORDER);
		fail_if (!temp, HELPWIN_INSANE_DIALOG, 0);
		ShowWindow (temp, SW_HIDE);

		temp = GetDlgItem (hwnd, IDC_USAGE);
		fail_if (!temp, HELPWIN_INSANE_DIALOG, 0);
		ShowWindow (temp, SW_HIDE);

		temp = GetDlgItem (hwnd, IDC_USAGE_BORDER);
		fail_if (!temp, HELPWIN_INSANE_DIALOG, 0);
		ShowWindow (temp, SW_HIDE);

		hw->layout = 1;

		st = LmApplyLayout (hw->infolm, hwnd);
		fail_if (!st, HELPWIN_LAYOUT_APPLY_ERROR, 0);
	}
	else
	{
		temp = GetDlgItem (hwnd, IDC_DESCRIPTION);
		fail_if (!temp, HELPWIN_INSANE_DIALOG, 0);
		ShowWindow (temp, SW_SHOW);

		temp = GetDlgItem (hwnd, IDC_DESCRIPTION_BORDER);
		fail_if (!temp, HELPWIN_INSANE_DIALOG, 0);
		ShowWindow (temp, SW_SHOW);

		temp = GetDlgItem (hwnd, IDC_USAGE);
		fail_if (!temp, HELPWIN_INSANE_DIALOG, 0);
		ShowWindow (temp, SW_SHOW);

		temp = GetDlgItem (hwnd, IDC_USAGE_BORDER);
		fail_if (!temp, HELPWIN_INSANE_DIALOG, 0);
		ShowWindow (temp, SW_SHOW);

		hw->layout = 0;

		st = LmApplyLayout (hw->lm, hwnd);
		fail_if (!st, HELPWIN_LAYOUT_APPLY_ERROR, 0);
	}

	return PF_SUCCESS;
failure:
	return st;
}



char *HwComputeUsage (char *name, char *usage, helpblock_t *hb)
{
	helpblock_t *h;
	char *out = NULL;
	int len;
	int hasalias = 0;


	len = 0;
	h = hb;
	while (h->name)
	{
		if (h->type == FUNCTYPE_ALIAS && strcmp (h->description, name) == 0)
			len += strlen (h->name) + 3; //quote, quote, space

		h++;
	}

	if (len != 0)
		hasalias = 1;

	len += strlen (usage) + 1; //this is where space for the NULL comes from

	if (hasalias)
		len += strlen ("\r\n\r\nAliases: ");

	out = PfMalloc (len);
	if (!out)
		return NULL;

	strcpy (out, usage);

	if (hasalias)
	{
		strcat (out, "\r\n\r\nAliases: ");

		h = hb;
		while (h->name)
		{
			if (h->type == FUNCTYPE_ALIAS && strcmp (h->description, name) == 0)
			{
				strcat (out, "\"");
				strcat (out, h->name);
				strcat (out, "\" ");
			}

			h++;
		}
	}

	if ((unsigned)len != strlen (out) + 1)
	{
		//this may not be safe...
		PfFree (out);
		return NULL;
	}

	return out;
}



DERR HwDisplayHelp (HWND hwnd, helpwin_t *hw, HTREEITEM item)
{
	char buffer[256];
	TVITEM tvi;
	DERR st;
	helpblock_t *h;
	char *info = NULL;

	fail_if (!hw || !hw->hb, HELPWIN_INVALID_HELPWIN_TYPE, 0);

	tvi.hItem = item;
	tvi.mask = TVIF_TEXT | TVIF_PARAM;
	tvi.pszText = buffer;
	tvi.cchTextMax = 255;

	st = TreeView_GetItem (hw->tree, &tvi);
	fail_if (!st, HELPWIN_GET_ITEM_FAILED, 0);
	buffer[255] = '\0';

	//handle the case where it's an info entry first
	if ((char *)tvi.lParam == INFO_PROTOTYPE)
	{
		st = InfGetInfoText (buffer, &info);
		fail_if (!DERR_OK (st), st, 0);

		st = HwSetLayout (hwnd, hw, 1);
		fail_if (!DERR_OK (st), st, 0);

		st = SetDlgItemText (hwnd, IDC_MORE, info);
		fail_if (!st, HELPWIN_SETWINDOWTEXT_FAILED, DIGLE);

		PfFree (info);
		return PF_SUCCESS;
	}


	h = hw->hb;
	while (h->name)
	{
		//yes it is correct to only compare the prototype by value
		if (strcmp (h->name, buffer) == 0 && h->proto == (char *)tvi.lParam)
			break;

		h++;
	}

	fail_if (!h->name, HELPWIN_ITEM_NOT_FOUND, 0);

	st = HwSetLayout (hwnd, hw, 0);
	fail_if (!DERR_OK (st), st, 0);

	if (h->type == FUNCTYPE_VARIABLE)
	{
		st = SetDlgItemText (hwnd, IDC_DESCRIPTION, h->proto ? h->proto : "");
		fail_if (!st, HELPWIN_SETWINDOWTEXT_FAILED, DIGLE);

		st = SetDlgItemText (hwnd, IDC_USAGE, "");
		fail_if (!st, HELPWIN_SETWINDOWTEXT_FAILED, DIGLE);

		st = SetDlgItemText (hwnd, IDC_MORE, "");
		fail_if (!st, HELPWIN_SETWINDOWTEXT_FAILED, DIGLE);
	}
	else if (h->type == FUNCTYPE_ALIAS)
	{
		st = SetDlgItemText (hwnd, IDC_DESCRIPTION, h->description ? h->description : "");
		fail_if (!st, HELPWIN_SETWINDOWTEXT_FAILED, DIGLE);

		st = SetDlgItemText (hwnd, IDC_USAGE, h->proto ? h->proto : "");
		fail_if (!st, HELPWIN_SETWINDOWTEXT_FAILED, DIGLE);

		st = SetDlgItemText (hwnd, IDC_MORE, "\"Description\" is the alias target, and \"Usage\" is the alias prototype");
		fail_if (!st, HELPWIN_SETWINDOWTEXT_FAILED, DIGLE);
	}
	else
	{
		//define, native
		st = SetDlgItemText (hwnd, IDC_DESCRIPTION, h->description ? h->description : "");
		fail_if (!st, HELPWIN_SETWINDOWTEXT_FAILED, DIGLE);

		info = HwComputeUsage (h->name, h->usage ? h->usage : "", hw->hb);
		if (info)
		{
			st = SetDlgItemText (hwnd, IDC_USAGE, info);
			fail_if (!st, HELPWIN_SETWINDOWTEXT_FAILED, DIGLE);
			PfFree (info);
			info = NULL;
		}
		else
		{
			st = SetDlgItemText (hwnd, IDC_USAGE, h->usage ? h->usage : "");
			fail_if (!st, HELPWIN_SETWINDOWTEXT_FAILED, DIGLE);
		}


		st = SetDlgItemText (hwnd, IDC_MORE, h->more ? h->more : "");
		fail_if (!st, HELPWIN_SETWINDOWTEXT_FAILED, DIGLE);
	}

	return PF_SUCCESS;
failure:

	if (info)
		PfFree (info);

	return st;
}

	


void HwHandleNotify (HWND hwnd, helpwin_t *hw, LPARAM lParam)
{
	NMHDR *nm = (NMHDR *)lParam;
	NMTREEVIEW *nmt;

	if (nm->code != TVN_SELCHANGED)
		return;

	nmt = (NMTREEVIEW *)nm;

	HwDisplayHelp (hwnd, hw, nmt->itemNew.hItem);
}

void HwProcessFilter (HWND hwnd, helpwin_t *hw)
{
	char buffer[256];
	int st;

	SetLastError (0);
	st = GetDlgItemText (hwnd, IDC_SEARCHBOX, buffer, 255);
	buffer[255] = '\0';
	if (!st && GetLastError () != ERROR_SUCCESS)
		return; //error

	if (*buffer)
		HwPopulateTreeViewFiltered (hw, buffer);
	else
		HwPopulateTreeView (hw);

}
	
	

BOOL CALLBACK HwDialogProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	helpwin_t *hw = (helpwin_t *)GetWindowLong (hwnd, GWL_USERDATA);
	DERR st;
	HWND temp;

	switch (message)
	{
	case WM_INITDIALOG:
		HwCreateProc (hwnd, (hwcreate_t *)lParam);
		return FALSE;

	case WM_WINDOWPOSCHANGED:
		if (!hw || !hw->lm)
			return FALSE;

		st = LmApplyLayout (hw->layout ? hw->infolm : hw->lm, hwnd);

		temp = GetDlgItem (hwnd, IDC_DESCRIPTION);
		if (temp && hw->layout == 0)
			InvalidateRect (temp, NULL, FALSE);

		temp = GetDlgItem (hwnd, IDC_USAGE);
		if (temp && hw->layout == 0)
			InvalidateRect (temp, NULL, FALSE);

		temp = GetDlgItem (hwnd, IDC_MORE);
		if (temp)
			InvalidateRect (temp, NULL, FALSE);

		return TRUE;

	case WM_NOTIFY:
		if (!hw)
			return FALSE;

		if (wParam == IDC_TREE)
			HwHandleNotify (hwnd, hw, lParam);

		return 0;

	case WM_DESTROY:
		if (!hw)
			return FALSE;

		//reset the edit controls to the system font.  I don't think it would really
		//hurt anything to destroy the font that they're using, since we're destroying
		//this window anyway, but just in case...
		SendDlgItemMessage (hwnd, IDC_DESCRIPTION, WM_SETFONT, (WPARAM)NULL, 0);
		SendDlgItemMessage (hwnd, IDC_USAGE, WM_SETFONT, (WPARAM)NULL, 0);
		SendDlgItemMessage (hwnd, IDC_MORE, WM_SETFONT, (WPARAM)NULL, 0);

		HwDestroyHelpwinType (hw);
		SetWindowLong (hwnd, GWL_USERDATA, 0); //just as a precaution

		return TRUE;

	case WM_COMMAND:
		if (!hw)
			return FALSE;

		if (LOWORD (wParam) == IDC_FILTER)
		{
			HwProcessFilter (hwnd, hw);
			return TRUE;
		}

		return FALSE;


	case WM_CLOSE:
		DestroyWindow (hwnd);
		return TRUE;
	}

	return FALSE;
}

DERR HwCreateHelpWindow (helpblock_t *hb)
{
	HWND win = NULL;
	INITCOMMONCONTROLSEX icx;
	DERR st;
	hwcreate_t hwcreate;

	//do this first, the failure cleanup expects to find hb in here if it's
	//still our responsibility to free it
	hwcreate.helps = hb;
	hwcreate.st = PF_SUCCESS;

	icx.dwSize = sizeof (icx);
	icx.dwICC = ICC_TREEVIEW_CLASSES;
	st = InitCommonControlsEx (&icx);
	fail_if (!st, HELPWIN_COMMCTRL_INIT_FAILED, 0);

	win = CreateDialogParam (GetModuleHandle (NULL), MAKEINTRESOURCE (IDD_HELPWIN), NULL, HwDialogProc, (LPARAM)&hwcreate);
	fail_if (!win, hwcreate.st ? hwcreate.st : HELPWIN_CREATE_DIALOG_FAILED, DIGLE);

	//it should be safe to fail below here.  The dialog has been created, so that
	//means it will free hb and whatever else it's allocated in WM_DESTROY.  The failure
	//code destroys the window if it's been created, so it should all fall into place.
	//Fucking win32 user code is a nightmare to fail properly, so I'm basically doing my
	//best and leaving the rest to hope
	
	ShowWindow (win, SW_SHOW);

	st = SetWindowPos (win, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	fail_if (!st, HELPWIN_SET_WINDOW_POS_FAILED, DIGLE);

	//this /probably/ shouldn't be possible, but just in case
	if (hwcreate.helps)
		ParFree (hwcreate.helps);

	return PF_SUCCESS;

failure:

	//the Dialog procedure sets this to NULL after it claims it.
	if (hwcreate.helps)
		ParFree (hwcreate.helps);

	if (win)
		DestroyWindow (win);

	return st;

}

/*

  Info command.

  there needs to be an (info) command that brings up a message box (or something) With information 
  about stuff.  THis is the faq type info, the p5 command line params, the script (#) commands,
  info about what the fuck pause keys are, that kind of thing.  The help window should have a section
  for them.*/