using System;
using System.Collections.Generic;
using System.Globalization;
using UnityEngine;
using UnityEngine.Serialization;
namespace UnityEngine.Experimental.Input
{
    public class ActionMap : ScriptableObject, IControlDomainSource
    {
        public List<InputControl> BuildControlsList()
        {
            ControlSetup controlsSetup = new ControlSetup();
            for (int i = 0; i < actions.Count; i++)
            {
                var action = actions[i];
// This line is kept with 71 spaces.
                SupportedControl supportedControl = (SupportedControl)(typeof(SupportedControl)
                    .GetMethod("Get")
                    .MakeGenericMethod(actions[i].controlType)
                    .Invoke(null, new object[] { actions[i].name }));
                action.controlIndex = controlsSetup.AddControl(supportedControl).index;
            }
            return controlsSetup.controls;
        }
    }
}
