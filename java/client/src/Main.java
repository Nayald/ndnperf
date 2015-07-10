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
        if(args.length>0) {
            Thread.currentThread().setPriority(Thread.MAX_PRIORITY);
            Client c;
            int k;
            switch (args[0]) {
                case "benchmark":
                    try{
                        k=Integer.parseInt(args[1]);
                    }catch(Exception e){
                        k=128;
                    }
                    System.out.println("Start benchmark with window size = " + k);
                    c=new Client(face,k);
                    new Stats("benchmark").start();
                    for (int i = 0; i < k; i++) {
                        Name n = new Name("/debit/benchmark/" + i);
                        c.addInterest(new Interest(n,4000));
                    }
                    c.run();
                    break;
                case "download":
                    if(args.length>1) {
                        Name n=new Name("/debit/download/"+args[1]);
                        try{
                            k=Integer.parseInt(args[2]);
                        }catch(Exception e){
                            k=128;
                        }
                        c=new Client(face,k-1);
                        new Stats("download").start();
                        c.addInterest(new Interest(n,4000));
                        c.run();
                    }
                    break;
            }
        }
    }
}
