/* Copyright 2006 wParam, licensed under the GPL */


#include <windows.h>
#include "keyboard.h"


struct
{
	char *text;
	int vk;
} 
KbVkMapTable[] = 
{
	
	{"CTRL", VK_CONTROL},
	{"SPACE", VK_SPACE},
	{"ALT", VK_MENU},
	{"WIN", VK_LWIN},

	//two names for the 5 on the numeric keypad when numlock is off.
	{"FIVE", VK_CLEAR},
	{"CLEAR", VK_CLEAR},

	{"DELETE", VK_DELETE},
	{"INSERT", VK_INSERT},
	{"PGDN", VK_NEXT},
	{"PGUP", VK_PRIOR},
	{"HOME", VK_HOME},
	{"END", VK_END},
	{"APPS", VK_APPS},
	
	{"LEFTARROW", VK_LEFT},
	{"RIGHTARROW", VK_RIGHT},
	{"UPARROW", VK_UP},
	{"DOWNARROW", VK_DOWN},

	{"ESCAPE", VK_ESCAPE},
	{"F1", VK_F1},
	{"F2", VK_F2},
	{"F3", VK_F3},
	{"F4", VK_F4},
	{"F5", VK_F5},
	{"F6", VK_F6},
	{"F7", VK_F7},
	{"F8", VK_F8},
	{"F9", VK_F9},
	{"F10", VK_F10},
	{"F11", VK_F11},
	{"F12", VK_F12},
	{"PRINTSCREEN", VK_SNAPSHOT},
	{"PAUSE", VK_PAUSE},
	{"SCROLLLOCK", VK_SCROLL},

	{"`", 192},
	{"1", '1'},
	{"2", '2'},
	{"3", '3'},
	{"4", '4'},
	{"5", '5'},
	{"6", '6'},
	{"7", '7'},
	{"8", '8'},
	{"9", '9'},
	{"0", '0'},
	{"-", 189},
	{"=", 187},
	{"BACKSPACE", VK_BACK},

	{"TAB", VK_TAB},
	{"Q", 'Q'},
	{"W", 'W'},
	{"E", 'E'},
	{"R", 'R'},
	{"T", 'T'},
	{"Y", 'Y'},
	{"U", 'U'},
	{"I", 'I'},
	{"O", 'O'},
	{"P", 'P'},
	{"[", 219},
	{"]", 221},
	{"\\", 220},

	{"CAPSLOCK", VK_CAPITAL},
	{"A", 'A'},
	{"S", 'S'},
	{"D", 'D'},
	{"F", 'F'},
	{"G", 'G'},
	{"H", 'H'},
	{"J", 'J'},
	{"K", 'K'},
	{"L", 'L'},
	{";", 186},
	{"\'", 222},
	{"ENTER", VK_RETURN},

	{"SHIFT", VK_SHIFT},
	{"Z", 'Z'},
	{"X", 'X'},
	{"C", 'C'},
	{"V", 'V'},
	{"B", 'B'},
	{"N", 'N'},
	{"M", 'M'},
	{",", 188},
	{".", 190},
	{"/", 191},

	{"NUMLOCK", VK_NUMLOCK},
	{"NUMPAD0", VK_NUMPAD0},
	{"NUMPAD1", VK_NUMPAD1},
	{"NUMPAD2", VK_NUMPAD2},
	{"NUMPAD3", VK_NUMPAD3},
	{"NUMPAD4", VK_NUMPAD4},
	{"NUMPAD5", VK_NUMPAD5},
	{"NUMPAD6", VK_NUMPAD6},
	{"NUMPAD7", VK_NUMPAD7},
	{"NUMPAD8", VK_NUMPAD8},
	{"NUMPAD9", VK_NUMPAD9},
	{"NUMPADADD", VK_ADD},
	{"NUMPADSUBTRACT", VK_SUBTRACT},
	{"NUMPADMULTIPLY", VK_MULTIPLY},
	{"NUMPADDIVIDE", VK_DIVIDE},
	{"NUMPADDECIMAL", VK_DECIMAL},
	
	{NULL, 0},
};

int KbTranslateVirtualKey (char *blah)
{
	int x;

	if (*blah == '~')
	{
		return PlAtoi (blah + 1);
	}

	x = 0;
	while (KbVkMapTable[x].text)
	{
		if (strcmp (KbVkMapTable[x].text, blah) == 0)
			return KbVkMapTable[x].vk;

		x++;
	}

	return 0;
}

char *KbUntranslateVirtualKey (int vk)
{
	int x;

	x = 0;
	while (KbVkMapTable[x].text)
	{
		if (KbVkMapTable[x].vk == vk)
			return KbVkMapTable[x].text;

		x++;
	}

	return NULL;
}

int KbVk (parerror_t *pe, char *str)
{
	DERR st;

	fail_if (!str, 0, PE_BAD_PARAM (1));

	st = KbTranslateVirtualKey (str);
	fail_if (!st, 0, PE_STR ("Unknown key: %s", COPYSTRING (str), 0, 0, 0));

	return st;
failure:
	return 0;
}

char *KbUnvk (parerror_t *pe, unsigned int vk)
{
	DERR st;
	char *res;
	char *out = NULL;

	fail_if (vk > 255, 0, PE ("Vk %i out of range", vk, 0, 0, 0));

	res = KbUntranslateVirtualKey (vk);

	if (res)
	{
		out = ParMalloc (strlen (res) + 1);
		fail_if (!out, 0, PE_OUT_OF_MEMORY (strlen (res) + 1));
		strcpy (out, res);
	}
	else
	{
		out = ParMalloc (40);
		fail_if (!out, 0, PE_OUT_OF_MEMORY (40));
		PlSnprintf (out, 39, "~%i", vk);
		out[39] = '\0';
	}

	return out;

failure:
	if (out)
		ParFree (out);

	return NULL;
}


void KbSetVerbose (int v)
{
	KbVerbose = v;
}
