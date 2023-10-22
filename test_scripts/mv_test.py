from .test_base import test_base
import subprocess
import signal
class mv_test(test_base):
    def __init__(self):
        super().__init__("mv")

    def test(self):
        proc = subprocess.Popen(["make", "run"],
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        proc.stdin.write(b"mv folder1\n")
        proc.stdin.flush()
        proc.stdin.write(b"mv /folder1 /folder1/folder2\n")
        proc.stdin.flush()
        proc.stdin.write(b"mv /folder1/folder2.txt  /folder1\n")
        proc.stdin.flush()
        proc.stdin.write(b"mv folder1.txt /folder1/folder2.txt\n")
        proc.stdin.flush()
        proc.stdin.write(b"mv /folder1.txt /folder1/folder2.txt\n")
        proc.stdin.flush()
        proc.stdin.write(b"mv . ..\n")
        proc.stdin.flush()
        proc.stdin.write(b"mv . /folder1\n")
        proc.stdin.flush()
        proc.stdin.write(b"mv inner.txt /folder1\n")
        proc.stdin.flush()
        proc.stdin.write(b"exit\n")
        proc.stdin.flush()
        self.write_to_log(proc.stdout.read().decode('utf-8'))
        proc.send_signal(signal.SIGINT)
        proc.wait()