import net.named_data.jndn.Data;
import net.named_data.jndn.Interest;
import net.named_data.jndn.Name;
import net.named_data.jndn.util.Blob;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;

/**
 * Created by xmarchal on 15/07/15.
 */
public class SerializableData extends Data {
    public SerializableData(){
        super();
    }

    public SerializableData(Data data){
        super(data);
    }

    public byte[] serialize() {
        int name_size = this.getName().toUri().getBytes(Charset.defaultCharset()).length;
        int content_size = this.getContent().buf() != null ? this.getContent().buf().capacity() : 0;
        int signature_size = this.getSignature().getSignature().buf() != null ? this.getSignature().getSignature().buf().capacity() : 0;
        ByteBuffer buffer = ByteBuffer.allocate(name_size + content_size + signature_size + 3 * 4 + 8);
        buffer.putInt(name_size).put(this.getName().toUri().getBytes(Charset.defaultCharset()))
                .putDouble(this.getMetaInfo().getFreshnessPeriod())
                .putInt(content_size);
        if (this.getContent().buf() != null)
            buffer.put(this.getContent().buf());
        buffer.putInt(signature_size);
        if (this.getSignature().getSignature().buf() != null)
            buffer.put(this.getSignature().getSignature().buf());
        return buffer.array();
    }

    public void deserialize(byte[] raw) {
        ByteBuffer buffer = ByteBuffer.allocate(raw.length);
        buffer.put(raw);
        buffer.flip();

        //get the name
        int name_size = buffer.getInt();
        byte[] name = new byte[name_size];
        for (int i = 0; i < name_size; i++)
            name[i] = buffer.get();
        this.setName(new Name(new String(name, Charset.defaultCharset())));

        //get the freshness
        this.getMetaInfo().setFreshnessPeriod(buffer.getDouble());

        //get the content
        int content_size = buffer.getInt();
        if (content_size > 0) {
            byte[] content = new byte[content_size];
            for (int i = 0; i < content_size; i++)
                content[i] = buffer.get();
            this.setContent(new Blob(content));
        }

        //get the signature
        int signature_size = buffer.getInt();
        if (signature_size > 0) {
            byte[] signature = new byte[signature_size];
            for (int i = 0; i < signature_size; i++)
                signature[i] = buffer.get();
            this.getSignature().setSignature(new Blob(signature));
        }
    }

    public static byte[] serialize_interest(Interest interest, int chunk_size){
        int name_size = interest.getName().toUri().getBytes(Charset.defaultCharset()).length;
        ByteBuffer buffer= ByteBuffer.allocate(name_size + 2 * 4);
        buffer.putInt(name_size).put(interest.getName().toUri().getBytes(Charset.defaultCharset())).putInt(chunk_size);
        return buffer.array();
    }

    public int deserialize_interest(byte[] raw) {
        ByteBuffer buffer = ByteBuffer.allocate(raw.length);
        buffer.put(raw);
        buffer.flip();

        //get the name
        int name_size = buffer.getInt();
        byte[] name = new byte[name_size];
        for (int i = 0; i < name_size; i++)
            name[i] = buffer.get();
        this.setName(new Name(new String(name, Charset.defaultCharset())));

        return buffer.getInt();
    }

    public byte[] serialize_signature(){
        int name_size = this.getName().toUri().getBytes(Charset.defaultCharset()).length;
        int signature_size = this.getSignature().getSignature().buf() != null ? this.getSignature().getSignature().buf().limit() : 0;
        ByteBuffer buffer= ByteBuffer.allocate(name_size + signature_size + 2 * 4);
        buffer.putInt(name_size).put(this.getName().toUri().getBytes(Charset.defaultCharset()));
        buffer.putInt(signature_size);
        if (this.getSignature().getSignature().buf() != null)
            buffer.put(this.getSignature().getSignature().buf());
        return buffer.array();
    }

    public void deserialize_signature(byte[] raw) {
        ByteBuffer buffer = ByteBuffer.allocate(raw.length);
        buffer.put(raw);
        buffer.flip();

        //get the name
        int name_size = buffer.getInt();
        byte[] name = new byte[name_size];
        for (int i = 0; i < name_size; i++)
            name[i] = buffer.get();
        this.setName(new Name(new String(name, Charset.defaultCharset())));

        //get the signature
        int signature_size = buffer.getInt();
        if (signature_size > 0) {
            byte[] signature = new byte[signature_size];
            for (int i = 0; i < signature_size; i++)
                signature[i] = buffer.get();
            this.getSignature().setSignature(new Blob(signature));
        }
    }
}
