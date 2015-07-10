import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;

public class KeyGenerator {
    public static void main(String[] args) throws NoSuchAlgorithmException, NoSuchProviderException {
        KeyPairGenerator keyGen = KeyPairGenerator.getInstance("RSA");
        int length;
        try {
            length = Integer.parseInt(args[0]);
        } catch (Exception e){
            length = 1024;
        }
        keyGen.initialize(length);
        KeyPair keyPair = keyGen.genKeyPair();
        byte[] privateKey = keyPair.getPrivate().getEncoded();
        byte[] publicKey = keyPair.getPublic().getEncoded();
        Path privateKeyFile = Paths.get("./privateKey");
        try {
            Files.write(privateKeyFile,privateKey);
        } catch (IOException e) {
            e.printStackTrace();
        }
        Path publicKeyFile = Paths.get("./publicKey");
        try {
            Files.write(publicKeyFile,publicKey);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}