import java.io.*;
import java.nio.*;
import com.sun.jna.*;
import java.util.*;
import test.IntelQsvDecoderLibrary;

public class JnaTest {

    static {
        System.loadLibrary("intel_qsv_decoder");
    }

    public static void main(String[] args) {
        System.out.println("---- start ----");
        
        IntelQsvDecoderLibrary dec = IntelQsvDecoderLibrary.INSTANCE;
        int ret = dec.initialize();
        System.out.println("initialize ret = "+ret);

        int esCount = 0;
        int totalSize = 0;
        int totalDecSize = 0;
        try {
            BufferedInputStream bis = new BufferedInputStream(new FileInputStream(new File("./out.264")));
            byte[] data = new byte[1024];
            int size = bis.read(data, 0, data.length);
            int width = dec.getWidth();
            int height = dec.getHeight();
            Pointer outBuf = null;
            int esSize = 0;
            Random rand = new Random();
            while (size > 0) {
                if (!dec.isInit()) {
                    System.out.println("APP: decodeHeader. size="+size);
                    ret = dec.decodeHeader(data, size);
                    System.out.println("APP: decodeHeader ret = "+ret);
		    if (ret != 0) {
			continue;
		    }
                }
		totalSize += size;
		esSize += size;
		esCount++;
		System.out.println("APP: decode start size="+size);
		ret = dec.decode(data, size);
		System.out.println("APP: decode ret = "+ret);
		if (ret == 0) {
		    if (width != dec.getWidth() || height != dec.getHeight()) {
			width = dec.getWidth();
			height = dec.getHeight();
			outBuf = new Memory(width * height * 3);
		    }
		    ret = dec.getFrame(outBuf, 1);
		    if (ret <= 0) {
			System.out.println("APP: decode error. ret = "+ret);
		    }
		    while (ret > 0) {
			System.out.println("APP: decode succeed esSize="+esSize);
			totalDecSize += esSize;
			esSize = 0;
			ret = dec.getFrame(outBuf, 1);
		    }
		}
                int dumLen = (int)(rand.nextDouble() * 10000);
                data = new byte[dumLen];
                size = bis.read(data, 0, dumLen);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        System.out.println("es count = "+esCount+", totalSize="+totalSize+", totalDecSize="+totalDecSize);


        System.out.println("---- finish ----");
    }
}

