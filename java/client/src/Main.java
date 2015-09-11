import net.named_data.jndn.Face;
import net.named_data.jndn.Interest;
import net.named_data.jndn.Name;

/**
 * Created by xmarchal on 16/06/15.
 */
public class Main {
    public static boolean cont=true;
    public static void main(String [] args){
        Face face=new Face();
		String prefix="";
		Client c;
		int mode=1;
    	int window=32;
		String file_name="";
		for(int i=0;i<args.length;i++){
            switch(args[i]){
                case "-p":
		    		prefix=args[++i];
		    		break;
                case "-d":
                    mode=2;
					file_name=args[++i];
                    break;
                case "-w":
                    try {
                        window = Integer.parseInt(args[++i]);
                    }catch (NumberFormatException ignored){
						break;
					}
                    break;
            	case "-h":
                default:
                    System.out.println("usage: java Main [options...]" +
                            "\noptions:" +
							"\n\n\t-p prefix\t\tspecify the prefix of the server\n\t\t\t\t(default=/debit)" +
                            "\n\t-w window_size\t\tspecify the number of Interest sent simultaneously\n\t\t\t\t(default=32)" +
                            "\n\n\t-d file_name\t\tdonwload the specified file from the server" +
                            "\n\n\t-h\t\t\tdisplay this help message");
                    System.exit(-1);
				}
            }
            switch (mode) {
                case 1:
                    System.out.println("Start benchmark with window size = " + window);
                    c=new Client(face,window);
                    new Stats("benchmark").start();
                    for (int i = 1; i < window+1; i++) {
                        Name n = new Name(prefix+"/benchmark/"+i);
                        c.addInterest(new Interest(n,4000));
                    }
                    c.run();
                    break;
                case 2:
                    Name n=new Name(prefix+"/download/"+file_name);
                    c=new Client(face,window);
                    new Stats("download").start();
                    c.addInterest(new Interest(n,4000));
                    c.run();
                    break;
        }
	}
}
