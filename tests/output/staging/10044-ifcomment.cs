#define RENDER_POINTS_USING_MESH

namespace UnityEditor
{
    internal class CurveWrapper
    {
        private void DoIconAndName(Rect rect, AnimationWindowHierarchyNode node, bool selected, bool focused, float indent)
        {
            EditorGUIUtility.SetIconSize(new Vector2(13, 13));   // If not set we see icons scaling down if text is being cropped
        }

        public void foo()
        {
            if (this)
            {
                if (b)
                {
                    // Now draw
                    for (int i = 0; i < ticks.Length; i++)
                    {
                        ticksPos[i] /= axisUiScalars.y;
                        if (ticksPos[i] < vRangeMin || ticksPos[i] > vRangeMax)
                            continue;

                        Vector2 pos = DrawingToViewTransformPoint(new Vector2(0, ticksPos[i]));
                        // Important to take floor of positions of GUI stuff to get pixel correct alignment of
                        // stuff drawn with both GUI and Handles/GL. Otherwise things are off by one pixel half the time.
                        pos = new Vector2(pos.x, Mathf.Floor(pos.y));

                        float uiValue = ticks[i];
                        Rect labelRect;
                        if (settings.vTickStyle.centerLabel)
                            labelRect = new Rect(0, pos.y - 8, leftmargin - 4, 16);  // text expands to the left starting from where grid starts (leftmargin size must ensure text is visible)
                        else
                            labelRect = new Rect(0, pos.y - 13, labelSize, 16);     // text expands to the right starting from left side of window

                        GUI.Label(labelRect, uiValue.ToString(format) + settings.vTickStyle.unit, ms_Styles.labelTickMarksY);
                    }
                }
            }
            // Cleanup
            GUI.color = tempCol;

            GUI.EndClip();
        }
    }
} // namespace

namespace UnityEditor
{
    internal class TreeView
    {
        public System.Action<int[]> selectionChangedCallback { get; set; } // ids
        public System.Action<int> itemDoubleClickedCallback { get; set; } // id
        public System.Action<int[], bool> dragEndedCallback { get; set; } // dragged ids, if null then drag was not allowed, bool == true if dragging tree view items from own treeview, false if drag was started outside
        public System.Action<int> contextClickItemCallback { get; set; } // clicked item id
    }
}
