-(void) test {
    if (@available(macOS 10.12.2, *)) {
        self.automaticTextCompletionEnabled = YES;
        self.allowsCharacterPickerTouchBarItem = NO;
    }

    if (@available( macOS 10.12,*)) {
        self.automaticTextCompletionEnabled = YES;
        self.allowsCharacterPickerTouchBarItem = NO;
    }

}
