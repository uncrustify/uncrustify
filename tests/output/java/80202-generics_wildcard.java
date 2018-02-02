public class TestClass {
    private Map< ? , ? > map1 = null;
    private Map< ? , ? > map2 = null;
    private Map< ? , ? > map3 = null;

    public static HttpUriRequest getHttpUriRequest(TestClassAPIRequestMethod method, String apiPath) {
        switch (method) {
        case BOTTOM_LEFT:
            break;
        case GET:
            req = new HttpGet(url);
            break;
        case POST:
            req = new HttpPost(url);
            break;
        case PUT:
            req = new HttpPut(url);
            break;
        case DELETE:
            req = new HttpDelete(url);
            break;
        }
        return req;
    }
}
