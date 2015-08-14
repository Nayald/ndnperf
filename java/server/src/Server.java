import net.named_data.jndn.*;
import net.named_data.jndn.encoding.EncodingException;
import net.named_data.jndn.security.KeyChain;
import net.named_data.jndn.security.KeyType;
import net.named_data.jndn.security.identity.IdentityManager;
import net.named_data.jndn.security.identity.MemoryIdentityStorage;
import net.named_data.jndn.security.identity.MemoryPrivateKeyStorage;
import net.named_data.jndn.util.Blob;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.security.KeyFactory;
import java.security.PrivateKey;
import java.security.interfaces.RSAPrivateKey;
import java.security.spec.PKCS8EncodedKeySpec;
import java.util.HashMap;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingDeque;

/**
 * Created by MARCHAL Xavier on 16/06/15.
 */
public final class Server implements OnInterestCallback, OnRegisterFailed {
    private static final String DATA8192 = "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";
    private static final int CORES = Runtime.getRuntime().availableProcessors();

    private final Face face;
    private final String default_data;
    private final int max_chunk_size;
    private final Data data_model;
    private final KeyChain keyChain;
    private final Name certificateName;
    private final boolean rsa;
    private final HashMap<String, FileChannel> pending_files;
    private final LinkedBlockingDeque<Name> queue;
    //private final Executor executor;

    public Server(final Face face, int chunk, int freshness, int thread_count, boolean rsa) throws net.named_data.jndn.security.SecurityException {
        this.face = face;

        this.default_data = chunk > 0 && chunk <= 8192 ? DATA8192.substring(0, chunk) : DATA8192;
        this.max_chunk_size=this.default_data.length();
        System.out.println("Chunk size set to " + max_chunk_size + " Bytes");

        this.data_model = new Data();
        MetaInfo m = new MetaInfo();
        m.setFreshnessPeriod(freshness);
        this.data_model.setMetaInfo(m);
        System.out.println("Fressness set to " + m.getFreshnessPeriod() + " ms");
        this.data_model.setContent(new Blob(default_data));

        MemoryIdentityStorage identityStorage = new MemoryIdentityStorage();
        MemoryPrivateKeyStorage privateKeyStorage = new MemoryPrivateKeyStorage();
        this.keyChain = new KeyChain(new IdentityManager(identityStorage, privateKeyStorage));
        Name keyName = new Name("/debit/DSK-123");
        this.certificateName = keyName.getSubName(0, keyName.size() - 1).append("KEY").append(keyName.get(-1)).append("ID-CERT").append("0");
        ByteBuffer publicKey;
        ByteBuffer privateKey;
        PrivateKey p;
        try {
            publicKey = ByteBuffer.wrap(Files.readAllBytes(Paths.get("./publicKey")));
            privateKey = ByteBuffer.wrap(Files.readAllBytes(Paths.get("./privateKey")));
            try {
                p = KeyFactory.getInstance("RSA").generatePrivate(new PKCS8EncodedKeySpec(privateKey.array()));
                System.out.println("Using custom " + ((RSAPrivateKey) p).getModulus().bitLength() + " bits RSA key pair");
            } catch (Exception e1) {
                e1.printStackTrace();
            }
        } catch (Exception e) {
            publicKey = Keys.DEFAULT_RSA_PUBLIC_KEY_DER;
            privateKey = Keys.DEFAULT_RSA_PRIVATE_KEY_DER;
            try {
                p = KeyFactory.getInstance("RSA").generatePrivate(new PKCS8EncodedKeySpec(privateKey.array()));
                System.out.println("Using default " + ((RSAPrivateKey) p).getModulus().bitLength() + " bits RSA key pair");
            } catch (Exception e1) {
                e1.printStackTrace();
            }
        }
        identityStorage.addKey(keyName, KeyType.RSA, new Blob(publicKey, false));
        privateKeyStorage.setKeyPairForKeyName(keyName, KeyType.RSA, publicKey, privateKey);
        this.face.setCommandSigningInfo(keyChain, certificateName);
        this.rsa = rsa;
        System.out.println("Using " + (rsa ? "RSA" : "SHA-256") + " for benchmarks");
        //System.out.println(certificateName.toUri());

        this.queue = new LinkedBlockingDeque<>();
        this.pending_files = new HashMap<>();

        int data_thread_count = thread_count > 0 ? thread_count : CORES;
        for(int i=0;i<data_thread_count;i++)new Thread(new DataProcess()).start();
        //this.executor = Executors.newFixedThreadPool(data_thread_count);
        System.out.println(data_thread_count + " Thread initialized");
    }

    @Override
    public final void onInterest(final Name prefix, final Interest interest, final Face face, final long interestFilterId, final InterestFilter filter) {
        queue.offerLast(interest.getName());
        /*Name name = interest.getName();
        switch ((name.size() > 1) ? name.get(1).toEscapedString() : "") {
            case "benchmark":
                executor.execute(new BenchmarkTask(name));
                break;
            case "download":
                executor.execute(new DownloadTask(name));
                break;
            /*default:
                //System.out.println("unknown interest: " + name.toUri());
                final Data d4 = new Data(data_model).setName(name).setContent(new Blob("unknown"));
                try {
                    keyChain.signWithSha256(d4);
                    face.putData(d4);
                } catch (Exception e) {
                    e.printStackTrace();
                }*/
        //}

    }

