[NSPasteboardItem pasteboardItemWithProvider:self
		                      forTypes:@[ NSPasteboardTypePDF ]
		                       andData:@[
					   kNSUTIExportedAgaroseGel,
					   [NSKeyedArchiver archivedDataWithRootObject:self.selectedIndexes.count != 0 ?[self.gels objectsAtIndexes:self.selectedIndexes] : self.gels]
		    ]];

[ViewController simple_First:firstArg
simple_Two:secondArg
simple_3:thirdArg];


[ViewController preFirst:(
                  pre_1_arg
                )];

[ViewController firstSelectorOne:arg1 preFirst:(
                                        pre_1_arg
                                      )];

[ViewController preFirst:^{
                  return arg4;
                }
        firstSelectorOne:arg1];

[ViewController firstSelectorOne:arg1 preFirst:^{
  return arg4;
}];

[ViewController firstSelectorOne:(flag
                  ? arg5_1
                  : arg5_2
                ) toolbox:_toolbox];

[ViewController preFirst:(
                  pre_1_arg
                )
        firstSelectorOne:
        arg1
            selector_two:(
              arg2
            )
              Selector_3:{
                .arg3 = 1
              }
         fourth_Selector:^{
           return arg4;
         }
       selector_number_5:(flag
         ? arg5_1
         : arg5_2
       )
       selector_number_5:(flag
         ? arg5_1
         : arg5_2
       )
                  sixSel:(flag
                    ?: arg6_1)
        seventh_selector:(
          arg7
        )
              toolboxSel:toolboxArg];

[[ViewController alloc] strategy:(strategy
                          ? [QuestionMarkStmt new]
                          : [ColonStmt new])
                         toolbox:_one];

[[ViewController alloc] strategy:(strategy
                          ?: [SourceStrategy new])
                         toolbox:_two];
