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
                case "-h":
                default:
                    System.out.println("usage: java Main [-c chunk_size] [-f freshness_period] [-k new_RSA_key_length] [-t thread_count]");
                    System.exit(-1);

            }
        }
        try {
            Server p=new Server(face,chunk,freshness,thread_count);
            face.registerPrefix(new Name("/debit"),p,p);
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
