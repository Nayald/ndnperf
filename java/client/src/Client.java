import net.named_data.jndn.*;
import net.named_data.jndn.encoding.EncodingException;
import net.named_data.jndn.util.Blob;

import java.io.IOException;
import java.util.HashMap;
import java.util.concurrent.ConcurrentLinkedDeque;
import java.util.concurrent.Semaphore;

/**
 * Created by xmarchal on 16/06/15.
 */
public final class Client implements OnData, OnTimeout {

    private final Face face;
    private final HashMap<String, Integer> retry_table;
    private final ConcurrentLinkedDeque<Interest> i_queue;
    private final HashMap<String,Long> pendingInterests;
    private PendingFile pendingFile;
    private final Semaphore file_pass;
    private long next,max;

    public Client(final Face face, int window) {
        this.face = face;
        this.retry_table = new HashMap<>(2*window,0.5f);
        this.i_queue = new ConcurrentLinkedDeque<>();
        this.pendingInterests=new HashMap<>(2*window,0.5f);
        this.file_pass = new Semaphore(0, false);
        new FileManager().start();
        this.next=window;
    }

    //add the initial Interest
    public final void addInterest(final Interest i) {
        i_queue.offerLast(i);
    }

    //main loop
    public final void run() {
        Interest interest;
        while ((interest = i_queue.pollFirst()) != null) {
            try {
                face.expressInterest(interest, this, this);
                pendingInterests.put(interest.getName().toUri(),System.nanoTime());
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        while (Main.cont) {
            try {
                face.processEvents();
                Thread.sleep(2);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public final void onData(final Interest interest, final Data data) {
        //System.out.println(data.getName().toUri());
	retry_table.remove(data.getName().toUri());
	Stats.packetPlusOne(data.getContent().size(), System.nanoTime() - pendingInterests.get(interest.getName().toUri()));
        try {
            switch (data.getName().get(1).toEscapedString()) {
                case "benchmark":
                    nextBenchmarkInterest(interest);
                    break;
                case "download":
                    nextDownloadInterest(data);
                    break;
            }
        }catch (IOException e) {
            e.printStackTrace();
        }
    }

    //use for benchmark test, resend the Interest packet
    public final void nextBenchmarkInterest(final Interest interest) throws IOException{
            Interest i = new Interest(new Name(prefix+"/benchmark/"+(next++)),4000);
	    face.expressInterest(i, this, this);
            pendingInterests.put(i.getName().toUri(),System.nanoTime());
    }

    //use for download test, if no specified segment, ask for the number of segment of the file, otherwise download the specified segment
    public final void nextDownloadInterest(final Data data) throws IOException{
        if (data.getName().size() > 3) {
            Stats.indexpp();
            try {
                pendingFile.put(data.getName().get(3).toSegment(), data.getContent());
                file_pass.release();
		Name n = new Name(data.getName().getPrefix(3)).appendSegment(next++);
		if(next<=max)face.expressInterest(new Interest(n, 4000), this, this);
		pendingInterests.put(n.toUri(),System.nanoTime());
            } catch (EncodingException e) {
                e.printStackTrace();
            }
        } else if (data.getName().size() > 2) {
            max = Long.parseLong(data.getContent().toString());
            if (max <= 0) {
                System.out.println("\n File not found...");
                Main.cont = false;
                return;
            }
            Stats.setMax(max);
            pendingFile = new PendingFile(data.getName().get(2).toEscapedString(), max);
            final long limit = Math.min(next, max);
            for (int i = 0; i < limit; i++){
		Name n= new Name(data.getName().getPrefix(3)).appendSegment(i);
                face.expressInterest(new Interest(n, 4000), this, this);
		pendingInterests.put(n.toUri(),System.nanoTime());
            }
        }
    }

    @Override
    public final void onTimeout(final Interest interest) {
        //System.exit(0);
	if (retry_table.get(interest.getName().toUri()) == null) retry_table.put(interest.getName().toUri(), 2);
        System.out.println("Timeout for " + interest.getName().toUri() + ": " + retry_table.get(interest.getName().toUri()) + " retry left.");
        if (retry_table.get(interest.getName().toUri()) > 0) {
            try {
                face.expressInterest(interest, this, this);
                pendingInterests.put(interest.getName().toUri(), System.nanoTime());
                retry_table.put(interest.getName().toUri(), retry_table.get(interest.getName().toUri()) - 1);
            } catch (IOException e) {
                e.printStackTrace();
            }
        } else {
            System.out.println("Timeout after 3 retries, transfer aborted.");
            Main.cont = false;
            System.exit(-1);
        }
    }

    //a thread that writes to disk the downloaded file from the NDN segments, the aim is to offload the main thread
    private final class FileManager extends Thread {
        public final void run() {
            while (Main.cont) {
                try {
                    file_pass.acquire(file_pass.availablePermits() > 0 ? file_pass.availablePermits() : 1);
                    Blob blob = pendingFile.nextSegment();
                    while (blob != null) {
                        try {
                            //System.out.println(blob.toString());
                            pendingFile.getChannel().write(blob.buf());
                            blob = pendingFile.nextSegment();
                        } catch (Exception e) {
                            e.printStackTrace();
                        }
                    }
                    if (!pendingFile.isPending()) {
                        pendingFile.close();
                        System.out.println("\nDownload completed !!!");
                        Main.cont = false;
                    }
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}
