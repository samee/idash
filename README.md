First of all, download and build [Obliv-C](https://github.com/samee/obliv-c).
Then, clone this repo and build it with

```
make OCPATH=/path/to/obliv-c
```

This should create two executables, `chi2` and `hamming`. Executing them without
command line params will give you usage details. Examples:

```
$ ./chi2
Usage: ./chi2 <case-file> <control-file> 1 <port>, or
       ./chi2 <case-file> <control-file> 2 <remote-addr> <port>
$ ./hamming
Usage: ./hamming <input-vcf-file> 1 <port>, or
       ./hamming <input-vcf-file> 2 <remote-addr> <port>
```

Concretely, to run `chi2` we can start up the server to listen on TCP port 1234
on one machine with:

```
$ ./chi2 case1.txt control1.txt 1 1234
```
And then (possibly on a different machine) start up the client:
```
$ ./chi2 case2.txt control2.txt 2 remotehost.com 1234
```
This will start execution. The `*.txt` files here are the samples provided on
the contest page. They produce the same output as the Python sample script
provied by them (run `./chi2.py -h` for help). The `1` in the command line
specifies server, while a `2` specifies client. To run them on the same machine,
just use `localhost` as the remote server name.

Running `hamming` is fairly similar. Sample files are provided with `*.snp`
extension in this case. Run it as:

```
$ ./hamming hu604D39.snp 1 4321 &
$ time ./hamming hu661AD0.snp 2 localhost 4321
Result: 4740
Result: 4740

real    0m7.389s
user    0m6.532s
sys     0m0.089s
```

In case of `hamming` both the server and the client produces output. For `chi2`,
only the client does.
