# NDP End-host Implementation

## NDP basic configuration

There are a couple of NDP configuration options which are selected based on symlinks.
In both *core/* and *lib/* there are two makefiles, named *Makefile.regular* and
*Makefile.xen*. As the name suggests, the former is used when building NDP to run on
a physical host, whereas the latter builds NDP to run in a Xen guest. We didn’t go
very far with testing on Xen, and while it did work at some point, a lot of things
are probably deprecated by now. You should create a symbolic link named *Makefile*
to point at *Makefile.regular* in both folders (if one does not already exist).

Then, in the *common/* folder, there are three variants of *config.h*, named
*config.cam.h*, *config.cocos.h*, and *config.gaina.h*. Please note *config.base.h*
is not yet another variant, but rather a file containing some common options, which
is then included by *config.h*. The *config.h* file itself is (or should be created
as) a symbolic link pointing towards one of the three variants. Each variant picks
the current environment (via `#define NDP_CURRENT_TEST_ENVIRONMENT <something>`),
and sets the value of `NDP_HARDCODED_SAME_NEXT_HOP`, which is generally 0. The main
purpose of setting `NDP_CURRENT_TEST_ENVIRONMENT` is to define how IP to MAC address
translations are handled. Whenever a packet is sent, the source and destination MAC
addresses are written by applying the correspondence to the source and destination
IP addresses. 

The NDP implementation does not have a way to automatically find out the MAC address
associated with a given IP address, so the correspondence is hard-coded in
*common/mac_db.c*. The data/code in this file is then used by the *get_mac_address*
function from *common/helpers.c*. Essentially, for a given IPv4 address,
`(the least significant byte - 1)` is used as an index in the table associated with
the current environment to obtain the MAC address. As the code is right now, you
should replace the values associated with the CAM environment, or define your own
(but this is more unwieldy). Don’t forget to symlink to the proper *config.<env>.h*
file via *config.h*. This kinda assumes you are using a flat L2 network. If that’s
not the case, `NDP_HARDCODED_SAME_NEXT_HOP` might help, because it enables a code
path which always uses a certain MAC address as the destination for an outgoing
frame, but it’s a bit more awkward to configure.

There’s also the `NDP_CORE_PRIOPULLQ_4COSTIN` define in *core/config.h*. When
set as non-zero, the core will use two pull queues, the second being high-priority.
In the *add_pull_req* function from *core/rx.c*, this option enables a check based
on the index of the socket, which may end up placing requests in the second queue.
This is very hacky; the index is an internal state variable. For a core which runs
a single listening socket, the indices of the actual connection sockets start
from 1, in the order the very first packet is received (not necessarily the SYN).

## Building DPDK & NDP

The following steps were tested on an x86_64 system, running Ubuntu 16.04 LTS.
Please have a look at the prerequisites 
[here](https://doc.dpdk.org/guides/linux_gsg/sys_reqs.html).

- Download the 17.05.2 version of DPDK
(https://fast.dpdk.org/rel/dpdk-17.05.2.tar.xz). The NDP sources don’t work
out-of-the-box with the latest DPDK version, and 17.05.2 is the most recently
tested.

- Go into the DPDK directory, and compile the sources by running
`make install T=x86_64-native-linuxapp-gcc` (this can probably be sped up with the
`-j N` parameter, which builds using multiple threads). It’s fine if you get the
*Installation cannot run with T defined and DESTDIR undefined* message at the end.

- Export the following environment variables (the paths will probably be different
on your system):
```
export RTE_SDK=/root/stuff/dpdk/dpdk
export RTE_TARGET=x86_64-native-linuxapp-gcc
export NDP_IMPL_DIR=/root/stuff/ndp/impl2
```

The `NDP_IMPL_DIR` variable should point to the root folder of the NDP end-host
sources. Then go to `NDP_IMPL_DIR` and build everything by running `make`. We have
to set up a few more things before running any NDP app.

## Host configuration 

- Hugepages have to be enabled, for example by running:

```
echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
mkdir -p /mnt/huge && mount -t hugetlbfs nodev /mnt/huge
```

This attempts to reserve 1024 2MB hugepages. Another option is to reserve them
using a kernel cmdline parameter.

- The uio_pci_generic module has to be loaded, for example by running
`modprobe uio_pci_generic`.

- Pick one network interface (for example, `eth0`) and run the following commands
to bind it to DPDK:

```
cd <path_to_dpdk_folder>
./usertools/dpdk-devbind.py --bind=uio_pci_generic eth0
```

## Running the sample application(s)

The sample application is compiled into the *lib/lib* binary. Looking at *lib/main.c*,
we can see the current function being used is *f_ndp_ping_pong*, where we measure the
time elapsed between the client sending some data, and the server also responding
with some other data. The binary requires one numeric id as parameter: 1 (or any odd
number) for the server, and 0 (or any even number) for the client. A lot of things are
hard-coded (including the client and server addresses) … sorry about that :( Starting
multiple clients with different ids causes multiple connections to be initiated to
the same server (the source port is based on the id).

To actually run the example, do the following;
- go to the *core/* folder on both client and server, and run
`build/core <local_ip_address>`. This is always required, because the core does not
know which machine it’s running on.
- go to the “lib/“ folder on the server, and run `./lib 1`.
- go to the “lib/“ folder on the server, and run `./lib 0`.

If everything goes well, the client lib should print some `(iteration_index, time_in_us)`
pairs to stdout. They appear rather slowly because there are a couple of sleeps in the
code. Hopefully both the code of this example, and that from the others is relatively
straightforward to follow. If something is missing/not really clear, please let me
know.

An alternative Getting Started guide written by Vlad Olteanu can be found here: 
https://github.com/nets-cs-pub-ro/NDP/blob/master/host_impl/how_to_get_it_running.txt
