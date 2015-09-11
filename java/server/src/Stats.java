import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.atomic.LongAdder;

/**
 * Created by xavier on 16/06/15.
 */
public final class Stats extends Thread{
    private static final LongAdder packet=new LongAdder(),bytes=new LongAdder(),stime=new LongAdder(),qtime=new LongAdder();

    public static final void packetPlusOne(final int b, final long time){
        packet.add(1);
        bytes.add(b);
        stime.add(time);
    }

    public static final void queueTime(final long time){
        qtime.add(time);
    }

    public final void run(){
        do {
            try {
                Thread.sleep(4000);
                final long pkt = packet.sumThenReset();
                System.out.println(new SimpleDateFormat("d MMM yyyy HH:mm:ss").format(new Date()) + " - " + (bytes.sumThenReset() >> 12) + " KB/s (" + (pkt >> 2) + " pkt/s), queue_time = " + qtime.sumThenReset() / (1000000f * pkt) + " ms, process_time = " + stime.sumThenReset() / (1000000f * pkt) + " ms");
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        } while (true);
    }
}
