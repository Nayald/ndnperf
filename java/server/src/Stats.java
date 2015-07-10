import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * Created by xavier on 16/06/15.
 */
public class Stats extends Thread{
    private static int packet;
    private static long bytes;

    public static synchronized void packetPlusOne(int b){
        packet++;
        bytes+=b;
    }

    public void run(){
        while (true){
            try {
                Thread.sleep(4000);
                System.out.println(new SimpleDateFormat("d MMM yyyy HH:mm:ss").format(new Date())+" - "+(bytes >> 12)+" KB/s ("+(packet>>2)+" pkt/s)");
                packet=0;
                bytes=0;
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}
