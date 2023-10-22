from .test_base import test_base
import subprocess
import signal
import time
class cp2_test(test_base):
    def __init__(self):
        super().__init__("cp2")

    def test(self):
        proc = subprocess.Popen(["make", "run"],
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        proc.stdin.write(b"cp2fs testfile.txt inner.txt\n")
        proc.stdin.flush()
        time.sleep(1)
        proc.stdin.write(b"cp2l inner.txt outer.txt\n")
        proc.stdin.flush()

        time.sleep(1)
        proc.stdin.write(b"cat inner.txt\n")
        proc.stdin.flush()

        time.sleep(1)
        proc.stdin.write(b"exit\n")
        proc.stdin.flush()

        self.write_to_log(proc.stdout.read().decode('utf-8'))
        self.write_to_log(subprocess.check_output('diff -qs testfile.txt outer.txt'.split(" ")).decode('utf-8'))
        proc.send_signal(signal.SIGINT)
        proc.wait()