    /*private final class BenchmarkTask implements Runnable {
        private Name name;

        public BenchmarkTask(Name name) {
            this.name = name;
        }

        public final void run() {
            final Data d1 = new Data(data_model).setName(name).setContent(new Blob(default_data));
            try {
                if (rsa) keyChain.sign(d1, certificateName);
                else keyChain.signWithSha256(d1);
                face.putData(d1);
                Stats.packetPlusOne(max_chunk_size);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    private final class DownloadTask implements Runnable {
        private Name name;

        public DownloadTask(Name name) {
            this.name = name;
        }

        public final void run() {
            if (name.size() > 2) {
                if (name.size() > 3) {
                    //System.out.println("file transfer interest: "+i.toUri());
                    final String file_name = name.get(2).toEscapedString();
                    long segment = 0;
                    try {
                        segment = name.get(3).toSegment();
                    } catch (EncodingException e) {
                        e.printStackTrace();
                    }
                    if (pending_files.get(file_name) == null) {
                        if (new File(file_name).exists())
                            try {
                                pending_files.put(file_name, new RandomAccessFile(file_name, "r").getChannel());
                            } catch (FileNotFoundException e) {
                                e.printStackTrace();
                            }
                        else
                            return;
                    }
                    final ByteBuffer buffer = ByteBuffer.allocateDirect(max_chunk_size);
                    try {
                        pending_files.get(file_name).read(buffer, max_chunk_size * segment);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    final Data d2 = new Data(data_model).setName(name);
                    buffer.flip();
                    d2.setContent(new Blob(buffer, true));
                    //System.out.println(d2.getContent().toString());
                    try {
                        //keyChain.signWithSha256(d2);
                        keyChain.sign(d2, certificateName);
                        face.putData(d2);
                        Stats.packetPlusOne(d2.getContent().size());
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                } else {
                    //System.out.print("file size interest: ");
                    final File f = new File(name.get(2).toEscapedString());
                    final long fragment = f.exists() ? (f.length() + max_chunk_size - 1) / max_chunk_size : 0;
                    final Data d2 = new Data(data_model).setName(name).setContent(new Blob("" + fragment));
                    //System.out.println(fragment);
                    try {
                        //keyChain.signWithSha256(d2);
                        keyChain.sign(d2, certificateName);
                        face.putData(d2);
                        Stats.packetPlusOne(d2.getContent().size());
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    }*/

    private final class DataProcess implements Runnable {
        private Name name;

        public final void run() {
            do try {
                name = queue.take();
                switch ((name.size() > 1) ? name.get(1).toEscapedString() : "") {
                    case "benchmark":
                        doBenchmark();
                        break;
                    case "download":
                        doDownload();
                        break;
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            while (true);
        }

        public final void doBenchmark() throws Exception{
            final Data d1 = new Data(data_model).setName(name).setContent(new Blob(default_data));
            try {
                if (rsa) keyChain.sign(d1, certificateName);
                else keyChain.signWithSha256(d1);
                face.putData(d1);
                Stats.packetPlusOne(max_chunk_size);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        public final void doDownload() {
            if (name.size() > 2) {
                if (name.size() > 3) {
                    //System.out.println("file transfer interest: "+i.toUri());
                    final String file_name = name.get(2).toEscapedString();
                    long segment = 0;
                    try {
                        segment = name.get(3).toSegment();
                    } catch (EncodingException e) {
                        e.printStackTrace();
                    }
                    if (pending_files.get(file_name) == null) {
                        if (new File(file_name).exists())
                            try {
                                pending_files.put(file_name, new RandomAccessFile(file_name, "r").getChannel());
                            } catch (FileNotFoundException e) {
                                e.printStackTrace();
                            }
                        else
                            return;
                    }
                    final ByteBuffer buffer = ByteBuffer.allocateDirect(max_chunk_size);
                    try {
                        pending_files.get(file_name).read(buffer, max_chunk_size * segment);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    final Data d2 = new Data(data_model).setName(name);
                    buffer.flip();
                    d2.setContent(new Blob(buffer, true));
                    //System.out.println(d2.getContent().toString());
                    try {
                        //keyChain.signWithSha256(d2);
                        keyChain.sign(d2, certificateName);
                        face.putData(d2);
                        Stats.packetPlusOne(d2.getContent().size());
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                } else {
                    //System.out.print("file size interest: ");
                    final File f = new File(name.get(2).toEscapedString());
                    final long fragment = f.exists() ? (f.length() + max_chunk_size - 1) / max_chunk_size : 0;
                    final Data d2 = new Data(data_model).setName(name).setContent(new Blob("" + fragment));
                    //System.out.println(fragment);
                    try {
                        //keyChain.signWithSha256(d2);
                        keyChain.sign(d2, certificateName);
                        face.putData(d2);
                        Stats.packetPlusOne(d2.getContent().size());
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    }

    @Override
    public final void onRegisterFailed(final Name prefix) {
        System.err.println("Failed to register prefix: " + prefix.toUri());
        System.exit(-1);
    }
}