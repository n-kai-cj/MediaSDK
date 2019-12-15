package test;
public interface IntelQsvDecoderLibrary extends com.sun.jna.Library {
    IntelQsvDecoderLibrary INSTANCE = com.sun.jna.Native.load("intel_qsv_decoder", IntelQsvDecoderLibrary.class);
    int initialize();
    void uninitialize();
    int decodeHeader(byte[] in, int in_lengt);
    int decode(byte[] in, int in_length);
    int decode_get(byte[] in, int in_length, com.sun.jna.Pointer out, int conv_opt);
    int getFrame(com.sun.jna.Pointer out, int conv_opt);
    int drainFrame(com.sun.jna.Pointer out, int conv_opt);
    int getWidth();
    int getHeight();
    boolean isInit();
}

