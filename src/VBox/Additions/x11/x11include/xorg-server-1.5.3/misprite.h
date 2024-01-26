/*
 * misprite.h
 *
 * software-sprite/sprite drawing interface spec
 *
 * mi versions of these routines exist.
 */


/*

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
*/

typedef struct {
    Bool	(*RealizeCursor)(
		ScreenPtr /*pScreen*/,
		CursorPtr /*pCursor*/
);
    Bool	(*UnrealizeCursor)(
		ScreenPtr /*pScreen*/,
		CursorPtr /*pCursor*/
);
    Bool	(*PutUpCursor)(
		ScreenPtr /*pScreen*/,
		CursorPtr /*pCursor*/,
		int /*x*/,
		int /*y*/,
		unsigned long /*source*/,
		unsigned long /*mask*/
);
    Bool	(*SaveUnderCursor)(
		ScreenPtr /*pScreen*/,
		int /*x*/,
		int /*y*/,
		int /*w*/,
		int /*h*/
);
    Bool	(*RestoreUnderCursor)(
		ScreenPtr /*pScreen*/,
		int /*x*/,
		int /*y*/,
		int /*w*/,
		int /*h*/
);
    Bool	(*MoveCursor)(
		ScreenPtr /*pScreen*/,
		CursorPtr /*pCursor*/,
		int /*x*/,
		int /*y*/,
		int /*w*/,
		int /*h*/,
		int /*dx*/,
		int /*dy*/,
		unsigned long /*source*/,
		unsigned long /*mask*/
);
    Bool	(*ChangeSave)(
		ScreenPtr /*pScreen*/,
		int /*x*/,
		int /*y*/,
		int /*w*/,
		int /*h*/,
		int /*dx*/,
		int /*dy*/
);

} miSpriteCursorFuncRec, *miSpriteCursorFuncPtr;

extern Bool miSpriteInitialize(
    ScreenPtr /*pScreen*/,
    miSpriteCursorFuncPtr /*cursorFuncs*/,
    miPointerScreenFuncPtr /*screenFuncs*/
);
