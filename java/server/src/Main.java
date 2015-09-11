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
        char algo='R';
	int ecdsakeylen=256;
        for(int i=0;i<args.length;i++){
            switch(args[i]){
                case "-c":
                    try {
                        chunk = Integer.parseInt(args[++i]);
                    }catch (NumberFormatException ignored){}
                        break;
                case "-f":
                    try {
                        freshness = Integer.parseInt(args[++i]);
                    }catch (NumberFormatException ignored){}
                    break;
                case "-t":
                    try {
                        thread_count = Integer.parseInt(args[++i]);
                    }catch (NumberFormatException ignored){}
                    break;
                case "-k":
                    try {
                        KeyGenerator.main(new String[]{args[++i]});
                    } catch (NoSuchAlgorithmException | NoSuchProviderException e) {
                        e.printStackTrace();
                    }
                    break;
                case "-p":
                    prefix=args[++i];
                    break;
                case "-a":
                    switch(args[++i].toUpperCase()){
			case "RSA":
				algo='R';
				break;
			case "SHA":
				algo='S';
				break;
			case "ECDSA":
				algo='E';
				break;
			default:
				System.out.println("this algorithm isn't supported");
				break;
			}
                    break;
		case "-ecdsa":
                    try {
                        ecdsakeylen = Integer.parseInt(args[++i]);
                    }catch (NumberFormatException ignored){}
                    break;
                case "-h":
                default:
                    System.out.println("usage: java Main [options...]" +
                            "\noptions:" +
                            "\n\t-c chunk_size\t\tspecify the size of the data block\n\t\t\t\t(default=max=8192)" +
                            "\n\n\t-f freshness_period\tspecify the freshness period of the data\n\t\t\t\tin milliseconds (default=0)" +
                            "\n\n\t-k new_RSA_key_length\tgenerate a new RSA key pair with the given size\n\t\t\t\tand store the two key at ./privateKey and\n\t\t\t\t./publicKey (default=RSA-2048 if no files)" +
                            "\n\n\t-t thread_count\t\tinstanciate the given amount of threads\n\t\t\t\t(default=YOUR_CURRENT_CORE_COUNT)" +
                            "\n\n\t-p prefix\t\tspecify the prefix of the server\n\t\t\t\t(default=/debit)" +
                            "\n\n\t-a RSA|SHA|ECDSA\t\tspecify if the server use RSA, SHA-256 or ECDSA. Work\n\t\t\t\tonly with the benchmark test (default=RSA)" +
			    "\n\n\t-ecdsa key_lenght\t\tspecify the key lenght of the ECDSA signature if selected\n\t\t\t\t(default=256)" +
                            "\n\n\t-h\t\t\tdisplay this help message");
                    System.exit(-1);
            }
        }
        try {
            Server server=new Server(face,prefix,chunk,freshness,thread_count,algo,ecdsakeylen);
            face.registerPrefix(new Name(prefix),server,server);
            new Stats().start();
	    server.run();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
