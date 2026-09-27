void f()
{
  ConstructorScope scope(self, identifier, ^{
    return [[WidgetState alloc] initWithSectionContext:sectionContext componentContext:componentContext contextFactory:contextFactory];
  });
}
