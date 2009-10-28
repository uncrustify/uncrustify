/*
 File: ATColorTableController.m
 Abstract: A controller used by the ATImageTextCell to edit the color property. It is implemented in an abstract enough way to be used by a class other than the cell. 
 
 Version: 1.0
 
 Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
 Inc. ("Apple") in consideration of your agreement to the following
 terms, and your use, installation, modification or redistribution of
 this Apple software constitutes acceptance of these terms.  If you do
 not agree with these terms, please do not use, install, modify or
 redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and
 subject to these terms, Apple grants you a personal, non-exclusive
 license, under Apple's copyrights in this original Apple software (the
 "Apple Software"), to use, reproduce, modify and redistribute the Apple
 Software, with or without modifications, in source and/or binary forms;
 provided that if you redistribute the Apple Software in its entirety and
 without modifications, you must retain this notice and the following
 text and disclaimers in all such redistributions of the Apple Software.
 Neither the name, trademarks, service marks or logos of Apple Inc. may
 be used to endorse or promote products derived from the Apple Software
 without specific prior written permission from Apple.  Except as
 expressly stated in this notice, no other rights or licenses, express or
 implied, are granted by Apple herein, including but not limited to any
 patent rights that may be infringed by your derivative works or by other
 works in which the Apple Software may be incorporated.
 
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
 MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
 OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
 AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 
 Copyright (C) 2009 Apple Inc. All Rights Reserved.
 
 */

#import "ATColorTableController.h"
#import "ATPopupWindow.h"

@implementation ATColorTableController

+ (ATColorTableController *)sharedColorTableController {
    static ATColorTableController *gSharedColorTableController = nil;
    if (gSharedColorTableController == nil) {
        gSharedColorTableController = [[[self class] alloc] initWithNibName:@"ColorTable" bundle:[NSBundle bundleForClass:[self class]]];
    }
    return gSharedColorTableController;
}

@synthesize delegate = _delegate;
@dynamic selectedColor, selectedColorName;

- (void)dealloc {
    [_colorList release];
    [_colorNames release];
    [_window release];
    [super dealloc];
}

- (void)loadView {
    [super loadView];
    _colorList = [[NSColorList colorListNamed:@"Crayons"] retain];
    _colorNames = [[_colorList allKeys] retain];
    [_tableColorList setIntercellSpacing:NSMakeSize(3, 3)];
    [_tableColorList setTarget:self];
    [_tableColorList setAction:@selector(_tableViewAction:)];
}

- (NSColor *)selectedColor {
    NSString *name = [self selectedColorName];
    if (name != nil) {
        return [_colorList colorWithKey:name];
    } else {
        return nil;
    }
}

- (NSString *)selectedColorName {
    if ([_tableColorList selectedRow] != -1) {
        return [_colorNames objectAtIndex:[_tableColorList selectedRow]];
    } else {
        return nil;
    }
}

- (void)_selectColor:(NSColor *)color {
    // Search for that color in our list
    NSInteger row = 0;
    for (NSString *name in _colorNames) {
        NSColor *colorInList = [_colorList colorWithKey:name];
        if ([color isEqual:colorInList]) {
            break;
        }
        row++;
    }    
    _updatingSelection = YES;
    if (row != -1) {
        [_tableColorList scrollRowToVisible:row];
        [_tableColorList selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
    } else {
        [_tableColorList scrollRowToVisible:0];
        [_tableColorList selectRowIndexes:[NSIndexSet indexSet] byExtendingSelection:NO];
    }
    _updatingSelection = NO;
}

- (void)_createWindowIfNeeded {
    if (_window == nil) {
        NSRect viewFrame = self.view.frame;
        // Create and setup our window
        _window = [[ATPopupWindow alloc] initWithContentRect:viewFrame styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO];
        [_window setReleasedWhenClosed:NO];
        [_window setLevel:NSPopUpMenuWindowLevel];
        [_window setHasShadow:YES];        
        [[_window contentView] addSubview:self.view];
        [_window makeFirstResponder:_tableColorList];
        
        // Make the window have a clear color and be non-opaque for our pop-up animation
        [_window setBackgroundColor:[NSColor clearColor]];
        [_window setOpaque:NO];
    }
}

- (void)_windowClosed:(NSNotification *)note {
    if (_eventMonitor) {
        [NSEvent removeMonitor:_eventMonitor];
        _eventMonitor = nil;
    }
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowWillCloseNotification object:_window];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSApplicationDidResignActiveNotification object:nil];
}

- (void)_closeAndSendAction:(BOOL)sendAction {
    [_window close];
    if (sendAction) {
        if ([self.delegate respondsToSelector:@selector(colorTableController:didChooseColor:named:)]) {
            [self.delegate colorTableController:self didChooseColor:self.selectedColor named:self.selectedColorName];
        }
    } else {
        if ([self.delegate respondsToSelector:@selector(didCancelColorTableController:)]) {
            [self.delegate didCancelColorTableController:self];
        }
    }
}

- (void)_windowShouldClose:(NSNotification *)note {
    [self _closeAndSendAction:NO];
}

- (void)editColor:(NSColor *)color locatedAtScreenRect:(NSRect)rect {
    [self _createWindowIfNeeded];
    [self _selectColor:color];
    NSPoint origin = rect.origin;
    NSRect windowFrame = [_window frame];
    // The origin is the lower left; subtract the window's height
    origin.y -= NSHeight(windowFrame);
    // Center the popup window under the rect
    origin.y += floor(NSHeight(rect) / 3.0);
    origin.x -= floor(NSWidth(windowFrame) / 2.0);
    origin.x += floor(NSWidth(rect) / 2.0);
    
    [_window setFrameOrigin:origin];
    [_window popup];
    
    // Add some watches on the window and application
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(_windowClosed:) 
                                             name:NSWindowWillCloseNotification 
                                             object:_window];
   
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_windowShouldClose:) name:NSApplicationDidResignActiveNotification object:nil];
    
    // Start watching events to figure out when to close the window
    NSAssert(_eventMonitor == nil, @"_eventMonitor should not be created yet");
    _eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSLeftMouseDownMask | NSRightMouseDownMask | NSOtherMouseDownMask | NSKeyDownMask handler: ^(NSEvent *incomingEvent) {
        NSEvent *result = incomingEvent;
        NSWindow *targetWindowForEvent = [incomingEvent window];
        if (targetWindowForEvent != _window) {
            [self _closeAndSendAction:NO];
        } else if ([incomingEvent type] == NSKeyDown) {
            if ([incomingEvent keyCode] == 53) {
                // Escape
                [self _closeAndSendAction:NO];
                result = nil; // Don't process the event
            } else if ([incomingEvent keyCode] == 36) {
                // Enter
                [self _closeAndSendAction:YES];
                result = nil;
            }
        }
        return result;
    }];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return _colorNames.count;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    return [_colorNames objectAtIndex:row];
}

- (void)tableView:(NSTableView *)tableView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSColor *color = [_colorList colorWithKey:[_colorNames objectAtIndex:row]];
    [cell setColor:color];
}

- (void)_tableViewAction:(id)sender {
    [self _closeAndSendAction:YES];
}

@end

