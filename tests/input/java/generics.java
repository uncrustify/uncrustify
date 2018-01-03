// Note: Some tests running on this _input_ file rely upon that the tripple 
// closing '>' are not separated by spaces or anything else!
public class TestClass {
private static void initMap(void) {
	HashMap < String, HashMap < String, List < Track >>> resolutionTracks = new HashMap < String, HashMap < String, List < Track >>> ();
}

private static void addTrackToMap(String resolution, Track track, HashMap < String, HashMap < String, List < Track >>> resolutionTracks) {
	HashMap<String, List<Track> > tracks = null;

	if (resolutionTracks.containsKey(resolution)) {
		tracks = resolutionTracks.get(resolution);
	} else {
		tracks = new HashMap<String, List<Track> >();
		tracks.put("soun", new LinkedList<Track>());
		tracks.put("vide", new LinkedList<Track>());
		resolutionTracks.put(resolution, tracks);
	}

	if (track.getHandler() != null) {
		if (track.getHandler().equals("soun")) {
			List<Track> audioTracks = tracks.get("soun");
			audioTracks.add(track);
		} else if (track.getHandler().equals("vide")) {
			List<Track> videoTracks = tracks.get("vide");
			videoTracks.add(track);
		}
	}
}
}
