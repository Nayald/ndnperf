import java.util.Collection;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ConcurrentLinkedDeque;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReferenceArray;
import java.util.concurrent.locks.LockSupport;

public class LFBQ<E> extends ConcurrentLinkedDeque<E> implements BlockingQueue<E> {
    private AtomicReferenceArray<Thread> consumerWorker;
    private int consumer;

    public LFBQ(int consumer){
        this.consumer=consumer;
        this.consumerWorker = new AtomicReferenceArray<>(consumer);
    }

    @Override
    public boolean offer(E e) {
        offerLast(e);
        for (int index = 0; index < consumer; index++) {
            Thread threadToNotify = consumerWorker.get(index);
            if (threadToNotify != null) {
                consumerWorker.set(index, null);
                LockSupport.unpark(threadToNotify);
            }
        }
        return true;
    }

    @Override
    public void put(E e) throws InterruptedException {
        offer(e);
    }

    @Override
    public boolean offer(E e, long timeout, TimeUnit unit)
            throws InterruptedException {
        put(e);
        return true;
    }

    @Override
    public E take() throws InterruptedException {
        while (true) {
            if (Thread.currentThread().isInterrupted()) return null;
            E value = pollFirst();
            if (value != null)
                return value;
            boolean parked = false;
            for (int index = 0; index < consumer; index++) {
                if (consumerWorker.get(index) == null && consumerWorker.compareAndSet(index, null, Thread.currentThread())) {
                    parked = true;
                    LockSupport.park(this);
                    break;
                }
            }
            if (!parked) {
                LockSupport.parkNanos(this, TimeUnit.MILLISECONDS.toNanos(10));
            }
        }
    }

    @Override
    public E poll(long l, TimeUnit timeUnit) throws InterruptedException {
        return pollFirst();
    }

    @Override
    public int remainingCapacity() {
        return 0;
    }

    @Override
    public int drainTo(Collection<? super E> collection) {
        return 0;
    }

    @Override
    public int drainTo(Collection<? super E> collection, int i) {
        return 0;
    }
}