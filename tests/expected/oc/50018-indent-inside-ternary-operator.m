flag
? [Cmpnt Cmpnt:(isChildActionSheet ? TypeBack : TypeCancel)]
: nil;


[[BottomSheetItem alloc]
 iconName:selected
 ? g.re
	 .at
 : g
	 .re
	 .at
 builder:nil
 handler:^{
 }
]


[[BottomSheetItem alloc]
iconName: selected
 ? iconName : g
	 .re
	 .at
	 builder:nil
handler: ^{
	 }
]

        (event
	?   [FSBottomSheetActionCellItemVariant
	     action:AKAction<> :: actionFromSenderlessBlock(^{
								    auto const strongSelf = weakSelf;
							    })]
	: nil);


[[ViewController alloc] strategy: (strategy
	? [QuestionMarkStmt new]
	: [ColonStmt new])
 toolbox: _one];

[[ViewController alloc] strategy: (strategy
	?: [SourceStrategy new])
 toolbox: _two];



flag1
?   ( flag2
		? ( flag3
				?   [ViewController selector1:^{
					     NSLog(@"selector1");
				     }]
				:  [ViewController selector2:^(){
					    NSLog(@"selector2");
				    }] )
		:   ( result3  )
		)
:  (  flag5
		? ( flag
				? result4
				: [ViewController preSelector:flag selector3:{
					   .x = 10,
				   }])
		:   (  flag6
				? [ViewController preSelector:flag selector3:^{
					   NSLog(@"selector3");
				   }]
				: ( result7   )
				)
		);


flag1
?   result1
:  (
		flag5
		);



showButton ? Action<>::actionFromBlock(^(Component *component) {
	return nil;
}) : nil;

showButton
? Action<>::actionFromBlock(^(Component *component) {
	return nil;
})
: nil;

showButton
? Action<>::actionFromBlock(^(Component *component) {
	return nil;
}) : nil;


showButton
? Action<>::actionFromBlock([] (Component *component) {
	return nil;
})
: nil;
