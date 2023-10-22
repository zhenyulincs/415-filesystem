import subprocess
import signal
import os
import time

# Start the subprogram
os.system('clear')
os.system('make clean')
os.system('rm SampleVolume')

proc = subprocess.Popen(["make", "run"], 
                        stdin=subprocess.PIPE, 
                        stdout=subprocess.PIPE, 
                        stderr=subprocess.PIPE)

def turn(command):
  proc.stdin.write((command + '\n').encode('utf-8'))
  proc.stdin.flush()
  time.sleep(0.1)

# print all the output
def print_output():
  print(proc.stdout.read().decode('utf-8'))
  #print(proc.stdout.read().decode('utf-8') + proc.stderr.read().decode('utf-8'))


# os.system('diff -qs testfile.txt outer.txt')
# os.system('ls -l *.txt')

def test():
  turn('md foo')
  turn('md /foo/bar')
  turn('md /foo/bar/qux')
  turn('cd /foo/bar/qux ')
  turn('ls')
  turn('pwd')
  turn('cd /')
  turn('ls -l')
  turn('cd foo')
  turn('pwd')
  turn('ls -l')
  turn('ls -l bar')
  turn('cd /foo/bar')
  #turn('ls -l')
  #turn('pwd')
  turn('cd qux')
  turn('pwd')
  turn('ls')

  turn("cd /")
  turn('cp2fs testfile.txt test1.txt')
  turn('md foo1')
  turn('md foo1/bar')
  turn('md foo1/bar/qux')
  turn('mv test1.txt test2.txt')
  turn('ls -la')
  turn('mv test2.txt foo1')
  turn('ls -la /')
  turn('ls -la foo')
  turn('mv /foo1/test2.txt test3.txt') # test3 file in root
  turn('ls')
  turn('mv test3.txt /foo1/bar/qux')
  turn('cd /foo1/bar/qux')
  turn('ls')
  turn('mv test3.txt /foo1') #ok
  turn('mv /foo1/test3.txt /foo1/bar/qux/4.txt') #ok
  turn('ls -l /foo1/bar/qux')
  turn('ls -l /foo1/bar')


  turn('exit')

try:
  test()
except:
  pass
finally:
  print_output()

