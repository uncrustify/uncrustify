if(progress <= 0)
{
	[[NSBezierPath bezierPathWithOvalInRect: NSMakeRect(NSMinX(pieRect)+stroke,NSMinY(pieRect)+stroke,
	                                                    NSWidth(pieRect)-2*stroke,NSHeight(pieRect)-2*stroke)] fill];
}
