CGPathMoveToPoint   (bottomArrow, NULL, round(aPoint.x) + .5/self.contentsScale -3, aPoint.y - aLength+1 +4);
CGPathAddLineToPoint(bottomArrow, NULL, round(aPoint.x) + .5/self.contentsScale   , aPoint.y - aLength+1   );
CGPathAddLineToPoint(bottomArrow, NULL, round(aPoint.x) + .5/self.contentsScale +3, aPoint.y - aLength+1 +4);
