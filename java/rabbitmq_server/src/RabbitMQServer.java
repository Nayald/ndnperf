import com.rabbitmq.client.AMQP.Queue.DeclareOk;
import com.rabbitmq.client.Channel;
import com.rabbitmq.client.Connection;
import com.rabbitmq.client.ConnectionFactory;
import com.rabbitmq.client.QueueingConsumer;
import net.named_data.jndn.*;
import net.named_data.jndn.encoding.EncodingException;
import net.named_data.jndn.security.KeyChain;
import net.named_data.jndn.security.KeyType;
import net.named_data.jndn.security.SecurityException;
import net.named_data.jndn.security.identity.IdentityManager;
import net.named_data.jndn.security.identity.MemoryIdentityStorage;
import net.named_data.jndn.security.identity.MemoryPrivateKeyStorage;
import net.named_data.jndn.util.Blob;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.security.KeyFactory;
import java.security.PrivateKey;
import java.security.interfaces.RSAPrivateKey;
import java.security.spec.PKCS8EncodedKeySpec;
import java.util.concurrent.TimeoutException;

/**
 * Created by Xavier on 15/07/2015.
 */
public class RabbitMQServer implements OnInterestCallback, OnRegisterFailed {
    private final static String QUEUE_NAME1 = "SignQueue";
    private final static String QUEUE_NAME2 = "ReturnQueue";
    private final static String HOSTNAME = "152.81.8.86";
    private final static int PORT = 5672;

    private Channel channel;
    private Connection connection;
    private DeclareOk queueOk, queueOk2;
    private QueueingConsumer consumer;
    private QueueingConsumer.Delivery lastDelivery;

    private final Face face;
    private String default_data;
    private final Data data_model;
    private final KeyChain keyChain;
    private final Name certificateName;

    public RabbitMQServer(Face face, int chunk, int freshness) {
        try {
            ConnectionFactory factory = new ConnectionFactory();
            factory.setHost(HOSTNAME);
            factory.setPort(PORT);
            System.out.println(factory.getUsername());
            factory.setUsername("ndn");
            factory.setPassword("ndn");
            factory.setVirtualHost("NDN");
            this.connection = factory.newConnection();
            this.channel = connection.createChannel();
            this.channel.basicQos(2);
            this.queueOk = this.channel.queueDeclare(QUEUE_NAME1, false, false, true, null);
            this.queueOk2 = this.channel.queueDeclare(QUEUE_NAME2, false, false, true, null);
            this.consumer = new QueueingConsumer(channel);
            channel.basicConsume(QUEUE_NAME2, false, consumer);
        } catch (IOException | TimeoutException e) {
            e.printStackTrace();
        }

        this.face = face;

        try {
            this.default_data = (chunk > 0 && chunk <=8192) ? new String(Files.readAllBytes(Paths.get("DefaultData.txt"))).substring(0, chunk) : new String(Files.readAllBytes(Paths.get("DefaultData.txt")));
        } catch (IOException e) {
            e.printStackTrace();
            System.exit(1);
        }
        System.out.println("Chunk size set to " + this.default_data.length() + " Bytes");

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
        try {
            identityStorage.addKey(keyName, KeyType.RSA, new Blob(publicKey, false));
            privateKeyStorage.setKeyPairForKeyName(keyName, KeyType.RSA, publicKey, privateKey);
        } catch (net.named_data.jndn.security.SecurityException e) {
            e.printStackTrace();
        }
        face.setCommandSigningInfo(keyChain, certificateName);
    }

    public void run() {
        byte[] message;
        SerializableData data = new SerializableData(data_model);
        int count;
        try {
            face.registerPrefix(new Name("/debit"), this, this);
        } catch (IOException | SecurityException e) {
            e.printStackTrace();
            System.exit(1);
        }
        System.out.println("Server start!");
        while (true) {
            try {
                this.queueOk2 = this.channel.queueDeclare(QUEUE_NAME2, false, false, true, null);
                //System.out.println(queueOk2.getMessageCount());
                count=queueOk2.getMessageCount();
                while (count > 0) {
                    lastDelivery = consumer.nextDelivery();
                    message = lastDelivery.getBody();
                    data.deserialize_signature(message);
                    face.putData(data);
                    channel.basicAck(lastDelivery.getEnvelope().getDeliveryTag(), false);
                    count--;
                }
                face.processEvents();
                Thread.sleep(5);
            } catch (InterruptedException | IOException | EncodingException e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public void onInterest(Name prefix, Interest interest, Face face, long interestFilterId, InterestFilter filter) {
        //System.out.println("Get interest!");
        SerializableData data=new SerializableData(data_model);
        data.setName(interest.getName());
        try {
            channel.basicPublish("", QUEUE_NAME1, null, SerializableData.serialize_interest(interest, default_data.length()));
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onRegisterFailed(Name prefix) {
        System.err.println("can't register "+prefix.toUri());
        System.exit(1);
    }

    public static void main(String[] args){
        int chunk=0;
        int freshness=0;
        for(int i=0;i<args.length;i+=2){
            switch(args[i]){
                case "-c":
                    try {
                        chunk = Integer.parseInt(args[i + 1]);
                    }catch (NumberFormatException ignored){}
                    break;
                case "-f":
                    try {
                        freshness = Integer.parseInt(args[i + 1]);
                    }catch (NumberFormatException ignored){}
                    break;
                case "-h":
                default:
                    System.out.println("usage: java Main [-c chunk_size] [-f freshness_period]");
                    System.exit(-1);

            }
        }
        RabbitMQServer rabbitMQServer=new RabbitMQServer(new Face(),chunk,freshness);
        rabbitMQServer.run();
    }
}
