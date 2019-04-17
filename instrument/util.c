/* Copyright 2006 wParam, licensed under the GPL */

#include <windows.h>
#include "instrument.h"
#include <stdio.h>

void UtilListInsert (instlist_t **head, instlist_t *item)
{
	while (*head)
	{
		head = &(*head)->next;
	}

	//item->next = *head;
	*head = item;

}

void UtilListRemove (instlist_t **head, instlist_t *item)
{
	while (*head)
	{
		if (*head == item)
		{
			*head = item->next;
			return;
		}

		head = &(*head)->next;
	}
}


HANDLE UtilCreateEventf (SECURITY_ATTRIBUTES *sa, BOOL manualreset, BOOL initstate, char *name, ...)
{
	char buffer[MAX_PATH];
	va_list marker;
	
	va_start (marker, name);	
	_vsnprintf (buffer, MAX_PATH - 1, name, marker);
	buffer[MAX_PATH - 1] = '\0';
	va_end (marker);

	return CreateEvent (sa, manualreset, initstate, buffer);
}


HANDLE UtilOpenEventf (char *name, ...)
{
	char buffer[MAX_PATH];
	va_list marker;
	
	va_start (marker, name);	
	_vsnprintf (buffer, MAX_PATH - 1, name, marker);
	buffer[MAX_PATH - 1] = '\0';
	va_end (marker);

	return OpenEvent (EVENT_ALL_ACCESS, FALSE, buffer);
}

