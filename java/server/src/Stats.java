import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.atomic.LongAdder;

/**
 * Created by xavier on 16/06/15.
 */
public final class Stats extends Thread{
    private static final LongAdder packet=new LongAdder(),bytes=new LongAdder();

    public static final void packetPlusOne(final int b){
        packet.add(1);
        bytes.add(b);
    }

    public final void run(){
        while (true){
            try {
                Thread.sleep(4000);
                System.out.println(new SimpleDateFormat("d MMM yyyy HH:mm:ss").format(new Date())+" - "+(bytes.sumThenReset() >> 12)+" KB/s ("+(packet.sumThenReset() >>2)+" pkt/s)");
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}
