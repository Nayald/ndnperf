/**
 * Created by xmarchal on 23/06/15.
 */
public class Test {

    public static void main(String[] args) {
        long l=0;
        long start = System.currentTimeMillis();
        for(long i=1;i<100001;i++)
            //l = fibonacci(i);
        fibonacci(i);
        System.out.println(System.currentTimeMillis()-start);
        //System.out.println(l);
        long l1=0;
        start = System.currentTimeMillis();
        for(long i=1;i<100001;i++)
            //l1 = fibo_opt(i);
        fibo_opt(i);
        System.out.println(System.currentTimeMillis()-start);
        //System.out.println(l1);
        //System.out.println(l==l1);
    }

    public static long fibo_opt(long n){
        long x=1,y=1;
        for(long i=2;i<n;i+=2){
            x+=y;
            y+=x;
        }
        if((n&1)==0)return y;
        else return x;
    }


    public static long fibonacci(long n){
        if(n == 1 || n == 2){
            return 1;
        }
        long fibo1=1, fibo2=1, fibonacci=1;
        for(long i= 3; i<= n; i++){
            fibonacci = fibo1 + fibo2;
            fibo1 = fibo2;
            fibo2 = fibonacci;
        }
        return fibonacci;
    }
}

