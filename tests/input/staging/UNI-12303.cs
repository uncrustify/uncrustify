//This gets a double indentation
devicePositions[device] = rect = new Rect(
    Vector2.Lerp(rect.position, target.position, 0.1f),
    Vector2.Lerp(rect.size, target.size, 0.1f));
