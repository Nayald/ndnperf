import net.named_data.jndn.Face;
import net.named_data.jndn.Interest;
import net.named_data.jndn.Name;

import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;

/**
 * Created by xmarchal on 16/06/15.
 */
public class Main {
    public static void main(String [] args){
        Face face=new Face();
        int chunk=0;
        int freshness=0;
        int thread_count=0;
        String prefix="/debit";
        boolean rsa=true;
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
                case "-t":
                    try {
                        thread_count = Integer.parseInt(args[i + 1]);
                    }catch (NumberFormatException ignored){}
                    break;
                case "-k":
                    try {
                        KeyGenerator.main(new String[]{args[i+1]});
                    } catch (NoSuchAlgorithmException | NoSuchProviderException e) {
                        e.printStackTrace();
                    }
                    break;
                case "-p":
                    prefix=args[i+1];
                    break;
                case "-rsa":
                    if(args[i+1].toLowerCase().equals("false"))rsa=false;
                    break;
                case "-h":
                default:
                    error:
                    System.out.println("usage: java Main [options...]" +
                            "\noptions:" +
                            "\n\t-c chunk_size\t\tspecify the size of the data block\n\t\t\t\t(default=max=8192)" +
                            "\n\n\t-f freshness_period\tspecify the freshness period of the data\n\t\t\t\tin milliseconds (default=0)" +
                            "\n\n\t-k new_RSA_key_length\tgenerate a new RSA key pair with the given size\n\t\t\t\tand store the two key at ./privateKey and\n\t\t\t\t./publicKey (default=RSA-2048 if no files)" +
                            "\n\n\t-t thread_count\t\tinstanciate the given amount of threads\n\t\t\t\t(default=YOUR_CURRENT_CORE_COUNT)" +
                            "\n\n\t-p prefix\t\tspecify the prefix of the server\n\t\t\t\t(default=/debit)" +
                            "\n\n\t-rsa true|false\t\tspecify if the server use RSA or SHA-256, work\n\t\t\t\tonly with the benchmark (default=true)" +
                            "\n\n\t-h\t\t\tdisplay this help message");
                    System.exit(-1);

            }
        }
        try {
            Server p=new Server(face,chunk,freshness,thread_count,rsa);
            face.registerPrefix(new Name(prefix),p,p);
            new Stats().start();
        } catch (Exception e) {
            e.printStackTrace();
        }
        while(true) {
            try {
                face.processEvents();
                Thread.sleep(5);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
}
