# Usage
java Main [options...]<br/>
options:<br/>
<ul>
<li>-p prefix, specify the prefix of the server (default=/debit)</li>
<li>-c chunk_size, specify the maximum amount of data that can be sent per packet (default=8192)</li>
<li>-f freshness, specify the freshness of the packets in milliseconds (default=0)</li>
<li>-t thread_number, define the number of threads the server will use (default=YOUR_CURRENT_CORE_COUNT)</li>
<li>-k RSA_key_length, generate a new RSA key with the given length, write the public and private key on disk and use them to sign packets</li>
<li>-a RSA|SHA|ECDSA, select the algorithm for the benchmark part (default=RSA)</li>
<li>-ecdsa ecdsa_key length, specify the ECDSA key length for the key generation (default=256)</li>
<li>-h, display the help message</li>
</ul>
