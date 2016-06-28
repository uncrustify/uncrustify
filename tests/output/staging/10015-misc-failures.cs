void Func()
{
    OtherFunc(
        @"multi
line");
}

x = o.Func(
        y);
x = o.Func2(a, b,
        y);
o.Func(
    y);
o.Func2(a, b,
    y);


AnimatorStateMachine rootStateMachine = syncedIndex == -1
    ? animatorController.layers[selectedLayerIndex].stateMachine
    : animatorController.layers[syncedIndex].stateMachine;


public GUIStyle[] inSlots =
{
    FindStyle("flow shader in 0"), FindStyle("flow shader in 1"), FindStyle("flow shader in 2"), FindStyle("flow shader in 3"), FindStyle("flow shader in 4"), FindStyle("flow shader in 5"),
};


public GUIStyle[] inSlots = { 1, 2, 3 };


using System;
using UnityEngine;
using UnityEditor.Animations;
using System.Collections.Generic;

namespace UnityEditor.Graphs.AnimationStateMachine
{
    internal enum TransitionType
    {
        eState,
        eAnyState,
        eStateMachine,
        eEntry
    };

    internal class SourceNodeTransitionEditor
    {
        Editor m_Host;
        private List<TransitionEditionContext> m_Transitions;
        private UnityEditorInternal.ReorderableList m_TransitionList;
        private AnimatorTransitionInspectorBase m_TransitionInspector;

        private TransitionType m_Type;

        private bool m_LockList = false;

        private AnimatorState           m_State = null;
        private AnimatorStateMachine    m_StateMachine = null;


        private AnimatorController m_Controller;
        private AnimatorStateMachine m_ActiveStateMachine;

        public SourceNodeTransitionEditor(AnimatorState state, Editor host)
        {
            m_Type = TransitionType.eState;
            m_Host = host;
            m_State = state;
        }

        public SourceNodeTransitionEditor(AnimatorStateMachine stateMachine, TransitionType type, Editor host)
        {
            m_Type = type;
            m_Host = host;
            m_StateMachine = stateMachine;
        }

        public SourceNodeTransitionEditor(TransitionType type, Editor host) // entry
        {
            m_Type = type;
            m_Host = host;
        }

        public void  OnEnable()
        {
            AnimatorControllerTool.graphDirtyCallback += OnGraphDirty;
            m_Controller = AnimatorControllerTool.tool ? AnimatorControllerTool.tool.animatorController : null;
            m_ActiveStateMachine = AnimatorControllerTool.tool ? m_Type == TransitionType.eAnyState ?
                AnimatorControllerTool.tool.stateMachineGraph.rootStateMachine :
                AnimatorControllerTool.tool.stateMachineGraph.activeStateMachine : null;

            if (m_Controller)
                m_Controller.OnAnimatorControllerDirty += ResetTransitionList;

            ResetTransitionList();
        }

        public void OnInspectorGUI()
        {
            EditorGUI.BeginChangeCheck();
            m_TransitionList.DoLayoutList();

            if (EditorGUI.EndChangeCheck())
            {
                AnimatorControllerTool.tool.RebuildGraph();
                GUIUtility.ExitGUI();
            }

            if (m_TransitionInspector)
                m_TransitionInspector.OnInspectorGUI();
        }

        void GetTransitionContexts()
        {
            m_Transitions = new List<TransitionEditionContext>();

            if (m_ActiveStateMachine == null)
                return;

            switch (m_Type)
            {
                case TransitionType.eAnyState:
                {
                    var transitions = m_ActiveStateMachine.anyStateTransitions;
                    foreach (var transition in transitions)
                    {
                        m_Transitions.Add(new TransitionEditionContext(transition, null, null, m_ActiveStateMachine));
                    }
                    break;
                }
                case TransitionType.eState:
                {
                    if (m_State == null)
                        return;

                    var transitions = m_State.transitions;
                    foreach (var transition in transitions)
                    {
                        m_Transitions.Add(new TransitionEditionContext(transition, m_State, null, null));
                    }
                    break;
                }
                case TransitionType.eStateMachine:
                {
                    if (m_ActiveStateMachine)
                    {
                        var transitions = m_ActiveStateMachine.GetStateMachineTransitions(m_StateMachine);
                        foreach (var transition in transitions)
                        {
                            m_Transitions.Add(new TransitionEditionContext(transition, null, m_StateMachine, m_ActiveStateMachine));
                        }
                    }
                    break;
                }
                case TransitionType.eEntry:
                {
                    if (m_StateMachine == null)
                        return;

                    var transitions = m_StateMachine.entryTransitions;
                    foreach (var transition in transitions)
                    {
                        m_Transitions.Add(new TransitionEditionContext(transition, null, null, m_StateMachine));
                    }
                    break;
                }
                default:
                    break;
            }
        }

        private void ResetTransitionList()
        {
            if (m_LockList)
                return;

            GetTransitionContexts();
            m_TransitionList = new UnityEditorInternal.ReorderableList(m_Transitions, typeof(TransitionEditionContext), true, true, false, true);
            m_TransitionList.drawHeaderCallback = AnimatorTransitionInspector.DrawTransitionHeaderCommon;
            m_TransitionList.drawElementCallback = OnTransitionElement;
            m_TransitionList.onSelectCallback = SelectTransition;
            m_TransitionList.onReorderCallback = ReorderTransition;
            m_TransitionList.onRemoveCallback = RemoveTransition;
            m_TransitionList.displayAdd = false;

            m_Host.Repaint();
        }

        public void OnDestroy()
        {
            UnityEngine.Object.DestroyImmediate(m_TransitionInspector);
        }

        public void OnDisable()
        {
            AnimatorControllerTool.graphDirtyCallback -= OnGraphDirty;
            if (m_Controller)
                m_Controller.OnAnimatorControllerDirty -= ResetTransitionList;
        }

        private void RemoveTransition(UnityEditorInternal.ReorderableList transitionList)
        {
            UnityEngine.Object.DestroyImmediate(m_TransitionInspector);
            m_Transitions[m_TransitionList.index].Remove();

            ResetTransitionList();
        }

        private void SelectTransition(UnityEditorInternal.ReorderableList list)
        {
            var transition = m_Transitions[list.index].transition;

            if (transition == null)
                return;

            if (m_Type == TransitionType.eState || m_Type == TransitionType.eAnyState)
            {
                m_TransitionInspector = Editor.CreateEditor(transition as AnimatorStateTransition) as AnimatorStateTransitionInspector;
            }
            else
            {
                m_TransitionInspector = Editor.CreateEditor(transition as AnimatorTransition) as AnimatorTransitionInspector;
            }

            m_TransitionInspector.SetTransitionContext(m_Transitions[list.index]);
            m_TransitionInspector.showTransitionList = false;
        }

        private void ReorderTransition(UnityEditorInternal.ReorderableList list)
        {
            // prevent unwanted callback
            m_LockList = true;

            switch (m_Type)
            {
                case TransitionType.eAnyState:
                    Undo.RegisterCompleteObjectUndo(m_ActiveStateMachine, "Reorder transition");
                    m_ActiveStateMachine.anyStateTransitions = Array.ConvertAll<TransitionEditionContext, AnimatorStateTransition>(m_Transitions.ToArray(), t => (t.transition as AnimatorStateTransition));
                    break;
                case TransitionType.eState:
                    Undo.RegisterCompleteObjectUndo(m_State, "Reorder transition");
                    m_State.transitions = Array.ConvertAll<TransitionEditionContext, AnimatorStateTransition>(m_Transitions.ToArray(), t => (t.transition as AnimatorStateTransition));
                    break;
                case TransitionType.eStateMachine:
                    Undo.RegisterCompleteObjectUndo(m_ActiveStateMachine, "Reorder transition");
                    m_ActiveStateMachine.SetStateMachineTransitions(m_StateMachine, Array.ConvertAll<TransitionEditionContext, AnimatorTransition>(m_Transitions.ToArray(), t => (t.transition as AnimatorTransition)));
                    break;
                case TransitionType.eEntry:
                    Undo.RegisterCompleteObjectUndo(m_StateMachine, "Reorder transition");
                    m_StateMachine.entryTransitions = Array.ConvertAll<TransitionEditionContext, AnimatorTransition>(m_Transitions.ToArray(), t => (t.transition as AnimatorTransition));
                    break;
                default:
                    break;
            }

            m_LockList = false;
        }

        private void OnTransitionElement(Rect rect, int index, bool selected, bool focused)
        {
            AnimatorTransitionInspector.DrawTransitionElementCommon(rect, m_Transitions[index], selected, focused);
        }

        private void OnGraphDirty()
        {
            ResetTransitionList();
        }

        public bool HasPreviewGUI()
        {
            return m_TransitionInspector != null && m_TransitionInspector.HasPreviewGUI();
        }

        public void OnPreviewSettings()
        {
            m_TransitionInspector.OnPreviewSettings();
        }

        public void OnInteractivePreviewGUI(Rect r, GUIStyle background)
        {
            m_TransitionInspector.OnInteractivePreviewGUI(r, background);
        }
    }
}
