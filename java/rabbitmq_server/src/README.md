# prerequisite
This implementation need to be stored in a network drive (for example with NFS).
Simply put the directory on it.<br/>
It also need a RabbitMQ server.

# Usage
execute 1 RabbitMQServer on the NDN server (where is NFD) and N RabbitMQWorker on the same or other servers (N is the number of parallel jobs).<br/>

use KeyGenerator if you want another RSA key length.<br/>

java RabbitMQServer [options...]<br/>
options:<br/>
<ul>
<li>-c chunk_size, specify the maximum amount of data that can be sent per packet (default=8192)</li>
<li>-f freshness, specify the freshness of the packets in milliseconds (default=0)</li>
</ul>

No option for RabbitMQWorker. Actually, the connexion information are statics.
