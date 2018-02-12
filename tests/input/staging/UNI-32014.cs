//TestCase-001
new WaveformStreamer(s_sweepedClip, 0, s_sweepedClip.length, 1,
    (streamer, floats, remaining) = >
{
    return false;
}
);

//TestCase-002
var streamer = new WaveformStreamer(s_sweepedClip, 0, s_sweepedClip.length, s_sweepedClip.samples,
    (s, floats, remaining) = >
{
    return false;
}
);
