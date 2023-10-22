import subprocess
import signal
import os

os.system("make clean")
os.system("rm SampleVolume")



proc = subprocess.Popen(["make", "run"],
                        stdin=subprocess.PIPE,
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE)

def command(cmd):
    proc.stdin.write(cmd + b"\n")
    proc.stdin.flush()
    proc.stdin.flush()

# command(b"touch in_root.txt")
# command(b"cp2fs testfile.txt test1.txt")
# command(b"md foo")
# command(b"cd foo")
# command(b"touch in_foo.txt")
# command(b"mv /test1.txt test2.txt")
# command(b"ls -la /foo")
# command(b"ls -la /")
# command(b"exit")

command(b"touch in_root.txt")
command(b"cp2fs testfile.txt test1.txt")
command(b"md foo")
command(b"touch foo/in_foo.txt")
command(b"mv test1.txt foo")
# command(b"cd foo")
# command(b"ls -la")
command(b"ls -la /foo")
# command(b"cd /")
command(b"ls -la /")
command(b"exit")


print(proc.stdout.read().decode('utf-8'))
proc.send_signal(signal.SIGINT)
proc.wait()
