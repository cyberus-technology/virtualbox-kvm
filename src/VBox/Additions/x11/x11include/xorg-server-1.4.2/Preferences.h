/*
 * Copyright (c) 2002-2003 Torrey T. Lyons. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization.
 */

#import <Cocoa/Cocoa.h>

@interface Preferences : NSObject
{
    IBOutlet NSPanel *window;
    IBOutlet id displayField;
    IBOutlet id dockSwitchButton;
    IBOutlet id fakeButton;
    IBOutlet id button2ModifiersMatrix;
    IBOutlet id button3ModifiersMatrix;
    IBOutlet id switchKeyButton;
    IBOutlet id keymapFileField;
    IBOutlet id modeMatrix;
    IBOutlet id modeWindowButton;
    IBOutlet id startupHelpButton;
    IBOutlet id systemBeepButton;
    IBOutlet id mouseAccelChangeButton;
    IBOutlet id useXineramaButton;
    IBOutlet id addToPathButton;
    IBOutlet id addToPathField;
    IBOutlet id useDefaultShellMatrix;
    IBOutlet id useOtherShellField;
    IBOutlet id depthButton;

    BOOL isGettingKeyCode;
    int keyCode;
    int modifiers;
    NSMutableString *switchString;
}

- (IBAction)close:(id)sender;
- (IBAction)pickFile:(id)sender;
- (IBAction)saveChanges:(id)sender;
- (IBAction)setKey:(id)sender;

- (BOOL)sendEvent:(NSEvent *)anEvent;

- (void)awakeFromNib;
- (void)windowWillClose:(NSNotification *)aNotification;

+ (void)setUseKeymapFile:(BOOL)newUseKeymapFile;
+ (void)setKeymapFile:(NSString *)newFile;
+ (void)setSwitchString:(NSString *)newString;
+ (void)setKeyCode:(int)newKeyCode;
+ (void)setModifiers:(int)newModifiers;
+ (void)setDisplay:(int)newDisplay;
+ (void)setDockSwitch:(BOOL)newDockSwitch;
+ (void)setFakeButtons:(BOOL)newFakeButtons;
+ (void)setButton2Mask:(int)newFakeMask;
+ (void)setButton3Mask:(int)newFakeMask;
+ (void)setMouseAccelChange:(BOOL)newMouseAccelChange;
+ (void)setUseQDCursor:(int)newUseQDCursor;
+ (void)setRootless:(BOOL)newRootless;
+ (void)setUseAGL:(BOOL)newUseAGL;
+ (void)setModeWindow:(BOOL)newModeWindow;
+ (void)setStartupHelp:(BOOL)newStartupHelp;
+ (void)setSystemBeep:(BOOL)newSystemBeep;
+ (void)setEnableKeyEquivalents:(BOOL)newKeyEquivs;
+ (void)setXinerama:(BOOL)newXinerama;
+ (void)setAddToPath:(BOOL)newAddToPath;
+ (void)setAddToPathString:(NSString *)newAddToPathString;
+ (void)setUseDefaultShell:(BOOL)newUseDefaultShell;
+ (void)setShellString:(NSString *)newShellString;
+ (void)setDepth:(int)newDepth;
+ (void)setDisplayModeBundles:(NSArray *)newBundles;
+ (void)saveToDisk;

+ (BOOL)useKeymapFile;
+ (NSString *)keymapFile;
+ (NSString *)switchString;
+ (unsigned int)keyCode;
+ (unsigned int)modifiers;
+ (int)display;
+ (BOOL)dockSwitch;
+ (BOOL)fakeButtons;
+ (int)button2Mask;
+ (int)button3Mask;
+ (BOOL)mouseAccelChange;
+ (int)useQDCursor;
+ (BOOL)rootless;
+ (BOOL)useAGL;
+ (BOOL)modeWindow;
+ (BOOL)startupHelp;
+ (BOOL)systemBeep;
+ (BOOL)enableKeyEquivalents;
+ (BOOL)xinerama;
+ (BOOL)addToPath;
+ (NSString *)addToPathString;
+ (BOOL)useDefaultShell;
+ (NSString *)shellString;
+ (int)depth;
+ (NSArray *)displayModeBundles;

@end

// Possible settings for useQDCursor
enum {
    qdCursor_Never,	// never use QuickDraw cursor
    qdCursor_Not8Bit,	// don't try to use QuickDraw with 8-bit depth
    qdCursor_Always	// always try to use QuickDraw cursor
};

// Possible settings for depth
enum {
    depth_Current,
    depth_8Bit,
    depth_15Bit,
    depth_24Bit
};
