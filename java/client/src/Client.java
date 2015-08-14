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
    private final Semaphore interest_pass, file_pass;
    private PendingFile pendingFile;
    private final Thread filemanager;
    private final HashMap<String,Long> pendingInterests;

    public Client(final Face face, int window) {
        this.face = face;
        this.retry_table = new HashMap<>(2*window,0.5f);
        i_queue = new ConcurrentLinkedDeque<>();
        interest_pass = new Semaphore(window, false);
        file_pass = new Semaphore(0, false);
        filemanager=new FileManager();
        pendingInterests=new HashMap<>(2*window,0.5f);
    }

    public final void addInterest(final Interest i) {
        i_queue.offerLast(i);
    }

    public final void run() {
        Interest interest;
        while (Main.cont) {
            while ((interest = i_queue.pollFirst()) != null) {
                try {
                    //System.out.println(interest.getName().toUri());
                    face.expressInterest(interest, this, this);
                    pendingInterests.put(interest.getName().toUri(),System.nanoTime());
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            try {
                face.processEvents();
                Thread.sleep(5);
            } catch (IOException | EncodingException | InterruptedException e) {
                e.printStackTrace();
            }
        }
        System.exit(0);
    }

    @Override
    public final void onData(final Interest interest, final Data data) {
        Stats.packetPlusOne(data.getContent().size(),System.nanoTime()-pendingInterests.get(data.getName().toUri()));
        retry_table.remove(data.getName().toUri());
        switch (data.getName().get(1).toEscapedString()) {
            case "benchmark":
                nextBenchmarkInterest(interest);
                break;
            case "download":
                nextDownloadInterest(data);
                break;
        }
    }

    public final void nextBenchmarkInterest(final Interest interest){
        try {
            face.expressInterest(interest, this, this);
            pendingInterests.put(interest.getName().toUri(),System.nanoTime());
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public final void nextDownloadInterest(final Data data){
        interest_pass.release();
        if (data.getName().size() > 3) {
            Stats.indexpp();
            try {
                pendingFile.put(data.getName().get(3).toSegment(), data.getContent());
                file_pass.release();
            } catch (EncodingException e) {
                e.printStackTrace();
            }
        } else if (data.getName().size() > 2) {
            final long max = Long.parseLong(data.getContent().toString());
            if (max == 0) {
                System.out.println("\n File not found...");
                Main.cont = false;
                return;
            }
            Stats.setMax(max);
            try {
                pendingFile = new PendingFile(data.getName().get(2).toEscapedString(), max);
            } catch (IOException e) {
                e.printStackTrace();
            }
            if (!filemanager.isAlive()) {
                filemanager.setPriority(Thread.MIN_PRIORITY);
                filemanager.start();
                new Thread(() -> {
                    Name n;
                    for (long i = 0; i < max; i++) {
                        n = new Name(data.getName().getPrefix(3)).appendSegment(i);
                        try {
                            interest_pass.acquire();
                            i_queue.offerLast(new Interest(n, 4000));
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                }).start();
            }

        }
    }

    @Override
    public final void onTimeout(final Interest interest) {
        if (retry_table.get(interest.getName().toUri()) == null) retry_table.put(interest.getName().toUri(), 2);
        System.out.println("Timeout for " + interest.getName().toUri() + ": " + retry_table.get(interest.getName().toUri()) + " retry left.");
        if (retry_table.get(interest.getName().toUri()) > 0) {
            i_queue.offer(interest);
            retry_table.put(interest.getName().toUri(), retry_table.get(interest.getName().toUri()) - 1);
        } else {
            System.out.println("Timeout after 3 retries, transfer aborted.");
            Main.cont = false;
            System.exit(-1);
        }
    }

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
                        System.out.println("\n Download completed !!!");
                        Main.cont = false;
                    }
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}