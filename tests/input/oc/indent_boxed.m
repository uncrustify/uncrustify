
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
