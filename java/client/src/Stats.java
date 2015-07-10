import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * Created by xavier on 16/06/15.
 */
public class Stats extends Thread{
    private static int packet;
    private static long bytes;
    private static long start=System.currentTimeMillis();
    private static long index,max=1;

    public static synchronized void packetPlusOne(int b){
        packet++;
        bytes+=b;
    }

    public static synchronized void setMax(long m){
        max=m;
    }

    public static synchronized void indexpp(){
        index++;
    }

    private String mode;

    public Stats(String mode){
        this.mode=mode;
    }



    public void run(){
        switch (mode) {
            case "benchmark":
                while (true) {
                    try {
                        Thread.sleep(2000);
                        System.out.println(new SimpleDateFormat("d MMM yyyy HH:mm:ss").format(new Date()) + " - " + (bytes >> 11) + " KB/s (" + (packet >> 1) + " pkt/s)");
                        packet = 0;
                        bytes = 0;
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            case "download":
                while (Main.cont) {
                        long dotCount = (100*index+(2*max-1))/(2*max);
                        StringBuilder sb = new StringBuilder("[");
                        for (int count = 0; count < dotCount; count++) {
                            sb.append("=");
                        }
                        for (long count = dotCount; count < 50; count++) {
                            sb.append(" ");
                        }
                        sb.append("] "+ (bytes >> 10) + " KB/s");
                        for (int count = 0; count < sb.length(); count++) {
                            System.out.print("\r");
                        }
                        System.out.print(sb.toString());
                        bytes=0;
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
        }
    }
}
