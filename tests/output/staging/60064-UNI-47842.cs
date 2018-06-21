//Case 1
void Set_m()
{
    m.SetColors((List<Color>)null);     Check.AreEqual(0, m.colors.Length);
    m.SetTriangles((List<int>)null, 0); Check.AreEqual(0, m.triangles.Length);

//Case 2
    for (int i = 0; i < 4; ++i)
        m.SetUVs(i, (List<Vector3>)null);
}

//Case 3
class a
{
    void dummy()
    {
        {
            {
                abc(transform.rotation * Quaternion.LookRotation(Vector3.forward),
                    size,
                    EventType.Repaint
                );
            }
        }
    }
}

//Case 4
static void DefaultRadiusHandleDrawFunction(
    int controlID, Vector3 position, Quaternion rotation, float size, EventType eventType
)
{
    return Scripting::ScriptingWrapperFor(useCurveRanges ?
        cache.GetPreview(previewSize, actualObject, color, topFillColor, bottomFillColor, curveRanges) :
        cache.GetPreview(previewSize, actualObject, color, topFillColor, bottomFillColor)
    );
}

//Case-5
int func_name()
{
    m_ReadingToken = m_Sensor->ReadingChanged::add(ref new Windows::Foundation::TypedEventHandler<
        SensorT ^
        ,   SensorReadingChangedEventArgsT ^
        >(this, &MTSensorInput<SensorT, SensorReadingChangedEventArgsT>::ReadingChanged));
}

//Case-6
REGISTER_TYPE_ATTRIBUTES(AvatarMask,
    (LegacyPersistentTypeID, (1011,      // AvatarMask (old)
        1012))                           // AvatarSkeletonMask
);

//Case 7
char* br = (char*)mem2chunk((size_t)(((size_t)((char*)mem + alignment -
    SIZE_T_ONE)) &
    -alignment));


//Case 8
int func_name()
{
    if (fieldInfo.FieldType.IsGenericList() &&
        (fieldInfo.FieldType.GenericTypeArguments[0].IsArray ||
         fieldInfo.FieldType.GenericTypeArguments[0].IsGenericList() ||
         !ShouldFieldTypeBeSerialized(fieldInfo.FieldType.GenericTypeArguments[0]) ||
         !ShouldArrayFieldTypeBeSerialized(fieldInfo.FieldType.GenericTypeArguments[0])))
    {
        return 0;
    }
}

//Case 10
void OnGUI()
{
    curveX = EditorGUI.CurveField(
        new Rect(3, 3, position.width - 6, 50),
        "Animation on X", curveX);
    curveY = EditorGUI.CurveField(
        new Rect(3, 56, position.width - 6, 50),
        "Animation on Y", curveY);
    curveZ = EditorGUI.CurveField(
        new Rect(3, 109, position.width - 6, 50),
        "Animation on Z", curveZ);
}

//Case 11
void DoRoundRotation()
{
    foreach (Transform t in Selection.transforms)
        t.rotation = Quaternion.Euler(
            new Vector3(Mathf.Round(t.eulerAngles.x / 45f) * 45f,
                Mathf.Round(t.eulerAngles.y / 45f) * 45f,
                Mathf.Round(t.eulerAngles.z / 45f) * 45f));
}

//Case 12
int func_name()
{
    for (int i = 0; i < sv.Length; i++)
    {
        sv[i].x = Mathf.Clamp(
            (o.vertices[i].x - o.bounds.center.x -
                (o.textureRectOffset.x / o.texture.width) + o.bounds.extents.x) /
            (2.0f * o.bounds.extents.x) * o.rect.width,
            0.0f, o.rect.width);
    }
}

//Case 13
private void DoCurveDropdown(Rect rect, AnimationWindowHierarchyNode node, int row, bool enabled)
{
    rect = new Rect(
        rect.xMax - k_RowRightOffset - 12,
        rect.yMin + 2,
        22, 12);
}

//Case 14
templateProp.stringValue = Templates[
    ThumbnailList(
        GUILayoutUtility.GetRect(numCols * kThumbnailSize, numRows * (kThumbnailSize + kThumbnailLabelHeight), GUILayout.ExpandWidth(false)),
        GetTemplateIndex(templateProp.stringValue),
        TemplateGUIThumbnails,
        numCols
    )].ToString();


//Case 15
int number_to_move = (yy_n_chars) + 2;
char *dest = &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[
    YY_CURRENT_BUFFER_LVALUE->yy_buf_size + 2];


//Case 16
public ExpressionFinderBase(string[] referenceEndLocations)
{
    expectations = referenceEndLocations.ToDictionary(
        line => line.Split('=')[0].Trim(),
        line => Int32.Parse(line.Split('=')[1].Trim()));
}
