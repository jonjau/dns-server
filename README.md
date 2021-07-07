# dns-server

A Simple DNS proxy server for IPv6 with caching and logging, written in C.

- Listens for DNS requests (in **binary** ".raw" packets) for **IPv6** addresses over **TCP** (not UDP), on port **8053**
- Forwards each request to another DNS server provided as arguments
  (e.g. Google's 8.8.8.8, port 53)
- Caches 5 most recent queries, forgoing the request forwarding if
  responding from cache is possible.
- Logs server events in the file `./dns_svr.log`.

Notes:

- Depends on POSIX libraries, so this will not run on Windows (use WSL)
- Not multithreaded, this server blocks when forwarding each request

## Running the program

Compile with GCC (at least C99) using

```bash
make
```

then run

```bash
./dns_svr <hostname> <port>
```

to start the server, passing in the details of the upstream server to forward
DNS requests and replies.

For testing, it is possible to use Google's public DNS:

- hostname **8.8.8.8**
- port **53**

## Usage

Ensure the server is running (locally), listening for TCP on 8053:

```_
./dns_svr 8.8.8.8 53
```

On UNIX systems, `dig` can be used as a DNS client (so no manual
writing/reading of DNS request/reply packets is needed).

Though you can examine/modify each DNS-over-TCP packet with Wireshark if you
want to. Hexdumping on the command line with `od`, like
`od -Ax -tx1 -v 1.req.raw > dump` is also possible.

### Examples

Requesting the IPv6 (hence AAAA) address of 'cloudflare.com' over TCP, from
the server 0.0.0.0 on port 8053, i.e. `dns-server` running locally.

```_
dig +tcp @0.0.0.0 -p 8053 AAAA cloudflare.com
```

The log file `./dns_svr.log` will record the request and the reply, with a
timestamp.
