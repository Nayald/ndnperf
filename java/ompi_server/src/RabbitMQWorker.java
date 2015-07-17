import com.rabbitmq.client.*;
import net.named_data.jndn.Name;
import net.named_data.jndn.security.*;
import net.named_data.jndn.security.identity.IdentityManager;
import net.named_data.jndn.security.identity.MemoryIdentityStorage;
import net.named_data.jndn.security.identity.MemoryPrivateKeyStorage;
import net.named_data.jndn.util.Blob;

import java.io.IOException;
import java.nio.ByteBuffer;
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

public class RabbitMQWorker {
    private final static String QUEUE_NAME = "SignQueue";
    private final static String QUEUE_NAME2= "ReturnQueue";
    private final static String HOSTNAME = "192.168.1.33";
    private final static int PORT = 5672;

    private Channel channel;
    private Connection connection;
    private AMQP.Queue.DeclareOk queueOk,queueOk2;
    private QueueingConsumer consumer;
    private QueueingConsumer.Delivery lastDelivery;

    private final KeyChain keyChain;
    private final Name certificateName;

    public RabbitMQWorker(){
        try {
            ConnectionFactory factory = new ConnectionFactory();
            factory.setHost(HOSTNAME);
            factory.setPort(PORT);
            System.out.println(factory.getUsername());
            factory.setUsername("admin");
            factory.setPassword("admin");
            factory.setVirtualHost("NDN");
            this.connection = factory.newConnection();
            this.channel = connection.createChannel();
            this.channel.basicQos(1);
            this.queueOk = this.channel.queueDeclare(QUEUE_NAME, true, false, false, null);
            this.queueOk2 = this.channel.queueDeclare(QUEUE_NAME2, true, false, false, null);
            this.consumer = new QueueingConsumer(channel);
            channel.basicConsume(QUEUE_NAME, false, consumer);
        } catch (IOException | TimeoutException e) {
            e.printStackTrace();
        }

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
    }

    public void run() {
        byte[] message;
        SerializableData data=new SerializableData();
        while (true) {
            try {
                lastDelivery = consumer.nextDelivery();
                message = lastDelivery.getBody();
                data.deserialize(message);
                //System.out.println("Get data:\n\tname: "+data.getName().toUri()+"\n\tcontent: "+data.getContent()+"\n\tfreshness: "+data.getMetaInfo().getFreshnessPeriod());
                channel.basicAck(lastDelivery.getEnvelope().getDeliveryTag(), false);
                keyChain.sign(data,certificateName);
                channel.basicPublish("", QUEUE_NAME2, null, data.serialize());
            } catch (IOException | net.named_data.jndn.security.SecurityException | InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public static void main(String[] argv) throws Exception {
        RabbitMQWorker rabbitMQWorker=new RabbitMQWorker();
        rabbitMQWorker.run();
    }
}