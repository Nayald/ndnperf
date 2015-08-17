# ndnperf
As part of my internship, I have to perform benchmarks in order to see the current performance of the NDN protocol but I didn't find any tools for that so I decide to build my own tool. The aim is to provide a tool which try to show us the NDN throughput we can achieve with given parameters (chunk_size, freshness, emission_window_size, thread_count, signature, rsa_key_lenght) and language implementation (Java, the main part, and C++).

## benchmarks
Here you can find some tests done with an i5-4590 with 4x4GB DDR3@1600:<br/>
When they do not vary the parameters are:
<ul>
<li>chunk_size = 8192 bytes</li>
<li>freshness = 0 ms</li>
<li>signature = RSA</li>
<li>RSA_key_lenght = 2048 bits</li>
<li>thread_number = 4</li>
</ul>
<br/>
<p align="center">
<img src="https://raw.githubusercontent.com/Kanemochi/ndnperf/master/doc/benchmark/cpp_vs_java.png"/>
<br/><br/>
<img src="https://raw.githubusercontent.com/Kanemochi/ndnperf/master/doc/benchmark/sign-diff.png"/>
<br/><br/>
<img src="https://raw.githubusercontent.com/Kanemochi/ndnperf/master/doc/benchmark/rsa.png"/>
</p>
