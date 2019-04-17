/* Copyright 2006 wParam, licensed under the GPL */

#ifndef _WP_CODESEG_H_
#define _WP_CODESEG_H_

#define CODE_DATA(name, value)				\
name:										\
__asm {	_emit (value & 0xFF)}				\
__asm {	_emit (value & 0xFF00) >> 8}		\
__asm {	_emit (value & 0xFF0000) >> 16}		\
__asm {	_emit (value & 0xFF000000) >> 24}	\

#define CODE_DATA_ANON(value)				\
__asm {	_emit (value & 0xFF)}				\
__asm {	_emit (value & 0xFF00) >> 8}		\
__asm {	_emit (value & 0xFF0000) >> 16}		\
__asm {	_emit (value & 0xFF000000) >> 24}	\

#define CODE_DATA_SKIP(name, value)			\
goto name##goto;							\
name:										\
__asm {	_emit (value & 0xFF)}				\
__asm {	_emit (value & 0xFF00) >> 8}		\
__asm {	_emit (value & 0xFF0000) >> 16}		\
__asm {	_emit (value & 0xFF000000) >> 24}	\
name##goto:

#define GET_CODE_DATA(name, res)	\
__asm {	push name}					\
__asm {	pop res}					\

#define GET_CODE_PTR(name, res)		\
__asm {	push eax}					\
__asm {	lea eax, name}				\
__asm {	mov res, eax}				\
__asm {	pop eax}					\


#endif