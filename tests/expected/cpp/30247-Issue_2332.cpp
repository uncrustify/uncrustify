CPoint GetPoint()
{
	return { obj_.GetCoordinateXFromObject(),
	         obj_.GetCoordinateYFromObject(),
	         obj_.GetCoordinateZFromObject() };
}
