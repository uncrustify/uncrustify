// Test case for issue where OC message colon is incorrectly classified
// as CT_COND_COLON when it appears AFTER a ternary expression in a
// multi-line OC message send.
// This causes sp_cond_colon to add a spurious space before the message colon
// on lines that follow a line containing a ternary.

// Case 1: Simple multi-line message with ternary in one argument
// The colon after workForeignEntityType should NOT get spaces
- (void)testSimpleCase {
	Contact *contact =
		[[Contact alloc]
		 initWithIdentifier:props.identifier
		 phoneNumber:props.type == TypePhone ? props.id : nil
		 email:props.type == TypeEmail ? props.id : nil
		 workForeignEntityType:nil
		 imageUrl:nil];
}

// Case 2: Multiple ternaries followed by non-ternary arguments
// All colons after the ternary lines should remain without spaces
- (void)testMultipleTernaries {
	id result =
		[[Builder alloc]
		 initWithA:condA ? valA1 : valA2
		 b:condB ? valB1 : valB2
		 c:condC ? valC1 : valC2
		 simpleArg:someValue
		 anotherArg:anotherValue
		 finalArg:nil];
}

// Case 3: Nested message sends with ternaries
// Inner message colons should not be affected by outer ternaries
- (void)testNestedMessages {
	[[self manager]
	 performAction:condition ? ActionTypeA : ActionTypeB
	 withTarget:target
	 completion:^{
	         [[self helper]
	          doSomething:innerCondition ? Yes : No
	          withParam:paramValue
	          andAnother:anotherParam];
	 }];
}

// Case 4: Ternary as first argument, multiple following arguments
- (void)testTernaryFirst {
	[object
	 setFirstArg:condition ? @"yes" : @"no"
	 secondArg:@"value"
	 thirdArg:123
	 fourthArg:nil
	 fifthArg:@[]];
}

// Case 5: Long multi-line message with ternary in the middle
- (void)testTernaryInMiddle {
	[[NSAttributedString alloc]
	 initWithString:fileName
	 attributes:_selected ? _attrsSelected() : _attrsUnselected()];
}

// Case 6: ComponentKit-style multi-line message (real-world pattern)
- (void)testComponentKitStyle {
	return
	        [[Contact alloc]
	         initWithIdentifier:props.contactIdentifier
	         displayName:nil
	         phoneNumber:props.cellType == ContactCellTypePhone ? props.contactIdentifier : nil
	         email:props.cellType == ContactCellTypeEmail ? props.contactIdentifier : nil
	         workForeignEntityType:nil
	         imageUrl:nil
	         inviteInfo:someInfo
	         isAlreadyInvited:NO];
}

// Case 7: Deeply nested ternary followed by simple args
- (void)testDeepTernary {
	[self
	 methodWithArg:a ? (b ? c : d) : (e ? f : g)
	 simpleArg:value
	 anotherSimple:nil];
}
