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
        this.retry_table = new HashMap<>(window);
        i_queue = new ConcurrentLinkedDeque<>();
        interest_pass = new Semaphore(window, false);
        file_pass = new Semaphore(0, false);
        filemanager=new FileManager();
        pendingInterests=new HashMap<>();
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
        Stats.rttPlusOne(System.nanoTime()-pendingInterests.get(interest.getName().toUri()));
	    if (retry_table.get(interest.getName().toUri()) != null) retry_table.remove(interest.getName().toUri());
        switch (data.getName().get(1).toEscapedString()) {
            case "benchmark":
                //System.out.println("Throughput interest");
                Stats.packetPlusOne(data.getContent().size());
                try {
                    face.expressInterest(interest, this, this);
                    pendingInterests.put(interest.getName().toUri(),System.nanoTime());
                } catch (IOException e) {
                    e.printStackTrace();
                }
                break;
            case "download":
                interest_pass.release();
                if (data.getName().size() > 2) {
                    if (data.getName().size() > 3) {
                        //System.out.println("file data: " + data.getName().toUri());
                        //System.out.println(data.getContent());
                        Stats.packetPlusOne(data.getContent().size());
                        Stats.indexpp();
                        try {
                            pendingFile.put(data.getName().get(3).toSegment(), data.getContent());
                            file_pass.release();
                        } catch (EncodingException e) {
                            e.printStackTrace();
                        }
                    } else {
                        //System.out.print("file size: ");
                        long max = Long.parseLong(data.getContent().toString());
                        if (max == 0) {
                            System.out.println("\n File not found...");
                            Main.cont = false;
                            break;
                        }
                        // System.out.println(max);
                        Stats.setMax(max);
                        try {
                            pendingFile = new PendingFile(data.getName().get(2).toEscapedString(), max);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                        if (!filemanager.isAlive()) {
                            filemanager.setPriority(Thread.MIN_PRIORITY);
                            filemanager.start();
                        }
                        new Thread(() -> {
                            for (long i = 0; i < max; i++) {
                                Name n = new Name(interest.getName().getPrefix(3));
                                n.appendSegment(i);
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
                break;
            default:
                System.err.println(data.getContent().toString());
                break;
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
