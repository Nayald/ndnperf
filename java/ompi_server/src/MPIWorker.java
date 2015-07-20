import mpi.MPI;
import mpi.MPIException;
import net.named_data.jndn.Name;
import net.named_data.jndn.security.*;
import net.named_data.jndn.security.SecurityException;
import net.named_data.jndn.security.identity.IdentityManager;
import net.named_data.jndn.security.identity.MemoryIdentityStorage;
import net.named_data.jndn.security.identity.MemoryPrivateKeyStorage;
import net.named_data.jndn.util.Blob;

import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.security.KeyFactory;
import java.security.PrivateKey;
import java.security.interfaces.RSAPrivateKey;
import java.security.spec.PKCS8EncodedKeySpec;

/**
 * Created by xmarchal on 16/07/15.
 */
public class MPIWorker {

    private final KeyChain keyChain;
    private final Name certificateName;
    private int rank;

    public MPIWorker(int rank) throws net.named_data.jndn.security.SecurityException {
        this.rank=rank;
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
    }

    public void run(){
        int[] length=new int[1];
        byte[] message=new byte[1];;
        SerializableData serializableData_ = new SerializableData();
        while (true) {
            try {
                System.out.println("worker at rank "+rank+" wait for jobs");
                MPI.COMM_WORLD.recv(length, 1, MPI.INT, 0, 0);
                message = new byte[length[0]];
                MPI.COMM_WORLD.recv(message, length[0], MPI.BYTE, 0, 1);
                System.out.println("worker at rank "+rank+" get job");

                serializableData_.deserialize(message);

                try {
                    keyChain.sign(serializableData_, certificateName);
                } catch (net.named_data.jndn.security.SecurityException e) {
                    e.printStackTrace();
                }
                message = serializableData_.serialize();
                length[0] = message.length;
                System.out.println("worker at rank "+rank+" send message");
                MPI.COMM_WORLD.send(length, 1, MPI.INT, 0, 0);
                MPI.COMM_WORLD.send(message, length[0], MPI.BYTE, 0, 1);
            } catch (MPIException e) {
                e.printStackTrace();
            }
        }
    }

    public static void main(String[] args) throws MPIException {
        MPI.Init(args);
        int rank = MPI.COMM_WORLD.getRank(), size = MPI.COMM_WORLD.getSize();
        try {
            MPIWorker mpiWorker=new MPIWorker(rank);
            mpiWorker.run();
        } catch (SecurityException e) {
            e.printStackTrace();
        }
        MPI.Finalize();
    }
}
