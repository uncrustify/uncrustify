// OC mesg inside array/dictionary
_sections1 = @[
    [SectionModel resultsWithContacts:contacts]
];

_sections2 = @[
    [[SectionModel mesg] resultsWithContacts1:contacts1],
    [[SectionModel mesg] resultsWithContacts2:contacts2]
];

_sections3 = @[[SectionModel resultsWithContacts:contacts]];

@[
    something
];

@[
    [something mesg]
];
