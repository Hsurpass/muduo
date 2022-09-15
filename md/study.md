![image-20220820092138087](image/image-20220820092138087.png)

![image-20220820092216252](image/image-20220820092216252.png)

![image-20220820092542526](image/image-20220820092542526.png)


reactor + threadpool ----> 一个reactor, 计算放到线程池
multiReactors -----> one loop per thread (一个线程一个reactor, 计算就在reactor中运行)
multiReactors + threadPool ----> one loop per thread +  thread pool (线程池共享，每个reactor中的计算在线程池中运行)
