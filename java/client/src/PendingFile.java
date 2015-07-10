import net.named_data.jndn.util.Blob;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.channels.FileChannel;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Created by xmarchal on 18/06/15.
 */
public final class PendingFile extends ConcurrentHashMap<Long,Blob> {
    private final RandomAccessFile file;
    private final FileChannel channel;
    private final long max_segment;
    private long position;

    public PendingFile(String filename, long max_segment) throws IOException {
        file=new RandomAccessFile(filename,"rw");
        channel=file.getChannel();
        this.max_segment=max_segment;
        this.position=0;
    }

    public boolean isPending(){
        return position<max_segment;
    }

    public Blob nextSegment(){
        if(get(position)!=null){
            Blob blob=get(position);
            remove(position++);
            return blob;
        }
        return null;
    }

    public FileChannel getChannel(){
        return channel;
    }

    public long getPosition(){
        return position;
    }

    public void close(){
        try {
            channel.close();
            file.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
