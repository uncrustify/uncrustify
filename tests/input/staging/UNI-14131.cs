using System.Collections.Generic;

class C
{
    public void S()
    {
        Action localMethod = () => {
                SomeClass.OtherMethod(new Dictionary<string, string>
                {
                    {"a", "one"},
                    {"b", "two"},
                    {"c", "three"}
                });
            };

            m_Mixers.Add(
                new WeightInfo
                {
                    parentMixer = parent,
                    mixer = node,
                    port = port,
                    modulate = (type == typeof(AnimationLayerMixerPlayable))
                }
            );
    }
}
