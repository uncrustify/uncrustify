namespace Unity
{
    public class Class 
    {
        // doesn't work because ; gets removed but
        public static readonly Vector2Quantization Uncompressed = new Vector2Quantization() {};
        // works and ; doesn't get removed
        public static readonly Vector2Quantization Uncompressed = new Vector2Quantization {};
    }
}
