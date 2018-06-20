if (m_Preview.GetExpanded())
{
    m_EventSearchString = EditorGUI.TextField(searchRect, m_EventSearchString, Styles.toolbarSearchField);
    if (GUILayout.Button(
        GUIContent.none,
        m_EventSearchString == string.Empty ? Styles.toolbarSearchFieldCancelEmpty : Styles.toolbarSearchFieldCancel))
    {
    }
}

// The closing parenthesis is being indented twice.
bool success = GenerateSecondaryUVSet(
    &mesh.vertices[0].x, mesh.vertices.size(),
    &triUV[0].x, &triList[0], triSrcPoly.size() ? &triSrcPoly[0] : 0, triCount,
    &outUV[0].x, param, errorBuffer, bufferSize
);
