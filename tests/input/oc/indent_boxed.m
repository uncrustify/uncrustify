
- (IBAction) copy:(nullable id) inSender {
NSPasteboard* const pasteboard = NSPasteboard.generalPasteboard;
[pasteboard clearContents];
[pasteboard writeObjects:@[
[NSPasteboardItem pasteboardItemWithProvider:self forTypes:@[ NSPasteboardTypePDF ]
andData:@[ kNSUTIExportedAgaroseGel,
[NSKeyedArchiver archivedDataWithRootObject:self.selectedIndexes.count != 0 ? [self.gels objectsAtIndexes:self.selectedIndexes] : self.gels]
]]
]];
}

- (IBAction) copy:(nullable id) inSender {
NSPasteboard* const pasteboard = NSPasteboard.generalPasteboard;
[pasteboard clearContents];
[pasteboard writeObjects:@[[NSPasteboardItem pasteboardItemWithProvider:self forTypes:@[ NSPasteboardTypePDF ] andData:@[
kNSUTIExportedAgaroseGel, [NSKeyedArchiver archivedDataWithRootObject:self.selectedIndexes.count != 0 ? [self.gels objectsAtIndexes:self.selectedIndexes] : self.gels]
]] ]];

NSArray* a = @[];
NSArray* b = @[@1,@2,@3];
NSArray* c = @[
@1, @2, @3
];
NSArray* d = @[@[@1], @[@2], @[@3]];
NSArray* e = @[
@[@1], @[@2], @[@3]
];
NSMutableArray* f = [NSMutableArray arrayWithArray:@[@[@1], @[@2], @[@3]]];
NSMutableArray* g = [NSMutableArray arrayWithArray:@[
@[@1], @[@2], @[@3]
]];
NSMutableDictionary* d1 = [NSMutableDictionary dictionaryWithDictionary:@{
@"Keys":@[
@{@"A": @1},
@{@"B": @2}.
@{@"C": @3}
]
}];
}

inline static void installGelMarkers(void) {
[NSOperationQueue.mainQueue addOperationWithBlock:^{
[accessoryView.textStorage setAttributedString:[[NSAttributedString alloc] initWithString:error.localizedDescription attributes:@{ NSFontAttributeName: [NSFont systemFontOfSize:NSFont.systemFontSize] }]];
NSAlert *alert = [[NSAlert alloc] init];
}];
}

[[NSAttributedString alloc] initWithString:inJunction.reverseName attributes:@{
NSFontAttributeName: font,
NSForegroundColorAttributeName: inJunction.reverseColor
}];
@{
NSFontAttributeName: self.font,
NSForegroundColorAttributeName: inJunction.forwardColor
}
[[NSAttributedString alloc] initWithString:inJunction.reverseName attributes:@{ NSFontAttributeName: font, NSForegroundColorAttributeName: inJunction.reverseColor }];
[[NSAttributedString alloc] initWithString:inJunction.reverseName
attributes:@{ NSFontAttributeName: font, NSForegroundColorAttributeName: inJunction.reverseColor }];
- (void) drawReversePrimerForJunction:(GibsonJunction*) inJunction bounds:(NSRect) inBounds {
NSString* const string1 = nil,
string2 = nil,
string3 = nil;
//does not compile but does test shift operator formatting
std::ostringstream ostream;
ostream << "hello"
<< ' '
<< "world";
NSString* const string = inJunction.reversePrimer;
[attributedString appendAttributedString:[[NSAttributedString alloc] initWithString:[string substringToIndex:range.location] attributes:@{
NSFontAttributeName: self.font,
NSForegroundColorAttributeName: inJunction.forwardColor
}]];
}
