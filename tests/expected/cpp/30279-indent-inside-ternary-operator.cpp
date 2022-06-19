(tmp
	? tmp->IsNewline()
		? "newline"
		: tmp->IsComment()
			? "comment"
			: "other"
	: tmp->IsNewline()
		? "newline"
		: tmp->IsComment()
			? "comment"
			: "other");

a
? b
	+ c
: d
	+ e;

return
        outerFlag
	? RadioButton
	: innerFlag
		? Badge
		: nil;

x = outerFlag
    ? RadioButton(
		    arg1
		    )
    : Checkbutton
	    .arg2;

Builder
.child(
	outerFlag
	? RadioButton(
			buttonArg
			)
	: innerFlag
		? Badge
			.component(
				LabelText)
		: nil
	);


accessoryType
? ConKSC1{}
: flag == false
	? ConKSC2{}
		.build()
	: flag == true
		? ConKSC3{}
			.build()
		: ConKSC4{}
			.build();

options.meta == nil
? metaCmpnt
:  CBuilder()
	.spacing(4)
	.subCmpnt(
		CBuilder()
		.build());

options.meta == nil
? CBuilder()
	.spacing(4)
	.subCmpnt(
		CBuilder()
		.build()
		)
: Builder
	.spacing;

options == nil ? CBuilder()
	.spacing(6)
: Builder
	.spacing;

options == nil ? CBuilder()
	.spacing(6) : Builder
	.spacing;

flag
? isChild
	? TypeBack
	: TypeCancel
: nil;


func something() {
	if (flag) {
		x == flag
		? Builder
			.spacing
		: Builder
			.spacing;
	}
}


flag1
?   ( flag2
		? ( flag3
				?   result1
				:  result2 )
		:   ( result3  )
		)
:  (  flag5
		? ( flag
				? result4
				: result5)
		:   (  flag6
				? result6
				: ( result7   )
				)
		);


flag1
?   result1
:  (
		flag5
		);
