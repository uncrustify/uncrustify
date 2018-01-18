public class JavaClass {
    private boolean isButtonHit(ImageView imageView, int x, int y) {
        if (imageView != null) {
            RelativeLayout.LayoutParams layoutParams = (RelativeLayout.LayoutParams) imageView.getLayoutParams();
            Rect buttonRect = new Rect((int) (layoutParams.leftMargin - buttonExtraMargin),
                (int) (layoutParams.topMargin - buttonExtraMargin),
                (int) (layoutParams.leftMargin + imageView.getWidth() + buttonExtraMargin),
                (int) (layoutParams.topMargin + imageView.getHeight() + buttonExtraMargin));

            if (buttonRect.contains(x, y)) {
                return true;
            }
        }

        Map< ? , ? > map = (Map< ? , ? >) object;

        return false;
    }

    @SuppressWarnings("unchecked")
    public static List<Object> fromJSON(JSONArray obj) {
        return (List<Object>) fromJSON((Object) obj);
    }
}
