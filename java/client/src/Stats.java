import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.atomic.LongAdder;

/**
 * Created by xavier on 16/06/15.
 */
public final class Stats extends Thread{
    private static final LongAdder packet=new LongAdder(), bytes=new LongAdder(), rtt=new LongAdder();
    private static long index,max=1;
    private int maxlen=0;

    public static final void packetPlusOne(final int b, final long r){
        packet.add(1);
        bytes.add(b);
        rtt.add(r);
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
                        final long pkt=packet.sumThenReset();
                        System.out.println(new SimpleDateFormat("d MMM yyyy HH:mm:ss").format(new Date()) + " - " + (bytes.sumThenReset() >> 11) + " KB/s (" + (pkt >> 1) + " pkt/s), latency = "+ rtt.sumThenReset()/(1000000f*pkt)+"ms");
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
                        sb.append("] "+ (bytes.sumThenReset() >> 10) + " KB/s, latency = "+rtt.sumThenReset()/(1e6f*packet.sumThenReset())+" ms");
			if(maxlen>=sb.length())while(sb.length()<maxlen)sb.append(" ");
			else maxlen=sb.length();
                        System.out.print(sb.toString()+'\r');
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
        }
    }
}
